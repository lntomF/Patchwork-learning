#include "app_cli_parse.h"
#include <string.h>
#include <stdio.h>

#define LOG_TAG_CLI "Cli_parse"
static char shell_buff[SHELL_MAX_LEN];
static uint16_t shell_idx;//存到哪里了，位置记录

static void Shell_Exce(void)
{
    shell_buff[shell_idx] = '\0';
    if(strcmp(shell_buff,"LED_ON") == 0)
    {
        elog_i(LOG_TAG_CLI,"LED_ON");
    }else if(strcmp(shell_buff, "REBOOT") == 0) {
        elog_i(LOG_TAG_CLI,"OK: Rebooting...\r\n");
    }else{
        elog_i(LOG_TAG_CLI,"Unknown CMD:%s\r\n",shell_buff);
    }
}

bool Shell_parsebyte(uint8_t byte)
{
    bool is_line_finish = false;
    if(byte == '\n' || byte == '\r')// 1. 检测结束符 (回车或换行)
    {
        if(shell_idx > 0)
        {
            Shell_Exce();
            shell_idx = 0;
        }
        is_line_finish = true;
    }else if(shell_idx < SHELL_MAX_LEN-1)// 2. 正常字符 (过滤掉乱码)
    {
        shell_buff[shell_idx++] = (char)byte;
    }else{
        shell_idx = 0;
    }
    return is_line_finish;
}

