#ifndef RING_H
#define RING_H

#include <stdbool.h>
#include <stdint.h>
#include <pthread.h>

#define DGRAM_WORDS         (1024U)
#define RING_SLOTS          (8U)
#define RING_BUFFER_SIZE    (DGRAM_WORDS * RING_SLOTS)

typedef struct
{
    pthread_mutex_t lock;
    uint32_t read_index;
    uint32_t write_index;
    uint32_t buffer[RING_BUFFER_SIZE];
    uint32_t count;
    uint32_t slots;
} ring_t;

bool RingInit(ring_t* const ring);
bool RingPush(ring_t* const ring, const uint32_t* buffer);
bool RingPop(ring_t* const ring, uint32_t* buffer);
uint32_t RingCount(ring_t* const ring);
void RingClear(ring_t* const ring);
void RingFree(ring_t* const ring);

#endif
