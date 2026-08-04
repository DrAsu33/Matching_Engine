use crate::domain::{Order, TradeLog};
use std::ffi::c_void;

pub type CallBackPtr = extern "C" fn(tradelog: TradeLog);

// SAFETY: every declaration must remain ABI-compatible with
// cpp/include/matching_engine/c_api.hpp.
unsafe extern "C" {
    pub(super) fn matching_engine_new() -> *mut c_void;
    #[cfg(test)]
    pub(super) fn matching_engine_new_for_test(pool_size: usize, max_orders: u64) -> *mut c_void;
    pub(super) fn matching_engine_free(ptr: *mut c_void);
    pub(super) fn matching_engine_place_order(
        ptr: *mut c_void,
        side: u8,
        oid: u64,
        uid: u64,
        price: u64,
        amount: u64,
    );
    pub(super) fn matching_engine_cancel_order(ptr: *mut c_void, id: u64);
    pub(super) fn matching_engine_register_fn_ptr(ptr: *mut c_void, fn_ptr: CallBackPtr);
    pub(super) fn matching_engine_place_orders_batch(
        ptr: *mut c_void,
        orders: *const Order,
        count: usize,
    );
}
