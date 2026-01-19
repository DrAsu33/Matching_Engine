mod models;
mod input;


fn main() -> std::io::Result<()>
{
    let filename = "test.txt";
    let _orders = input::load_orders_file(filename)?;
    // launch the engine

    Ok(())
}