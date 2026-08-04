#pragma once
#include "matching_engine/order_queue.hpp"
#include "matching_engine/types.hpp"
#include <vector>

// Dense price-level index with a side-specific best-price sentinel. Valid prices
// are in the half-open interval [1, max_price).
template<Side side, Price max_price>
struct OrderBook
{
    std::vector<OrderQueue> buckets;
    Price best_price;
    OrderBook();
    inline void reset();
    inline bool empty() const;
    void add(std::vector<OrderCore>& pool, uint64_t price, int32_t node_idx);
    OrderQueue* get_best_queue();
};

template<Side side, Price max_price>
OrderBook<side, max_price>::OrderBook()
{
    buckets.resize(max_price);
    reset();
}

// Resets the cached sentinel. Call only when all price-level queues are empty.
template<Side side, Price max_price>
inline void OrderBook<side, max_price>::reset()
{
    if constexpr (side == Side::BID)
        best_price = 0;
    else 
        best_price = max_price;
}

template<Side side, Price max_price>
inline bool OrderBook<side, max_price>::empty() const
{
    if constexpr (side == Side::BID)
        return best_price == 0;
    else
        return best_price == max_price;
}

// Appends a node at its price level and updates the cached best price.
template<Side side, Price max_price>
void OrderBook<side, max_price>::add(std::vector<OrderCore>& pool, uint64_t price, int32_t node_idx)
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

// Returns the best non-empty price level, refreshing the cached price if necessary.
template<Side side, Price max_price>
OrderQueue* OrderBook<side, max_price>::get_best_queue() 
{
    if (empty()) [[unlikely]] 
        return nullptr;
    // The cached level remains valid for the common case.
    if (!buckets[best_price].empty()) {
        return &buckets[best_price];
    }
    // A depleted level triggers a directional scan for the next populated price.
    if constexpr (side == Side::BID) 
    {
        while (best_price > 0)
        {
            best_price--;
            if (best_price == 0) break;
            if (!buckets[best_price].empty()) return &buckets[best_price];
        }
    } 
    else 
    {
        while (best_price < max_price)
        {
            best_price++;
            if (best_price == max_price) break;
            if (!buckets[best_price].empty()) return &buckets[best_price];
        }
    }
    return nullptr;
}
