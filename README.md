# Matching Engine (Rust & C++ Hybrid)

这是一个高性能的、基于内存的撮合引擎，采用 **Rust** 作为上层业务逻辑与异步日志系统，核心撮合算法则通过 **C++17** 实现，以达到极致的性能表现。

## 性能表现
在典型硬件配置下，本引擎表现出卓越的处理能力：
* **测试环境**：Intel Core i9-14900HX
* **吞吐量 (TPS)**：在**开启异步日志回调**的情况下，处理能力约为 **2300万单/秒 (2300w TPS)**。
* **处理延迟**：单笔订单处理延迟保持在**纳秒 (ns)** 级别。
> **注意**：性能受配置开关 `ENABLE_LOGGING` 影响。关闭日志可进入纯粹跑分模式，进一步降低延迟。

## 核心特性

* **高性能核心**：核心撮合逻辑使用 C++ 编写，通过双向链表和预分配内存池（Intrusive Linked List & Memory Pool）实现  时间复杂度的订单处理。
* **Rust & C++ 混合编程**：利用 Rust 的安全性管理内存与多线程，通过 FFI (Foreign Function Interface) 调用 C++ 撮合内核。
* **低延迟异步日志**：基于 Rust 的 `mpsc` 频道实现异步日志落盘，撮合主线程与 IO 线程解耦，确保日志记录不阻塞撮合流程。
* **内存池管理**：预分配了可容纳 800 万个订单节点的内存池，减少运行时内存分配（malloc/free）带来的延迟开销。
* **指令集优化**：在编译阶段针对不同平台进行了深度优化，如在 Windows 环境下开启 AVX2 指令集优化。

## 系统架构

1. **领域模型 (`src/domain`)**：定义订单、方向、成交和校验错误，不依赖 FFI、文件或线程。
2. **Rust 引擎接口 (`src/engine`)**：将 `unsafe` FFI、回调桥接和安全的引擎生命周期封装集中在语言边界。
3. **输入输出适配器 (`src/adapters`)**：负责 CSV 输入与异步成交日志，不进入撮合核心。
4. **应用层 (`src/application`)**：组织 benchmark 等用例，由 `src/main.rs` 完成依赖装配。
5. **C++ 撮合核心 (`cpp/include`, `cpp/src/matching_engine.cpp`)**：维护订单簿、订单队列与内存池。
6. **C ABI 适配层 (`cpp/src/c_api.cpp`)**：隔离 Rust/C++ 数据转换和导出函数，避免 C++ 核心依赖 Rust。

## 性能表现

项目包含内置的 Benchmark 工具，可直接对引擎进行压力测试。

* **测试入口**：`src/application/benchmark.rs`
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





## 核心设计细节

为了实现极致性能，本项目在数据结构和内存管理上进行了针对性优化：

### 1. 基于内存池的 Intrusive Linked List (侵入式链表)

传统的 `std::list` 会为每个节点单独分配内存，导致严重的内存碎片和缓存不命中。本引擎采用了侵入式设计：

* **结构集成**：`OrderNode` 结构体直接包含 `prev` 和 `next` 指针（在此实现中为 `int32_t` 类型的索引）。
* **内存池化**：所有节点存储在预分配的 `std::vector<OrderNode>` (名为 `order_pool`) 中。
* **优势**：
* ** 分配与释放**：通过 `pool_head` 管理空闲链表，`alloc_node` 和 `free_node` 仅需简单的索引交换。
* **缓存友好**：节点在物理内存中紧凑排列，大幅提升了 CPU 缓存命中率。
* **无分支哨兵**：`OrderQueue` 使用哨兵节点（Sentinel）来消除 `push` 和 `erase` 操作中的分支预测开销。



### 2. 使用 Vector 替代 Map/Unordered_map

在撮合引擎的 Hot Path（热路径）上，字典查找往往是瓶颈。本引擎通过空间换时间的策略消灭了哈希表和红黑树：

* **价格映射 (`Buckets`)**：由于价格范围已知且离散（`MAX_PRICE = 100,000`），我们直接使用 `std::vector<OrderQueue>`。每个价格点对应一个数组下标。
* **优点**：查找特定价格的队列只需一次内存偏移计算（），远快于 `std::map` 的  或 `std::unordered_map` 的哈希计算。


* **订单定位 (`OrderLocation`)**：通过 `std::vector<OrderLocation>` 存储所有活跃订单的物理位置。
* **实现**：使用 `OrderId` 直接作为 `vector` 索引。撤单时，通过 ID 直接定位到节点在 `order_pool` 中的下标进行 `erase`。



### 3. 价格搜索优化

* **Best Price 缓存**：`OrderBook` 维护了一个 `best_price` 变量。
* **线性扫描延迟更新**：当当前最优价的订单被吃完时，引擎会进行线性扫描以寻找下一个最优价。由于交易通常集中在盘口附近（高斯分布），这种扫描在实际场景中非常高效。
