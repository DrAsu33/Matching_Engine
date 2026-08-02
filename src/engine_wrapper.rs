use crate::models::{Order, Side, TradeLog};
use std::ffi::c_void;

pub type CallBackPtr = extern "C" fn(tradelog: TradeLog);

unsafe extern "C" {
    fn matching_engine_new() -> *mut c_void;
    #[cfg(test)]
    fn matching_engine_new_for_test(pool_size: usize, max_orders: u64) -> *mut c_void;
    fn matching_engine_free(ptr: *mut c_void);
    #[allow(dead_code)]
    fn matching_engine_place_order(
        ptr: *mut c_void,
        side: u8,
        oid: u64,
        uid: u64,
        price: u64,
        amount: u64,
    );
    #[allow(dead_code)]
    fn matching_engine_cancel_order(ptr: *mut c_void, id: u64);
    fn matching_engine_register_fn_ptr(ptr: *mut c_void, fn_ptr: CallBackPtr);

    fn matching_engine_place_orders_batch(
        ptr: *mut c_void,
        orders: *const crate::models::Order,
        count: usize,
    );
}

pub struct EngineWrapper {
    ptr: *mut c_void,
}

impl EngineWrapper {
    pub fn new() -> Self {
        unsafe {
            EngineWrapper {
                ptr: matching_engine_new(),
            }
        }
    }

    #[cfg(test)]
    fn new_for_test(pool_size: usize, max_orders: u64) -> Self {
        unsafe {
            EngineWrapper {
                ptr: matching_engine_new_for_test(pool_size, max_orders),
            }
        }
    }
    #[allow(dead_code)]
    pub fn place_order(&mut self, order: &Order) {
        let side_raw: u8 = match order.side() {
            Side::Bid => 0,
            Side::Ask => 1,
        };
        unsafe {
            matching_engine_place_order(
                self.ptr,
                side_raw,
                order.id(),
                order.user_id(),
                order.price(),
                order.amount(),
            );
        }
    }
    #[allow(dead_code)]
    pub fn cancel_order(&mut self, id: u64) {
        unsafe {
            matching_engine_cancel_order(self.ptr, id);
        }
    }
    #[allow(dead_code)]
    pub fn regiser_fn_ptr(&mut self, ptr: CallBackPtr) {
        unsafe {
            matching_engine_register_fn_ptr(self.ptr, ptr);
        }
    }

    pub fn place_orders_batch(&mut self, orders: &[crate::models::Order]) {
        unsafe {
            matching_engine_place_orders_batch(self.ptr, orders.as_ptr(), orders.len());
        }
    }
}

impl Drop for EngineWrapper {
    fn drop(&mut self) {
        unsafe {
            matching_engine_free(self.ptr);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::sync::Mutex;

    static TRADE_LOGS: Mutex<Vec<TradeLog>> = Mutex::new(Vec::new());

    extern "C" fn collect_trade(trade: TradeLog) {
        TRADE_LOGS.lock().unwrap().push(trade);
    }

    #[test]
    fn matches_by_price_then_fifo_and_preserves_remaining_quantity() {
        TRADE_LOGS.lock().unwrap().clear();

        let mut engine = EngineWrapper::new_for_test(16, 16);
        engine.regiser_fn_ptr(collect_trade);

        let orders = [
            Order::new(1, 1001, Side::Ask, 101, 5).unwrap(),
            Order::new(2, 1002, Side::Ask, 100, 5).unwrap(),
            Order::new(3, 1003, Side::Ask, 100, 5).unwrap(),
            Order::new(4, 1004, Side::Bid, 101, 12).unwrap(),
            Order::new(5, 1005, Side::Bid, 101, 3).unwrap(),
        ];

        engine.place_orders_batch(&orders);

        let trades = TRADE_LOGS.lock().unwrap();
        assert_eq!(trades.len(), 4);

        assert_trade(&trades[0], 2, 4, 100, 5);
        assert_trade(&trades[1], 3, 4, 100, 5);
        assert_trade(&trades[2], 1, 4, 101, 2);
        assert_trade(&trades[3], 1, 5, 101, 3);
        assert!(trades.iter().all(|trade| trade.taker_side == Side::Bid));
    }

    fn assert_trade(
        trade: &TradeLog,
        maker_order_id: u64,
        taker_order_id: u64,
        price: u64,
        amount: u64,
    ) {
        assert_eq!(trade.maker_order_id, maker_order_id);
        assert_eq!(trade.taker_order_id, taker_order_id);
        assert_eq!(trade.price, price);
        assert_eq!(trade.amount, amount);
    }
}
