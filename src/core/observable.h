#pragma once
#include "core/rw_spinlock.h"
#include <set>

namespace core {


    template <class Observer>
    class Observable
    {
    public:

        typedef std::shared_ptr<Observer> ObserverPtr;

        void addObserver(const ObserverPtr& observer)
        {
            R4WSpinLock::WriteGuard guard(m_locker);
            m_observers.insert(observer);
        }

        void removeObserver(const ObserverPtr& observer)
        {
            R4WSpinLock::WriteGuard guard(m_locker);
            m_observers.erase(observer);
        }

        template <class Subject>
        void notifyObservers(Subject subj)
        {
            R4WSpinLock::ReadGuard guard(m_locker);
            for (auto& observer : m_observers)
                subj(*observer);
        }

    protected:

        R4WSpinLock m_locker;
        std::set<ObserverPtr> m_observers;
    };

}
