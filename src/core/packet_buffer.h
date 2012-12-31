#pragma once
#include "core/packet.h"
#include "core/ack_utils.h"
#include <atomic>
#include <array>


namespace core {


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
        std::array<std::atomic<PacketPtr>, N> m_buffer;
    };

}
