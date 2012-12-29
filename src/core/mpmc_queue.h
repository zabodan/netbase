#pragma once
#include <atomic>


namespace core {

#pragma pack (push, 1)
template <typename T>
struct mpmc_queue
{
private:

    // cache line size on modern x86 processors (in bytes)
    enum { CACHE_LINE_SIZE = 64 };

    struct node
    {
        explicit node(const T& val)
            : value(val), next(nullptr)
        {}
        
        T value;
        std::atomic<node*> next;
        char pad[CACHE_LINE_SIZE - sizeof(T)- sizeof(std::atomic<node*>)];
    };

    char pad0[CACHE_LINE_SIZE];

    // for one consumer at a time
    node* first;
    char pad1[CACHE_LINE_SIZE - sizeof(node*)];

    // shared among consumers
    std::atomic<bool> consumerLock;
    char pad2[CACHE_LINE_SIZE - sizeof(std::atomic<bool>)];

    // for one producer at a time
    std::atomic<node*> last;
    char pad3[CACHE_LINE_SIZE - sizeof(std::atomic<node*>)];

public:
    
    mpmc_queue()
    {
        first = last = new node(T());
        consumerLock = false;
    }

    ~mpmc_queue()
    {
        while (first != nullptr)
        {
            // release the list
            node* tmp = first;
            first = tmp->next;
            delete tmp;
        }
    }

    // aka produce
    void enqueue(const T& value)
    {
        node* tmp = new node(value);
        node* old = last.exchange(tmp, std::memory_order_acq_rel);
        old->next = tmp;
    }

    // aka consume
    bool dequeue(T& result)
    {
        while (consumerLock.exchange(true, std::memory_order_acquire)) {}

        node* theFirst = first;
        node* theNext = first->next;
        
        // if queue is nonempty
        if (first->next.load(std::memory_order_relaxed) != nullptr)
        {
            first = first->next;
            result = std::move(first->value);
            consumerLock.store(false, std::memory_order_release);

            delete theFirst;
            return true;
        }

        consumerLock.store(false, std::memory_order_release);
        return false;
    }
};
#pragma pack (pop)

}
