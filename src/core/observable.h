#pragma once
#include "core/fast_spinlock.h"
#include <set>

namespace core {


    template <class Observer>
    class Observable
    {
    public:

        typedef std::shared_ptr<Observer> ObserverPtr;

        void addObserver(const ObserverPtr& observer)
        {
            FastSpinLock::Guard guard(m_locker);
            m_observers.insert(observer);
        }

        void removeObserver(const ObserverPtr& observer)
        {
            FastSpinLock::Guard guard(m_locker);
            m_observers.erase(observer);
        }

        template <class Subject>
        void notifyObservers(Subject subj)
        {
            FastSpinLock::Guard guard(m_locker);
            for (auto& observer : m_observers)
                subj(*observer);
        }

    protected:

        FastSpinLock m_locker;
        std::set<ObserverPtr> m_observers;
    };

}
