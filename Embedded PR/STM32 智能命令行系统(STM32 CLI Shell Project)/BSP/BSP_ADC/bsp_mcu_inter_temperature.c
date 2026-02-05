#include "bsp_mcu_inter_temperature.h"
#include "adc.h"
#include <stdio.h>

/**
 * @brief STM32F4系列内部温度传感器校准参数
 * @note 这些参数来自STM32F411官方数据手册
 *       不同系列的参数可能略有不同，需要参考相应的数据手册
 */
#define V25             0.76f       // 25℃时的参考电压（单位：V）
#define AVG_SLOPE       0.0025f     // 温度-电压斜率（2.5 mV/℃ = 0.0025 V/℃）
#define VREF_INT        1.21f       // 内部参考电压标准值（1.21V）

/**
 * @brief 读取ADC原始转换值
 * @return ADC的12位原始转换值 (0-4095)
 * 
 * @note 工作流程：
 *       1. 启动ADC转换
 *       2. 等待转换完成（轮询）
 *       3. 读取转换结果
 *       4. 停止ADC（单次模式下可省略）
 * 
 * @note 说明：
 *       - HAL_ADC_Start()：启动单次转换
 *       - HAL_ADC_PollForConversion()：轮询等待转换完成（超时10ms）
 *       - HAL_ADC_GetValue()：读取转换结果
 *       - HAL_ADC_Stop()：停止ADC
 */
static uint32_t BSP_ADC_ReadRaw(void)
{
    // 启动ADC转换
    HAL_ADC_Start(&hadc1);
    
    // 轮询等待转换完成（超时时间10ms）
    HAL_ADC_PollForConversion(&hadc1, 10);
    
    // 读取转换的原始值（12位，范围0-4095）
    uint32_t val = HAL_ADC_GetValue(&hadc1);
    
    // 停止ADC（单次转换模式下可以省略，但为了安全还是停止）
    HAL_ADC_Stop(&hadc1);
    
    return val;
}

/**
 * @brief 获取MCU芯片内部温度
 * @return 芯片当前温度（单位：℃，精度约±3℃）
 * 
 * @note 原理说明：
 *       STM32内置温度传感器是一个PN结，其正向压降随温度变化而变化
 *       通过测量其输出电压，可以反推当前温度
 *       
 *       关键参数：
 *       - Vsense：温度传感器输出电压
 *       - V25：25℃时的参考电压（0.76V）
 *       - AvgSlope：温度斜率（2.5mV/℃）
 *       
 *       转换公式：
 *       Temp = ((Vsense - V25) / AvgSlope) + 25
 * 
 * @note 精度影响因素：
 *       1. VDDA供电波动会影响ADC转换精度
 *       2. 工作温度范围：-40℃ ~ 125℃
 *       3. 绝对精度：±3℃（官方规格）
 * 
 * @note 工作步骤：
 *       1. 读取内部参考电压通道（Vrefint）- 用于补偿VDDA波动
 *       2. 读取温度传感器通道
 *       3. 计算实际VDDA电压
 *       4. 计算温度传感器电压Vsense
 *       5. 代入公式计算温度
 */
float BSP_Get_ChipTemp(void)
{
    // 步骤1：读取内部参考电压通道（Vrefint）
    // 用途：补偿VDDA电压波动对ADC转换的影响
    // Vrefint通道通常在ADC配置中的Rank1
    uint32_t vref_raw = BSP_ADC_ReadRaw();
    
    // 步骤2：读取温度传感器通道
    // 温度传感器通道通常在ADC配置中的Rank2
    uint32_t temp_raw = BSP_ADC_ReadRaw();
    
    // 步骤3：计算实际的VDDA供电电压
    // 原始方法：vdda = (VREF_INT_CAL / vref_raw) * 3.3V
    // 简化处理：直接使用标准值3.3V（如不需要高精度）
    // 精确方法：vdda = (1.21 * 4095.0) / vref_raw （根据实际Vref补偿）
    float vdda = 3.3f;  // 简化版本，假设供电稳定
    // 精确版本：float vdda = (VREF_INT * 4095.0f) / vref_raw;
    
    // 步骤4：将ADC原始值转换为电压值
    // 公式：V = (ADC值 / 4095) * VDDA
    // 其中4095是12位ADC的最大值
    float v_sense = (float)temp_raw * vdda / 4095.0f;
    
    // 步骤5：代入STM32F4温度转换公式
    // Temp = ((Vsense - V25) / AvgSlope) + 25
    // 例如：
    // - Vsense = 0.76V，则 Temp = ((0.76 - 0.76) / 0.0025) + 25 = 25℃
    // - Vsense = 0.71V，则 Temp = ((0.71 - 0.76) / 0.0025) + 25 = 5℃
    float temp = ((v_sense - V25) / AVG_SLOPE) + 25.0f;
    
    return temp;
}

