use matching_engine::adapters::trade_logger;
use matching_engine::application::benchmark;
use matching_engine::engine::EngineWrapper;

// Default runtime configuration.
const ENABLE_LOGGING: bool = false;
const DATA_FILE: &str = "test.csv";
const LATENCY_WARMUP_ORDERS: usize = 100_000;
const LATENCY_SAMPLE_EVERY: usize = 100;

fn main() -> Result<(), Box<dyn std::error::Error>> {
    let orders = benchmark::load_data(DATA_FILE)?;

    let throughput_result = {
        println!("Launching throughput engine...");
        let mut engine = EngineWrapper::new();
        let logger_handle = trade_logger::start(&mut engine, ENABLE_LOGGING);

        let result = benchmark::run_throughput(&mut engine, &orders)?;
        trade_logger::stop(logger_handle);
        result
    };
    benchmark::print_throughput_report(&throughput_result, ENABLE_LOGGING);

    let latency_result = {
        println!("Launching latency engine...");
        let mut engine = EngineWrapper::new();
        let config = benchmark::LatencyConfig {
            warmup_orders: LATENCY_WARMUP_ORDERS,
            sample_every: LATENCY_SAMPLE_EVERY,
        };

        benchmark::run_latency(&mut engine, &orders, config)?
    };
    benchmark::print_latency_report(&latency_result);

    println!("Benchmark complete.");

    Ok(())
}
