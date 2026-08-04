use crate::domain::TradeLog;
use crate::engine::{EngineWrapper, callback};
use std::fs::File;
use std::io::{BufWriter, Write};
use std::sync::mpsc;
use std::thread::{self, JoinHandle};

/// Starts the asynchronous trade logger when logging is enabled.
pub fn start(engine: &mut EngineWrapper, enable: bool) -> Option<JoinHandle<()>> {
    if !enable {
        println!("[Config] Logging is DISABLED. Running in pure benchmark mode.");
        return None;
    }

    let (tx, rx) = mpsc::channel::<TradeLog>();

    callback::set_global_sender(tx);
    engine.regiser_fn_ptr(callback::callback);

    let handle = thread::spawn(move || {
        println!("[LogThread] 日志线程已启动...");
        let file = File::create("trades.log").unwrap();
        let mut writer = BufWriter::new(file);

        for trade in rx {
            let _ = writeln!(
                writer,
                "[Order: {:>6}] from User: {:>6} was taken by [Order: {:>6}] from User: {:>6}. Price: {:>6}, Amount: {:>6}",
                trade.maker_order_id,
                trade.maker_user_id,
                trade.taker_order_id,
                trade.taker_user_id,
                trade.price,
                trade.amount
            );
        }
        writer.flush().unwrap();
        println!("[LogThread] 日志落盘完毕。");
    });

    Some(handle)
}

/// Stops the logger after draining all queued trade events.
pub fn stop(logger_handle: Option<JoinHandle<()>>) {
    if let Some(handle) = logger_handle {
        // Dropping the final sender disconnects the channel; the receiver drains queued
        // events before exiting.
        callback::teardown_global_sender();
        handle.join().unwrap();
    }
}
