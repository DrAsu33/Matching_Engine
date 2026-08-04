use crate::adapters::csv_input;
use crate::domain::Order;
use crate::engine::EngineWrapper;
use std::fmt;
use std::time::{Duration, Instant};

const TIMER_CALIBRATION_SAMPLES: usize = 10_000;

#[derive(Debug, Clone, Copy)]
pub struct LatencyConfig {
    pub warmup_orders: usize,
    pub sample_every: usize,
}

impl Default for LatencyConfig {
    fn default() -> Self {
        Self {
            warmup_orders: 100_000,
            sample_every: 100,
        }
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum BenchmarkError {
    EmptyOrderSet,
    ZeroSampleInterval,
}

impl fmt::Display for BenchmarkError {
    fn fmt(&self, formatter: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            Self::EmptyOrderSet => write!(formatter, "benchmark requires at least one order"),
            Self::ZeroSampleInterval => write!(formatter, "sample interval must be non-zero"),
        }
    }
}

impl std::error::Error for BenchmarkError {}

#[derive(Debug)]
pub struct ThroughputResult {
    pub total_orders: usize,
    pub total_seconds: f64,
    pub orders_per_second: f64,
    pub amortized_ns_per_order: f64,
}

#[derive(Debug)]
pub struct LatencyResult {
    pub measured_orders: usize,
    pub warmup_orders: usize,
    pub sample_every: usize,
    pub sample_count: usize,
    pub total_seconds: f64,
    pub orders_per_second: f64,
    pub timer_overhead_ns: u64,
    pub mean_ns: f64,
    pub min_ns: u64,
    pub p50_ns: u64,
    pub p90_ns: u64,
    pub p99_ns: u64,
    pub p99_9_ns: u64,
    pub max_ns: u64,
}

pub fn load_data(filename: &str) -> std::io::Result<Vec<Order>> {
    println!("Loading orders from {}...", filename);
    let orders = csv_input::load_orders_file(filename)?;
    println!("Loaded {} orders.", orders.len());
    Ok(orders)
}

pub fn run_throughput(
    engine: &mut EngineWrapper,
    orders: &[Order],
) -> Result<ThroughputResult, BenchmarkError> {
    if orders.is_empty() {
        return Err(BenchmarkError::EmptyOrderSet);
    }

    println!("Starting batch throughput benchmark...");

    let start = Instant::now();
    engine.place_orders_batch(orders);
    let duration = start.elapsed();

    let total_seconds = duration.as_secs_f64();
    let orders_per_second = orders.len() as f64 / total_seconds;
    let amortized_ns_per_order = duration.as_nanos() as f64 / orders.len() as f64;

    Ok(ThroughputResult {
        total_orders: orders.len(),
        total_seconds,
        orders_per_second,
        amortized_ns_per_order,
    })
}

pub fn run_latency(
    engine: &mut EngineWrapper,
    orders: &[Order],
    config: LatencyConfig,
) -> Result<LatencyResult, BenchmarkError> {
    if orders.is_empty() {
        return Err(BenchmarkError::EmptyOrderSet);
    }
    if config.sample_every == 0 {
        return Err(BenchmarkError::ZeroSampleInterval);
    }

    let warmup_orders = config.warmup_orders.min(orders.len().saturating_sub(1));
    for order in &orders[..warmup_orders] {
        engine.place_order(order);
    }

    let measured = &orders[warmup_orders..];
    let sample_capacity = measured.len().div_ceil(config.sample_every);
    let mut samples = Vec::with_capacity(sample_capacity);
    let timer_overhead_ns = measure_timer_overhead_ns();

    println!("Starting sampled single-order latency benchmark...");
    let run_start = Instant::now();
    for (index, order) in measured.iter().enumerate() {
        if index % config.sample_every == 0 {
            let start = Instant::now();
            engine.place_order(order);
            samples.push(duration_ns(start.elapsed()));
        } else {
            engine.place_order(order);
        }
    }
    let total_seconds = run_start.elapsed().as_secs_f64();

    samples.sort_unstable();
    let sum_ns: u128 = samples.iter().map(|&sample| u128::from(sample)).sum();

    Ok(LatencyResult {
        measured_orders: measured.len(),
        warmup_orders,
        sample_every: config.sample_every,
        sample_count: samples.len(),
        total_seconds,
        orders_per_second: measured.len() as f64 / total_seconds,
        timer_overhead_ns,
        mean_ns: sum_ns as f64 / samples.len() as f64,
        min_ns: samples[0],
        p50_ns: percentile_nearest_rank(&samples, 0.50),
        p90_ns: percentile_nearest_rank(&samples, 0.90),
        p99_ns: percentile_nearest_rank(&samples, 0.99),
        p99_9_ns: percentile_nearest_rank(&samples, 0.999),
        max_ns: samples[samples.len() - 1],
    })
}

pub fn print_throughput_report(result: &ThroughputResult, logging_enabled: bool) {
    println!("---------------- Batch Throughput ----------------");
    println!("Logging callback      : {logging_enabled}");
    println!("Total orders          : {}", result.total_orders);
    println!("Elapsed               : {:.6} s", result.total_seconds);
    println!(
        "Throughput            : {:.2} orders/s",
        result.orders_per_second
    );
    println!(
        "Amortized time/order  : {:.2} ns (not a latency percentile)",
        result.amortized_ns_per_order
    );
    println!("--------------------------------------------------");
}

pub fn print_latency_report(result: &LatencyResult) {
    println!("--------------- Sampled Latency ------------------");
    println!("Path                  : Rust -> C ABI -> C++ (logging disabled)");
    println!("Samples               : raw; timer and FFI overhead included");
    println!("Warm-up orders        : {}", result.warmup_orders);
    println!("Measured orders       : {}", result.measured_orders);
    println!("Sampling interval     : 1 / {} orders", result.sample_every);
    println!("Latency samples       : {}", result.sample_count);
    println!("Elapsed               : {:.6} s", result.total_seconds);
    println!(
        "Single-call throughput: {:.2} orders/s",
        result.orders_per_second
    );
    println!("Timer overhead (p50)  : {} ns", result.timer_overhead_ns);
    println!("Mean                  : {:.2} ns", result.mean_ns);
    println!("Min                   : {} ns", result.min_ns);
    println!("p50                   : {} ns", result.p50_ns);
    println!("p90                   : {} ns", result.p90_ns);
    println!("p99                   : {} ns", result.p99_ns);
    println!("p99.9                 : {} ns", result.p99_9_ns);
    println!("Max                   : {} ns", result.max_ns);
    println!("--------------------------------------------------");
}

fn measure_timer_overhead_ns() -> u64 {
    let mut samples = Vec::with_capacity(TIMER_CALIBRATION_SAMPLES);
    for _ in 0..TIMER_CALIBRATION_SAMPLES {
        let start = Instant::now();
        samples.push(duration_ns(start.elapsed()));
    }
    samples.sort_unstable();
    percentile_nearest_rank(&samples, 0.50)
}

fn duration_ns(duration: Duration) -> u64 {
    u64::try_from(duration.as_nanos()).unwrap_or(u64::MAX)
}

fn percentile_nearest_rank(sorted_samples: &[u64], quantile: f64) -> u64 {
    debug_assert!(!sorted_samples.is_empty());
    debug_assert!((0.0..=1.0).contains(&quantile));

    let rank = (quantile * sorted_samples.len() as f64).ceil() as usize;
    sorted_samples[rank.saturating_sub(1).min(sorted_samples.len() - 1)]
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::domain::Side;

    #[test]
    fn calculates_nearest_rank_percentiles() {
        let samples = [10, 20, 30, 40, 50];

        assert_eq!(percentile_nearest_rank(&samples, 0.50), 30);
        assert_eq!(percentile_nearest_rank(&samples, 0.90), 50);
        assert_eq!(percentile_nearest_rank(&samples, 0.99), 50);
        assert_eq!(percentile_nearest_rank(&samples, 0.0), 10);
        assert_eq!(percentile_nearest_rank(&samples, 1.0), 50);
    }

    #[test]
    fn samples_single_order_latency_after_warmup() {
        let orders = [
            Order::new(1, 1001, Side::Bid, 90, 1).unwrap(),
            Order::new(2, 1002, Side::Bid, 91, 1).unwrap(),
            Order::new(3, 1003, Side::Bid, 92, 1).unwrap(),
            Order::new(4, 1004, Side::Bid, 93, 1).unwrap(),
        ];
        let mut engine = EngineWrapper::new_for_test(8, 8);

        let result = run_latency(
            &mut engine,
            &orders,
            LatencyConfig {
                warmup_orders: 1,
                sample_every: 2,
            },
        )
        .unwrap();

        assert_eq!(result.warmup_orders, 1);
        assert_eq!(result.measured_orders, 3);
        assert_eq!(result.sample_count, 2);
        assert!(result.min_ns <= result.p50_ns);
        assert!(result.p50_ns <= result.max_ns);
    }
}
