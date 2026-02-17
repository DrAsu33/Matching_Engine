#include "engine.h"
#include <iostream>


// get the callback function's ptr
inline void MatchingEngine::register_callback(CallBackPtr fn_ptr)
{
    callback_fn_ptr = fn_ptr;
}

// The constructor. Pre-allocating the memory pool of orders
MatchingEngine::MatchingEngine()
{
    order_pool.resize(POOL_SIZE);
    // Pre-linking "nodes". Note that all "prev"s and the last "next" are set to -1 already
    for(size_t i = 0; i < POOL_SIZE - 1; i++)
        order_pool[i].next = (int32_t)(i + 1);
    pool_head = 0;
    // Assign sentinels
    for(auto& queue : asks.buckets)
        queue.init(order_pool, alloc_node());
    for(auto& queue : bids.buckets)
        queue.init(order_pool, alloc_node());
}

// Allocating an unused node and returns the index. Note that the data is not synced
int32_t MatchingEngine::alloc_node()
{
    int32_t index = pool_head;
    pool_head = order_pool[index].next;
    return index;
}

// Free the unused node. The latest freed node will be the first to be allocated (cache-friendly)
void MatchingEngine::free_node(int32_t index)
{
    order_pool[index].next = pool_head;
    order_pool[index].prev = -1;
    pool_head = index;
}

// returns the remaining amount
Quantity MatchingEngine::match_bid(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount)
{
    // if matchable
    while(amount > 0)
    {
        // Calling the func get_best_queue assures the best_price is updated
        auto best_price_list_ptr = asks.get_best_queue(order_pool);
        // If the map is empty or the price is unsatisfying
        if(best_price_list_ptr == nullptr || asks.best_price > price) break;

        // The inner loop that deals with the queue at the bestprice
        while(amount > 0)
        {
            // The queue is eaten up
            if(best_price_list_ptr->empty(order_pool)) break;

            auto best_ask_index = best_price_list_ptr->begin(order_pool);
            OrderNode& best_order = order_pool[best_ask_index];

            Quantity trade_amount = std::min(amount, best_order.amount);

            // form a log and call the callback function(if registered)
            if(callback_fn_ptr)
            {
                TradeLog log{best_order.id, best_order.user_id, taker_oid, taker_uid, best_order.price, trade_amount, Side::BID};
                callback_fn_ptr(log);
            }

            amount -= trade_amount;
            best_order.amount -= trade_amount;

            // if the best order was completed, it has to be deleted
            if(best_order.amount == 0)
            {
                best_price_list_ptr->erase(order_pool, best_ask_index);
                hashmap_id.erase(best_order.id);
                free_node(best_ask_index);
            }
        }
    }
    return amount; 
}

// add the bid order to the orderbook
inline void MatchingEngine::add_bid(OrderId oid, UserId uid, Price price, Quantity amount)
{
    // Initialize the new node
    int32_t new_index = alloc_node();
    OrderNode& new_node = order_pool[new_index];
    new_node.id = oid;
    new_node.user_id = uid;
    new_node.price = price;
    new_node.amount = amount;

    bids.add(order_pool, price, new_index);
    hashmap_id[oid] = OrderLocation{new_index, Side::BID, price};
}

// returns the remaining amount
Quantity MatchingEngine::match_ask(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount)
{
    // if matchable
    while(amount > 0)
    {
        // Calling the func get_best_queue assures the best_price is updated
        auto best_price_list_ptr = bids.get_best_queue(order_pool);
        // If the map is empty or the price is unsatisfying
        // Note: For selling, we break if the Best Bid is LOWER than our Ask price
        if(best_price_list_ptr == nullptr || bids.best_price < price) break;

        // The inner loop that deals with the queue at the bestprice
        while(amount > 0)
        {
            // The queue is eaten up
            if(best_price_list_ptr->empty(order_pool)) break;

            auto best_bid_index = best_price_list_ptr->begin(order_pool);
            OrderNode& best_order = order_pool[best_bid_index];

            Quantity trade_amount = std::min(amount, best_order.amount);

            // form a log and call the callback function(if registered)
            if(callback_fn_ptr)
            {
                // Taker side is ASK here
                TradeLog log{best_order.id, best_order.user_id, taker_oid, taker_uid, best_order.price, trade_amount, Side::ASK};
                callback_fn_ptr(log);
            }

            amount -= trade_amount;
            best_order.amount -= trade_amount;

            // if the best order was completed, it has to be deleted
            if(best_order.amount == 0)
            {
                best_price_list_ptr->erase(order_pool, best_bid_index);
                hashmap_id.erase(best_order.id);
                free_node(best_bid_index);
            }
        }
    }
    return amount; 
}

// add the ask order to the orderbook
inline void MatchingEngine::add_ask(OrderId oid, UserId uid, Price price, Quantity amount)
{
    // Initialize the new node
    int32_t new_index = alloc_node();
    OrderNode& new_node = order_pool[new_index];
    new_node.id = oid;
    new_node.user_id = uid;
    new_node.price = price;
    new_node.amount = amount;

    asks.add(order_pool, price, new_index);
    hashmap_id[oid] = OrderLocation{new_index, Side::ASK, price};
}

// the main function placing an order
void MatchingEngine::place_limit_order(Side side, OrderId oid, UserId uid, Price price, Quantity amount)
{
    Quantity remaining;
    if(side == Side::ASK)
    {
        remaining = match_ask(oid, uid, price, amount);
        if(remaining > 0)
        { 
            add_ask(oid, uid, price, remaining);
        }
    }
    else
    {
        remaining = match_bid(oid, uid, price, amount);
        if(remaining > 0)
        {
            add_bid(oid, uid, price, remaining);
        }
    }
}

// cancel the specific order according to its id
void MatchingEngine::cancel_order(OrderId id)
{
    // If the ID parameter is wrong
    auto it = hashmap_id.find(id);
    if(it == hashmap_id.end())
        return;

    OrderLocation location = it->second;
    if(location.side == Side::BID)
    {
        auto& queue = bids.buckets[location.price];
        queue.erase(order_pool, location.order_index);
        free_node(location.order_index);
    }
    else // Side::ASK
    {
        auto& queue = asks.buckets[location.price];
        queue.erase(order_pool, location.order_index);
        free_node(location.order_index);
    }

    hashmap_id.erase(it);
}

// The functions needed
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

    void matching_engine_place_order(MatchingEngine* self, uint8_t side_raw, uint64_t oid, uint64_t uid, uint64_t price, uint64_t amount)
    {
        if (!self)
        {
            std::cerr << "MatchingEngine pointer is null\n";
            return;
        }

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
        self->place_limit_order(side, oid, uid, price, amount);
    }

    void matching_engine_cancel_order(MatchingEngine* self, uint64_t id)
    {
        if(self != nullptr)
            self->cancel_order(id);
    }

    void matching_engine_register_fn_ptr(MatchingEngine* self, CallBackPtr fn_ptr)
    {
        if(self != nullptr)
            self->register_callback(fn_ptr);
    }
}