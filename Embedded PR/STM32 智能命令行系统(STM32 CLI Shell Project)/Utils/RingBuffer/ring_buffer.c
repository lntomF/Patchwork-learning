#include "ring_buffer.h"

void RB_Init(RingBuffer_t *rb, uint8_t *pool, uint32_t size) {
    rb->buffer = pool;
    rb->size = size;
    rb->head = 0;
    rb->tail = 0;
}

bool RB_Write(RingBuffer_t *rb, uint8_t data) {
    uint32_t next_head = rb->head + 1;
    if (next_head >= rb->size) {
        next_head = 0;
    }
    // 如果头部追上了尾部，说明满了 (保留一个字节不存，用于区分空和满)
    if (next_head == rb->tail) {
        return false; // 缓冲区满
    }
    rb->buffer[rb->head] = data;
    rb->head = next_head;
    return true;
}

bool RB_Read(RingBuffer_t *rb, uint8_t *data) {
    if (rb->head == rb->tail) {
        return false; // 缓冲区空
    }

    *data = rb->buffer[rb->tail];
    
    uint32_t next_tail = rb->tail + 1;
    if (next_tail >= rb->size) {
        next_tail = 0;
    }
    rb->tail = next_tail;

    return true;
}

bool RB_IsEmpty(RingBuffer_t *rb) {
    return (rb->head == rb->tail);
}
