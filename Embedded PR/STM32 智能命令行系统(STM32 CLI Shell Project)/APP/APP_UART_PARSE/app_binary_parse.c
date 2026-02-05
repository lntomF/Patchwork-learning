#include "app_binary_parse.h"

#define LOG_TAG_BIN "Binary_parse"

/**
 * @brief 二进制帧解析状态枚举
 * 状态机流转: WAIT_HEADER -> READ_IDS -> READ_LEN -> READ_PAYLOD -> CHECK_SUM -> WAIT_HEADER
 */
typedef enum
{
    STATE_WAIT_HEADER = 0, // 等待帧头
    STATE_READ_IDS,        // 读取设备ID、系统ID、消息ID、序列号 (4字节)
    STATE_READ_LEN,        // 读取负载长度 (1字节)
    STATE_READ_PAYLOD,     // 读取负载数据 (可变长度)
    STATE_CHECK_SUM        // 读取并校验检验和 (1字节)
} Parse_state_t;

static Parse_state_t g_state = STATE_WAIT_HEADER; // 当前解析状态
static binary_parse_t g_parse;                    // 正在拼装的帧数据结构
static uint16_t g_rx_idx = 0;                     // 字节计数器 (用于读取各个状态的字节位置)
static uint8_t g_check_sum = 0;                   // 累积计算的校验和 (用于校验数据完整性)

/**
 * @brief 处理完整的二进制帧数据
 * @param[in] pasre 指向完整解析帧的指针
 * @note 该函数在帧数据完整且校验和正确后被调用
 */
static void Handle_Parse(binary_parse_t *pasre)
{
    if (pasre->msg_id == 0x51)
    {
        elog_i(LOG_TAG_BIN, "Binary Recv! Seq:%d, Len:%d", pasre->seq, pasre->payload_len);
    }
}

/**
 * @brief 逐字节解析二进制协议帧 (状态机方式)
 *
 * @param[in] byte 待解析的单个字节
 * @return true  - 一帧数据解析完成 (可能成功或失败)
 * @return false - 帧数据尚未完成，继续等待后续字节
 *
 * @note 协议帧格式: [帧头] [设备ID] [系统ID] [消息ID] [序列号] [长度] [负载] [校验和]
 *       该函数通过状态机逐个字节处理接收的数据，累积计算校验和用于验证帧的完整性
 */
bool Binary_parseByte(uint8_t byte)
{
    bool is_parse_finished = false;
    switch (g_state)
    {
    case STATE_WAIT_HEADER:
        // 等待帧头标志，检测到帧头后转入读ID状态
        if (BINARY_HEAD == byte)
        {
            g_state = STATE_READ_IDS;
            g_rx_idx = 0;
            g_check_sum = BINARY_HEAD; // 帧头纳入校验和计算
        }
        break;

    case STATE_READ_IDS:
        // 依次读取4个ID字节: device_id, system_id, msg_id, seq
        if (0 == g_rx_idx)
            g_parse.device_id = byte;
        else if (1 == g_rx_idx)
            g_parse.system_id = byte;
        else if (2 == g_rx_idx)
            g_parse.msg_id = byte;
        else if (3 == g_rx_idx)
            g_parse.seq = byte;

        g_check_sum += byte; // 累加到校验和
        g_rx_idx++;

        if (4 == g_rx_idx) // 4个ID字节读取完成
        {
            g_state = STATE_READ_LEN;
        }
        break;

    case STATE_READ_LEN:
        // 读取负载长度字段
        g_parse.payload_len = byte;
        g_check_sum += byte;

        // 长度校验：超过最大值则丢弃该帧
        if (g_parse.payload_len >= BINAERY_MAX_LEN)
        {
            g_state = STATE_WAIT_HEADER;
            is_parse_finished = true;
        }
        else if (0 == g_parse.payload_len) // 无负载数据，直接跳到校验和
        {
            g_state = STATE_CHECK_SUM;
        }
        else // 有负载数据，转入读负载状态
        {
            g_state = STATE_READ_PAYLOD;
            g_rx_idx = 0; // 重置计数器供负载数组索引使用
        }
        break;

    case STATE_READ_PAYLOD:
        // 逐字节读取负载数据到缓冲区
        g_parse.payload[g_rx_idx] = byte;
        g_check_sum += byte;
        g_rx_idx++;

        if (g_rx_idx >= g_parse.payload_len) // 负载数据读取完成
        {
            g_state = STATE_CHECK_SUM;
        }
        break;

    case STATE_CHECK_SUM:
        // 读取接收到的校验和，与计算的校验和比较
        g_parse.checksum = byte;
        if (g_check_sum == g_parse.checksum) // 校验成功
        {
            Handle_Parse(&g_parse); // 调用处理函数处理完整的帧
        }
        else // 校验失败，输出错误信息
        {
            elog_i(LOG_TAG_BIN, "Err: Cal=%02X, Recv=%02X\r\n", g_check_sum, byte);
            elog_e(LOG_TAG_BIN, "Checksum Error!");
        }

        g_state = STATE_WAIT_HEADER; // 返回等待状态，准备接收下一帧
        is_parse_finished = true;
        break;

    default:
        g_state = STATE_WAIT_HEADER;
        break;
    }
    return is_parse_finished;
}
