#pragma once
#include "matching_engine/types.hpp"

#include <vector>

struct OrderQueue
{
    // The indices of the "list"
    int32_t head = -1;
    int32_t rear = -1;

    inline void push_back(std::vector<OrderCore>& pool, int32_t new_index);
    inline void pop_front(std::vector<OrderCore>& pool);
    inline bool empty() const;
    inline int32_t begin() const;
};

// Check whether the queue is empty.
inline bool OrderQueue::empty() const 
{
    return head == -1;
}

// Get the first index of the list
inline int32_t OrderQueue::begin() const
{
    return head;
}

// Remember to initialize node[new_index] after this func
inline void OrderQueue::push_back(std::vector<OrderCore>& pool, int32_t new_index)
{
    pool[new_index].next = -1;
    if(rear != -1) [[likely]] // not empty
        pool[rear].next = new_index;
    else
        head = new_index;
    rear = new_index;
}

// Pop the first node. The node should be freed manually
// First make sure not empty!!
inline void OrderQueue::pop_front(std::vector<OrderCore>& pool)
{
    head = pool[head].next;
    if(head == -1) [[unlikely]]
        rear = -1;
}
