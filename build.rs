fn main() {
    let mut build = cc::Build::new();

    // Compile both the matching core and its C ABI adapter. Contract assertions remain
    // enabled outside release builds to catch boundary violations during development.
    build
        .cpp(true)
        .include("cpp/include")
        .file("cpp/src/matching_engine.cpp")
        .file("cpp/src/c_api.cpp");
    if std::env::var("PROFILE").as_deref() == Ok("release") {
        build.define("NDEBUG", None);
    }

    let compiler = build.get_compiler();

    // Apply compiler-specific language and optimization flags.
    if compiler.is_like_msvc() {
        // MSVC requires C++20 for the branch-likelihood attributes used by the core.
        build.flag("/std:c++20");

        build
            .flag("/O2")
            .flag("/Ob2")
            .flag("/Oi")
            .flag("/Ot")
            .flag("/GL")
            .flag("/EHsc")
            .flag("/MD");

        // MSVC builds require AVX2-capable hosts.
        build.flag("/arch:AVX2");

        // Suppress conversion warnings for the current C ABI and pool-index conversions.
        build.flag("/wd4244").flag("/wd4267");

        println!("cargo:warning=Compiling with MSVC flags...");
    } else {
        build
            .flag("-std=c++17")
            .flag("-O3")
            .flag("-march=native")
            .flag("-Wall");
    }

    build.compile("matching_engine_cpp");

    // Rebuild the native library whenever a source file, public header, or profile changes.
    println!("cargo:rerun-if-changed=cpp/src/matching_engine.cpp");
    println!("cargo:rerun-if-changed=cpp/src/c_api.cpp");
    println!("cargo:rerun-if-changed=cpp/include/matching_engine/c_api.hpp");
    println!("cargo:rerun-if-changed=cpp/include/matching_engine/matching_engine.hpp");
    println!("cargo:rerun-if-changed=cpp/include/matching_engine/types.hpp");
    println!("cargo:rerun-if-changed=cpp/include/matching_engine/order_book.hpp");
    println!("cargo:rerun-if-changed=cpp/include/matching_engine/order_queue.hpp");
    println!("cargo:rerun-if-env-changed=PROFILE");
}
