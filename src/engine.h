#pragma once
#include "order.h"
#include <list>
#include <map>
#include <algorithm>
#include <unordered_map>

// type of func ptr which receives TradeLog
using CallBackPtr = void* (*)(const TradeLog);

// the core data structure of the engine
class MatchingEngine
{
public:
    using OrderList = std::list<Order>;
    // the orderbook of the asks (ascending order)
    using AsksMap = std::map<Price, OrderList>;
    // the orderbook of the bids (descending order)
    using BidsMap = std::map<Price, OrderList, std::greater<Price>>;

    // the location of order
    struct OrderLocation
    {
        OrderList::iterator order_iterator;
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

    Quantity match_bid(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount);
    Quantity match_ask(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount);
    inline void add_bid(OrderId oid, UserId uid, Price price, Quantity amount);
    inline void add_ask(OrderId oid, UserId uid, Price price, Quantity amount);

public:
    // Pre-allocating might help reduce runtime latency. Optimaizations needed later
    MatchingEngine() = default;
    inline void register_callback(CallBackPtr fn_ptr);
    void place_limit_order(Side side, OrderId id, UserId uid, Price price, Quantity amount);
    void cancel_order(OrderId id);
};