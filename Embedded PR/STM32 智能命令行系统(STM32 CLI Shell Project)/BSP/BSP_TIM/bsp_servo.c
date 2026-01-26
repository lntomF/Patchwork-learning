#include "bsp_servo.h"
#define LOG_SERVO "bsp_servo"
extern TIM_HandleTypeDef htim2;

void BSP_Servo_Init(void)
{
    HAL_TIM_PWM_Start(&htim2,TIM_CHANNEL_1);
}

void BSP_Servo_SetAngle(uint8_t angle)
{
    if(angle > 180) angle = 180;
    uint32_t pulse = 500 + (angle * 2000 / 180);
	
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse);
}

