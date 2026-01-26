use crate::models::{Order, Side};
use std::ffi::c_void;

unsafe extern "C" {
    fn matching_engine_new() -> *mut c_void;
    fn matching_engine_free(ptr: *mut c_void);
    fn matching_engine_place_order(ptr: *mut c_void, side: u8, id: u64, price: u64, amount: u64);
    fn matching_engine_cancel_order(ptr: *mut c_void, id: u64);
}

pub struct EngineWrapper
{
    ptr : *mut c_void,
}

impl EngineWrapper
{
    pub fn new() -> Self
    {
        unsafe 
        {
            return EngineWrapper{ptr : matching_engine_new()};
        }
    }

    pub fn place_order(&mut self, order : &Order)
    {
        let side_raw : u8 = match order.side
        {
            Side::Bid => 0,
            Side::Ask => 1,
        };
        unsafe
        {
            matching_engine_place_order(self.ptr, side_raw, order.id, order.price, order.amount);
        }
    }

    pub fn cancel_order(&mut self, id : u64)
    {
        unsafe 
        {
            matching_engine_cancel_order(self.ptr, id);
        }
    }
}

impl Drop for EngineWrapper
{
    fn drop(&mut self)
    {
        unsafe
        {
            matching_engine_free(self.ptr);
        }
    }
}