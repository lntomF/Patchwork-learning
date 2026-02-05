#include "bsp_uart_driver.h"

/* USER CODE BEGIN Variables */
#define LOG_TAG_U "BSP_UART_LOG"
extern UART_HandleTypeDef huart1;          // UART1句柄（由STM32CubeMX生成）
extern DMA_HandleTypeDef hdma_usart1_rx;   // DMA接收句柄（由STM32CubeMX生成）
#if 0 //使用队列传输
QueueHandle_t uart_Mailbox = NULL;
#endif 
#if 1 //使用二值信号量去传输
SemaphoreHandle_t uart_Semaphore = NULL;   // 二值信号量，用于同步DMA接收完成事件
#endif

/**
 * @brief 环形缓冲区实例定义
 * @note 用途：临时存储DMA接收的数据，供APP任务读取
 *       大小：4096字节，足以缓存多个数据包
 */
#define RX_POOL_SIZE 4096
static uint8_t uart_rx_pool[RX_POOL_SIZE];       // 环形缓冲区数据池
static RingBuffer_t g_uart_rx_rb;                // 环形缓冲区实例

/**
 * @brief DMA接收缓冲区定义
 * @note DMA直接写入此缓冲区，不经过软件，效率最高
 *       大小：2048字节，支持循环模式（circular DMA）
 */
#define DMA_RX_BUF_SIZE 2048
static uint8_t dma_rx_buf[DMA_RX_BUF_SIZE];    // DMA原始接收缓冲区
static uint16_t old_pos = 0;                    // 上一次处理的DMA位置，用于计算新增数据量
volatile uint32_t g_drop_cnt = 0;              // 丢包计数器（环形缓冲区满时递增）

/* USER CODE END Variables */

/**
 * @brief UART和DMA初始化函数
 * @note 工作流程：
 *       1. 初始化环形缓冲区
 *       2. 创建FreeRTOS同步原语（信号量或队列）
 *       3. 启动DMA接收+空闲中断模式
 * @note 同步机制选择：
 *       - 队列方案：单字节接收，开销大，已禁用
 *       - 信号量方案：批量接收，效率高，当前使用
 */
void BSP_UART_Init(void)
{
    // 初始化环形缓冲区，为后续数据缓存做准备
    RB_Init(&g_uart_rx_rb, uart_rx_pool, RX_POOL_SIZE);
    
#if 0 //创建队列传输（已禁用）
    /* USER CODE BEGIN RTOS_QUEUES */
    /* add queues, ... */
    //queue 1
    uart_Mailbox = NULL;
    uart_Mailbox = xQueueCreate(1, 4);
    /* USER CODE END RTOS_QUEUES */
    if(uart_Mailbox == NULL){
        elog_i(LOG_TAG_U, "uart_Mailbox create failed");
    }
    else{
        elog_i(LOG_TAG_U, "uart_Mailbox create success");
    }
#endif 

#if 1 //创建二值信号量去传输（当前使用）
    // 创建二值信号量，用于同步DMA接收完成事件
    uart_Semaphore = xSemaphoreCreateBinary();
    if(NULL == uart_Semaphore){
        elog_i(LOG_TAG_U, "uart_Semaphore create failed");
    }else{
        elog_i(LOG_TAG_U, "uart_Semaphore create success");
    }
    // 初始化DMA位置指针，供后续卷绕处理使用
    old_pos = 0;
#endif 

#if 0 //启动中断接收(没有使用DMA，已禁用)
    // 单字节中断接收方式，已被DMA方案替代
    if(HAL_OK == HAL_UART_Receive_IT(&huart1, &uart_rx_temp, 1)){
        elog_i(LOG_TAG_U, "HAL_UART_Receive_IT success");
    }
    else{
        elog_i(LOG_TAG_U, "HAL_UART_Receive_IT failed");
    }
#endif    

#if 1 //启动DMA+空闲中断接收（当前使用）
    // HAL_UARTEx_ReceiveToIdle_DMA：
    // - ReceiveToIdle：在接收到UART空闲信号时触发中断（数据停止传输时）
    // - DMA：使用DMA自动接收，不占用CPU
    // 这个组合方案既保证了实时性，也提高了CPU效率
    if(HAL_OK == HAL_UARTEx_ReceiveToIdle_DMA(&huart1, dma_rx_buf, DMA_RX_BUF_SIZE)){
        elog_i(LOG_TAG_U, "HAL_UARTEx_ReceiveToIdle_DMA success");
    }
    else{
        elog_i(LOG_TAG_U, "HAL_UARTEx_ReceiveToIdle_DMA failed");
    }
#endif 
}

/**
 * @brief UART数据读取函数
 * @param[out] data 存储读取数据的缓冲区
 * @param[in] len 请求读取的最大字节数
 * @return 实际读取的字节数（可能小于len，因为环形缓冲区可能没有那么多数据）
 * @note 工作流程：
 *       1. 从环形缓冲区逐字节读取
 *       2. 如果缓冲区中数据不足len字节，则返回实际读取的数量
 */
uint32_t BSP_UART_Read(uint8_t *data, uint32_t len)
{
    uint32_t read_len = 0;
    // 逐字节从环形缓冲区读取，直到缓冲区空或达到请求长度
    while(read_len < len){
        if(true == RB_Read(&g_uart_rx_rb, &data[read_len])){
            read_len++;
        }
        else{
            // 缓冲区已读完，返回已读的字节数
            break;
        }
    }
    return read_len;
}

#if 0 //使用单字节中断接收的回调函数（已禁用）
/**
 * @brief UART接收完成中断回调（单字节模式，已禁用）
 * @note 单字节中断方案的问题：
 *       1. 每接收一个字节就触发一次中断，开销大
 *       2. 不适合高速、批量数据接收
 *       已被DMA+空闲中断方案替代
 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    // 将接收到的单字节写入环形缓冲区
    RB_Write(&g_uart_rx_rb, uart_rx_temp);
    
#if 0 //使用队列传输
    xQueueSendFromISR(uart_Mailbox, &g_uart_rx_rb, &xHigherPriorityTaskWoken);
#endif
    
#if 1 //使用二值信号量去传输
    xSemaphoreGiveFromISR(uart_Semaphore, &xHigherPriorityTaskWoken);
#endif
    
    // 继续接收下一个字节
    HAL_UART_Receive_IT(&huart1, &uart_rx_temp, 1);
}
#endif 

#if 1 //使用DMA+空闲中断接收的回调函数（当前使用）
/**
 * @brief UART空闲+DMA接收完成中断回调
 * @param[in] huart UART句柄
 * @param[in] Size DMA当前写入位置（字节下标，0-DMA_RX_BUF_SIZE之间）
 * 
 * @note 核心难点：处理循环DMA的卷绕
 *       - 循环DMA持续写入固定大小的缓冲区，写到末尾后自动回到开头
 *       - 每次中断时Size表示DMA当前写到的位置
 *       - 需要通过Size和old_pos的差值计算出新增了多少数据
 *       
 * @note 两种数据增长情况：
 *       1. 正常线性增长：Size > old_pos（直接取差值）
 *       2. 发生卷绕：Size < old_pos（从old到缓冲区末尾，再从0到Size）
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    /* Prevent unused argument(s) compilation warning */
    UNUSED(huart);
    UNUSED(Size);
    
    uint16_t length;
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    
    /* === 防止误触发/重复触发 === */
    // 如果当前DMA位置和上次相同，说明没有新数据，直接退出
    // 这个检查很重要，可以避免处理空数据
    if(Size == old_pos){
        return;
    }

#if 1
    /**
     * @brief 循环DMA数据提取核心算法
     * 
     * Size: DMA当前写入位置（0到DMA_RX_BUF_SIZE之间）
     * old_pos: 上次我们处理完数据的位置
     * 
     * 例子：
     * - 缓冲区大小：256字节
     * - 情况1（正常）：old_pos=10, Size=50
     *   新增数据：50-10=40字节，在[10, 50)区间
     * - 情况2（卷绕）：old_pos=240, Size=30
     *   新增数据：(256-240) + 30 = 46字节
     *   第一段：[240, 256) = 16字节
     *   第二段：[0, 30) = 30字节
     */
    if(Size > old_pos){
        // 情况1：正常线性增长，直接计算差值
        length = Size - old_pos;
        // 将DMA缓冲区中[old_pos, Size)的数据写入环形缓冲区
        for(int i = 0; i < length; i++){
            RB_Write(&g_uart_rx_rb, dma_rx_buf[old_pos + i]);
        }
    }
    else{
        // 情况2：发生卷绕，DMA写到了缓冲区末尾，又从头开始
        // 例如：old_pos=240, Size=30, 缓冲区大小=256
        
        // 第一段：从old_pos到缓冲区末尾
        length = DMA_RX_BUF_SIZE - old_pos;
        for(int i = 0; i < length; i++){
            RB_Write(&g_uart_rx_rb, dma_rx_buf[old_pos + i]);
        }
        
        // 第二段：从缓冲区开头到Size
        for(int i = 0; i < Size; i++){
            RB_Write(&g_uart_rx_rb, dma_rx_buf[i]);
        }
    }
#endif 

#if 0
    /**
     * @brief 另一种卷绕处理方法（使用模运算）
     * 优点：代码更简洁
     * 缺点：逻辑不如分情况讨论直观
     */
    // 计算新增数据长度（自动处理卷绕）
    length = Size - old_pos;
    if(length < 0){
        length += DMA_RX_BUF_SIZE;
    }
    
    // 循环写入，取余操作自动处理回绕
    for(int i = 0; i < length; i++){
        uint16_t dma_idx = (old_pos + i) % DMA_RX_BUF_SIZE;
        
        // 【诊断关键点】检查是否写入失败
        if(RB_Write(&g_uart_rx_rb, dma_rx_buf[dma_idx]) == false){
            // 环形缓冲区满了，数据丢失，计数器递增
            g_drop_cnt++;
        }
    }
#endif
    
    // 更新位置指针，供下一次中断使用
    old_pos = Size;
    
    // 如果Size恰好等于缓冲区大小，说明DMA刚好写满并卷绕回0
    // 为了安全起见，手动设置old_pos=0，准备下一轮接收
    if(old_pos == DMA_RX_BUF_SIZE){
        old_pos = 0;
    }
    
    // 释放信号量，通知APP任务有数据可读
    xSemaphoreGiveFromISR(uart_Semaphore, &xHigherPriorityTaskWoken);
    // 触发上文文切换（如果有更高优先级任务被唤醒）
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
#endif

/**
 * @brief UART错误回调函数
 * @param[in] huart UART句柄
 * @note 可能的错误类型：
 *       - ORE (Overrun Error)：接收缓冲区溢出，数据丢失
 *       - NE (Noise Error)：信号线噪声错误
 *       - FE (Framing Error)：帧格式错误（停止位异常）
 *       - PE (Parity Error)：奇偶校验错误
 * @note 错误恢复策略：
 *       1. 清除错误标志位
 *       2. 重新启动DMA接收
 *       3. 重置软件指针（old_pos）
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    // 1. 判断是否是USART1（可能有多个UART实例）
    if(huart->Instance == USART1){
        // 2. 仅在DMA接收过程中处理错误
        
        // 3. 【关键】重启DMA时硬件计数器会归零
        // 因此必须同步重置软件指针old_pos，否则下次计算会出错
        // 注意：old_pos是static变量，只能在本文件访问
        
        // 4. 清除各种UART错误标志位
        __HAL_UART_CLEAR_OREFLAG(huart);    // 清除过载错误标志
        __HAL_UART_CLEAR_NEFLAG(huart);     // 清除噪声错误标志
        __HAL_UART_CLEAR_FEFLAG(huart);     // 清除帧格式错误标志
        __HAL_UART_CLEAR_PEFLAG(huart);     // 清除奇偶校验错误标志
        
        // 5. 暴力重启DMA接收（CPR急救方式）
        // 注意：这会将DMA写指针重置到0
        HAL_UARTEx_ReceiveToIdle_DMA(huart, dma_rx_buf, DMA_RX_BUF_SIZE);
        
        // 6. 关键：同步重置软件指针
        // 由于DMA已重启，硬件指针变为0，软件指针也必须变为0
        old_pos = 0;
    }
}
