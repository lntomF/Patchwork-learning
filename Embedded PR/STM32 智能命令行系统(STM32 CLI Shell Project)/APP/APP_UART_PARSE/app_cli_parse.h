#ifndef __APP_CLI_PARSE_H__
#define __APP_CLI_PARSE_H__

#include <stdint.h>
#include <stdbool.h>
#include "elog.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "bsp_servo.h"
#include "bsp_led_driver.h"
#include "bsp_mcu_inter_temperature.h"

#define SHELL_MAX_LEN 64

bool Shell_parsebyte(uint8_t byte);
//ÖØ¹¹shellº¯Êý
typedef void(*shell_func)(char *arg);
typedef struct 
{
    char *name;
    shell_func func;
    char *help;
}Shell_command_t;


#endif //end __APP_CLI_PARSE_H__
