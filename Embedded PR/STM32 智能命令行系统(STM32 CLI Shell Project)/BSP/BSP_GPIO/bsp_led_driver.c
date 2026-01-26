#include "bsp_led_driver.h"
#include "gpio.h"

// 设置 LED 状态
// state: 1=亮, 0=灭 (假设低电平点亮)
void BSP_LED_Set(uint8_t state)
{
    if (state) {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET); // 亮
    } else {
        HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);   // 灭
    }
}

// 翻转 LED (方便做心跳指示)
void BSP_LED_Toggle(void)
{
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
}
