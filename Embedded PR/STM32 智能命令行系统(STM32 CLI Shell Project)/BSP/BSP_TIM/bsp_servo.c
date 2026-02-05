#include "bsp_servo.h"
#define LOG_SERVO "bsp_servo"
extern TIM_HandleTypeDef htim2;     // 定时器2句柄（由STM32CubeMX生成）

/**
 * @brief 舵机/电机PWM初始化函数
 * @note 功能：启动定时器2的PWM输出通道1
 *       PWM参数由STM32CubeMX配置：
 *       - 时钟源：APB1（通常84MHz）
 *       - 分频系数、重装值等由CubeMX自动设置
 */
void BSP_Servo_Init(void)
{
    // 启动TIM2的PWM输出（通道1）
    // 舵机通常接在这个引脚上
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
}

/**
 * @brief 舵机角度设置函数
 * @param[in] angle 舵机角度 (0-180度)
 *                  0度：左转到底
 *                  90度：中间位置
 *                  180度：右转到底
 * 
 * @note 工作原理：
 *       标准舵机PWM参数：
 *       - 周期：20ms（50Hz频率）
 *       - 脉冲宽度范围：0.5ms~2.5ms
 *       - 0.5ms对应0度，1.5ms对应90度，2.5ms对应180度
 *       
 *       计算公式：
 *       pulse_width_ms = 0.5 + (angle / 180) * 2.0
 *       pulse_width_us = pulse_width_ms * 1000
 *       
 *       例如：
 *       - angle=0°：pulse = 0.5ms
 *       - angle=90°：pulse = 1.5ms
 *       - angle=180°：pulse = 2.5ms
 * 
 * @note 本代码的计算（假设TIM配置为20ms周期）：
 *       pulse = 500 + (angle * 2000 / 180)
 *       其中500对应0.5ms，2000对应2.0ms范围
 */
void BSP_Servo_SetAngle(uint8_t angle)
{
    // 限制角度范围在0-180度
    if(angle > 180){
        angle = 180;
    }
    
    // 计算对应的PWM脉冲值
    // 公式：pulse = 500 + (angle * 2000 / 180)
    // 这里假设TIM配置的周期对应0-4000的计数值
    // 500 -> 0.5ms (0°)
    // 2500 -> 2.5ms (180°)
    uint32_t pulse = 500 + (angle * 2000 / 180);
    
    // 设置PWM比较值，改变脉冲宽度
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse);
}

