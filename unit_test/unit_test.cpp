#include "stdafx.h"
#include "core/ack_utils.h"
#include "core/concurrent_queue.h"

#include "test_logger.h"
#include "test_packet_dispatcher.h"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>
#include <chrono>


using namespace core;


BOOST_AUTO_TEST_CASE(ack_util_test)
{
    BOOST_CHECK(moreRecentSeqNum(4, 2));
	BOOST_CHECK(!moreRecentSeqNum(2, 5));
    BOOST_CHECK(!moreRecentSeqNum(3,3));
	BOOST_CHECK(moreRecentSeqNum(2, 65530));
	BOOST_CHECK(!moreRecentSeqNum(65530, 5));

    ack33_t ack33; // templated case, one of { 9, 17, 33, 65 } 
    ack33.updateForSeqNum(1); BOOST_CHECK(ack33.ackBits() == 0x0001 && ack33.latestSeqNum() == 1); // bits = 00000001, ack = 1
    ack33.updateForSeqNum(3); BOOST_CHECK(ack33.ackBits() == 0x0006 && ack33.latestSeqNum() == 3); // bits = 00000110, ack = 3
    ack33.updateForSeqNum(2); BOOST_CHECK(ack33.ackBits() == 0x0007 && ack33.latestSeqNum() == 3); // bits = 00000111, ack = 3
    ack33.updateForSeqNum(7); BOOST_CHECK(ack33.ackBits() == 0x0078 && ack33.latestSeqNum() == 7); // bits = 01111000, ack = 7
    ack33.updateForSeqNum(6); BOOST_CHECK(ack33.ackBits() == 0x0079 && ack33.latestSeqNum() == 7); // bits = 01111001, ack = 7
    
    ack49_t ack49; // special case
    ack49.updateForSeqNum(1); BOOST_CHECK(ack49 == 0x00010001); // bits = 00000001, ack = 1
    ack49.updateForSeqNum(3); BOOST_CHECK(ack49 == 0x00060003); // bits = 00000110, ack = 3
    ack49.updateForSeqNum(2); BOOST_CHECK(ack49 == 0x00070003); // bits = 00000111, ack = 3
    ack49.updateForSeqNum(7); BOOST_CHECK(ack49 == 0x00780007); // bits = 01111000, ack = 7
    ack49.updateForSeqNum(6); BOOST_CHECK(ack49 == 0x00790007); // bits = 01111001, ack = 7

    std::vector<uint16_t> packets;
    ack49.forEachAckedSeqNum([&](uint16_t seqNum){
        packets.push_back(seqNum);
    });
    
    const uint16_t expected[] = { 7, 6, 3, 2, 1, 0 };
    BOOST_CHECK(packets.size() == sizeof(expected) / sizeof(uint16_t));
    BOOST_CHECK(std::equal(packets.begin(), packets.end(), expected));
}


BOOST_AUTO_TEST_CASE(mpsc_queue_test)
{
    mpsc_queue<int> queue;
    int tmp = 0;

    queue.push(1);
    queue.push(2);
    BOOST_CHECK(queue.pop(tmp) && tmp == 1);
    queue.push(3);
    BOOST_CHECK(queue.pop(tmp) && tmp == 2);
    BOOST_CHECK(queue.pop(tmp) && tmp == 3);
    BOOST_CHECK(!queue.pop(tmp));
}


BOOST_AUTO_TEST_CASE(mpmc_queue_test)
{
    mpmc_queue<int> queue;
    int tmp = 0;

    queue.push(1);
    queue.push(2);
    BOOST_CHECK(queue.pop(tmp) && tmp == 1);
    queue.push(3);
    BOOST_CHECK(queue.pop(tmp) && tmp == 2);
    BOOST_CHECK(queue.pop(tmp) && tmp == 3);
    BOOST_CHECK(!queue.pop(tmp));
}


BOOST_AUTO_TEST_CASE(logger_streaming)
{
    TestLogger testLog;

    testLog << "Hello World!";
    BOOST_CHECK(testLog.release() == " Hello World!");

    testLog << 0.1234;
    BOOST_CHECK(testLog.release() == " 0.1234");

    testLog << set_fixed(3) << 1.0 / 3.0;
    BOOST_CHECK(testLog.release() == " 0.333");

    testLog << set_fixed(3) << 0.3;
    BOOST_CHECK(testLog.release() == " 0.300");

    testLog << 65536 << -12345;
    BOOST_CHECK(testLog.release() == " 65536 -12345");

    testLog << std::chrono::milliseconds(10);
    BOOST_CHECK(testLog.release() == " 10ms");
}


BOOST_AUTO_TEST_CASE(packet_dispatcher)
{
    core::PacketDispatcher dispatcher;
    auto listener1 = std::make_shared<TestProtocolListener>();
    auto listener2 = std::make_shared<TestProtocolListener>();
    auto listener3 = std::make_shared<TestProtocolListener>();

    dispatcher.registerListener(10, listener1);
    dispatcher.registerListener(10, listener2);
    dispatcher.registerListener(20, listener2);
    dispatcher.registerListener(20, listener3);

    const udp::endpoint cDummyPeer;
    TestConnection testConn(cDummyPeer);

    auto p1 = std::make_shared<core::Packet>(10);
    auto p2 = std::make_shared<core::Packet>(20);
    auto p3 = std::make_shared<core::Packet>(30);
    BOOST_CHECK(p1->buffer().size() == sizeof(core::PacketHeader));

    core::PacketPtr tmp;
    
    dispatcher.dispatchPacket(testConn, p1);
    BOOST_CHECK(listener1->tryRelease(tmp) && tmp == p1);
    BOOST_CHECK(listener2->tryRelease(tmp) && tmp == p1);
    BOOST_CHECK(!listener3->tryRelease(tmp));

    dispatcher.dispatchPacket(testConn, p2);
    BOOST_CHECK(!listener1->tryRelease(tmp));
    BOOST_CHECK(listener2->tryRelease(tmp) && tmp == p2);
    BOOST_CHECK(listener3->tryRelease(tmp) && tmp == p2);

    dispatcher.dispatchPacket(testConn, p3);
    BOOST_CHECK(!listener1->tryRelease(tmp));
    BOOST_CHECK(!listener2->tryRelease(tmp));
    BOOST_CHECK(!listener3->tryRelease(tmp));

}