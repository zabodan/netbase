#include "stdafx.h"
#include "core/ack_utils.h"
#include "core/concurrent_queue.h"

#define BOOST_TEST_MAIN
#include <boost/test/unit_test.hpp>

using namespace core;

BOOST_AUTO_TEST_CASE(ack_util_test)
{
    BOOST_CHECK(moreRecentSeqNum(4, 2));
	BOOST_CHECK(!moreRecentSeqNum(2, 5));
    BOOST_CHECK(!moreRecentSeqNum(3,3));
	BOOST_CHECK(moreRecentSeqNum(2, 65530));
	BOOST_CHECK(!moreRecentSeqNum(65530, 5));

	uint16_t ack = 0;		// yeah, we received packet zero by default
	uint32_t ackBits = 0;
	
	updateAcks(ack, ackBits, 1); BOOST_CHECK(ack == 1 && ackBits == 0x0001);	// 0000 0001
	updateAcks(ack, ackBits, 3); BOOST_CHECK(ack == 3 && ackBits == 0x0006);	// 0000 0110
	updateAcks(ack, ackBits, 2); BOOST_CHECK(ack == 3 && ackBits == 0x0007);	// 0000 0111
	updateAcks(ack, ackBits, 7); BOOST_CHECK(ack == 7 && ackBits == 0x0078);	// 0111 1000
	updateAcks(ack, ackBits, 6); BOOST_CHECK(ack == 7 && ackBits == 0x0079);	// 0111 1001
}


BOOST_AUTO_TEST_CASE(mpmc_queue_test)
{
    mpsc_queue<int> queue;
    int tmp = 0;

    queue.push(1);
    queue.push(2);
    queue.push(3);
    BOOST_CHECK(queue.pop(tmp) && tmp == 1);
    queue.push(4);
    BOOST_CHECK(queue.pop(tmp) && tmp == 2);
    BOOST_CHECK(queue.pop(tmp) && tmp == 3);
    BOOST_CHECK(queue.pop(tmp) && tmp == 4);
    BOOST_CHECK(!queue.pop(tmp));
}

