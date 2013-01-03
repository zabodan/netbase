#pragma once
#include "core/ioservice_resource.h"
#include <boost/asio/io_service.hpp>


namespace core {

    typedef boost::asio::io_service IOService;
    typedef std::shared_ptr<IOService> IOServicePtr;


    class IOServiceThread : private boost::noncopyable
    {
    public:

        IOServiceThread() : m_service(new IOService), m_keepalive(*m_service)
        {
            m_thread.reset(new std::thread([&]{ run(); }));
        }

        explicit IOServiceThread(const IOServicePtr& io) : m_service(io), m_keepalive(*m_service)
        {
            m_thread.reset(new std::thread([&]{ run(); }));
        }

        ~IOServiceThread()
        {
            m_service->stop();
            m_thread->join();
        }

        const IOServicePtr& getService() const
        {
            return m_service;
        }

        void addResource(const IOResourcePtr& resource)
        {
            m_resources.insert(resource);
        }

        void removeResource(const IOResourcePtr& resource)
        {
            m_resources.erase(resource);
        }

    private:

        void run();

        // shared ioservice
        IOServicePtr m_service;

        // as boost::asio reference says: create work item on ioservice
        // to make it run without any handles until stopped
        IOService::work m_keepalive;

        // underlying io thread
        std::unique_ptr<std::thread> m_thread;

        // dependent resources, which must be destroyed after ioservice thread stops
        std::set<std::shared_ptr<IOResource>> m_resources;
    };

    typedef std::shared_ptr<IOServiceThread> IOServiceThreadPtr;

}
