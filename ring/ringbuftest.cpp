// #define RINGBUF_DEBUG 1
#include "ringbuf.c"
#include <gtest/gtest.h>

#define RING_BUFFER_SIZE1 32

TEST(RingTest, Init) {
    uint8_t buffer[RING_BUFFER_SIZE1];
    Ring ring = ringInit(buffer, sizeof(buffer));
    EXPECT_EQ(sizeof(buffer), ring.size);
    EXPECT_EQ(buffer, ring.data);
    EXPECT_EQ(0, ring.head);
    EXPECT_EQ(0, ring.tail);


    EXPECT_EQ(RING_STATUS_EMPTY, ringStatus(&ring));
    EXPECT_NE(RING_STATUS_FAIL, ringStatus(&ring));

    EXPECT_EQ(RING_BUFFER_SIZE1 - sizeof(RingPos_t) - 1, ringFree(&ring));
}

TEST(RingTest, InvalidData) {
    uint8_t buffer[RING_BUFFER_SIZE1];
    Ring ring = ringInit(NULL, sizeof(buffer));
    EXPECT_TRUE(ring.data == NULL);
    EXPECT_EQ(0, ring.size);

    ring = ringInit(buffer, sizeof(RingPos_t));
    EXPECT_TRUE(ring.data == NULL);
    EXPECT_EQ(0, ring.size);

    uint8_t d0[4] = {0,1,2,3};

    // ringPush
    RING_STATUS status = ringPush(NULL, d0, sizeof(d0));
    EXPECT_EQ(RING_STATUS_FAIL, status);
    status = ringPush(&ring, NULL, 1);
    EXPECT_EQ(RING_STATUS_FAIL, status);
    EXPECT_NE(RING_STATUS_OK, status);

    // ringFree
    EXPECT_EQ(0, ringFree(NULL));
    
    // ringStatus
    EXPECT_EQ(RING_STATUS_FAIL, ringStatus(NULL));

    // ringPeek
    EXPECT_EQ(0, ringPeek(NULL, (uint8_t **)&d0));
    EXPECT_EQ(0, ringPeek(&ring, NULL));

    // ringPop
    EXPECT_EQ(0, ringPop(NULL, (uint8_t *)d0, sizeof(d0)));
    EXPECT_EQ(0, ringPop(&ring, NULL, sizeof(d0)));
}

TEST(RingTest, Basic) {

    uint8_t buffer[RING_BUFFER_SIZE1];
    Ring ring = ringInit(buffer, sizeof(buffer));

    uint8_t d0[4] = {0,1,2,3};
    uint8_t d1[5] = {4,5,6,7,8};
    uint8_t dout[RING_BUFFER_SIZE1] = {};

    RING_STATUS status = ringPush(&ring, d0, 0);
    EXPECT_EQ(RING_STATUS_FAIL, status);
    EXPECT_NE(RING_STATUS_OK, status);

    status = ringPush(&ring, d0, sizeof(d0));
    EXPECT_EQ(RING_STATUS_OK, status);

    // Validate ringFree
    // The first bytes of a element contain the size of the data elem and is
    // the size and type RingPos_t.
    RingPos_t used = sizeof(d0) + sizeof(RingPos_t);
    RingPos_t free = RING_BUFFER_SIZE1 - used - sizeof(RingPos_t) - 1;
    EXPECT_EQ(free, ringFree(&ring));

    // Validate head/tail
    EXPECT_EQ(used, ring.head);
    EXPECT_EQ(0, ring.tail);

    // Ring peek
    uint8_t * d2 = NULL;
    RingPos_t size = ringPeek(&ring, &d2);
    EXPECT_EQ(sizeof(d0), size);
    ASSERT_TRUE(NULL != d2);
    EXPECT_EQ(0, d2[0]);
    EXPECT_EQ(1, d2[1]);
    EXPECT_EQ(2, d2[2]);
    EXPECT_EQ(3, d2[3]);

    // Validate 2nd ringPush
    status = ringPush(&ring, d1, sizeof(d1));
    EXPECT_EQ(RING_STATUS_OK, status);
    used += sizeof(d1) + sizeof(RingPos_t);
    free = RING_BUFFER_SIZE1 - used - sizeof(RingPos_t) - 1;
    // Validate head, tail, free
    EXPECT_EQ(free, ringFree(&ring));
    EXPECT_EQ(used, ring.head);
    EXPECT_EQ(0, ring.tail);

    // Validate ringPop
    size = ringPop(&ring, dout, sizeof(dout));
    EXPECT_EQ(4, size);
    EXPECT_EQ(0, dout[0]);
    EXPECT_EQ(1, dout[1]);
    EXPECT_EQ(2, dout[2]);
    EXPECT_EQ(3, dout[3]);
    EXPECT_EQ(sizeof(d0) + sizeof(RingPos_t), ring.tail);

    // Validate free, head, tail
    // Report largest contiguous regions of free memory so the previous pop
    // doesn't increase it
    free = RING_BUFFER_SIZE1 - used - sizeof(RingPos_t);
    EXPECT_EQ(free, ringFree(&ring));
    EXPECT_EQ(sizeof(d0) + sizeof(d1) + 2*sizeof(RingPos_t), ring.head);
    EXPECT_EQ(sizeof(d0) + sizeof(RingPos_t), ring.tail);

    // Validate ringpeek after ringPop
    d2 = NULL;
    size = ringPeek(&ring, &d2);
    EXPECT_EQ(sizeof(d1), size);
    ASSERT_TRUE(NULL != d2);
    EXPECT_EQ(4, d2[0]);
    EXPECT_EQ(5, d2[1]);
    EXPECT_EQ(6, d2[2]);
    EXPECT_EQ(7, d2[3]);
    EXPECT_EQ(8, d2[4]);

    size = ringPop(&ring, dout, sizeof(dout));
    EXPECT_EQ(5, size);
    EXPECT_EQ(4, dout[0]);
    EXPECT_EQ(5, dout[1]);
    EXPECT_EQ(6, dout[2]);
    EXPECT_EQ(7, dout[3]);
    EXPECT_EQ(8, dout[4]);

    // Validate free, head, tail
    EXPECT_EQ(sizeof(d0) + sizeof(d1) + 2*sizeof(RingPos_t), ring.head);
    EXPECT_EQ(ring.head, ring.tail);
    free = RING_BUFFER_SIZE1 - sizeof(RingPos_t) - ring.head;
    EXPECT_EQ(free, ringFree(&ring));

    // ringPop w/o available data
    size = ringPop(&ring, dout, sizeof(dout));
    EXPECT_EQ(0, size);
    EXPECT_EQ(sizeof(d0) + sizeof(d1) + 2*sizeof(RingPos_t), ring.head);
    EXPECT_EQ(sizeof(d0) + sizeof(d1) + 2*sizeof(RingPos_t), ring.tail);
    EXPECT_EQ(RING_STATUS_EMPTY, ringStatus(&ring));

    EXPECT_EQ(free, ringFree(&ring));

    // ringPeek w/o available data
    d2 = NULL;
    EXPECT_EQ(0, ringPeek(&ring, &d2));
    ASSERT_TRUE(NULL == d2);

}

TEST(RingTest, Wrapping) {
    uint8_t buffer[RING_BUFFER_SIZE1] = {};
    uint8_t d0[] = {1,2,3,4,5,6,7,8};
    uint8_t d1[] = {1,2,3,4,5,6,7,8,9};
    uint8_t dout[RING_BUFFER_SIZE1] = {};
    uint8_t *d2 = NULL;
    RingPos_t used = 0;
    RingPos_t size = 0;
    RING_STATUS status = RING_STATUS_FAIL;
    Ring ring = ringInit(buffer, sizeof(buffer));

    // Start to fill the ring
    status = ringPush(&ring, d0, sizeof(d0));
    used += sizeof(d0) + sizeof(RingPos_t);
    EXPECT_EQ(RING_STATUS_OK, status);

    status = ringPush(&ring, d0, sizeof(d0));
    used += sizeof(d0) + sizeof(RingPos_t);
    EXPECT_EQ(RING_STATUS_OK, status);
    EXPECT_EQ(used, ring.head);

    // Create some space at the front of the ring
    // ringPop just drops data when a NULL ptr is provided for outbuf
    size = ringPop(&ring, NULL, sizeof(dout));
    EXPECT_EQ(sizeof(d0), size);
    EXPECT_EQ(sizeof(d0) + sizeof(RingPos_t), ring.tail);
    EXPECT_EQ(2*(sizeof(d0) + sizeof(RingPos_t)), ring.head);
    EXPECT_EQ(
            RING_BUFFER_SIZE1 - sizeof(RingPos_t) - 2*(sizeof(d0) + sizeof(RingPos_t)),
            ringFree(&ring));

    // Nearly fill the ring to the back. Only leave 1 byte at the end of the ring
    status = ringPush(&ring, d1, sizeof(d1));
    used += sizeof(d1) + sizeof(RingPos_t);
    EXPECT_EQ(RING_STATUS_OK, status);
    EXPECT_EQ(RING_BUFFER_SIZE1-1, ring.head);
    EXPECT_EQ(sizeof(d0) + sizeof(RingPos_t), ring.tail);
    EXPECT_EQ(sizeof(d0)-1, ringFree(&ring));

    // Ensure the head can't move to the same spot as the tail when pushing
    // This means there is one byte that can never be used
    status = ringPush(&ring, d0, sizeof(d0));
    EXPECT_EQ(RING_STATUS_FAIL, status);

    // Ensure head moves to front when pushing data without enough room at the
    // back of the ring
    status = ringPush(&ring, d0, sizeof(d0)-1);
    EXPECT_EQ(RING_STATUS_OK, status);
    EXPECT_EQ(0, ringFree(&ring));
    EXPECT_EQ(sizeof(d0)-1 + sizeof(RingPos_t), ring.head);
    EXPECT_EQ(sizeof(d0) + sizeof(RingPos_t), ring.tail);

    // Test contiguous data wrapping when there is room at the front of the ring
    // but not the back
    size = ringPop(&ring, dout, sizeof(dout));
    EXPECT_EQ(sizeof(d0), size);
    size = ringPop(&ring, dout, sizeof(dout));
    EXPECT_EQ(sizeof(d1), size);

    // Tail needs to wrap back to zero when there isn't room for another
    // RingPos_t size element
    size = ringPop(&ring, dout, sizeof(dout));
    EXPECT_EQ(sizeof(d0)-1, size);
    EXPECT_EQ(d0[0], dout[0]);
    EXPECT_EQ(d0[1], dout[1]);
    EXPECT_EQ(d0[2], dout[2]);
    EXPECT_EQ(d0[3], dout[3]);
    EXPECT_EQ(d0[4], dout[4]);
    EXPECT_EQ(d0[5], dout[5]);
    EXPECT_EQ(d0[6], dout[6]);
    EXPECT_EQ(d0[7], dout[7]);

    // Verify the ring is empty when head and tail are at the same point (but
    // not at the front)
    EXPECT_EQ(RING_STATUS_EMPTY, ringStatus(&ring));
    EXPECT_EQ(RING_BUFFER_SIZE1 - ring.head - sizeof(RingPos_t), ringFree(&ring));

    EXPECT_EQ(RING_STATUS_OK, ringPush(&ring, d1, sizeof(d1)));
    EXPECT_EQ(sizeof(d0)-1 + sizeof(RingPos_t), ring.tail);
    EXPECT_EQ(sizeof(d0)-1 + sizeof(d1) + 2*sizeof(RingPos_t), ring.head);
    EXPECT_EQ(RING_BUFFER_SIZE1 - ring.head - sizeof(RingPos_t), ringFree(&ring));

    // Add some more data
    EXPECT_EQ(RING_STATUS_OK, ringPush(&ring, d1, sizeof(d1)));

    // Make a space at the front that's larger than the space at the back
    size = ringPop(&ring, dout, sizeof(dout));

    // Test head wrapping because not enough data at back of buffer
    // Push data that doesn't fit at the back but fits in the front
    EXPECT_EQ(RING_STATUS_OK, ringPush(&ring, d0, sizeof(d0)));
    
    // Ensure ringInit overwirtes the data
    uint8_t byte = 0;
    RingPos_t i;
    for (i = 0; i < RING_BUFFER_SIZE1; i++) {
        byte |= ring.data[i];
    }
    EXPECT_NE(0, byte);

    byte = 0;
    ring = ringInit(buffer, RING_BUFFER_SIZE1);
    for (i = 0; i < RING_BUFFER_SIZE1; i++) {
        byte |= ring.data[i];
    }
    EXPECT_EQ(0, byte);

    // Fill with data so we can test tail wrapping 
    memset(buffer, 0xff, sizeof(buffer));

    // Ensure the head correctly wraps when there isn't enough room at the back
    // of the ring but there is as the front
    EXPECT_EQ(RING_STATUS_OK, ringPush(&ring, d1, sizeof(d1)));
    EXPECT_EQ(RING_STATUS_OK, ringPush(&ring, d1, sizeof(d1)));
    EXPECT_EQ(sizeof(d1), ringPop(&ring, dout, sizeof(dout)));
    EXPECT_EQ(sizeof(d1) + sizeof(RingPos_t), ring.tail);
    EXPECT_EQ(RING_STATUS_OK, ringPush(&ring, d0, sizeof(d0)));
    EXPECT_EQ(sizeof(d0) + sizeof(RingPos_t), ring.head);

    // Ensure the tail correctly wraps when there is empty space at the back of
    // the ring
    ringPop(&ring, dout, sizeof(dout));

    EXPECT_EQ(sizeof(d0), ringPeek(&ring, &d2));
    EXPECT_EQ(d0[0], dout[0]);
    EXPECT_EQ(d0[1], dout[1]);
    EXPECT_EQ(d0[2], dout[2]);
    EXPECT_EQ(d0[3], dout[3]);
    EXPECT_EQ(d0[4], dout[4]);
    EXPECT_EQ(d0[5], dout[5]);
    EXPECT_EQ(d0[6], dout[6]);
    EXPECT_EQ(d0[7], dout[7]);
    EXPECT_EQ(sizeof(d0), ringPop(&ring, dout, sizeof(dout)));
    EXPECT_EQ(sizeof(d0) + sizeof(RingPos_t), ring.tail);

    // TODO ringPop without enough room
    // TODO ringPop with NULL outbuf (just drop the data)
    // Ring status isn't reporting correctly for ringPush
    // Fails to wrap tail around.
}
