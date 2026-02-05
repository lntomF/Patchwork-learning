#include "app_dispatcher.h"

#define LOG_DIS "dis"
#define TIMEOUT_THRESHOLD_MS  200

/**
 * @brief 数据分发器状态枚举
 * @note 状态机流转：
 *       IDLE -> CLI/BIN -> 处理 -> IDLE
 *       根据接收的第一个字节判断数据类型，选择相应的解析器处理
 */
typedef enum
{
    DISPATCHER_IDLE = 0,      // 空闲状态，等待新数据
    DISPATCHER_BUSY_CLI,      // 正在处理CLI（文本命令）数据
    DISPATCHER_BUSY_BIN,      // 正在处理二进制协议数据
} dispatcher_state_t;

/**
 * @brief 分发器当前状态标志
 * @note 用于追踪分发器的工作状态，决定后续字节的处理方式
 */
static dispatcher_state_t dis_state = DISPATCHER_IDLE;

/**
 * @brief 数据分发器入口函数
 * @param[in] byte 单个接收的字节
 * @note 工作流程：
 *       1. 根据当前状态和接收字节内容判断数据类型
 *       2. 识别CLI数据：可打印字符(0x20-0x7E)、回车(\r)、换行(\n)
 *       3. 识别二进制数据：帧头标志(BINARY_HEAD)
 *       4. 将字节转发给对应的解析器处理
 *       5. 当解析器返回true表示一帧完成，分发器回到IDLE状态
 *       
 * @note 状态转移图：
 *       IDLE ─── 可打印字符/\r/\n ──> BUSY_CLI ─── Shell_parsebyte返回true ──> IDLE
 *       IDLE ─── BINARY_HEAD ──> BUSY_BIN ─── Binary_parseByte返回true ──> IDLE
 *       IDLE ─── 其他字符 ──> IDLE (丢弃)
 */
void Dispatcher_Input(uint8_t byte)
{
    switch (dis_state)
    {
    case DISPATCHER_IDLE:
        // 空闲状态：判断接收到的字节属于哪种协议
        
        // 检查是否为CLI数据 (可打印字符、回车或换行)
        // 0x20-0x7E: 可打印ASCII字符范围
        if ((0x20 <= byte && byte <= 0x7E) || (byte == '\r') || (byte == '\n'))
        {
            dis_state = DISPATCHER_BUSY_CLI;
            if (true == Shell_parsebyte(byte))
            {
                // CLI数据完整，一行命令接收完成
                dis_state = DISPATCHER_IDLE;
            }
        }
        // 检查是否为二进制数据 (帧头标志)
        else if (BINARY_HEAD == byte)
        {
            dis_state = DISPATCHER_BUSY_BIN;
            Binary_parseByte(byte);
        }
        // 其他无效字节，保持IDLE状态
        else
        {
            dis_state = DISPATCHER_IDLE;
        }
        break;

    case DISPATCHER_BUSY_CLI:
        // 正在处理CLI命令行数据，继续解析后续字节
        if (true == Shell_parsebyte(byte))
        {
            // 一行命令完整接收，返回IDLE等待下一条命令
            dis_state = DISPATCHER_IDLE;
        }
        // 若返回false，继续在BUSY_CLI状态等待更多字节
        break;

    case DISPATCHER_BUSY_BIN:
        // 正在处理二进制协议数据，继续解析后续字节
        if (true == Binary_parseByte(byte))
        {
            // 一帧二进制数据完整接收，返回IDLE等待下一帧
            dis_state = DISPATCHER_IDLE;
        }
        // 若返回false，继续在BUSY_BIN状态等待更多字节
        break;

    default:
        // 异常状态复位
        dis_state = DISPATCHER_IDLE;
        break;
    }
}

