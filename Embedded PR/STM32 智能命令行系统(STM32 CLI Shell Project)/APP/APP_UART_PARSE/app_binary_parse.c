#include "app_binary_parse.h"

#define LOG_TAG_BIN "Binary_parse"

typedef enum{
    STATE_WAIT_HEADER = 0,
    STATE_READ_IDS,
    STATE_READ_LEN,
    STATE_READ_PAYLOD,
    STATE_CHECK_SUM
}Parse_state_t;

static Parse_state_t g_state = STATE_WAIT_HEADER; // 当前在哪个状态
static binary_parse_t g_parse; // 正在拼装的帧
static uint16_t g_rx_idx = 0;  // 计数器 (读到第几个字节了)
static uint8_t g_check_sum = 0;// 我自己算的校验和 (累加值)

static void Handle_Parse(binary_parse_t *pasre)//处理数据函数
{
    if(pasre->msg_id == 0x51)
    {
        elog_i(LOG_TAG_BIN,"Binary Recv! Seq:%d, Len:%d", pasre->seq, pasre->payload_len);
    }
}

bool Binary_parseByte(uint8_t byte)
{
    bool is_parse_finished = false;
    switch (g_state)
    {
    case STATE_WAIT_HEADER:
        if(BINARY_HEAD == byte)
        {
            g_state = STATE_READ_IDS;
            g_rx_idx = 0;
            g_check_sum = BINARY_HEAD;
        }
        break;
    case STATE_READ_IDS:
        if(0 == g_rx_idx) g_parse.device_id = byte;
        else if(1 == g_rx_idx) g_parse.system_id = byte;
        else if(2 == g_rx_idx) g_parse.msg_id = byte;
        else if(3 == g_rx_idx) g_parse.seq = byte;
        g_check_sum += byte;
        g_rx_idx++;
        if(4 == g_rx_idx)
        {
            g_state = STATE_READ_LEN;
        }
        break;
    case STATE_READ_LEN:
        g_parse.payload_len = byte;
        g_check_sum += byte;
        if(g_parse.payload_len >= BINAERY_MAX_LEN){
            g_state = STATE_WAIT_HEADER;
            is_parse_finished = true;
        }else if(0 == g_parse.payload_len){
            g_state = STATE_CHECK_SUM;
        }else{
            g_state = STATE_READ_PAYLOD;
            g_rx_idx = 0;// 重置计数器，给 payload 数组用
        }
        break;
    case STATE_READ_PAYLOD:
        g_parse.payload[g_rx_idx] = byte;
        g_check_sum += byte;
        g_rx_idx++;
        if(g_rx_idx >= g_parse.payload_len)
        {
            g_state = STATE_CHECK_SUM;
        }
        break;
    case STATE_CHECK_SUM:
        g_parse.checksum = byte;
        if(g_check_sum == g_parse.checksum)
        {
            Handle_Parse(&g_parse);
			
        }else{
			elog_i(LOG_TAG_BIN,"Err: Cal=%02X, Recv=%02X\r\n",g_check_sum, byte);
            elog_e(LOG_TAG_BIN,"Checksum Error!");
        }
        g_state = STATE_WAIT_HEADER;
        is_parse_finished = true;
        break;
    default:
        g_state = STATE_WAIT_HEADER;
        break;
    }
    return is_parse_finished;
}
