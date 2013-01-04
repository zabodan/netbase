#pragma once
#include "core/smart_socket.h"
#include "core/ioservice_thread.h"


namespace core
{

    using namespace std::chrono;


    class ModP1 : public IProtocolListener, public ISocketStateObserver
    {
    public:

        ModP1() : receivedCount(0) {}

        size_t receivedCount;

    protected:

        void receive(const Connection& conn, const PacketPtr& packet) override
        {
            const PacketHeader& header = packet->header();
            LogDebug() << "processed packet" << header.seqNum << "with protocol" << header.protocol << "from" << conn.peer();
            ++receivedCount;
        }
    };



    class TestServer
    {
    public:

        TestServer(IOServiceThread& ioThread, size_t port, size_t maxTicks)
        {
            m_socket = std::make_shared<SmartSocket>(ioThread.getService(), port);
            ioThread.addResource(m_socket);

            m_socket->addObserver(std::make_shared<SocketStateLogger>());

            m_modP1 = std::make_shared<ModP1>();
            m_socket->registerProtocolListener(1, m_modP1);

            m_serverThread.reset(new std::thread([=]{ run(maxTicks); }));
        }

        ~TestServer()
        {
            m_serverThread->join();
        }

    private:

        void run(size_t maxTicks)
        {
            try
            {
                auto projectedTickStart = system_clock::now();
                for (size_t tick = 0; tick < maxTicks; ++tick)
                {
                    auto ts1 = system_clock::now();

                    m_modP1->receivedCount = 0;
                    m_socket->dispatchReceivedPackets();

                    if (m_modP1->receivedCount > 0)
                    {
                        auto packet = std::make_shared<Packet>(2);
                        m_socket->sendEveryone(packet);
                    }

                    if (tick > 0 && tick % 10 == 0)
                    {
                        static const uint16_t cHeartBitProtocol = 3;
                        m_socket->sendEveryone(std::make_shared<Packet>(cHeartBitProtocol));
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
        }

        SmartSocketPtr m_socket;
        std::unique_ptr<std::thread> m_serverThread;
        std::shared_ptr<ModP1> m_modP1;
    };

}
