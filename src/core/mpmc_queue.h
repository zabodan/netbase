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
    node* m_tail;
    char pad1[CACHE_LINE_SIZE - sizeof(node*)];

    // shared among consumers
    std::atomic<bool> m_consumerLock;
    char pad2[CACHE_LINE_SIZE - sizeof(std::atomic<bool>)];

    // for one producer at a time
    std::atomic<node*> m_head;
    char pad3[CACHE_LINE_SIZE - sizeof(std::atomic<node*>)];

public:
    
    mpmc_queue()
    {
        m_head = m_tail = new node(T());
        m_consumerLock = false;
    }

    ~mpmc_queue()
    {
        while (m_tail != nullptr)
        {
            // release the list
            node* tmp = m_tail;
            m_tail = tmp->next;
            delete tmp;
        }
    }

    void push(const T& value)
    {
        node* tmp = new node(value);
        node* old = m_head.exchange(tmp, std::memory_order_acq_rel);
        old->next = tmp;
    }

    void push(T&& value)
    {
        node* tmp = new node(std::move(value));
        node* old = m_head.exchange(tmp, std::memory_order_acq_rel);
        old->next = tmp;
    }

    bool pop(T& result)
    {
        while (m_consumerLock.exchange(true, std::memory_order_acquire)) {}

        node* theTail = m_tail;
        node* theNext = m_tail->next;
        
        // if queue is nonempty
        if (theNext != nullptr)
        {
            result = std::move(theNext->value);
            m_tail = theNext;
            m_consumerLock.store(false, std::memory_order_release);

            delete theTail;
            return true;
        }

        m_consumerLock.store(false, std::memory_order_release);
        return false;
    }
};
#pragma pack (pop)

}
