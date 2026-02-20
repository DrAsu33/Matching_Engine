#pragma once
#include <cstdint>
#include <vector>

using Price = uint64_t;
using OrderId = uint64_t;
using Quantity = uint64_t;
using UserId = uint64_t;

// Order side indicator with fixed 1-byte underlying storage.
// The explicit uint8_t layout guarantees a stable binary
// representation for compact in-memory storage and
// serialization across engine boundaries.
enum class Side : uint8_t
{
    BID = 0,
    ASK = 1
};

// OrderNode is split into two structures sharing the same index:
//   - OrderCore  : hot fields used in matching (cache-friendly)
//   - OrderInfo  : cold fields (metadata)
// This separation improves cache locality during matching.
// `order_id` is the logical identifier, while the node index
// refers to its position in the internal array storage.

// Hot matching fields.
// Stored separately to keep the matching path cache-friendly.
// This structure is traversed frequently during order matching,
// so only fields required for quantity update and FIFO traversal
// are included here.
//
// `prev` and `next` are indices forming an intrusive linked list
// within the internal order pool.
struct OrderCore
{
    Quantity amount = 0;
    int32_t prev = -1;
    int32_t next = -1;
};
static_assert(sizeof(OrderCore) == 16, "OrderCore size must be strictly 16 bytes for Cache Line optimization!");

// Cold order metadata.
// Rarely accessed during the matching loop and therefore
// separated from OrderCore to avoid unnecessary cache line loads.
//
// Shares the same array index as its corresponding OrderCore.
struct OrderInfo
{
    OrderId id;
    UserId user_id;
    Price price;
};

// Snapshot of a completed trade.
// Generated after a successful match and used solely for
// logging or downstream reporting.
//
// This structure does not participate in matching logic.
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