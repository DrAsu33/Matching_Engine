#include "engine.h"
#include <iostream>

// returns the remaining amount
MatchingEngine::Quantity MatchingEngine::match_bid(Price price, Quantity amount)
{
    // if matchable
    while(amount > 0 && !asks.empty() && price >= asks.begin()->first)
    {
        auto& best_price_list = asks.begin()->second;
        auto best_ask = best_price_list.begin();
        Order& best_order = *best_ask;

        Quantity trade_amount = std::min(amount, best_order.amount);
        // form a log. omitted here
        amount -= trade_amount;
        best_order.amount -= trade_amount;

        // if the best order was completed, it has to be deleted
        if(best_order.amount == 0)
        {
            OrderId oid = best_order.id;
            best_price_list.erase(best_ask);
            hashmap_id.erase(oid);
            if(best_price_list.empty())
                asks.erase(asks.begin());
        }
    }
    return amount; 
}

// add the bid order to the orderbook
inline void MatchingEngine::add_bid(OrderId id, Price price, Quantity amount)
{
    auto& level = bids[price];
    level.emplace_back(Order{id, price, amount});
    auto it = std::prev(level.end());
    hashmap_id[id] = OrderLocation{it, Side::BID, price};
}

// returns the remaining amount
MatchingEngine::Quantity MatchingEngine::match_ask(Price price, Quantity amount)
{
    // if matchable
    while (amount > 0 && !bids.empty() && price <= bids.begin()->first)
    {
        auto& best_price_list = bids.begin()->second;
        auto best_bid = best_price_list.begin();
        Order& best_order = *best_bid;

        Quantity trade_amount = std::min(amount, best_order.amount);
        // form a log. omitted here
        amount -= trade_amount;
        best_order.amount -= trade_amount;

        // if the best order was completed, it has to be deleted
        if (best_order.amount == 0)
        {
            OrderId oid = best_order.id;
            best_price_list.erase(best_bid);
            hashmap_id.erase(oid);
            if (best_price_list.empty())
                bids.erase(bids.begin());
        }
    }
    return amount;
}

// add the ask order to the orderbook
inline void MatchingEngine::add_ask(OrderId id, Price price, Quantity amount)
{
    auto& level = asks[price];
    level.emplace_back(Order{id, price, amount});
    auto it = std::prev(level.end());
    hashmap_id[id] = OrderLocation{it, Side::ASK, price};
}

// the main function placing an order
void MatchingEngine::place_limit_order(Side side, OrderId id, Price price, Quantity amount)
{
    Quantity remaining;
    if(side == Side::ASK)
    {
        remaining = match_ask(price, amount);
        if(remaining > 0)
            add_ask(id, price, remaining);
    }
    else
    {
        remaining = match_bid(price, amount);
        if(remaining > 0)
            add_bid(id, price, remaining);
    }
}
// cancel the specific order according to its id
void MatchingEngine::cancel_order(OrderId id)
{
    auto it = hashmap_id.find(id);
    if(it == hashmap_id.end())
        return;

    OrderLocation location = it->second;
    if(location.side == Side::BID)
    {
        auto level_it = bids.find(location.price);
        // On most occasions we don't need the following "if", can be modified to "assert" later
        if(level_it != bids.end())
        {
            auto& list = level_it->second;
            list.erase(location.order_iterator);
            if(list.empty())
                bids.erase(level_it);
        }
    }
    else // Side::ASK
    {
        auto level_it = asks.find(location.price);
        // same as above
        if (level_it != asks.end())
        {
            auto& list = level_it->second;
            list.erase(location.order_iterator);
            if (list.empty())
                asks.erase(level_it);
        }
    }

    hashmap_id.erase(it);
}

extern "C"
{
    MatchingEngine* matching_engine_new()
    {
        return new MatchingEngine();
    }

    void matching_engine_free(MatchingEngine* self)
    {
        delete self;
    }

    void matching_engine_place_order(MatchingEngine* self, uint8_t side_raw, uint64_t id, uint64_t price, uint64_t amount)
    {
        Side side;
        if(side_raw == 0)
            side = Side::BID;
        else if(side_raw == 1)
            side = Side::ASK;
        else // wrong parameter
        {
            std::cerr << "side conversion failed!" << std::endl;
            return; // wrong parameter
        }
            
        
        self->place_limit_order(side, id, price, amount);
    }

    void matching_engine_cancel_order(MatchingEngine* self, uint64_t id)
    {
        if(self != nullptr)
            self->cancel_order(id);
    }
}