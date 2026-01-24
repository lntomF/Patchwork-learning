#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stdint.h>
#include <stdbool.h>

typedef struct {
    uint8_t *buffer;   // 指向实际内存的指针
    uint32_t size;     // 缓冲区总大小
    volatile uint32_t head;     // 写指针 (Write Index)
    volatile uint32_t tail;     // 读指针 (Read Index)
} RingBuffer_t;

// 初始化
void RB_Init(RingBuffer_t *rb, uint8_t *pool, uint32_t size);
// 写一个字节
bool RB_Write(RingBuffer_t *rb, uint8_t data);
// 读一个字节
bool RB_Read(RingBuffer_t *rb, uint8_t *data);
// 判断是否为空
bool RB_IsEmpty(RingBuffer_t *rb);

#endif
