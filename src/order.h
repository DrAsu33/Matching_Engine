#pragma once
#include <cstdint>

using Price = uint64_t;
using OrderId = uint64_t;
using Quantity = uint64_t;
using UserId = uint64_t;

// 1Byte, 0 for BID side and 1 for ASK side
enum class Side : uint8_t
{
    BID = 0, 
    ASK = 1
};

// struct order does not contain side since it's in the exact map
// contains order_id, price and amount
struct Order
{
    OrderId id;
    UserId user_id;
    Price price;
    Quantity amount;
};

// the information of trades
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