#pragma once
#include "order.h"
#include "orderqueue.h"
#include "orderbook.h"

#include <algorithm>
#include <unordered_map>

// type of func ptr which receives TradeLog
using CallBackPtr = void* (*)(const TradeLog);


// the core data structure of the engine
class MatchingEngine
{
public:
    constexpr static int32_t POOL_SIZE = 8000000;
    constexpr static OrderId MAX_ORDERS = 12000000;
    constexpr static Price MAX_PRICE = 100000;
    using OrderList = OrderQueue;
    // the orderbook of the asks (ascending order)
    using AsksMap = OrderBook<Side::ASK, MAX_PRICE>;
    // the orderbook of the bids (descending order)
    using BidsMap = OrderBook<Side::BID, MAX_PRICE>;

    // the location of order
    struct OrderLocation
    {
        // The pool_index indicated the order's physical address in the orderpool
        // initialized as -1 to show that the order was finished or cancelled
        // (do not exist in the order pool)
        // side is BID by default which actually means nothing
        int32_t pool_index = -1;
        Price price = 0;
        Side side = Side::BID;
        inline bool finished() const{ return pool_index == -1; }
        inline void setfinish() { pool_index = -1; }
    };

private:
    // The 2 orderbooks we need
    AsksMap asks;
    BidsMap bids;

    // the ptr of the callback function
    CallBackPtr callback_fn_ptr = nullptr;

    // the ordernode pool (pre-allocated)
    std::vector<OrderCore> order_core_pool;
    std::vector<OrderInfo> order_info_pool;
    int32_t pool_head = -1;  // shall be initialized as 0 in constructor function
    int32_t alloc_node();
    void free_node(int32_t index);

    // Find the information of an order according to its OrderID
    std::vector<OrderLocation> order_locations;

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