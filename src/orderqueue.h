#pragma once
#include "order.h"

struct OrderQueue
{
    int32_t sentinel = -1; // sentinel node is used to eliminate branch predicition

    void init(std::vector<OrderNode>& pool, int32_t sentinel_index);
    void push_back(std::vector<OrderNode>& pool, int32_t new_index);
    void erase(std::vector<OrderNode>& pool, int32_t node_index);
    inline bool empty(const std::vector<OrderNode>& pool) const;
    inline int32_t begin(const std::vector<OrderNode>& pool) const;
    inline int32_t end() const;
};

// Check whether the queue is empty.
inline bool OrderQueue::empty(const std::vector<OrderNode>& pool) const 
{
        return pool[sentinel].next == sentinel;
}

// Get the first index of the list
inline int32_t OrderQueue::begin(const std::vector<OrderNode>& pool) const
{
    return pool[sentinel].next;
}

inline int32_t OrderQueue::end() const
{
    return sentinel;
}

// The initialization of th queue. Use alloc_node() as the second parameter
void OrderQueue::init(std::vector<OrderNode>& pool, int32_t sentinel_index)
{
    sentinel = sentinel_index;
    pool[sentinel].prev = sentinel;
    pool[sentinel].next = sentinel;
}

// Remember to initialize node[new_index] after this func
void OrderQueue::push_back(std::vector<OrderNode>& pool, int32_t new_index)
{
    int32_t rear = pool[sentinel].prev;
    pool[sentinel].prev = pool[rear].next = new_index;
    pool[new_index].next = sentinel;
    pool[new_index].prev = rear;
}

// erase the specific order according to its index
void OrderQueue::erase(std::vector<OrderNode>& pool, int32_t node_index)
{
    int32_t prevnode = pool[node_index].prev;
    int32_t nextnode = pool[node_index].next;
    pool[prevnode].next = nextnode;
    pool[nextnode].prev = prevnode;
}