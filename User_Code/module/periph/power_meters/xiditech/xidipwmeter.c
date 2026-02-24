#include "bsp_dwt.h"
#include "bsp_fdcan.h"
#include "xidipwmeter.h"

#include <stdlib.h>
#include <string.h>

static PowerMeterInstance *power_meter_instance = NULL;

/**
 * @brief 功率计数据解码函数
 * @param instance FDCAN实例指针
 *
 * @note 数据格式:
 *        DATA[0]: 电流数值低8位
 *        DATA[1]: 电流数值高8位
 *        DATA[2]: 电压数值低8位
 *        DATA[3]: 电压数值高8位
 *        DATA[4]-DATA[7]: 保留
 *
 *        电压电流数值均为放大100倍后的整数，需要除以100.0得到实际值
 */
void PowerMeterDecode(FDCANInstance *instance)
{
    if (power_meter_instance == NULL)
        return;

    uint8_t *rxbuff = instance->rx_buff;
    PowerMeter_Measure_s *measure = &power_meter_instance->measure;

    // 重载守护进程
    // if (power_meter_instance->daemon != NULL)
    // {
    //     DaemonReload(power_meter_instance->daemon);
    // }

    // 计算时间间隔
    measure->dt = DWT_GetDeltaT(&power_meter_instance->feed_cnt);

    // 解析电流数据 (放大100倍，需要除以100)
    auto current_raw = (int16_t) ((rxbuff[1] << 8) | rxbuff[0]);
    measure->current = (float) current_raw / 100.0f;

    // 解析电压数据 (放大100倍，需要除以100)
    auto voltage_raw = (int16_t) ((rxbuff[3] << 8) | rxbuff[2]);
    measure->voltage = (float) voltage_raw / 100.0f;

    // 计算功率
    measure->power = measure->voltage * measure->current;

    // 计算累计能量 (功率 × 时间)
    measure->energy += measure->power * measure->dt * 0.001f; // dt单位是ms，转换为秒

    measure->update_cnt++;
}

/**
 * @brief 功率计丢失回调函数
 * @param power_meter_ptr 功率计实例指针
 */
void PowerMeterLostCallback(void *power_meter_ptr)
{
    PowerMeterInstance *power_meter = (PowerMeterInstance *) power_meter_ptr;
}

/**
 * @brief 功率计初始化
 * @param can_handle FDCAN句柄指针
 * @return PowerMeterInstance* 功率计实例指针
 */
PowerMeterInstance *PowerMeterInit(FDCAN_HandleTypeDef *can_handle)
{
    // 检查是否已经初始化
    if (power_meter_instance != NULL)
    {
        return power_meter_instance;
    }

    // 分配内存
    power_meter_instance = (PowerMeterInstance *) malloc(sizeof(PowerMeterInstance));
    if (power_meter_instance == NULL)
    {
        return NULL;
    }
    memset(power_meter_instance, 0, sizeof(PowerMeterInstance));

    // 配置FDCAN初始化参数 - 完全适配您的FDCAN驱动
    FDCAN_Init_Config_s can_config = {
            .can_handle = can_handle,
            .rx_id = 0x213, // 功率计反馈标识符
            .tx_id = 0x000, // 不需要发送，设为0
            .can_module_callback = PowerMeterDecode,
            .id = power_meter_instance, // 将实例指针作为ID传递
    };

    // 注册FDCAN实例 - 使用FDCANRegister函数
    power_meter_instance->can_instance = CANRegister(&can_config);
    if (power_meter_instance->can_instance == NULL)
    {
        free(power_meter_instance);
        power_meter_instance = NULL;
        return NULL;
    }

    // 注册守护进程
    // Daemon_Init_Config_s daemon_config = {
    //         .callback = PowerMeterLostCallback,
    //         .owner_id = power_meter_instance,
    //         .reload_count = 5, // 50ms未收到数据则认为丢失 (1000Hz发送，5个周期)
    // };
    // power_meter_instance->daemon = DaemonRegister(&daemon_config);

    return power_meter_instance;
}

/**
 * @brief 获取电压值
 * @param power_meter 功率计实例指针
 * @return float 电压值 (V)
 */
float PowerMeterGetVoltage(PowerMeterInstance *power_meter)
{
    if (power_meter == NULL)
        return 0.0f;
    return power_meter->measure.voltage;
}

/**
 * @brief 获取电流值
 * @param power_meter 功率计实例指针
 * @return float 电流值 (A)
 */
float PowerMeterGetCurrent(PowerMeterInstance *power_meter)
{
    if (power_meter == NULL)
        return 0.0f;
    return power_meter->measure.current;
}

/**
 * @brief 获取功率值
 * @param power_meter 功率计实例指针
 * @return float 功率值 (W)
 */
float PowerMeterGetPower(PowerMeterInstance *power_meter)
{
    if (power_meter == NULL)
        return 0.0f;
    return power_meter->measure.power;
}

/**
 * @brief 获取累计能量
 * @param power_meter 功率计实例指针
 * @return float 累计能量 (J)
 */
float PowerMeterGetEnergy(PowerMeterInstance *power_meter)
{
    if (power_meter == NULL)
        return 0.0f;
    return power_meter->measure.energy;
}

/**
 * @brief 重置累计能量
 * @param power_meter 功率计实例指针
 */
void PowerMeterResetEnergy(PowerMeterInstance *power_meter)
{
    if (power_meter != NULL)
    {
        power_meter->measure.energy = 0.0f;
    }
}

/**
 * @brief 获取功率计实例（用于外部访问）
 * @return PowerMeterInstance* 功率计实例指针
 */
PowerMeterInstance *GetPowerMeterInstance(void)
{
    return power_meter_instance;
}
