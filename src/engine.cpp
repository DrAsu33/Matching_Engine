#include "engine.h"
#include <cassert>
#include <iostream>

// ==========================================
// 【HFT 极客宏】彻底解决跨平台与编译器版本问题
// ==========================================
#if defined(_MSC_VER) && !defined(__clang__)
    #include <xmmintrin.h>
    #define PREFETCH_T0(addr) _mm_prefetch(reinterpret_cast<const char*>(addr), _MM_HINT_T0)
#else
    // 【修复1】这里必须是 __builtin_prefetch，绝对不能是 PREFETCH_T0 导致死循环
    #define PREFETCH_T0(addr) __builtin_prefetch(addr, 0, 1)
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x) (x)
    #define UNLIKELY(x) (x)
#endif
// ==========================================

// get the callback function's ptr
inline void MatchingEngine::register_callback(CallBackPtr fn_ptr)
{
    callback_fn_ptr = fn_ptr;
}

// The constructor. Pre-allocating the memory pool of orders
MatchingEngine::MatchingEngine()
{
    order_core_pool.resize(POOL_SIZE);
    order_info_pool.resize(POOL_SIZE);
    order_locations.resize(MAX_ORDERS);
    // Pre-linking "nodes". Note that all "prev"s and the last "next" are set to -1 already
    for(size_t i = 0; i < POOL_SIZE - 1; i++)
        order_core_pool[i].next = (int32_t)(i + 1);
    pool_head = 0;
}

// Allocating an unused node and returns the index. Note that the data is not synced
int32_t MatchingEngine::alloc_node()
{
    int32_t index = pool_head;
    pool_head = order_core_pool[index].next;
    return index;
}

// Free the unused node. The latest freed node will be the first to be allocated (cache-friendly)
void MatchingEngine::free_node(int32_t index)
{
    order_core_pool[index].next = pool_head;
    pool_head = index;
}

// returns the remaining amount
Quantity MatchingEngine::match_bid(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount)
{
    // if matchable
    while(amount > 0)
    {
        // Calling the func get_best_queue assures the best_price is updated
        auto best_price_list_ptr = asks.get_best_queue();
        // If the map is empty or the price is unsatisfying
        if(best_price_list_ptr == nullptr || asks.best_price > price) break;

        // Prefetching best_order's info
        if (!best_price_list_ptr->empty())
            PREFETCH_T0(&order_info_pool[best_price_list_ptr->begin()]);

        // The inner loop that deals with the queue at the bestprice
        while(amount > 0 && !best_price_list_ptr->empty())
        {
            int32_t best_ask_index = best_price_list_ptr->begin();
            OrderCore& best_order = order_core_pool[best_ask_index];

            // This is where cancelled nodes are cleared up!
            if(best_order.amount == 0) [[unlikely]]
            {
                best_price_list_ptr->pop_front(order_core_pool);
                free_node(best_ask_index);
                // Prefetch the next node's info
                if (!best_price_list_ptr->empty())
                    PREFETCH_T0(&order_info_pool[best_price_list_ptr->begin()]);
                continue;             
            }
            OrderInfo& best_order_info = order_info_pool[best_ask_index];
            Quantity trade_amount = std::min(amount, best_order.amount);

            // Prefetching next best_order's info
            auto next_ask_index = best_order.next;
            if (next_ask_index != -1)
                PREFETCH_T0(&order_info_pool[next_ask_index]); 

            // form a log and call the callback function(if registered)
            if(callback_fn_ptr)
            {
                TradeLog log{best_order_info.id, best_order_info.user_id, taker_oid, taker_uid, best_order_info.price, trade_amount, Side::BID};
                callback_fn_ptr(log);
            }

            amount -= trade_amount;
            best_order.amount -= trade_amount;

            // if the best order was completed, it has to be deleted
            if(best_order.amount == 0) [[likely]]
            {
                assert(best_order_info.id < MAX_ORDERS);
                order_locations[best_order_info.id].setfinish();
                best_price_list_ptr->pop_front(order_core_pool);
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
    OrderCore& new_node_core = order_core_pool[new_index];
    OrderInfo& new_node_info = order_info_pool[new_index];
    new_node_info.id = oid;
    new_node_info.user_id = uid;
    new_node_info.price = price;
    new_node_core.amount = amount;

    bids.add(order_core_pool, price, new_index);
    assert(oid < MAX_ORDERS);
    order_locations[oid] = OrderLocation{new_index, price, Side::BID}; // The compiler shall optimize it. No temporary variable will be constructed
}

// returns the remaining amount
Quantity MatchingEngine::match_ask(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount)
{
    // if matchable
    while(amount > 0)
    {
        // Calling the func get_best_queue assures the best_price is updated
        auto best_price_list_ptr = bids.get_best_queue();
        // If the map is empty or the price is unsatisfying
        if(best_price_list_ptr == nullptr || bids.best_price < price) break;

        // Prefetching best_order's info
        if (!best_price_list_ptr->empty())
            PREFETCH_T0(&order_info_pool[best_price_list_ptr->begin()]);

        // The inner loop that deals with the queue at the bestprice
        while(amount > 0 && !best_price_list_ptr->empty())
        {
            int32_t best_bid_index = best_price_list_ptr->begin();
            OrderCore& best_order = order_core_pool[best_bid_index];

            // This is where cancelled nodes are cleared up!
            if(best_order.amount == 0)
            {
                best_price_list_ptr->pop_front(order_core_pool);
                free_node(best_bid_index);
                // Prefetch the next node's info
                if (!best_price_list_ptr->empty())
                    PREFETCH_T0(&order_info_pool[best_price_list_ptr->begin()]);
                continue;           
            }

            OrderInfo& best_order_info = order_info_pool[best_bid_index];
            Quantity trade_amount = std::min(amount, best_order.amount);

            // Prefetching
            auto next_bid_index = best_order.next;
            if (next_bid_index != -1)
                PREFETCH_T0(&order_info_pool[next_bid_index]); 

            // form a log and call the callback function(if registered)
            if(callback_fn_ptr)
            {
                // Taker side is ASK here
                TradeLog log{best_order_info.id, best_order_info.user_id, taker_oid, taker_uid, best_order_info.price, trade_amount, Side::ASK};
                callback_fn_ptr(log);
            }

            amount -= trade_amount;
            best_order.amount -= trade_amount;

            // if the best order was completed, it has to be deleted
            if(best_order.amount == 0) [[likely]]
            {
                assert(best_order_info.id < MAX_ORDERS);
                order_locations[best_order_info.id].setfinish();
                best_price_list_ptr->pop_front(order_core_pool);
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
    OrderCore& new_node_core = order_core_pool[new_index];
    OrderInfo& new_node_info = order_info_pool[new_index];
    new_node_info.id = oid;
    new_node_info.user_id = uid;
    new_node_info.price = price;
    new_node_core.amount = amount;

    asks.add(order_core_pool, price, new_index);
    assert(oid < MAX_ORDERS);
    order_locations[oid] = OrderLocation{new_index, price, Side::ASK}; // The compiler shall optimize it. No temporary variable will be constructed
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
    if(id >= MAX_ORDERS)
        return;

    OrderLocation& location = order_locations[id];
    if(location.finished())
        return;
    int32_t index = location.pool_index;
    // Lazy deletion: it shall be freed in match_bid(ask) function
    // Here it's only set to be 0 amount
    order_core_pool[index].amount = 0;
    location.setfinish();
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

    void matching_engine_place_orders_batch(MatchingEngine* self, FFIOrder* orders, size_t count)
    {
        if (!self || !orders) [[unlikely]] 
            return;

        for(size_t i = 0; i < count; i++)
        {
            FFIOrder& order = orders[i];
            self->place_limit_order(order.side, order.id, order.user_id, order.price, order.amount);
        }
    }
}