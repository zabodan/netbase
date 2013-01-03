#pragma once
#include <atomic>
#include <map>


namespace core {


    template<class Key, class Value>
    class ConcurrentMap
    {
    public:
        enum { cMaxReaders = 8 };

        ConcurrentMap() : m_locker(cMaxReaders)
        {
            LogTrace() << "ConcurrentMap::ConcurrentMap()";
        }

        ~ConcurrentMap()
        {
            LogTrace() << "ConcurrentMap::~ConcurrentMap()";
        }

        void insert(const Key& key, const Value& value)
        {
            WriteLock guard(m_locker);
            m_data.insert(std::map<Key,Value>::value_type(key, value));
        }

        void remove(const Key& key)
        {
            WriteLock guard(m_locker);
            m_data.erase(key);
        }

        template <class Fn>
        void remove_if(const Fn& func)
        {
            WriteLock guard(m_locker);
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
            ReadLock guard(m_locker);
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
            ReadLock guard(m_locker);
            for (auto it = m_data.begin(); it != m_data.end(); ++it)
                func(it->second);
        }

    private:

        struct ReadLock
        {
            ReadLock(std::atomic<int>& locker) : m_locker(locker)
            {
                while (m_locker.fetch_sub(1) <= 0)
                    m_locker.fetch_add(1);
            }

            ~ReadLock()
            {
                m_locker.fetch_add(1);
            }

        private:
            std::atomic<int>& m_locker;
        };

        struct WriteLock
        {
            WriteLock(std::atomic<int>& locker) : m_locker(locker)
            {
                while (m_locker.fetch_sub(cMaxReaders) < cMaxReaders)
                    m_locker.fetch_add(cMaxReaders);
            }

            ~WriteLock()
            {
                m_locker.fetch_add(cMaxReaders);
            }

        private:
            std::atomic<int>& m_locker;
        };

        std::atomic<int> m_locker;
        std::map<Key, Value> m_data;
    };


}
