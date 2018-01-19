#ifndef RINGBUF_H
#define RINGBUF_H

#include <stdint.h>
#include <stddef.h>

typedef enum RING_STATUS {
    RING_STATUS_OK,
    RING_STATUS_EMPTY,
    RING_STATUS_FAIL
} RING_STATUS;

typedef uint16_t RingPos_t;

typedef struct Ring {
    uint8_t * data;
    RingPos_t size;
    RingPos_t head;
    RingPos_t tail;
    uint8_t overflow;
} Ring;

Ring ringInit(uint8_t * buffer, RingPos_t size);
RingPos_t ringAvail(Ring * ring);
RING_STATUS ringStatus(Ring * ring);
RING_STATUS ringPush(Ring * ring, uint8_t * data, RingPos_t size);
RingPos_t ringPeek(Ring * ring, uint8_t ** data);
RingPos_t ringPop(Ring * ring, uint8_t * outbuf, RingPos_t maxsize);
uint8_t ringOverflowed(Ring * ring);

#endif
