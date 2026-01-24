#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
typedef struct 
{
    uint8_t *buffer;
    uint8_t size;
    uint8_t head;
    uint8_t tail;
}Ringbuffer;

bool RB_wirte(Ringbuffer *rb,uint8_t data)
{
    int next_head = rb->head +1;
    if(next_head >= rb->size){
        next_head = 0;
    }
    if(next_head == rb->tail){
        return 0;     
    }
    rb->buffer[rb->head] = data;
    rb->head = next_head;
    return 1;
}
bool RB_read(Ringbuffer *rb, uint8_t *data)
{
    if(rb->head == rb->tail){
        return 0;
    }
    *data = rb->buffer[rb->tail];
    uint8_t next_tail = rb->tail +1;
    if(next_tail >= rb->size){
        next_tail = 0;
    }
    rb->tail = next_tail;
    return 1;
}
