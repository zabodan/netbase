#pragma once
#include <atomic>


namespace core {

    
    template <int cMaxReaders>
    class RWSpinLockBase
    {
    public:

        struct ReadGuard
        {
            ReadGuard(RWSpinLockBase& owner);
            ~ReadGuard();
        private:
            std::atomic<int>& m_locker;
        };

        struct WriteGuard
        {
            WriteGuard(RWSpinLockBase& owner);
            ~WriteGuard();
        private:
            std::atomic<int>& m_locker;
        };

        // default ctr
        RWSpinLockBase() : m_locker(cMaxReaders) {}

    private:
        std::atomic<int> m_locker;
    };

    
    template <int N>
    inline RWSpinLockBase<N>::ReadGuard::ReadGuard(RWSpinLockBase& owner) : m_locker(owner.m_locker)
    {
        while (m_locker.fetch_sub(1) <= 0)
            m_locker.fetch_add(1);
    }

    template <int N>
    inline RWSpinLockBase<N>::ReadGuard::~ReadGuard()
    {
        m_locker.fetch_add(1);
    }

    template <int N>
    inline RWSpinLockBase<N>::WriteGuard::WriteGuard(RWSpinLockBase& owner) : m_locker(owner.m_locker)
    {
        while (m_locker.fetch_sub(N) < N)
            m_locker.fetch_add(N);
    }

    template <int N>
    inline RWSpinLockBase<N>::WriteGuard::~WriteGuard()
    {
        m_locker.fetch_add(N);
    }

    typedef RWSpinLockBase<4> R4WSpinLock;
}
