use crate::bridge;
use crate::engine_wrapper::EngineWrapper;
use crate::models::TradeLog;
use std::fs::File;
use std::io::{BufWriter, Write};
use std::sync::mpsc;
use std::thread::{self, JoinHandle};

// 点火：启动日志线程，返回一个线程句柄（如果开启了日志的话）
pub fn start(engine: &mut EngineWrapper, enable: bool) -> Option<JoinHandle<()>> {
    if !enable {
        println!("[Config] Logging is DISABLED. Running in pure benchmark mode.");
        return None;
    }

    // 1. 建立通信管道
    let (tx, rx) = mpsc::channel::<TradeLog>();

    // 2. 告诉 C++ 往哪里发数据
    bridge::set_global_sender(tx);
    engine.regiser_fn_ptr(bridge::callback);

    // 3. 启动后台工人
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

// 熄火：清理战场，确保数据写完
pub fn stop(logger_handle: Option<JoinHandle<()>>) {
    if let Some(handle) = logger_handle {
        // 剪断数据源，让上面的 for 循环自然结束
        bridge::teardown_global_sender();
        // 乖乖等后台工人把活干完再下班
        handle.join().unwrap();
    }
}
