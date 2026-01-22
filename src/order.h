#pragma once
#include <cstdint>

// struct order does not contain side since it's in the exact map
// contains order_id, price and amount
struct Order
{
    uint64_t id;
    uint64_t price;
    uint64_t amount;
};
