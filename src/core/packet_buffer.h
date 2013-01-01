#pragma once
#include "core/packet.h"
#include "core/ack_utils.h"
#include <atomic>
#include <array>


namespace core {

    typedef std::chrono::system_clock::time_point SCTimePoint;


    struct PacketExt
    {
        PacketExt() : resendLimit(0)
        {}

        PacketExt(const PacketPtr& p, size_t resend, uint16_t seqNum, uint16_t ack, uint32_t ackBits)
            : packet(p), resendLimit(resend), timestamp(system_clock::now())
        {
            packet->header().seqNum = seqNum;
            packet->header().ack = ack;
            packet->header().ackBits = ackBits;
        }

        PacketPtr packet;
        
        // how many times to try and resend this packet
        size_t resendLimit;
        
        // timestamp to measure latency
        SCTimePoint timestamp;
    };

        
    template <size_t N>
    class SendPacketBuffer
    {
    public:

        SendPacketBuffer() : m_head(0), m_tail(1)
        {}

        PacketExt store(const PacketPtr& p, size_t resend, uint16_t ack, uint32_t ackBits)
        {
            uint16_t seqNum = ++m_head;
            PacketExt pExt = get(seqNum);
            get(seqNum) = PacketExt(p, resend, seqNum, ack, ackBits);
            return pExt;
        }

        PacketExt release(uint16_t seqNum)
        {
            PacketExt pExt = get(seqNum);
            get(seqNum).packet = nullptr;
            
            // we must release any packet only once
            assert(pExt.packet);
            
            // if this is oldest packet in buffer, advance tail
            if (seqNum == m_tail)
                updateTail();

            return pExt;
        }

        bool contains(uint16_t seqNum) const
        {
            return get(seqNum).packet != nullptr;
        }

        bool empty() const
        {
            return m_tail == m_head + 1;
        }

        uint16_t latestSeqNum() const
        {
            return m_head;
        }

        uint16_t oldestSeqNum() const
        {
            return m_tail;
        }

        const SCTimePoint& oldestTime() const
        {
            return get(m_tail).timestamp;
        }

    private:

        PacketExt& get(uint16_t seqNum) { return m_buffer[seqNum % N]; }
        const PacketExt& get(uint16_t seqNum) const { return m_buffer[seqNum % N]; }

        void updateTail()
        {
            while (!get(m_tail).packet && !moreRecentSeqNum(m_tail, m_head))
                ++m_tail;
        }

        uint16_t m_head;
        uint16_t m_tail;
        PacketExt m_buffer[N];
    };


    template <size_t N>
    class RecvPacketBuffer
    {
    public:
        
        RecvPacketBuffer() : m_head(0), m_tail(1) {}
            
        PacketPtr insert(uint16_t seqNum, const PacketPtr& value)
        {
            if (moreRecentSeqNum(seqNum, m_head))
                m_head = seqNum;
            if (moreRecentSeqNum(m_tail, seqNum))
                m_tail = seqNum;
            return m_buffer[seqNum % N].exchange(value);
        }

        bool empty() const
        {
            return moreRecentSeqNum(m_tail, m_head);
        }

        PacketPtr removeLast()
        {
            uint16_t seqNum = m_tail.fetch_add(1);
            return m_buffer[seqNum % N].exchange(nullptr);
        }

    private:
        std::atomic<uint16_t> m_head;
        std::atomic<uint16_t> m_tail;
        std::atomic<PacketPtr> m_buffer[N]; // spinlock-protected exchange
    };

}
