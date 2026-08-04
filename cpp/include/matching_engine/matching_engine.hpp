#pragma once
#include "matching_engine/order_book.hpp"
#include "matching_engine/order_queue.hpp"
#include "matching_engine/types.hpp"

#include <cstddef>
#include <vector>

// Synchronous trade-event callback invoked on the matching thread.
using CallBackPtr = void (*)(TradeLog);

// Single-threaded, in-memory limit-order matching engine.
class MatchingEngine
{
public:
    constexpr static int32_t POOL_SIZE = 12000000;
    constexpr static OrderId MAX_ORDERS = 12000000;
    constexpr static Price MAX_PRICE = 100000;
    using OrderList = OrderQueue;
    // Asks are matched from the lowest price upward.
    using AsksMap = OrderBook<Side::ASK, MAX_PRICE>;
    // Bids are matched from the highest price downward.
    using BidsMap = OrderBook<Side::BID, MAX_PRICE>;

    // Direct-index entry for an active resting order.
    struct OrderLocation
    {
        // A negative pool index denotes an inactive or previously unused entry.
        int32_t pool_index = -1;
        Price price = 0;
        Side side = Side::BID;
        inline bool finished() const { return pool_index == -1; }
        inline void setfinish() { pool_index = -1; }
    };

private:
    AsksMap asks;
    BidsMap bids;

    CallBackPtr callback_fn_ptr = nullptr;

    // Hot and cold node storage share indices and are allocated once at startup.
    std::vector<OrderCore> order_core_pool;
    std::vector<OrderInfo> order_info_pool;
    OrderId order_capacity;
    int32_t pool_head = -1;
    int32_t alloc_node();
    void free_node(int32_t index);

    // Maps dense order identifiers directly to active pool locations.
    std::vector<OrderLocation> order_locations;

    Quantity match_bid(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount);
    Quantity match_ask(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount);
    inline void add_bid(OrderId oid, UserId uid, Price price, Quantity amount);
    inline void add_ask(OrderId oid, UserId uid, Price price, Quantity amount);

public:
    // The default constructor allocates the production capacities at startup.
    MatchingEngine();
    MatchingEngine(std::size_t pool_size, OrderId max_orders);
    void register_callback(CallBackPtr fn_ptr);
    void place_limit_order(Side side, OrderId id, UserId uid, Price price, Quantity amount);
    void cancel_order(OrderId id);
};
