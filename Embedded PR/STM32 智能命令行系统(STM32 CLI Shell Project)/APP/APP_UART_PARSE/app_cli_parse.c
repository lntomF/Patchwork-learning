#include "app_cli_parse.h"
#include <stdlib.h>
// --- 引入 FreeRTOS 头文件 (必须加) ---
#include "FreeRTOS.h"
#include "task.h"

#define LOG_TAG_CLI "Cli_parse"
static char shell_buff[SHELL_MAX_LEN];
static uint16_t shell_idx; // 存到哪里了，位置记录

extern volatile uint8_t g_cpu_load_enable;
// --- 命令: 模拟 CPU 负载 ---
// Usage: LOAD 1 (开), LOAD 0 (关)
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

// 定义一个足够大的缓存区来存放统计信息
// 设为 static 以免占用 CLI 任务的栈空间 (避免栈溢出)
// 假设你有 10 个任务，每个任务一行大概 40~50 字节，512 字节通常够了
static char pcWriteBuffer[1024];

// --- 命令: 查看系统状态 (TOP) ---
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
 * @brief LED 控制命令
 * @param args 传入的参数字符串，例如 "ON" 或 "OFF" 或 "TOGGLE"
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
static void Cmd_get_temp(char *args)
{
    float temp = BSP_Get_ChipTemp();
    elog_i(LOG_TAG_CLI, "Chip Temperature: %.2f C\r\n", temp);
}
/**
 * @brief 系统重启
 */
static void Cmd_Reboot(char *args)
{
    elog_i(LOG_TAG_CLI, "OK: Rebooting...\r\n");
}
/**
 * @brief Motor 控制命令
 * @param args 传入的参数字符串
 */
static void Cmd_Motor(char *args)
{
    if (args == NULL)
    {
        printf("Error: Missing speed\r\n");
        return;
    }
    // 这里我们不需要 strtok 了，因为剩下的就是一个纯数字字符串
    // 直接把 args 转成整数
    int speed = atoi(args);
    BSP_Servo_SetAngle(speed);
    elog_i(LOG_TAG_CLI, "Set Speed -> %d\r\n", speed);
}
/**
 * @brief 打印帮助信息
 */

extern const Shell_command_t g_shell_cmds[];
extern const uint8_t g_num_cmd;

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
static const Shell_command_t g_shell_cmds[] = {
    {"LED", Cmd_LED, "Control LED (Usage: LED ON/OFF/TOGGLE)"},
    {"MOTOR", Cmd_Motor, "Set Motor Speed (0-100)"},
    {"REBOOT", Cmd_Reboot, "Reboot System"},
    {"TEMP", Cmd_get_temp, "Get chip temperature!"},
    {"TOP", Cmd_Top, "Get System info"},
	{"LOAD", Cmd_SetLoad,"Set CPU Load for Stress Test (Usage: LOAD 1/0)"},
    {"HELP", Cmd_Help, "Show help list"}

};
// 自动计算命令数量
const uint8_t g_num_cmd = sizeof(g_shell_cmds) / sizeof(g_shell_cmds[0]);

static void Shell_Exce(void)
{
    shell_buff[shell_idx] = '\0';
    uint8_t i;
    char *saveptr = NULL;
    char *cmd_name = strtok_r(shell_buff, " ", &saveptr);
    if (NULL == cmd_name)
        return;
    for (i = 0; i < g_num_cmd; i++)
    {
        if (strcmp(cmd_name, g_shell_cmds[i].name) == 0)
        {
            // 3. 找到对应函数，把剩下的参数字符串(save_ptr)传给它
            g_shell_cmds[i].func(saveptr);
            return; // 执行完毕，退出
        }
    }
    elog_i(LOG_TAG_CLI, "Unknown CMD: '%s'. Try 'HELP'.\r\n", cmd_name);
}

bool Shell_parsebyte(uint8_t byte)
{
    bool is_line_finish = false;
    if (byte == '\n' || byte == '\r') // 1. 检测结束符 (回车或换行)
    {
        if (shell_idx > 0)
        {
            Shell_Exce();
            shell_idx = 0;
        }
        is_line_finish = true;
    }
    else if (shell_idx < SHELL_MAX_LEN - 1) // 2. 正常字符 (过滤掉乱码)
    {
        shell_buff[shell_idx++] = (char)byte;
    }
    else
    {
        shell_idx = 0;
    }
    return is_line_finish;
}
