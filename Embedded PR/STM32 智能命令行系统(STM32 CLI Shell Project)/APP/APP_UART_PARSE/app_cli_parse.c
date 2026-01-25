#include "app_cli_parse.h"
#include <stdlib.h>
#define LOG_TAG_CLI "Cli_parse"
static char shell_buff[SHELL_MAX_LEN];
static uint16_t shell_idx; // 存到哪里了，位置记录

/**
 * @brief LED 控制命令
 * @param args 传入的参数字符串，例如 "ON" 或 "OFF"
 */
static void Cmd_LED(char *args)
{
    if (NULL == args)
    {
        elog_w(LOG_TAG_CLI, "Error: Missing args (ON/OFF)\r\n");
        return;
    }
    char *save_ptr = NULL;
    char *args1 = strtok_r(args, " ", &save_ptr);
    if (NULL == args1)
        return;
    if (0 == strcmp(args1, "ON"))
    {
        elog_i(LOG_TAG_CLI, "LED -> ON\r\n");
    }
    else if (0 == strcmp(args1, "OFF"))
    {
        elog_i(LOG_TAG_CLI, "LED -> OFF\r\n");
    }
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
    {"LED", Cmd_LED, "Control LED (Usage: LED ON/OFF)"},
    {"MOTOR", Cmd_Motor, "Set Motor Speed (0-100)"},
    {"REBOOT", Cmd_Reboot, "Reboot System"},
    {"HELP", Cmd_Help, "Show help list"},
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
    elog_i(LOG_TAG_CLI,"Unknown CMD: '%s'. Try 'HELP'.\r\n", cmd_name);
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
