// Models

// 1Byte, 0 for BID side and 1 for ASK side
#[repr(u8)]
#[derive(Debug, Clone, Copy, PartialEq)]
pub enum Side {
    Bid = 0,
    Ask = 1,
}

#[repr(C)]
#[derive(Debug)]
pub struct Order {
    pub id: u64,
    pub user_id: u64,
    pub side: Side,
    pub price: u64,
    pub amount: u64,
}

#[repr(C)]
#[derive(Debug, Clone, Copy)]
pub struct TradeLog {
    pub maker_order_id: u64,
    pub maker_user_id: u64,
    pub taker_order_id: u64,
    pub taker_user_id: u64,
    pub price: u64,
    pub amount: u64,
    pub taker_side: Side,
}
