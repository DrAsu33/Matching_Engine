use crate::domain::TradeLog;
use std::sync::mpsc::Sender;

// The C callback currently carries no context pointer, so one process-wide sender
// bridges trade events to Rust. Callers must serialize sender updates with callback
// execution and permit only one active logger.
static mut GLOBAL_SENDER: Option<Sender<TradeLog>> = None;

/// Installs the sender used by the C++ trade callback.
pub fn set_global_sender(tx: Sender<TradeLog>) {
    // SAFETY: the current single-engine runtime installs the sender before matching
    // begins and does not mutate it while callbacks can execute.
    unsafe {
        GLOBAL_SENDER = Some(tx);
    }
}

/// Removes the sender so the logging thread can drain and terminate.
pub fn teardown_global_sender() {
    // SAFETY: the current runtime stops order processing before removing the sender.
    unsafe {
        GLOBAL_SENDER = None;
    }
}

/// Forwards a C++ trade event to the configured Rust logging channel.
pub extern "C" fn callback(tradelog: TradeLog) {
    // SAFETY: the current runtime does not update the sender during callback execution.
    // A closed channel indicates logger shutdown, so the send error is ignored.
    unsafe {
        let sender_ptr = &raw const GLOBAL_SENDER;
        if let Some(sender) = (*sender_ptr).as_ref() {
            let _ = sender.send(tradelog);
        }
    }
}
