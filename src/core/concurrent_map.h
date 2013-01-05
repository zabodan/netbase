#pragma once
#include "core/fast_spinlock.h"
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
            FastSpinLock::Guard guard(m_locker);
            m_data.insert(std::map<Key,Value>::value_type(key, value));
        }

        void remove(const Key& key)
        {
            FastSpinLock::Guard guard(m_locker);
            m_data.erase(key);
        }

        template <class Fn>
        void remove_if(const Fn& func)
        {
            FastSpinLock::Guard guard(m_locker);
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
            FastSpinLock::Guard guard(m_locker);
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
            FastSpinLock::Guard guard(m_locker);
            for (auto it = m_data.begin(); it != m_data.end(); ++it)
                func(it->second);
        }

    private:

        FastSpinLock m_locker;
        std::map<Key, Value> m_data;
    };


}
