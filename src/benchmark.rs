use crate::engine_wrapper::EngineWrapper;
use crate::input;
use crate::models::Order;
use std::time::Instant;

pub struct BenchResult {
    pub total_orders: usize,
    pub total_seconds: f64,
    pub tps: f64,
    pub latency_ns: f64,
}

pub fn load_data(filename: &str) -> std::io::Result<Vec<Order>> {
    println!("Loading orders from {}...", filename);
    let orders = input::load_orders_file(filename)?;
    println!("Loaded {} orders.", orders.len());
    Ok(orders)
}

pub fn run(engine: &mut EngineWrapper, orders: &[Order]) -> BenchResult {
    println!("Engine successfully launched! Starting Benchmark...");

    let start = Instant::now();

    engine.place_orders_batch(orders);

    let duration = start.elapsed();

    let total_seconds = duration.as_secs_f64();
    let tps = orders.len() as f64 / total_seconds;
    let latency_ns = duration.as_nanos() as f64 / orders.len() as f64;

    BenchResult {
        total_orders: orders.len(),
        total_seconds,
        tps,
        latency_ns,
    }
}

pub fn print_report(result: &BenchResult) {
    println!("--------------------------------------------------");
    println!("Done!");
    println!("Total Orders : {}", result.total_orders);
    println!("Time Elapsed : {:.6} seconds", result.total_seconds);
    println!("TPS (Orders/sec) : {:.2}", result.tps);
    println!("Latency per Order: {:.2} ns", result.latency_ns);
    println!("--------------------------------------------------");
}
