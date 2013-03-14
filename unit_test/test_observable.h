#pragma once
#include "core/observable.h"
#include <boost/optional.hpp>
#include <tuple>

class TestSimpleObserver
{
public:

    TestSimpleObserver() : m_simpleEventFired(false)
    {}

    void onSimpleEvent()
    {
        m_simpleEventFired = true;
    }

    bool simpleEventFired()
    {
        bool rv = m_simpleEventFired;
        m_simpleEventFired = false;
        return rv;
    }

private:
    bool m_simpleEventFired;
};


class TestComplexObserver
{
public:

    void onErrorEvent(const std::string& msg)
    {
        m_errorMessage = msg;
    }

    void onCustomEvent(int p1, int p2, double p3)
    {
        m_customEvent = std::make_tuple(p1, p2, p3);
    }

    bool tryGetError(std::string& msg)
    {
        if (m_errorMessage)
        {
            msg = *m_errorMessage;
            m_errorMessage.reset();
            return true;
        }

        return false;
    }

    bool tryGetTuple(std::tuple<int,int,double>& rv)
    {
        if (m_customEvent)
        {
            rv = *m_customEvent;
            m_customEvent.reset();
            return true;
        }

        return false;
    }


private:

    boost::optional<std::string> m_errorMessage;
    boost::optional<std::tuple<int,int,double>> m_customEvent;
};


class TestSubject :
    public core::Observable<TestSimpleObserver>,
    public core::Observable<TestComplexObserver>
{
public:

    void FireSimpleEvent()
    {
        notifyObservers(&TestSimpleObserver::onSimpleEvent);
    }
    
    void FireErrorEvent(const std::string& error)
    {
        notifyObservers(&TestComplexObserver::onErrorEvent, error);
    }

    void FireCustomEvent(int p1, int p2, double p3)
    {
        notifyObservers(&TestComplexObserver::onCustomEvent, p1, p2, p3);
    }
};

