#include "app_usart_task.h"
#define LOG_TAG_U "APP_USART_LOG"

/* USER CODE BEGIN Variables */

// extern QueueHandle_t uart_Mailbox;        // 使用队列传输（备用方案，已禁用）
extern SemaphoreHandle_t uart_Semaphore; // 使用二值信号量传输，用于同步DMA接收完成事件

/* USER CODE END Variables */

/**
 * @brief UART解析任务 (FreeRTOS任务函数)
 * @param[in] argument 任务参数（未使用）
 *
 * @note 功能说明：
 *       1. 初始化UART和舵机外设
 *       2. 等待信号量触发（由DMA接收完成中断触发）
 *       3. 从UART DMA缓冲区读取接收数据
 *       4. 逐字节将数据分发给数据分发器处理
 *       5. 支持两种同步机制（队列/信号量），当前使用信号量
 *
 * @note 数据流程：
 *       UART DMA接收 → 中断触发信号量 → 任务被唤醒 → 读取缓冲区数据 →
 *       逐字节分发给Dispatcher_Input → 识别协议类型 → 调用对应解析器
 *
 * @note 同步机制说明：
 *       - 队列方案：适合传输单个字节或小数据包，开销较大
 *       - 信号量方案：适合传输批量数据，效率高，当前采用此方案
 */
void UartParseTask(void *argument)
{
  /* USER CODE BEGIN UartParseTask */

  // 接收缓冲区，用于存储DMA一次接收的全部数据
  static uint8_t app_uart_rx_buffer[64] = {0};

  // 本次接收的数据长度和循环计数器
  uint32_t len, i;

  // 记录任务启动时的系统时钟滴答数，用于时间戳记录
  TickType_t startTick = xTaskGetTickCount();

  // 初始化UART外设（配置波特率、中断等）
  BSP_UART_Init();

  // 初始化舵机/电机外设（配置PWM等）
  BSP_Servo_Init();

  elog_i(LOG_TAG_U, "UartParseTask Started & Ready for DMA");
  elog_i(LOG_TAG_U, "[%lu]UartParseTask success", startTick);

  /* 无限循环：持续接收和处理UART数据 */
  for (;;)
  {
#if 0 
        // ========== 方案1：使用队列传输（已禁用）==========
        // 优点：数据保序，支持多个发送者
        // 缺点：单字节开销大，传输批量数据效率低
        if(xQueueReceive(uart_Mailbox, &app_uart_rx, portMAX_DELAY) == pdPASS)
        {
            elog_i(LOG_TAG_U, "UartParseTask receive uart_Mailbox success");
            // 从环形缓冲区逐字节读取数据并处理
            while(true == RB_Read(&g_uart_rx_rb, &app_uart_rx))
            {
                elog_i(LOG_TAG_U, "UartParseTask read data: %c", app_uart_rx);
                // TODO: Add your data processing logic here
            }
        }
#endif

#if 1
    // ========== 方案2：使用二值信号量传输（当前使用）==========
    // 优点：批量传输数据，效率高，同步简单
    // 缺点：只支持单个发送者

    // 等待信号量被触发（由DMA接收完成中断发送）
    // portMAX_DELAY：无限等待，直到接收到数据
    if (xSemaphoreTake(uart_Semaphore, portMAX_DELAY) == pdTRUE)
    {
      // 从UART DMA缓冲区读取一批接收的数据，返回实际接收的字节数
      len = BSP_UART_Read(app_uart_rx_buffer, sizeof(app_uart_rx_buffer));

      elog_i(LOG_TAG_U, "L:%d ", len); // 打印接收数据长度

      // 如果有有效数据接收
      if (len > 0)
      {
        // 逐字节处理接收到的数据
        for (i = 0; i < len; i++)
        {
          // 将字节分发给数据分发器
          // Dispatcher_Input会识别协议类型（CLI或二进制）
          // 并转发给对应的解析器处理
          Dispatcher_Input(app_uart_rx_buffer[i]);
        }
      }
    }

#endif

    // 释放CPU给其他任务运行，防止任务占用过多时间
    osDelay(1);
  }
  /* USER CODE END UartParseTask */
}
