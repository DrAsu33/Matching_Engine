#pragma once

#include "matching_engine/matching_engine.hpp"

#include <cstddef>
#include <cstdint>

// C ABI transfer object; field order and size must match Rust's repr(C) Order.
struct FFIOrder
{
    std::uint64_t id;
    std::uint64_t user_id;
    Side side;
    std::uint64_t price;
    std::uint64_t amount;
};
static_assert(sizeof(FFIOrder) == 40, "FFIOrder alignment mismatch with Rust");

extern "C"
{
    MatchingEngine* matching_engine_new();

#ifndef NDEBUG
    MatchingEngine* matching_engine_new_for_test(std::size_t pool_size, std::uint64_t max_orders);
#endif

    void matching_engine_free(MatchingEngine* self);
    void matching_engine_place_order(
        MatchingEngine* self,
        std::uint8_t side_raw,
        std::uint64_t oid,
        std::uint64_t uid,
        std::uint64_t price,
        std::uint64_t amount);
    void matching_engine_cancel_order(MatchingEngine* self, std::uint64_t id);
    void matching_engine_register_fn_ptr(MatchingEngine* self, CallBackPtr fn_ptr);
    void matching_engine_place_orders_batch(
        MatchingEngine* self,
        const FFIOrder* orders,
        std::size_t count);
}
