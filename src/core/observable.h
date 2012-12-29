#pragma once
#include <set>

namespace core {


    template <class Observer>
    class Observable
    {
    public:

        typedef std::shared_ptr<Observer> ObserverPtr;

        void addObserver(const ObserverPtr& observer)
        {
            m_observers.insert(observer);
        }

        void removeObserver(const ObserverPtr& observer)
        {
            m_observers.erase(observer);
        }

        template <class Subject>
        void notifyObservers(Subject subj)
        {
            for (auto& observer : m_observers)
                subj(*observer);
        }

    protected:

        std::set<ObserverPtr> m_observers;
    };

}
