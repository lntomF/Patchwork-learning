#ifndef __APP_BINARY_PARSE_H__
#define __APP_BINARY_PARSE_H__

#include <stdint.h>
#include <stdbool.h>
#include "elog.h"
#define BINARY_HEAD 0xEF
#define BINAERY_MAX_LEN 255
typedef struct 
{
    uint8_t device_id;
    uint8_t system_id;
    uint8_t msg_id;
    uint8_t seq;//АќађСа
    uint8_t payload_len;
    uint8_t payload[BINAERY_MAX_LEN];
    uint8_t checksum;
}binary_parse_t;

bool Binary_parseByte(uint8_t byte);




#endif //end __APP_BINARY_PARSE_H__
