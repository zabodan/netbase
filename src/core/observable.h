#pragma once
#include "core/fast_spinlock.h"
#include <set>

namespace core {


    class IObservable
    {
    public:

        template <typename Observer>
        void addObserver(const std::shared_ptr<Observer>& observer);

        template <typename Observer>
        void removeObserver(const std::shared_ptr<Observer>& observer);

        template <typename Observer, typename... Args, typename... Params>
        void notifyObservers(void (Observer::*evt)(Args...), const Params&... params);

    protected:

        virtual ~IObservable() {}
    };


    template <class Observer>
    class Observable : public virtual IObservable
    {
    private:

        typedef std::shared_ptr<Observer> ObserverPtr;
        friend class IObservable;

        void add(const ObserverPtr& observer)
        {
            FastSpinLock::Guard guard(m_observersLock);
            m_observers.insert(observer);
        }

        void remove(const ObserverPtr& observer)
        {
            FastSpinLock::Guard guard(m_observersLock);
            m_observers.erase(observer);
        }

        template <typename... Args, typename... Params>
        void notify(void (Observer::*evt)(Args...), const Params&... params)
        {
            FastSpinLock::Guard guard(m_observersLock);
            for (auto& observer : m_observers)
            {
                (*observer.*evt)(params...);
            }
        }

        std::set<ObserverPtr> m_observers;
        FastSpinLock m_observersLock;
    };



    template <typename Observer>
    inline void IObservable::addObserver(const std::shared_ptr<Observer>& observer)
    {
        dynamic_cast<Observable<Observer>*>(this)->add(observer);
    }


    template <typename Observer>
    inline void IObservable::removeObserver(const std::shared_ptr<Observer>& observer)
    {
        dynamic_cast<Observable<Observer>*>(this)->remove(observer);
    }


    template <typename Observer, typename... Args, typename... Params>
    inline void IObservable::notifyObservers(void (Observer::*evt)(Args...), const Params&... params)
    {
        dynamic_cast<Observable<Observer>*>(this)->notify(evt, params...);
    }

}
