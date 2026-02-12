mod models;
mod input;
mod engine_wrapper;


use std::sync::mpsc::{self, Sender}; // for async inter-thread communication
use std::thread;

use engine_wrapper::EngineWrapper;
use std::time::Instant;// 引入计时器

use crate::models::TradeLog;

use std::fs::File;
use std::io::{BufWriter, Write}; // 引入 Write trait

static mut GLOBAL_SENDER: Option<Sender<TradeLog>> = None;

extern "C" fn callback(tradelog : TradeLog)
{
    unsafe 
    {
        // Take the address of the global sender without creating a Rust reference
        let sender_ptr = &raw const GLOBAL_SENDER;
        // Read the Option from the raw pointer and borrow the inner Sender (no move)
        if let Some(sender) = (*sender_ptr).as_ref()
        {
            let _ = sender.send(tradelog);
        }
    }
}


fn main() -> std::io::Result<()>
{
    // 1. 建立管道：tx 是发信口，rx 是收信箱
    let (tx, rx) = mpsc::channel::<TradeLog>();

    // 2. 把发信口存起来，给 C++ 用
    unsafe {
        GLOBAL_SENDER = Some(tx);
    }


    // 3. THE LOGGER THREAD STARTS!
    let logger = thread::spawn(move || {
        println!("[LogThread] 日志线程已启动，等待数据...");
        let mut total_volume = 0;

        // 1. 创建一个文件
        let file = File::create("trades.log").unwrap();
        // 2. 给文件套上“加速器”（缓冲区），比如 8KB 存满了一次性写硬盘
        let mut writer = BufWriter::new(file);
        
        // rx 会阻塞等待，直到主线程发来数据
        for trade in rx 
        {
            total_volume += trade.amount;
            
            // 3. 拼装字符串（或者直接写二进制）
            let log_line = format!("[Order: {:>6}] from User: {:>6} was taken by [Order: {:>6}] from User: {:>6}. Price: {:>6}, Amount: {:>6}\n", 
                trade.maker_order_id, 
                trade.maker_user_id,
                trade.taker_order_id, 
                trade.taker_user_id,
                trade.price, 
                trade.amount
            );

            // 4. 写入缓冲区（这比 println 快 1000 倍）
            writer.write_all(log_line.as_bytes()).unwrap();
        }
        // 循环结束后刷入硬盘
        writer.flush().unwrap();
        println!("[LogThread] 全部处理完毕。总成交量: {}", total_volume);
    });
    // 3. THE LOGGER THREAD ENDS!
    
    // 1. 读取数据 (不计入撮合时间，或者你可以分开看)
    let filename = "test.csv"; // 确保文件名对应
    println!("Loading orders from {}...", filename);
    let orders = input::load_orders_file(filename)?;
    println!("Loaded {} orders.", orders.len());
    
    // 2. 初始化引擎
    let mut engine = EngineWrapper::new();
    engine.regiser_fn_ptr(callback);
    println!("Starting Benchmark...");
    
    // 3. 开始计时
    let start = Instant::now();
    
    // 4. 疯狂塞单
    for order in &orders
    {
        engine.place_order(order);
    }
    
    // 5. 停止计时
    let duration = start.elapsed();
    
    // 6. 计算结果
    let total_seconds = duration.as_secs_f64();
    let tps = orders.len() as f64 / total_seconds;
    
    println!("--------------------------------------------------");
    println!("Done!");
    println!("Total Orders : {}", orders.len());
    println!("Time Elapsed : {:.6} seconds", total_seconds);
    println!("TPS (Orders/sec) : {:.2}", tps);
    println!("Latency per Order: {:.2} ns", duration.as_nanos() as f64 / orders.len() as f64);
    println!("--------------------------------------------------");
    println!("Engine Done!");

    // 【修改点 2】关键！必须手动销毁全局变量里的 Sender
    // 只有 Sender 被销毁了，日志线程里的 for 循环才会检测到 "Channel Closed"，从而退出循环
    unsafe {
        GLOBAL_SENDER = None; 
    }

    // 【修改点 3】删掉 sleep，改成 join
    // 意思：主线程卡在这里等，直到 log_handle 代表的线程执行结束
    // 如果通道里还有 100万条数据，主线程就会在这里乖乖等它写完
    logger.join().unwrap();

    println!("Log thread finished safely. Bye!");

    Ok(())
}