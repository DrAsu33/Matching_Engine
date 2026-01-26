mod models;
mod input;
mod engine_wrapper;

use engine_wrapper::EngineWrapper;

fn main() -> std::io::Result<()>
{
    let filename = "test.txt";
    let orders = input::load_orders_file(filename)?;
    
    // launch the engine
    let mut engine = EngineWrapper::new();
    for (index, order) in orders.iter().enumerate()
    {
        engine.place_order(order);
    }
    Ok(())
}