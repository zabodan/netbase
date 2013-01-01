#pragma once
#include "core/logger.h"
#include <boost/asio/io_service.hpp>
#include <memory>
#include <thread>


namespace core {

    class IOServiceThread
    {
    public:

        IOServiceThread(boost::asio::io_service& service)
            : m_service(service)
        {
            m_thread.reset(new std::thread([&]{ run(); }));
        }

        ~IOServiceThread()
        {
            m_service.stop();
            m_thread->join();
        }

    private:

        void run()
        {
            //cDebug << "iothread: started ioservice event loop";
            try
            {
                m_service.run();
            }
            catch (const std::exception& ex)
            {
                cFatal << ex.what() << cWHERE;
            }
            catch (...)
            {
                cFatal << "unknown exception" << cWHERE;
            }
            //cDebug << "iothread: done";
        }

        boost::asio::io_service& m_service;
        std::unique_ptr<std::thread> m_thread;
    };

}
