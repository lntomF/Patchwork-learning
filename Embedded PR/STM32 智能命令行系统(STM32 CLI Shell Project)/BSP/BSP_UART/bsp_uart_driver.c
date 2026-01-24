#include "bsp_uart_driver.h"


/* USER CODE BEGIN Variables */
#define LOG_TAG_U "BSP_UART_LOG"
extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_rx;
#if 0 //使用队列传输
QueueHandle_t uart_Mailbox = NULL;
#endif 
#if 1 //使用二值信号量去传输
SemaphoreHandle_t uart_Semaphore = NULL;
#endif
// 环形缓冲区实例应用
#define RX_POOL_SIZE 4096
static uint8_t uart_rx_pool[RX_POOL_SIZE];
static RingBuffer_t g_uart_rx_rb;
// static uint8_t uart_rx_temp; // 单字节临时接收变量
//缓冲区设置 End
#define DMA_RX_BUF_SIZE 2048
static uint8_t dma_rx_buf[DMA_RX_BUF_SIZE]; // DMA 原始缓冲区
// 记录上一次处理到的位置 (这是唯一的难点)
static uint16_t old_pos = 0;
volatile uint32_t g_drop_cnt = 0;
/* USER CODE END Variables */


void BSP_UART_Init(void)
{
	RB_Init(&g_uart_rx_rb, uart_rx_pool, RX_POOL_SIZE);
#if 0 //创建队列传输
    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    //queue 1
    uart_Mailbox = NULL;
    uart_Mailbox = xQueueCreate(1,4);
  /* USER CODE END RTOS_QUEUES */
   if(uart_Mailbox == NULL){
    elog_i(LOG_TAG_U,"uart_Mailbox create failed");
    }
    else{
    elog_i(LOG_TAG_U,"uart_Mailbox create success");
  }
#endif 
#if 1 //创建二值信号量去传输
	uart_Semaphore = xSemaphoreCreateBinary();
	if(NULL == uart_Semaphore){
		elog_i(LOG_TAG_U,"uart_Semaphore failed");
	}else{
		elog_i(LOG_TAG_U,"uart_Semaphore success");
	}
	old_pos = 0;
#endif 
#if 0 //启动中断接收(没有使用DMA)
	if(HAL_OK == HAL_UART_Receive_IT(&huart1, &uart_rx_temp, 1)){
        elog_i(LOG_TAG_U,"HAL_UART_Receive_IT success");
    }
    else{
        elog_i(LOG_TAG_U,"HAL_UART_Receive_IT failed");
    }
#endif    
#if 1 //启动中断接收(使用DMA+空闲中断)
	if(HAL_OK==HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, DMA_RX_BUF_SIZE)){
        elog_i(LOG_TAG_U,"HAL_UARTEx_ReceiveToIdle_DMA success");
    }
    else{
        elog_i(LOG_TAG_U,"HAL_UARTEx_ReceiveToIdle_DMA failed");
    }
	
#endif 
}
uint32_t BSP_UART_Read(uint8_t *data ,uint32_t len)
{
    uint32_t read_len = 0;
    while (read_len < len){
        if(true == RB_Read(&g_uart_rx_rb, &data[read_len])){
            read_len++;
        }
        else{
            break;
        }
    }
    return read_len;
}
#if 0 //使用完全中断接收(没有使用DMA)
	void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
    {
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    /* NOTE: This function should not be modified, when the callback is needed,
            the HAL_UART_RxCpltCallback could be implemented in the user file
    */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        RB_Write(&g_uart_rx_rb, uart_rx_temp);
    #if 0 //使用队列传输
        xQueueSendFromISR(uart_Mailbox,&g_uart_rx_rb,&xHigherPriorityTaskWoken);
    #endif
    #if 1 //使用二值信号量去传输
        xSemaphoreGiveFromISR(uart_Semaphore,&xHigherPriorityTaskWoken);
    #endif
        HAL_UART_Receive_IT(&huart1, &uart_rx_temp, 1);
    
    }
#endif 
#if 1 //使用完全中断接收(使用DMA) HAL_UARTEx_RxEventCallback自带半满全满中断
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
	/* Prevent unused argument(s) compilation warning */
	UNUSED(huart);
	UNUSED(Size);
	uint16_t length;
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	/* === 修复的核心：防抖动/重复触发 === */
    // 如果当前 DMA 位置和上次一样，说明没有新数据，是误触发，直接退出！
    if (Size == old_pos)
    {
        return;
    }
#if 1
	/**
	 * 这里的逻辑是处理 Circular DMA (循环模式) 的指针计算
	 * Size: DMA 当前写到了缓冲区的第几个字节 (下标)
	 * old_pos: 上次我们处理到了第几个字节
	 */
	// 情况1: 正常线性增长 (例如 old=10, Size=50, 收到了 40 字节)
	if (Size > old_pos)
	{
		length = Size - old_pos;
		for (int i = 0; i < length; i++)
		{
			RB_Write(&g_uart_rx_rb, dma_rx_buf[old_pos + i]);
		}
	}
	// 情况2: 发生卷绕 (例如 old=250, Size=10, 缓冲区总长256)
	// 说明 DMA 填满了尾部，跑回开头写了 10 个字节
	else 
	{
		// 第一段：从 old 到 缓冲区末尾
		length = DMA_RX_BUF_SIZE - old_pos;
		for (int i = 0; i < length; i++)
		{
			RB_Write(&g_uart_rx_rb, dma_rx_buf[old_pos + i]);
		}

		// 第二段：从 0 到 Size
		for (int i = 0; i < Size; i++)
		{
			RB_Write(&g_uart_rx_rb, dma_rx_buf[i]);
		}
	}
#endif 
#if 0
	/* 2. 使用模运算计算数据长度 (自动处理卷绕) */
    // 逻辑：(当前位置 - 上次位置 + 总大小) % 总大小
    // 例如：Size=10, Old=2000, Buf=2048 -> (-1990 + 2048) % 2048 = 58 字节
    length = Size - old_pos;
    if (length < 0) 
    {
        length += DMA_RX_BUF_SIZE;
    }

    /* 3. 循环写入 RingBuffer */
    for (int i = 0; i < length; i++)
    {
        // 这里的取余操作 (old_pos + i) % Size 是为了处理从 Buffer 尾部回绕到头部的数据
        uint16_t dma_idx = (old_pos + i) % DMA_RX_BUF_SIZE;
        
        // 【关键诊断】检查是否写入失败！
        if (RB_Write(&g_uart_rx_rb, dma_rx_buf[dma_idx]) == false)
        {
             // 如果进到这里，说明 RingBuffer 满了！这就是由于“数据减少”的原因
             // 可以在这里打个断点验证，或者让 LED 闪烁报警
			g_drop_cnt++;
        }
    }
#endif
	
	// 更新位置，供下一次使用
	old_pos = Size;
	// 如果 Size 刚好等于 DMA_RX_BUF_SIZE，说明 DMA 满了刚卷绕回 0
	// 在某些 HAL 版本中，卷绕时 Size 可能等于 BUF_SIZE，下次进来是 0
	// 为了安全，如果 Size 到了末尾，我们将 old_pos 归零，准备下一次从头开始
	if (old_pos == DMA_RX_BUF_SIZE)
	{
		old_pos = 0;
	}
	// 释放信号量通知 APP
	xSemaphoreGiveFromISR(uart_Semaphore, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#endif
/* 这是 HAL 库的错误回调，当发生 ORE (过载) 等错误时会自动调用 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    // 1. 判断是否是我们关心的那个串口
    if (huart->Instance == USART1)
    {
        // 2. 只有在 DMA 接收过程中出错才处理
        // (虽然 HAL 库可能已经把状态改了，但我们可以强制重启)
        
        // 3. 极其重要：如果重启了 DMA，硬件计数器会归零
        // 所以我们必须把软件记录的 old_pos 也归零，否则下次计算 Size - old_pos 会出错
        // 注意：old_pos 需要是你那个文件里的全局变量或者能被访问到
        // extern volatile uint16_t old_pos; // 如果 old_pos 是 static，你需要想办法重置它
        // 这里假设你能通过某种方式复位 old_pos，或者接受这一次的数据包丢失
        
        /* 注意：由于 old_pos 是 static 变量，只能在那个函数内部访问。
           为了解决这个问题，建议把 HAL_UARTEx_ReceiveToIdle_DMA 再调用一次。
           
           但更粗暴有效的方法是：什么都不做，让 HAL 库自带的恢复机制去跑？
           不，HAL 库不会自动重启接收。
        */
        
        // 4. 清除错误标志 (读取 SR 和 DR，或者使用宏)
        __HAL_UART_CLEAR_OREFLAG(huart);
        __HAL_UART_CLEAR_NEFLAG(huart);
        __HAL_UART_CLEAR_FEFLAG(huart);
        __HAL_UART_CLEAR_PEFLAG(huart);

        // 5. 暴力重启接收 (CPR 急救)
        // 注意：这会重置 DMA 指针到 0
        HAL_UARTEx_ReceiveToIdle_DMA(huart, dma_rx_buf, DMA_RX_BUF_SIZE);
        
        // 6. 如果你能访问 old_pos，必须置 0
         old_pos = 0; 
    }
}
