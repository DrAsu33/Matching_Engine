#include "matching_engine/c_api.hpp"

#include <iostream>

extern "C"
{
    MatchingEngine* matching_engine_new()
    {
        return new MatchingEngine();
    }

#ifndef NDEBUG
    MatchingEngine* matching_engine_new_for_test(std::size_t pool_size, std::uint64_t max_orders)
    {
        return new MatchingEngine(pool_size, max_orders);
    }
#endif

    void matching_engine_free(MatchingEngine* self)
    {
        delete self;
    }

    void matching_engine_place_order(
        MatchingEngine* self,
        std::uint8_t side_raw,
        std::uint64_t oid,
        std::uint64_t uid,
        std::uint64_t price,
        std::uint64_t amount)
    {
        if (!self)
        {
            std::cerr << "MatchingEngine pointer is null\n";
            return;
        }

        Side side;
        if (side_raw == 0)
            side = Side::BID;
        else if (side_raw == 1)
            side = Side::ASK;
        else
        {
            std::cerr << "side conversion failed!\n";
            return;
        }

        self->place_limit_order(side, oid, uid, price, amount);
    }

    void matching_engine_cancel_order(MatchingEngine* self, std::uint64_t id)
    {
        if (self != nullptr)
            self->cancel_order(id);
    }

    void matching_engine_register_fn_ptr(MatchingEngine* self, CallBackPtr fn_ptr)
    {
        if (self != nullptr)
            self->register_callback(fn_ptr);
    }

    void matching_engine_place_orders_batch(
        MatchingEngine* self,
        const FFIOrder* orders,
        std::size_t count)
    {
        if (!self || !orders) [[unlikely]]
            return;

        for (std::size_t i = 0; i < count; ++i)
        {
            const FFIOrder& order = orders[i];
            self->place_limit_order(
                order.side,
                order.id,
                order.user_id,
                order.price,
                order.amount);
        }
    }
}
