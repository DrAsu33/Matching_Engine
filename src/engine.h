#pragma once
#include "order.h"
#include "orderqueue.h"
#include <map>
#include <algorithm>
#include <unordered_map>

// type of func ptr which receives TradeLog
using CallBackPtr = void* (*)(const TradeLog);


// the core data structure of the engine
class MatchingEngine
{
public:
    using OrderList = OrderQueue;
    // the orderbook of the asks (ascending order)
    using AsksMap = std::map<Price, OrderList>;
    // the orderbook of the bids (descending order)
    using BidsMap = std::map<Price, OrderList, std::greater<Price>>;
    constexpr static int32_t POOL_SIZE = 10000000;

    // the location of order
    struct OrderLocation
    {
        int32_t order_index;
        Side side;
        Price price;
    };

private:
    AsksMap asks;
    BidsMap bids;
    // find any order according to a specific order_id
    std::unordered_map<OrderId, OrderLocation> hashmap_id;

    // the ptr of the callback function
    CallBackPtr callback_fn_ptr = nullptr;

    // the order pool (pre-allocated)
    std::vector<OrderNode> order_pool;
    int32_t pool_head = -1;  // shall be initialized as 0 in constructor func
    int32_t alloc_node();
    void free_node(int32_t index);

    Quantity match_bid(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount);
    Quantity match_ask(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount);
    inline void add_bid(OrderId oid, UserId uid, Price price, Quantity amount);
    inline void add_ask(OrderId oid, UserId uid, Price price, Quantity amount);

public:
    // Pre-allocating might help reduce runtime latency. Optimaizations needed later
    MatchingEngine();
    inline void register_callback(CallBackPtr fn_ptr);
    void place_limit_order(Side side, OrderId id, UserId uid, Price price, Quantity amount);
    void cancel_order(OrderId id);
};