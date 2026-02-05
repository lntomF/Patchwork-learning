#include "app_cli_parse.h"
#include <stdlib.h>
// --- 引入 FreeRTOS 头文件 (必须加) ---
#include "FreeRTOS.h"
#include "task.h"

#define LOG_TAG_CLI "Cli_parse"
static char shell_buff[SHELL_MAX_LEN]; // 命令行输入缓冲区，存储用户输入的完整命令
static uint16_t shell_idx;             // 缓冲区写入位置索引，记录当前已输入的字节数

extern volatile uint8_t g_cpu_load_enable;

/**
 * @brief CPU负载压力测试命令处理函数
 * @param[in] args 命令参数字符串，"1"表示启用高负载模式，"0"表示关闭
 * @note 用途：模拟CPU高负载以测试系统在压力下的性能表现
 * @example 使用方法: LOAD 1 (启用) 或 LOAD 0 (关闭)
 */
static void Cmd_SetLoad(char *args)
{
    if (args == NULL)
    {
        elog_i(LOG_TAG_CLI, "Usage: LOAD 1 or 0\r\n");
        return;
    }

    int state = atoi(args);
    g_cpu_load_enable = state ? 1 : 0;

    elog_i(LOG_TAG_CLI, "CPU Stress Test: %s\r\n", g_cpu_load_enable ? "ON (High Load)" : "OFF (Idle)");
}

/**
 * @brief FreeRTOS任务统计信息缓冲区
 * @note 使用静态分配避免占用CLI任务栈空间，防止栈溢出
 *       假设最多10个任务，每行约40~50字节，1024字节足够存放所有任务信息
 */
static char pcWriteBuffer[1024];

/**
 * @brief 系统状态查看命令 (TOP命令)
 * @param[in] args 命令参数（此命令不需要参数）
 * @note 功能：
 *       1. 列出所有任务信息 (任务名、状态、优先级、栈剩余、编号)
 *       2. 显示CPU时间统计 (各任务占用的CPU百分比)
 * @note 任务状态说明: R=运行, B=阻塞, S=挂起, D=删除
 */
static void Cmd_Top(char *args)
{
    // 1. 打印任务列表 (Name, State, Prio, Stack, Num)
    // State: R=运行, B=阻塞, S=挂起, D=删除
    elog_i(LOG_TAG_CLI, "\r\n=======================================================\r\n");
    elog_i(LOG_TAG_CLI, "Task Name\tState\tPrio\tStack\tNum\r\n");
    elog_i(LOG_TAG_CLI, "-------------------------------------------------------\r\n");

    // 这个函数会把列表格式化到 buffer 里
    memset(pcWriteBuffer, 0, 1024);
    vTaskList(pcWriteBuffer);
    elog_i(LOG_TAG_CLI, "%s", pcWriteBuffer); // 打印出来
    elog_i(LOG_TAG_CLI, "=======================================================\r\n");

    // 2. 打印 CPU 使用率 (Name, AbsTime, %)
    // 如果你之前的 DWT 配置没问题，这里会显示百分比
    elog_i(LOG_TAG_CLI, "Task Name\tAbs Time\t%% Time\r\n");
    elog_i(LOG_TAG_CLI, "-------------------------------------------------------\r\n");

    // 这个函数会把 CPU 统计格式化到 buffer 里
    memset(pcWriteBuffer, 0, 1024);
    vTaskGetRunTimeStats(pcWriteBuffer);
    elog_i(LOG_TAG_CLI, "%s", pcWriteBuffer);
    elog_i(LOG_TAG_CLI, "=======================================================\r\n");
}

/**
 * @brief LED控制命令处理函数
 * @param[in] args 命令参数字符串，支持三种操作：
 *                 - "ON"：点亮LED
 *                 - "OFF"：熄灭LED
 *                 - "TOGGLE"：LED状态翻转
 * @note 参数格式：LED ON/OFF/TOGGLE
 */
static void Cmd_LED(char *args)
{
    if (NULL == args)
    {
        elog_w(LOG_TAG_CLI, "Error: Missing args (ON/OFF/TOGGLE)\r\n");
        return;
    }
    char *save_ptr = NULL;
    char *args1 = strtok_r(args, " ", &save_ptr);
    if (NULL == args1)
        return;
    if (0 == strcmp(args1, "ON"))
    {
        BSP_LED_Set(1);
        elog_i(LOG_TAG_CLI, "LED -> ON\r\n");
    }
    else if (0 == strcmp(args1, "OFF"))
    {
        BSP_LED_Set(0);
        elog_i(LOG_TAG_CLI, "LED -> OFF\r\n");
    }
    else if (0 == strcmp(args1, "TOGGLE"))
    {
        BSP_LED_Toggle();
        elog_i(LOG_TAG_CLI, "LED -> TOGGLE\r\n");
    }
}

/**
 * @brief 获取芯片温度命令处理函数
 * @param[in] args 命令参数（此命令不需要参数）
 * @note 功能：读取MCU内部温度传感器数据并显示当前芯片温度
 */
static void Cmd_get_temp(char *args)
{
    float temp = BSP_Get_ChipTemp();
    elog_i(LOG_TAG_CLI, "Chip Temperature: %.2f C\r\n", temp);
}

/**
 * @brief 系统重启命令处理函数
 * @param[in] args 命令参数（此命令不需要参数）
 * @note 功能：触发系统软复位，重新启动整个系统
 */
static void Cmd_Reboot(char *args)
{
    elog_i(LOG_TAG_CLI, "OK: Rebooting...\r\n");
}

/**
 * @brief 电机/舵机控制命令处理函数
 * @param[in] args 命令参数字符串，包含速度值 (0-100) 或角度值
 * @note 功能：设置电机转速或舵机角度
 * @example 使用方法: MOTOR 50 (设置速度为50)
 */
static void Cmd_Motor(char *args)
{
    if (args == NULL)
    {
        printf("Error: Missing speed\r\n");
        return;
    }
    // 将参数字符串转换为整数，直接赋值给舵机
    int speed = atoi(args);
    BSP_Servo_SetAngle(speed);
    elog_i(LOG_TAG_CLI, "Set Speed -> %d\r\n", speed);
}

/**
 * @brief 帮助信息显示命令处理函数
 * @param[in] args 命令参数（此命令不需要参数）
 * @note 功能：列出所有可用的命令及其用法说明
 */
static void Cmd_Help(char *args)
{
    uint8_t i = 0;
    elog_i(LOG_TAG_CLI, "--- Commands ---\r\n");
    for (i = 0; i < g_num_cmd; i++)
    {
        elog_i(LOG_TAG_CLI, "%-10s : %s\r\n", g_shell_cmds[i].name, g_shell_cmds[i].help);
    }
    elog_i(LOG_TAG_CLI, "--------------------------\r\n");
}

/**
 * @brief 命令表数组定义
 * @note 结构：命令名称、对应的处理函数指针、帮助文本描述
 *       新增命令只需在此数组中添加一行记录，系统会自动识别
 */
static const Shell_command_t g_shell_cmds[] = {
    {"LED", Cmd_LED, "Control LED (Usage: LED ON/OFF/TOGGLE)"},
    {"MOTOR", Cmd_Motor, "Set Motor Speed (0-100)"},
    {"REBOOT", Cmd_Reboot, "Reboot System"},
    {"TEMP", Cmd_get_temp, "Get chip temperature!"},
    {"TOP", Cmd_Top, "Get System info"},
    {"LOAD", Cmd_SetLoad, "Set CPU Load for Stress Test (Usage: LOAD 1/0)"},
    {"HELP", Cmd_Help, "Show help list"}};

// 自动计算命令表中的命令数量，避免手工修改导致的错误
const uint8_t g_num_cmd = sizeof(g_shell_cmds) / sizeof(g_shell_cmds[0]);

extern const Shell_command_t g_shell_cmds[];
extern const uint8_t g_num_cmd;

/**
 * @brief 命令执行函数
 * @note 工作流程：
 *       1. 使用字符串分割函数提取命令名称
 *       2. 遍历命令表查找匹配的命令
 *       3. 调用对应的处理函数并传递参数
 *       4. 若命令不存在则输出错误提示
 */
static void Shell_Exce(void)
{
    shell_buff[shell_idx] = '\0'; // 字符串结束符
    uint8_t i;
    char *saveptr = NULL;
    // 分割字符串，获取命令名 (第一个空格之前的部分)
    char *cmd_name = strtok_r(shell_buff, " ", &saveptr);
    if (NULL == cmd_name)
        return;

    // 遍历命令表，寻找匹配的命令
    for (i = 0; i < g_num_cmd; i++)
    {
        if (strcmp(cmd_name, g_shell_cmds[i].name) == 0)
        {
            // 找到匹配命令，调用对应函数，saveptr为剩余参数字符串
            g_shell_cmds[i].func(saveptr);
            return; // 执行完毕，退出
        }
    }
    // 命令不存在时输出提示信息
    elog_i(LOG_TAG_CLI, "Unknown CMD: '%s'. Try 'HELP'.\r\n", cmd_name);
}

/**
 * @brief 逐字节解析命令行输入
 * @param[in] byte 单个接收的字节
 * @return true  - 一行命令完整接收 (检测到回车/换行符)
 * @return false - 命令行数据尚未接收完成
 * @note 工作流程：
 *       1. 检测换行符 (\n) 或回车符 (\r) 判断命令完整性
 *       2. 接收到完整命令行时触发执行
 *       3. 缓冲区满或异常字符时清空重新开始
 */
bool Shell_parsebyte(uint8_t byte)
{
    bool is_line_finish = false;

    if (byte == '\n' || byte == '\r') // 检测命令行结束符 (回车或换行)
    {
        if (shell_idx > 0)
        {
            Shell_Exce();  // 有有效数据时执行命令
            shell_idx = 0; // 重置缓冲区索引，准备接收下一条命令
        }
        is_line_finish = true;
    }
    else if (shell_idx < SHELL_MAX_LEN - 1) // 正常字符，缓冲区未满
    {
        shell_buff[shell_idx++] = (char)byte;
    }
    else // 缓冲区满，丢弃数据并重置
    {
        shell_idx = 0;
    }
    return is_line_finish;
}
