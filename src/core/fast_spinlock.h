#pragma once
#include <atomic>


namespace core {


    class FastSpinLock
    {
    public:

        struct Guard
        {
            Guard(FastSpinLock& owner) : m_locker(owner.m_locker)
            {
                while (m_locker.exchange(true))
                {}
            }
            
           ~Guard()
           {
               m_locker.store(false, std::memory_order_release);
           }

        private:
            std::atomic<bool>& m_locker;
        };


        FastSpinLock() : m_locker(false)
        {}

        //template <class Fn>
        //void run_protected(Fn& func)
        //{
        //    Guard guard(*this);
        //    func();
        //}

    private:
        std::atomic<bool> m_locker;
    };

}
