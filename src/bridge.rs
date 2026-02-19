use std::sync::mpsc::Sender;
use crate::models::TradeLog;

// 全局静态变量藏在这里
static mut GLOBAL_SENDER: Option<Sender<TradeLog>> = None;

// 设置全局发送端
pub fn set_global_sender(tx: Sender<TradeLog>) {
    unsafe {
        GLOBAL_SENDER = Some(tx);
    }
}

// 销毁全局发送端 (通知日志线程结束)
pub fn teardown_global_sender() {
    unsafe {
        GLOBAL_SENDER = None;
    }
}

// C++ 调用的回调函数
pub extern "C" fn callback(tradelog: TradeLog) {
    unsafe {
        let sender_ptr = &raw const GLOBAL_SENDER;
        if let Some(sender) = (*sender_ptr).as_ref() {
            // 忽略发送错误（防止主线程崩了导致这里 panic）
            let _ = sender.send(tradelog);
        }
    }
}