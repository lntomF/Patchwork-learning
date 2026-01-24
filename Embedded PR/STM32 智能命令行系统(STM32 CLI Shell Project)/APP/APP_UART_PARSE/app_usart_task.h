#ifndef __APP_USART_REC_PARSE_H__
#define __APP_USART_REC_PARSE_H__

#include "stdint.h"
#include "bsp_uart_driver.h"
#include "ring_buffer.h"
#include "app_dispatcher.h"

void UartParseTask(void *argument);


#endif //end __APP_USART_REC_PARSE_H__
