#pragma once
#include <cstdint>

using Price = uint64_t;
using OrderId = uint64_t;
using Quantity = uint64_t;
using UserId = uint64_t;

// Stable one-byte representation shared across the C ABI.
enum class Side : uint8_t
{
    BID = 0,
    ASK = 1
};

// Hot fields traversed by the matching loop. `next` links nodes in a
// singly linked FIFO queue using indices into the preallocated pool.
struct alignas(16) OrderCore
{
    Quantity amount = 0;
    int32_t next = -1;
};
static_assert(sizeof(OrderCore) == 16, "OrderCore must remain 16 bytes");

// Cold metadata stored at the same pool index as its corresponding OrderCore.
struct OrderInfo
{
    OrderId id;
    UserId user_id;
    Price price;
};

// Event emitted for each completed maker/taker match.
struct TradeLog
{
    OrderId maker_order_id;
    UserId maker_user_id;
    OrderId taker_order_id;
    UserId taker_user_id;
    Price price;
    Quantity amount;
    Side taker_side;
};
