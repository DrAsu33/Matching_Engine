#include "matching_engine/matching_engine.hpp"

#include <algorithm>
#include <cassert>

// Provide a compiler-portable read-prefetch hint.
#if defined(_MSC_VER) && !defined(__clang__)
    #include <xmmintrin.h>
    #define PREFETCH_T0(addr) _mm_prefetch(reinterpret_cast<const char*>(addr), _MM_HINT_T0)
#else
    #define PREFETCH_T0(addr) __builtin_prefetch(addr, 0, 1)
#endif

#if defined(__GNUC__) || defined(__clang__)
    #define LIKELY(x) __builtin_expect(!!(x), 1)
    #define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
    #define LIKELY(x) (x)
    #define UNLIKELY(x) (x)
#endif

void MatchingEngine::register_callback(CallBackPtr fn_ptr)
{
    callback_fn_ptr = fn_ptr;
}

// Default instances use the configured maximum capacities.
MatchingEngine::MatchingEngine()
    : MatchingEngine(POOL_SIZE, MAX_ORDERS)
{
}

MatchingEngine::MatchingEngine(std::size_t pool_size, OrderId max_orders)
    : order_capacity(max_orders)
{
    assert(pool_size > 0);
    assert(pool_size <= static_cast<std::size_t>(INT32_MAX));
    assert(max_orders > 0 && max_orders <= MAX_ORDERS);

    order_core_pool.resize(pool_size);
    order_info_pool.resize(pool_size);
    order_locations.resize(max_orders);

    // Link every pool node into the initial free list.
    for(std::size_t i = 0; i + 1 < pool_size; i++)
        order_core_pool[i].next = (int32_t)(i + 1);
    pool_head = 0;
}

// Removes the head of the free list. Precondition: the pool is not exhausted.
int32_t MatchingEngine::alloc_node()
{
    int32_t index = pool_head;
    pool_head = order_core_pool[index].next;
    return index;
}

// Returns a node to the free list using LIFO order for cache locality.
void MatchingEngine::free_node(int32_t index)
{
    order_core_pool[index].next = pool_head;
    pool_head = index;
}

// Matches an incoming bid against asks and returns its unfilled quantity.
Quantity MatchingEngine::match_bid(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount)
{
    while(amount > 0)
    {
        // get_best_queue refreshes the cached best price before returning.
        auto best_price_list_ptr = asks.get_best_queue();
        if(best_price_list_ptr == nullptr || asks.best_price > price) break;

        if (!best_price_list_ptr->empty())
            PREFETCH_T0(&order_info_pool[best_price_list_ptr->begin()]);

        while(amount > 0 && !best_price_list_ptr->empty())
        {
            int32_t best_ask_index = best_price_list_ptr->begin();
            OrderCore& best_order = order_core_pool[best_ask_index];

            // Reclaim a lazily cancelled node when matching encounters it at the head.
            if(best_order.amount == 0) [[unlikely]]
            {
                best_price_list_ptr->pop_front(order_core_pool);
                free_node(best_ask_index);
                if (!best_price_list_ptr->empty())
                    PREFETCH_T0(&order_info_pool[best_price_list_ptr->begin()]);
                continue;             
            }
            OrderInfo& best_order_info = order_info_pool[best_ask_index];
            Quantity trade_amount = std::min(amount, best_order.amount);

            auto next_ask_index = best_order.next;
            if (next_ask_index != -1)
                PREFETCH_T0(&order_info_pool[next_ask_index]); 

            if(callback_fn_ptr)
            {
                TradeLog log{best_order_info.id, best_order_info.user_id, taker_oid, taker_uid, best_order_info.price, trade_amount, Side::BID};
                callback_fn_ptr(log);
            }

            amount -= trade_amount;
            best_order.amount -= trade_amount;

            // Fully filled makers leave both the active index and the price queue.
            if(best_order.amount == 0) [[likely]]
            {
                assert(best_order_info.id < order_capacity);
                order_locations[best_order_info.id].setfinish();
                best_price_list_ptr->pop_front(order_core_pool);
                free_node(best_ask_index);
            }
        }
    }
    return amount; 
}

// Adds an unfilled bid to the back of its price-time-priority queue.
inline void MatchingEngine::add_bid(OrderId oid, UserId uid, Price price, Quantity amount)
{
    int32_t new_index = alloc_node();
    OrderCore& new_node_core = order_core_pool[new_index];
    OrderInfo& new_node_info = order_info_pool[new_index];
    new_node_info.id = oid;
    new_node_info.user_id = uid;
    new_node_info.price = price;
    new_node_core.amount = amount;

    bids.add(order_core_pool, price, new_index);
    assert(oid < order_capacity);
    order_locations[oid] = OrderLocation{new_index, price, Side::BID};
}

// Matches an incoming ask against bids and returns its unfilled quantity.
Quantity MatchingEngine::match_ask(OrderId taker_oid, UserId taker_uid, Price price, Quantity amount)
{
    while(amount > 0)
    {
        // get_best_queue refreshes the cached best price before returning.
        auto best_price_list_ptr = bids.get_best_queue();
        if(best_price_list_ptr == nullptr || bids.best_price < price) break;

        if (!best_price_list_ptr->empty())
            PREFETCH_T0(&order_info_pool[best_price_list_ptr->begin()]);

        while(amount > 0 && !best_price_list_ptr->empty())
        {
            int32_t best_bid_index = best_price_list_ptr->begin();
            OrderCore& best_order = order_core_pool[best_bid_index];

            // Reclaim a lazily cancelled node when matching encounters it at the head.
            if(best_order.amount == 0)
            {
                best_price_list_ptr->pop_front(order_core_pool);
                free_node(best_bid_index);
                if (!best_price_list_ptr->empty())
                    PREFETCH_T0(&order_info_pool[best_price_list_ptr->begin()]);
                continue;           
            }

            OrderInfo& best_order_info = order_info_pool[best_bid_index];
            Quantity trade_amount = std::min(amount, best_order.amount);

            auto next_bid_index = best_order.next;
            if (next_bid_index != -1)
                PREFETCH_T0(&order_info_pool[next_bid_index]); 

            if(callback_fn_ptr)
            {
                TradeLog log{best_order_info.id, best_order_info.user_id, taker_oid, taker_uid, best_order_info.price, trade_amount, Side::ASK};
                callback_fn_ptr(log);
            }

            amount -= trade_amount;
            best_order.amount -= trade_amount;

            // Fully filled makers leave both the active index and the price queue.
            if(best_order.amount == 0) [[likely]]
            {
                assert(best_order_info.id < order_capacity);
                order_locations[best_order_info.id].setfinish();
                best_price_list_ptr->pop_front(order_core_pool);
                free_node(best_bid_index);
            }
        }
    }
    return amount; 
}

// Adds an unfilled ask to the back of its price-time-priority queue.
inline void MatchingEngine::add_ask(OrderId oid, UserId uid, Price price, Quantity amount)
{
    int32_t new_index = alloc_node();
    OrderCore& new_node_core = order_core_pool[new_index];
    OrderInfo& new_node_info = order_info_pool[new_index];
    new_node_info.id = oid;
    new_node_info.user_id = uid;
    new_node_info.price = price;
    new_node_core.amount = amount;

    asks.add(order_core_pool, price, new_index);
    assert(oid < order_capacity);
    order_locations[oid] = OrderLocation{new_index, price, Side::ASK};
}

void MatchingEngine::place_limit_order(Side side, OrderId oid, UserId uid, Price price, Quantity amount)
{
    // Internal preconditions. The production Rust wrapper validates these values;
    // other C ABI callers must do so before entering the core.
    assert(oid < order_capacity);
    assert(price > 0 && price < MAX_PRICE);
    assert(amount > 0);

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

void MatchingEngine::cancel_order(OrderId id)
{
    // Cancellation is idempotent for out-of-range and inactive identifiers.
    if(id >= order_capacity)
        return;

    OrderLocation& location = order_locations[id];
    if(location.finished())
        return;
    int32_t index = location.pool_index;
    // Mark the node inactive; a later matching traversal reclaims it at the queue head.
    order_core_pool[index].amount = 0;
    location.setfinish();
}
