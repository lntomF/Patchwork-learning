#include "app_usart_task.h"
#define LOG_TAG_U "APP_USART_LOG"

/* USER CODE BEGIN Variables */

// extern QueueHandle_t uart_Mailbox; //使用队列传输
extern SemaphoreHandle_t uart_Semaphore; //使用二值信号量传输

/* USER CODE END Variables */

void UartParseTask(void *argument)
{
  /* USER CODE BEGIN UartParseTask */
	static uint8_t app_uart_rx_buffer[64] = {0};
	uint32_t len,i;
	TickType_t startTick = xTaskGetTickCount();
	BSP_UART_Init();
	elog_i(LOG_TAG_U, "UartParseTask Started & Ready for DMA");
	elog_i(LOG_TAG_U,"[%lu]UartParseTask success",startTick);
   
  /* Infinite loop */
  for(;;) 
  {
#if 0 //使用队列传输
    if(xQueueReceive(uart_Mailbox,&app_uart_rx,portMAX_DELAY) == pdPASS){
      elog_i(LOG_TAG_U,"UartParseTask receive uart_Mailbox success");
      while(true == RB_Read(&g_uart_rx_rb, &app_uart_rx)){
          elog_i(LOG_TAG_U,"UartParseTask read data: %c",app_uart_rx);
          //TODO: Add your data processing logic here
      }
    }
#endif
#if 1 // 使用二值信号量传输
    if( xSemaphoreTake( uart_Semaphore, portMAX_DELAY ) == pdTRUE )
    { 
      len = BSP_UART_Read(app_uart_rx_buffer, sizeof(app_uart_rx_buffer));
	   elog_i(LOG_TAG_U, "L:%d ", len);
      if(len>0)
      {
        for(i = 0; i < len; i++)
        {
          Dispatcher_Input(app_uart_rx_buffer[i]);
        }
      }
    }

#endif
	
    osDelay(1);
  }
  /* USER CODE END UartParseTask */
}
