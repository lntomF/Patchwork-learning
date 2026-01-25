#include "app_dispatcher.h"
#define LOG_DIS "dis"
#define TIMEOUT_THRESHOLD_MS  200
typedef enum
{
    DISPATCHER_IDLE = 0,
    DISPATCHER_BUSY_CLI,
    DISPATCHER_BUSY_BIN,
} dispatcher_state_t;

static dispatcher_state_t dis_state = DISPATCHER_IDLE;

void Dispatcher_Input(uint8_t byte)
{
    switch (dis_state)
    {
    case DISPATCHER_IDLE:
        if ((0x20 <= byte && byte <= 0x7E) || (byte == '\r') || (byte == '\n'))
        {
            dis_state = DISPATCHER_BUSY_CLI;
            if (true == Shell_parsebyte(byte))
            {
                dis_state = DISPATCHER_IDLE;
            }
        }
        else if (BINARY_HEAD == byte)
        {
            dis_state = DISPATCHER_BUSY_BIN;
            Binary_parseByte(byte);
        }
        else
        {
            dis_state = DISPATCHER_IDLE;
        }
        break;
    case DISPATCHER_BUSY_CLI:
        if (true == Shell_parsebyte(byte))
        {
            dis_state = DISPATCHER_IDLE;
        }
        break;
    case DISPATCHER_BUSY_BIN:
        if (true == Binary_parseByte(byte))
        {
            dis_state = DISPATCHER_IDLE;
        }

        break;
    default:
        dis_state = DISPATCHER_IDLE;
        break;
    }
}

