use super::Side;

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
