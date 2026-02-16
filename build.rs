// build.rs

fn main() {
    // 告诉 Cargo 如果这些文件变了，需要重新编译 C++ 代码
    println!("cargo:rerun-if-changed=src/engine.cpp");
    println!("cargo:rerun-if-changed=src/engine.h");
    println!("cargo:rerun-if-changed=src/order.h");

    cc::Build::new()
        .cpp(true) // 启用 C++ 模式
        .std("c++17") // 【关键】我们需要 C++17 特性
        .file("src/engine.cpp") // 指定源文件
        .flag("-O3")
        .flag("-march=native") // 使用当前 CPU 的 AVX/AVX2 指令集
        .compile("matching_engine_core"); // 编译成名为 libmatching_engine_core.a 的静态库
}