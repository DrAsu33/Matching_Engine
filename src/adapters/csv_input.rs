// input

use crate::domain::{Order, Side};
use std::fs::File;
use std::io::{self, BufRead};

// Parser
pub fn load_orders_stream<R: BufRead>(reader: R) -> std::io::Result<Vec<Order>> {
    let mut orders: Vec<Order> = Vec::new();

    // parse data from one line to another
    for (index, line_res) in reader.lines().enumerate() {
        // read in each line
        let line = line_res?;
        let line = line.trim();
        if line.is_empty() {
            continue;
        }

        let parts: Vec<&str> = line.split(',').collect();
        // if there're less than 4 parts, throw out format error
        if parts.len() < 4 {
            eprintln!("Error[Line {}] : there are less than 4 cols.", index + 1);
            continue;
        }
        // read in the information in one line
        let user_id: u64 = match parts[0].trim().parse() {
            Ok(id) => id,
            Err(_) => {
                eprintln!(
                    "Error[Line {}] : User_id input has incorrect format",
                    index + 1
                );
                continue;
            }
        };

        let side: Side = match parts[1].trim() {
            "B" => Side::Bid,
            "A" => Side::Ask,
            _ => {
                eprintln!("Error[Line {}] : Invalid Side (must be B or A).", index + 1);
                continue;
            }
        };

        let price: u64 = match parts[2].trim().parse() {
            Ok(price) => price,
            Err(_) => {
                eprintln!(
                    "Error[Line {}] : Price input has incorrect format",
                    index + 1
                );
                continue;
            }
        };

        let amount: u64 = match parts[3].trim().parse() {
            Ok(amount) => amount,
            Err(_) => {
                eprintln!(
                    "Error[Line {}] : Amount input has incorrect format",
                    index + 1
                );
                continue;
            }
        };

        match Order::new((index + 1) as u64, user_id, side, price, amount) {
            Ok(order) => orders.push(order),
            Err(error) => eprintln!("Error[Line {}] : {}", index + 1, error),
        }
    }

    Ok(orders)
}

// read in data from a specific file
pub fn load_orders_file(filename: &str) -> std::io::Result<Vec<Order>> {
    // open the file according to the file name
    let file = File::open(filename)?;
    let reader = io::BufReader::new(file);
    load_orders_stream(reader)
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::io::Cursor;

    #[test]
    fn parses_two_valid_orders() {
        // Arrange：准备输入
        let csv = "\
1001, B, 100, 25
1002, A, 101, 10
";

        // Act：执行被测试代码
        let orders = load_orders_stream(Cursor::new(csv)).unwrap();

        // Assert：检查结果
        assert_eq!(orders.len(), 2);

        assert_eq!(orders[0].id(), 1);
        assert_eq!(orders[0].user_id(), 1001);
        assert_eq!(orders[0].side(), Side::Bid);
        assert_eq!(orders[0].price(), 100);
        assert_eq!(orders[0].amount(), 25);

        assert_eq!(orders[1].id(), 2);
        assert_eq!(orders[1].side(), Side::Ask);
    }

    #[test]
    fn skips_order_with_invalid_side() {
        let csv = "1001, X, 100, 25\n";

        let orders = load_orders_stream(Cursor::new(csv)).unwrap();

        assert!(orders.is_empty());
    }

    #[test]
    fn skips_order_with_zero_amount() {
        let csv = "1001, B, 100, 0\n";

        let orders = load_orders_stream(Cursor::new(csv)).unwrap();

        assert!(orders.is_empty());
    }

    #[test]
    fn skips_order_with_zero_price() {
        let csv = "1001, B, 0, 25\n";

        let orders = load_orders_stream(Cursor::new(csv)).unwrap();

        assert!(orders.is_empty());
    }

    #[test]
    fn skips_order_at_max_price() {
        let csv = "1001, B, 100000, 25\n";

        let orders = load_orders_stream(Cursor::new(csv)).unwrap();

        assert!(orders.is_empty());
    }
}
