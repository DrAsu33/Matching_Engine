// Models
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Side
{
    Bid, 
    Ask,
}

#[derive(Debug)]
pub struct Order
{
    pub id: u64,
    pub user_id: u64,
    pub side: Side,
    pub price: u64,
    pub amount: u64
}
