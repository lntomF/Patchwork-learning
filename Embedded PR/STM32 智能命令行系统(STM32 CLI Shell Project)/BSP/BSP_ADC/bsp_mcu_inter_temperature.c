#include "bsp_mcu_inter_temperature.h"
#include "adc.h"
#include <stdio.h>

// F4 系列典型参数 (查阅 F411 数据手册)
#define V25             0.76f      // 25℃ 时的电压 (Volts)
#define AVG_SLOPE       0.0025f    // 温度斜率 (2.5 mV/℃)
#define VREF_INT        1.21f      // 内部参考电压标准值 (1.21V)

// 读取 ADC 原始值（通用函数）
static uint32_t BSP_ADC_ReadRaw(void)
{
    
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint32_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1); // 单次转换模式下可省略
    return val;
}

// 获取芯片温度 (单位: ℃)
float BSP_Get_ChipTemp(void)
{
    // 1. 读取 内部参考电压通道 (用于校准电源波动)
    // 假设 Rank 1 是 Vrefint
    uint32_t vref_raw = BSP_ADC_ReadRaw();
    
    // 2. 读取 温度传感器通道
    // 假设 Rank 2 是 TempSensor
    uint32_t temp_raw = BSP_ADC_ReadRaw();
    
    // 3. 计算实际 VDDA 电压 (避免 LDO 供电不准导致的误差)
    // 实际 VDDA = 3.3 * (VREF_INT_CAL / vref_raw) -> 这里简化用标准值
    // Vref_int (1.21V) / Vdda = vref_raw / 4095
    float vdda = 3.3f; // 如果想更准，可以用: (1.21f * 4095.0f) / vref_raw;
    
    // 4. 计算温度传感器电压 Vsense
    float v_sense = (float)temp_raw * vdda / 4095.0f;
    
    // 5. 套用 F4 公式: Temp = ((Vsense - V25) / Avg_Slope) + 25
    float temp = ((v_sense - V25) / AVG_SLOPE) + 25.0f;
    
    return temp;
}

