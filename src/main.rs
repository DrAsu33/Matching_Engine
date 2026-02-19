mod models;
mod input;
mod engine_wrapper;
mod bridge;     // 新增
mod logger;     // 新增
mod benchmark;  // 新增

use engine_wrapper::EngineWrapper;

// === 配置开关 ===
const ENABLE_LOGGING: bool = true; // 修改这里来开启/关闭日志
const DATA_FILE: &str = "test.csv";

fn main() -> std::io::Result<()> {
    // 1. 准备数据
    let orders = benchmark::load_data(DATA_FILE)?;

    // 2. 初始化引擎
    println!("Engine launching...");
    let mut engine = EngineWrapper::new();

    // 3. 启动日志系统 (把开关传进去，内部决定要不要真的启动)
    let logger_handle = logger::start(&mut engine, ENABLE_LOGGING);

    // 4. 运行跑分
    let result = benchmark::run(&mut engine, &orders);

    // 5. 打印结果
    benchmark::print_report(&result);
    println!("Engine Done!");

    // 6. 关闭日志系统并清理
    logger::stop(logger_handle);

    Ok(())
}