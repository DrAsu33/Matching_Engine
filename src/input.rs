// input

use std::fs::File;
use std::io::{self, BufRead};
use crate::models::{Order, Side};

// Parser
pub fn load_orders_stream<R: BufRead>(reader: R) -> std::io::Result<Vec<Order>>
{
    let mut orders: Vec<Order> = Vec::new();

    // parse data from one line to another
    for(index, line_res) in reader.lines().enumerate()
    {
        // read in each line
        let line = line_res?;
        let line = line.trim();
        if line.is_empty() { continue; }

        let parts: Vec<&str> = line.split(',').collect();
        // if there're less than 4 parts, throw out format error
        if parts.len() < 4
        {
            eprintln!("Error[Line {}] : there are less than 4 cols.", index + 1);
            continue;
        }
        // read in the information in one line
        let user_id : u64 = match parts[0].trim().parse()
        {
            Ok(id) => id,
            Err(_) =>
            {
                eprintln!("Error[Line {}] : User_id input has incorrect format", index + 1);
                continue;
            }
        };
        
        let side : Side = match parts[1].trim()
        {
            "B" => Side::Bid,
            "A" => Side::Ask,
            _ =>
            {
                eprintln!("Error[Line {}] : Invalid Side (must be B or S).", index + 1);
                continue;
            }
        };

        let price : u64 = match parts[2].trim().parse()
        {
            Ok(price) => price,
            Err(_) =>
            {
                eprintln!("Error[Line {}] : Price input has incorrect format", index + 1);
                continue;
            }
        };
        
        let amount : u64 = match parts[3].trim().parse()
        {
            Ok(amount) => amount,
            Err(_) =>
            {
                eprintln!("Error[Line {}] : Amount input has incorrect format", index + 1);
                continue;
            }
        };

        orders.push(Order{id : (index + 1) as u64, user_id, side, price, amount});
    }

    return Ok(orders);
}

// read in data from a specific file
pub fn load_orders_file(filename : &str) -> std::io::Result<Vec<Order>>
{
    // open the file according to the file name
    let file = match File::open(filename)
    {
        Ok(f) => f, 
        Err(e) => return Err(e),
    };
    let reader = io::BufReader::new(file);
    return load_orders_stream(reader);
}