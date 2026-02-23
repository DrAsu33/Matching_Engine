fn main() {
    let mut build = cc::Build::new();

    // 1. 设置源文件与全局宏定义
    build.cpp(true)
         .file("src/engine.cpp")
         // 【关键新增】定义 NDEBUG 宏，彻底抹杀 C++ 代码中 assert 的运行时开销
         .define("NDEBUG", None);

    // 2. 获取编译器信息
    let compiler = build.get_compiler();

    // 3. 针对不同编译器设置 Flag
    if compiler.is_like_msvc() {
        // ============================
        // Windows MSVC 专用配置
        // ============================
        
        // 【关键修复】手动指定 C++20 标准，使用斜杠 /
        build.flag("/std:c++20");

        // 性能优化
        build.flag("/O2")       // 最大速度优化
             .flag("/Ob2")      // 激进内联
             .flag("/Oi")       // 启用内置函数
             .flag("/Ot")       // 代码速度优先
             .flag("/GL")       // 全程序优化 (LTO)
             .flag("/EHsc")     // 启用异常处理
             .flag("/MD");      // 使用多线程动态运行时

        // 指令集优化 (AVX2)
        build.flag("/arch:AVX2"); 

        // 屏蔽无关警告
        build.flag("/wd4244")
             .flag("/wd4267");
             
        println!("cargo:warning=Compiling with MSVC flags...");
    } else {
        // ============================
        // Linux/GCC/Clang 专用配置
        // ============================
        build.flag("-std=c++17") // Linux 下的标准写法
             .flag("-O3")
             .flag("-march=native")
             .flag("-Wall");
    }

    // 4. 执行编译
    build.compile("matching_engine_cpp");

    // 5. 监控文件变化
    println!("cargo:rerun-if-changed=src/engine.cpp");
    println!("cargo:rerun-if-changed=src/engine.h");
    println!("cargo:rerun-if-changed=src/order.h");
    println!("cargo:rerun-if-changed=src/orderbook.h");
    println!("cargo:rerun-if-changed=src/orderqueue.h");
}