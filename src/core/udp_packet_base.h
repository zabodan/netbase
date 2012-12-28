#pragma once
#include <boost/asio/ip/udp.hpp>
#include <memory>
#include <cstdint>


namespace core
{
    using boost::asio::ip::udp;


    static const size_t cMaxUdpPacketSize = 512;


    struct UdpPacketHeader
    {
        uint16_t protocol;
        uint16_t seqNum;
        uint16_t ack;
        uint32_t ackBits;
    };


    class UdpPacketBase
    {
    public:

        UdpPacketBase(uint16_t protocol)
        {
            m_buffer.reserve(cMaxUdpPacketSize);
            m_buffer.resize(sizeof(UdpPacketHeader));
            header().protocol = protocol;
        }

        UdpPacketBase(uint8_t* data, size_t len)
        {
            if (len < sizeof(UdpPacketHeader))
                throw std::runtime_error("UdpPacketBase: input data size is too small");
            m_buffer.assign(data, data + len);
        }

        UdpPacketHeader& header()
        {
            return *reinterpret_cast<UdpPacketHeader*>(m_buffer.data());
        }

        std::vector<uint8_t>& buffer()
        {
            return m_buffer;
        }

        const UdpPacketHeader& header() const
        {
            return *reinterpret_cast<const UdpPacketHeader*>(m_buffer.data());
        }

        const std::vector<uint8_t>& buffer() const
        {
            return m_buffer;
        }

    private:

        std::vector<uint8_t> m_buffer;
    };


}
