#include "ringbuf.h"
#include <string.h>

#ifdef RINGBUF_DEBUG
#include <stdio.h>
#endif

Ring ringInit(uint8_t * buffer, RingPos_t size) {
    Ring ring = {NULL, 0, 0, 0};
    if (buffer == NULL || size <= sizeof(RingPos_t)) {
        return ring;
    }
    ring.data = buffer;
    ring.size = size;
    ring.overflow = 0;
    memset(buffer, 0, size);

#ifdef RINGBUF_DEBUG
    printf("%s: Performed ring init. size=%u\n", __FUNCTION__, ring.size);
#endif

    return ring;
}

RingPos_t ringFree(Ring * ring) {
    if (ring == NULL || ring->data == NULL || ring->size <= sizeof(RingPos_t)) {
        return 0;
    }

    RingPos_t free_hi = 0;
    RingPos_t free_lo = 0;
    RingPos_t sizeAvail = ring->size;

    // Can't fill back of buffer completely when tail is at 0
    if (ring->tail == 0) {
        sizeAvail -= 1;
    }

    // Head in front of tail
    if (ring->head >= ring->tail) {
        // Don't count space needed for RingPos_t
        if (ring->head + sizeof(RingPos_t) <= sizeAvail) {
            free_hi = sizeAvail - sizeof(RingPos_t) - ring->head;
        }
        else {
            free_hi = 0;
        }

        // Don't count space needed for RingPos_t
        if (ring->tail >= sizeof(RingPos_t) + 1) {
            free_lo = ring->tail - sizeof(RingPos_t) - 1;
        }
        else {
            free_lo = 0;
        }
    }
    // Head behind tail
    else {
        // Don't count space needed for RingPos_t
        if (ring->tail >= ring->head + sizeof(RingPos_t)) {
            free_hi = ring->tail - ring->head - sizeof(RingPos_t) - 1;
        }
        else {
            free_hi = 0;
        }
    }

    RingPos_t free = free_hi > free_lo ? free_hi : free_lo;

#ifdef RINGBUF_DEBUG
    /* printf("%s: free=%3u head=%3u tail=%3u\n", __FUNCTION__, free, ring->head, ring->tail); */
#endif

    return free;
}

RING_STATUS ringStatus(Ring * ring) {
    if (ring == NULL || ring->data == NULL || ring->size <= sizeof(RingPos_t)) {
        return RING_STATUS_FAIL;
    }

    if (ring->head == ring->tail) {
        return RING_STATUS_EMPTY;
    }

    return RING_STATUS_OK;
}

RING_STATUS ringPush(Ring * ring, uint8_t * data, RingPos_t size) {
    if (ring == NULL || data == NULL || size <= 0 || ring->data == NULL) {
        return RING_STATUS_FAIL;
    }

    if (size > ringFree(ring)) {
#ifdef RINGBUF_DEBUG
    printf("%s: Push failed, ring full. free: %u size: %3u head=%3u tail=%3u\n",
            __FUNCTION__, ringFree(ring), size, ring->head, ring->tail);
#endif
        ++(ring->overflow);
        return RING_STATUS_FAIL;
    }

    RingPos_t newHead = (ring->head + sizeof(RingPos_t) + size);

    // Queue from front of ring when overflow would occur
    if (newHead >= ring->size) {
        // Indicate that wrapping occurred by seting next size val to 0
        if (ring->head + sizeof(RingPos_t) <= ring->size) {
            memset(ring->data + ring->head, 0, sizeof(RingPos_t));
        }
        ring->head = 0;
        newHead = (ring->head + sizeof(RingPos_t) + size);
    }

    memcpy(ring->data + ring->head, &size, sizeof(size));
    memcpy(ring->data + ring->head + sizeof(RingPos_t), data, size);

    ring->head = newHead;
#ifdef RINGBUF_DEBUG
    printf("%s: Push succeeded. free: %u pushsize: %3u newhead=%3u tail=%3u\n",
            __FUNCTION__, ringFree(ring), size, ring->head, ring->tail);
#endif
   return RING_STATUS_OK;
}

RingPos_t ringPeek(Ring * ring, uint8_t ** data) {
    if (ring == NULL || data == NULL || ring->data == NULL ||
            ring->size <= sizeof(RingPos_t)) {
        return 0;
    }

    if (ringStatus(ring) == RING_STATUS_EMPTY) {
        return 0;
    }

    RingPos_t size = 0;
    memcpy(&size, ring->data + ring->tail, sizeof(size));
    if (size == 0) {
        size = (RingPos_t)*(ring->data);
        ring->tail = 0;
    }

    *data = ring->data + ring->tail + sizeof(RingPos_t);

    return size;
}

RingPos_t ringPop(Ring * ring, uint8_t * outbuf, RingPos_t maxsize) {
    if (ring == NULL || ring->data == NULL || ring->size <= sizeof(RingPos_t)) {
        return 0;
    }

    // Empty ring
    if (ring->head == ring->tail) {
#ifdef RINGBUF_DEBUG
    printf("%s: Pop failed. Ring empty. free: %u head=%3u tail=%3u\n",
            __FUNCTION__, ringFree(ring), ring->head, ring->tail);
#endif
        return 0;
    }

    if (ring->tail + sizeof(RingPos_t) >= ring->size) {
        ring->tail = 0;
    }

    RingPos_t size = 0;
    memcpy(&size, ring->data + ring->tail, sizeof(size));

    // Wrap tail to front when data size is 0
    if (size == 0) {
        size = (RingPos_t)*(ring->data);
        ring->tail = 0;
    }
    RingPos_t copySize = maxsize < size ? maxsize : size;
    if (!outbuf) {
        copySize = 0;
    }

    if (outbuf && copySize > 0) {
        memcpy(outbuf, ring->data + ring->tail + sizeof(RingPos_t), copySize);
    }
    ring->tail = ring->tail + sizeof(RingPos_t) + size;
#ifdef RINGBUF_DEBUG
    printf("%s: Pop succeeded. free: %u popsize: %3u head=%3u newTail=%3u\n",
            __FUNCTION__, ringFree(ring), size, ring->head, ring->tail);
#endif
    return (maxsize == 0 || !outbuf) ? size : copySize;
}

uint8_t ringOverflowed(Ring * ring) {
    if (ring == NULL) {
        return 0;
    }

    uint8_t cnt = ring->overflow;
    ring->overflow = 0;

    return cnt;
}
