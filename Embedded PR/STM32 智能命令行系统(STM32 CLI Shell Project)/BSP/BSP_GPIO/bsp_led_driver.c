#include "bsp_led_driver.h"
#include "gpio.h"

/**
 * @brief LED状态设置函数
 * @param[in] state LED状态值
 *                  - 1 或 非0值：点亮LED（输出低电平，假设低电平激活）
 *                  - 0：熄灭LED（输出高电平）
 * 
 * @note 硬件配置说明：
 *       本代码假设使用低电平驱动模式（NPN三极管或低电平有效）
 *       如果使用高电平驱动，需要反转逻辑：
 *       - state=1时输出SET（高电平）
 *       - state=0时输出RESET（低电平）
 * 
 * @note GPIO宏定义（由STM32CubeMX生成）：
 *       - LED_GPIO_Port：LED所在的GPIO端口（如GPIOA）
 *       - LED_Pin：LED所在的GPIO引脚（如GPIO_PIN_5）
 */
void BSP_LED_Set(uint8_t state)
{
    if(state){
        // state非0时，点亮LED（输出低电平）
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);  // 亮
    }
    else{
        // state为0时，熄灭LED（输出高电平）
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);    // 灭
    }
}

/**
 * @brief LED状态翻转函数
 * @note 功能：切换LED的当前状态
 *       如果LED亮，翻转后变暗；如果LED暗，翻转后变亮
 * 
 * @note 应用场景：心跳灯、状态指示灯
 *       可以在定时器中周期调用此函数，实现LED闪烁效果
 */
void BSP_LED_Toggle(void)
{
    // 翻转GPIO引脚电平
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}
