#ifndef __APP_CLI_PARSE_H__
#define __APP_CLI_PARSE_H__

#include <stdint.h>
#include <stdbool.h>
#include "elog.h"
#define SHELL_MAX_LEN 64

bool Shell_parsebyte(uint8_t byte);

#endif //end __APP_CLI_PARSE_H__
