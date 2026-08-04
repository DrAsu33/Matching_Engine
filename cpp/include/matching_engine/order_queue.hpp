#pragma once
#include "matching_engine/types.hpp"

#include <vector>

struct OrderQueue
{
    // Indices into the OrderCore pool; -1 denotes an empty queue.
    int32_t head = -1;
    int32_t rear = -1;

    inline void push_back(std::vector<OrderCore>& pool, int32_t new_index);
    inline void pop_front(std::vector<OrderCore>& pool);
    inline bool empty() const;
    inline int32_t begin() const;
};

inline bool OrderQueue::empty() const 
{
    return head == -1;
}

inline int32_t OrderQueue::begin() const
{
    return head;
}

// Appends a node while preserving FIFO order within the price level.
inline void OrderQueue::push_back(std::vector<OrderCore>& pool, int32_t new_index)
{
    pool[new_index].next = -1;
    if(rear != -1) [[likely]]
        pool[rear].next = new_index;
    else
        head = new_index;
    rear = new_index;
}

// Removes the head. Precondition: the queue is non-empty. The caller owns reclamation.
inline void OrderQueue::pop_front(std::vector<OrderCore>& pool)
{
    head = pool[head].next;
    if(head == -1) [[unlikely]]
        rear = -1;
}
