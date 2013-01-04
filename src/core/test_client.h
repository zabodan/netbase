#pragma once
#include "core/smart_socket.h"
#include "core/ioservice_thread.h"


namespace core
{

    using namespace std::chrono;


    class TestClient
    {
    public:

        TestClient(IOServiceThread& ioThread, size_t port, size_t maxTicks)
        {
            m_socket = std::make_shared<SmartSocket>(ioThread.getService(), port);
            ioThread.addResource(m_socket);

            m_socket->addObserver(std::make_shared<SocketStateLogger>());

            udp::resolver resolver(*ioThread.getService());
		    udp::resolver::query serverQuery(udp::v4(), "localhost", "13999");
            udp::endpoint serverAddress = *resolver.resolve(serverQuery);

            m_conn = m_socket->getOrCreateConnection(serverAddress);
            m_thread.reset(new std::thread([=]{ run(maxTicks); }));
        }

        ~TestClient()
        {
            m_thread->join();
        }

    private:

        void run(size_t maxTicks)
        {
            LogTrace() << "[+] TestClient::run(" << maxTicks << ")";
            try
            {
                auto projectedTickStart = system_clock::now();
                for (size_t tick = 0; tick < maxTicks; ++tick)
                {
                    auto ts1 = system_clock::now();

                    if (m_conn->isDead())
                        break;

                    m_socket->dispatchReceivedPackets();

                    {
                        auto packet = std::make_shared<Packet>(1);
                        m_conn->asyncSend(packet);
                    }

                    auto ts2 = system_clock::now();
                    auto work_duration = duration_cast<milliseconds>(ts2 - ts1);

                    LogDebug() << "tick" << tick << "work done in" << work_duration;

                    projectedTickStart += milliseconds(50);
                    std::this_thread::sleep_until(projectedTickStart);
                }
            }
            catch (const std::exception& ex)
            {
                LogError() << ex.what();
            }
            LogTrace() << "[-] TestClient::run()";
        }

        SmartSocketPtr m_socket;
        ConnectionPtr m_conn;
        std::unique_ptr<std::thread> m_thread;
    };

}
