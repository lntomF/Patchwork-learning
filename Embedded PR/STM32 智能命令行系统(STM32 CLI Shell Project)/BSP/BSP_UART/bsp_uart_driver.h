#ifndef __BSP_UART_DRIVER_H__
#define __BSP_UART_DRIVER_H__

#include <stdint.h>
#include "FreeRTOS.h"
#include <stdint.h>
#include "cmsis_os.h"
#include "elog.h"
#include "usart.h"
#include "semphr.h"
#include "ring_buffer.h"
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
void BSP_UART_Init(void);
uint32_t BSP_UART_Read(uint8_t *data ,uint32_t len);




#endif //end __BSP_UART_DRIVER_H__

