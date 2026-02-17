#pragma once
#include "order.h"
#include "orderqueue.h"
#include <vector>

// A cache-friendly OrderBook. Use OrderBook<Side::BID, MAX_PRICE> for bidsmap
// and OrderBook<Side::ASK, MAX_PRICE> for bidsmap. 
// Note that 0 and the MAX_PRICE are not valid numbers.
template<Side side, Price max_price>
struct OrderBook
{
    vector<OrderQueue> buckets;
    Price best_price;
    OrderBook();
    inline void reset();
    inline bool empty() const;
    void add(std::vector<OrderNode>& pool, uint64_t price, int32_t node_idx);
    OrderQueue* get_best_queue(std::vector<OrderNode>& pool);
};

// The sentinels shall be constructed in the constructor of the MatchingEngine
template<Side side, Price max_price>
OrderBook<side, max_price>::OrderBook()
{
    buckets.resize(max_price);
    reset()
}

// Reset the whole book
template<Side side, Price max_price>
inline void OrderBook<side, max_price>::reset()
{
    if constexpr (side == Side::BID)
        best_price = 0;
    else 
        best_price = max_price;
}

// Check whether the orderbook is empty
template<Side side, Price max_price>
inline bool OrderBook<side, max_price>::empty() const
{
    if constexpr (side == Side::BID)
        return best_price == 0;
    else
        return best_price == max_price;
}

// Add a new ordernode.
template<Side side, Price max_price>
void OrderBook<side, max_price>::add(std::vector<OrderNode>& pool, uint64_t price, int32_t node_idx)
{
    buckets[price].push_back(pool, node_idx);
    if constexpr (side == Side::BID) 
    {
        if (price > best_price)
            best_price = price;
    } 
    else 
    {
        if (price < best_price)
            best_price = price;
    }
}

// Get the best queue ptr
template<Side side, Price max_price>
OrderQueue* get_best_queue(std::vector<OrderNode>& pool) 
{
    if (empty()) [[unlikely]] 
        return nullptr;
    // The hot path
    if (!buckets[best_price].empty(pool)) {
        return &buckets[best_price];
    }
    // Cold path. linear scanning can find out the next best_price
    if constexpr (side == Side::BID) 
    {
        while (best_price > 0)
        {
            best_price--;
            if (best_price == 0) break;
            if (!buckets[best_price].empty(pool)) return &buckets[best_price];
        }
    } 
    else 
    {
        while (best_price < max_price)
        {
            best_price++;
            if (best_price == max_price) break;
            if (!buckets[best_price].empty(pool)) return &buckets[best_price];
        }
    }
    return nullptr; // the map is empty.
}