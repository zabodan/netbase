#pragma once
#include "core/packet.h"


namespace core {

    // tells whether n1 is more recent sequence number than n2
    // note: respects uint16 overflow
    inline bool moreRecentSeqNum(uint16_t n1, uint16_t n2)
    {
        static const uint16_t mid = std::numeric_limits<uint16_t>::max() / 2;

        // full check without overflows is: (n1 > n2 && n1 - n2 < mid) || (n1 < n2 && n2 - n1 > mid)
        // but if we do use integer overflow with operation-, it can be simplified
        return uint16_t(n1 - n2) < mid;
    }


    // simple helper for packet comparison
    inline bool moreRecentPacket(const Packet& p1, const Packet& p2)
    {
        return moreRecentSeqNum(p1.header().seqNum, p2.header().seqNum);
    }


    // get ack bit for seqNum = ack - delta, delta <= 32
    inline uint32_t ackBitFromDelta(uint16_t delta)
    {
        assert(delta <= 32);
        return 1 << (delta - 1);
    }


    // we got packet from remote host with seqNum, and want to update our acks
    //  ack     - latest acknowledged packet
    //  ackBits - 32 more acks, right after ack
    //  seqNum  - fresh packet sequence number
    inline void updateAcks(uint16_t& ack, uint32_t& ackBits, uint16_t seqNum)
    {
        if (ack == seqNum)
            return;

        if (moreRecentSeqNum(seqNum, ack))
        {
            // delta is how many packets we are missing between ack (latest acknowledged) and seqNum
            uint16_t delta = seqNum - ack;

            // if delta is small we just shift ackBits and do not forget to mark old ack's bit
            if (delta <= 32)
                ackBits = (ackBits << delta) | ackBitFromDelta(delta);
            else
                ackBits = 0;

            ack = seqNum;
        }
        else
        {
            // delta is how many packets were after seqNum
            uint16_t delta = ack - seqNum;

            if (delta <= 32) // delta = 1 corresponds to bit zero
                ackBits |= ackBitFromDelta(delta);
        }
    }

}
