#pragma once
#include "core/rw_spinlock.h"
#include <map>


namespace core {


    template<class Key, class Value>
    class ConcurrentMap
    {
    public:

        ConcurrentMap()
        {
            LogTrace() << "ConcurrentMap::ConcurrentMap";
        }

        ~ConcurrentMap()
        {
            LogTrace() << "ConcurrentMap::~ConcurrentMap";
        }

        void insert(const Key& key, const Value& value)
        {
            R4WSpinLock::WriteGuard guard(m_locker);
            m_data.insert(std::map<Key,Value>::value_type(key, value));
        }

        void remove(const Key& key)
        {
            R4WSpinLock::WriteGuard guard(m_locker);
            m_data.erase(key);
        }

        template <class Fn>
        void remove_if(const Fn& func)
        {
            R4WSpinLock::WriteGuard guard(m_locker);
            for (auto it = m_data.begin(); it != m_data.end(); )
            {
                if (func(it->second))
                    m_data.erase(it++);
                else
                    ++it;
            }
        }

        bool find(const Key& key, Value& outValue)
        {
            R4WSpinLock::ReadGuard guard(m_locker);
            auto it = m_data.find(key);
            if (it != m_data.end())
            {
                outValue = it->second;
                return true;
            }
            return false;
        }

        template <class Fn>
        void for_each_value(const Fn& func)
        {
            R4WSpinLock::ReadGuard guard(m_locker);
            for (auto it = m_data.begin(); it != m_data.end(); ++it)
                func(it->second);
        }

    private:

        R4WSpinLock m_locker;
        std::map<Key, Value> m_data;
    };


}
