#pragma once
#include "order.h"
#include <list>
#include <map>
#include <algorithm>
#include <unordered_map>
#include <variant>

// the core data structure of the engine
class MatchingEngine
{
public:
    using Price = uint64_t;
    using OrderId = uint64_t;
    using Quantity = uint64_t;
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

    Quantity match_bid(Price price, Quantity amount);
    Quantity match_ask(Price price, Quantity amount);
    inline void add_bid(OrderId id, Price price, Quantity amount);
    inline void add_ask(OrderId id, Price price, Quantity amount);


public:
    // Pre-allocating might help reduce runtime latency. Optimaizations needed later
    MatchingEngine() = default;
    void place_limit_order(Side side, OrderId id, Price price, Quantity amount);
    void cancel_order(OrderId id);
};