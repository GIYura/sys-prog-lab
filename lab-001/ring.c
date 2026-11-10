#include <assert.h>
#include <string.h>

#include "ring.h"

bool RingInit(ring_t* const ring)
{
    assert(ring != NULL);

    ring->read_index = 0;
    ring->write_index = 0;
    ring->count = 0;
    ring->slots = RING_SLOTS;
    return pthread_mutex_init(&ring->lock, NULL) == 0;
}

bool RingPush(ring_t* const ring, const uint32_t* buffer)
{
    assert(ring != NULL);
    assert(buffer != NULL);

    pthread_mutex_lock(&ring->lock);
    if (ring->count == ring->slots)
    {
        pthread_mutex_unlock(&ring->lock);
        return false;
    }

    memcpy(&ring->buffer[ring->write_index * DGRAM_WORDS], buffer, DGRAM_WORDS * sizeof(uint32_t));
    ring->write_index = (ring->write_index + 1) % ring->slots;
    ring->count++;
    pthread_mutex_unlock(&ring->lock);

    return true;
}

bool RingPop(ring_t* const ring, uint32_t* buffer)
{
    assert(ring != NULL);
    assert(buffer != NULL);

    pthread_mutex_lock(&ring->lock);
    if (ring->count == 0)
    {
        pthread_mutex_unlock(&ring->lock);
        return false;
    }
    memcpy(buffer, &ring->buffer[ring->read_index * DGRAM_WORDS], DGRAM_WORDS * sizeof(uint32_t));
    ring->read_index = (ring->read_index + 1) % ring->slots;
    ring->count--;
    pthread_mutex_unlock(&ring->lock);

    return true;
}

uint32_t RingCount(ring_t* const ring)
{
    assert(ring != NULL);

    pthread_mutex_lock(&ring->lock);
    uint32_t result = ring->count;
    pthread_mutex_unlock(&ring->lock);

    return result;
}

void RingClear(ring_t* const ring)
{
    assert(ring != NULL);

    pthread_mutex_lock(&ring->lock);
    ring->read_index = 0;
    ring->write_index = 0;
    ring->count = 0;
    pthread_mutex_unlock(&ring->lock);
}

void RingFree(ring_t* const ring)
{
    assert(ring != NULL);

    RingClear(ring);
    pthread_mutex_destroy(&ring->lock);
}
