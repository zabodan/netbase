#pragma once
#include <atomic>


namespace core {

    // cache line size on modern x86 processors (in bytes)
    static const size_t cCacheLineSize = 64;


#pragma pack (push, 1)

    template <typename T>
    struct queue_base
    {
    protected:
        struct node
        {
            explicit node(const T& v) : value(v), next(nullptr) {}

            T value;
            std::atomic<node*> next;
        };
    };

    template <typename T>
    struct mpmc_queue : public queue_base<T>
    {
    public:
    
        mpmc_queue();
        ~mpmc_queue();

        void push(const T& value);
        bool pop(T& result);

    private:

        char pad0[cCacheLineSize];

        // for one consumer at a time
        node* m_first;
        char pad1[cCacheLineSize - sizeof(node*)];

        // shared among consumers
        std::atomic<bool> m_consumerLock;
        char pad2[cCacheLineSize - sizeof(std::atomic<bool>)];

        // for one producer at a time
        std::atomic<node*> m_last;
        char pad3[cCacheLineSize - sizeof(std::atomic<node*>)];
    };


    template <typename T>
    struct mpsc_queue : public queue_base<T>
    {
    public:
    
        mpsc_queue();
        ~mpsc_queue();

        void push(const T& value);
        bool pop(T& result);

    private:

        char pad0[cCacheLineSize];

        // for one consumer at a time
        node* m_first;
        char pad1[cCacheLineSize - sizeof(node*)];

        // for one producer at a time
        std::atomic<node*> m_last;
        char pad3[cCacheLineSize - sizeof(std::atomic<node*>)];
    };

#pragma pack (pop)


    template <typename T>
    inline mpmc_queue<T>::mpmc_queue()
    {
        m_last = m_first = new node(T());
        m_consumerLock = false;
    }

    template <typename T>
    inline mpmc_queue<T>::~mpmc_queue()
    {
        while (m_first != nullptr)
        {
            node* tmp = m_first;
            m_first = tmp->next;
            delete tmp;
        }
    }

    template <typename T>
    inline void mpmc_queue<T>::push(const T& value)
    {
        node* tmp = new node(value);
        node* old = m_last.exchange(tmp, std::memory_order_acq_rel);
        old->next = tmp;
    }

    template <typename T>
    inline bool mpmc_queue<T>::pop(T& result)
    {
        while (m_consumerLock.exchange(true, std::memory_order_acquire)) {}

        node* theFirst = m_first;
        node* theNext = m_first->next;
        
        // if queue is nonempty
        if (theNext != nullptr)
        {
            result = std::move(theNext->value);
            m_first = theNext;
            m_consumerLock.store(false, std::memory_order_release);

            delete theFirst;
            return true;
        }

        m_consumerLock.store(false, std::memory_order_release);
        return false;
    }



    template <typename T>
    inline mpsc_queue<T>::mpsc_queue()
    {
        m_last = m_first = new node(T());
    }

    template <typename T>
    inline mpsc_queue<T>::~mpsc_queue()
    {
        while (m_first != nullptr)
        {
            node* tmp = m_first;
            m_first = tmp->next;
            delete tmp;
        }
    }

    template <typename T>
    inline void mpsc_queue<T>::push(const T& value)
    {
        node* tmp = new node(value);
        node* old = m_last.exchange(tmp, std::memory_order_acq_rel);
        old->next = tmp;
    }

    template <typename T>
    inline bool mpsc_queue<T>::pop(T& result)
    {
        node* theFirst = m_first;
        node* theNext = m_first->next;
        
        // if queue is nonempty
        if (theNext != nullptr)
        {
            result = std::move(theNext->value);
            m_first = theNext;

            delete theFirst;
            return true;
        }
        return false;
    }


}
