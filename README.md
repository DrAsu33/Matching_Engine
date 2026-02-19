# Matching Engine (Rust & C++ Hybrid)

这是一个高性能的、基于内存的撮合引擎，采用 **Rust** 作为上层业务逻辑与异步日志系统，核心撮合算法则通过 **C++17** 实现，以达到极致的性能表现。

## 核心特性

* **高性能核心**：核心撮合逻辑使用 C++ 编写，通过双向链表和预分配内存池（Intrusive Linked List & Memory Pool）实现  时间复杂度的订单处理。
* **Rust & C++ 混合编程**：利用 Rust 的安全性管理内存与多线程，通过 FFI (Foreign Function Interface) 调用 C++ 撮合内核。
* **低延迟异步日志**：基于 Rust 的 `mpsc` 频道实现异步日志落盘，撮合主线程与 IO 线程解耦，确保日志记录不阻塞撮合流程。
* **内存池管理**：预分配了可容纳 800 万个订单节点的内存池，减少运行时内存分配（malloc/free）带来的延迟开销。
* **指令集优化**：在编译阶段针对不同平台进行了深度优化，如在 Windows 环境下开启 AVX2 指令集优化。

## 系统架构

1. **撮合内核 (`src/engine.cpp`)**：维护买卖盘（OrderBook），实现限价单撮合与订单撤单逻辑。
2. **数据模型 (`src/models.rs`, `src/order.h`)**：定义了订单（Order）、侧向（Side）和成交日志（TradeLog）等跨语言对齐的基础结构。
3. **Rust 封装层 (`src/engine_wrapper.rs`)**：提供安全的 Rust 接口，负责与 C++ 底层进行通信和对象生命周期管理。
4. **日志系统 (`src/logger.rs`, `src/bridge.rs`)**：通过 FFI 回调机制，将 C++ 产生的成交信息实时回传给 Rust 异步线程进行持久化。

## 性能表现

项目包含内置的 Benchmark 工具，可直接对引擎进行压力测试。

* **测试入口**：`src/benchmark.rs`
* **指标**：支持统计总订单数、运行总时长、TPS（每秒成交数）以及单笔订单处理延迟（纳秒级）。

## 快速开始

### 前置条件

* **Rust**: 需支持 `edition 2024`。
* **C++ 编译器**:
* **Windows**: 推荐使用 MSVC (Visual Studio 2022) 或支持 AVX2 的 GCC/Clang。
* **Linux**: 需要支持 C++17 的 GCC 或 Clang。



### 编译与运行

1. **生成测试数据**：
使用提供的 Python 脚本生成 1000 万笔模拟订单数据：
```bash
python gen_data.py

```


2. **编译项目**：
```bash
cargo build --release

```


*注意：Release 模式下开启了 LTO（Fat）和 Codegen Units 优化以获得最高性能。*
3. **运行 Benchmark**：
```bash
cargo run --release

```



## 项目配置

在 `src/main.rs` 中可以调整以下配置：

* `ENABLE_LOGGING`: 是否开启异步日志记录。关闭日志可获得纯粹的撮合性能。
* `DATA_FILE`: 指定输入的 CSV 订单文件路径。
