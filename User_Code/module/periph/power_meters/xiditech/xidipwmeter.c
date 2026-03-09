#include "bsp_dwt.h"
#include "bsp_fdcan.h"
#include "xidipwmeter.h"

#include <stdlib.h>
#include <string.h>

#include "daemon.h"

static XidiPowerMeterInstance *power_meter_instance = NULL;

/**
 * @brief 功率计数据解码函数
 * @param _instance FDCAN实例指针
 *
 * @note 新版数据格式 (大端模式):
 * DATA[0]: 电压高8位
 * DATA[1]: 电压低8位
 * DATA[2]: 电流高8位
 * DATA[3]: 电流低8位
 * DATA[4]: 功率高8位
 * DATA[5]: 功率低8位
 * DATA[6]: 标识符
 * DATA[7]: 校验和
 */
void XidiPowerMeterDecode(FDCANInstance *_instance)
{
    if (power_meter_instance == NULL)
        return;

    uint8_t *rxbuff = _instance->rx_buff;
    XidiPowerMeter_Msg_s *measure = &power_meter_instance->powermeter_msg;

    // 重载守护进程
    if (power_meter_instance->daemon != NULL)
    {
        DaemonReload(power_meter_instance->daemon);
    }

    // 计算时间间隔
    measure->dt = DWT_GetDeltaT(&power_meter_instance->feed_cnt);

    // 1. 解析电压 (高8位在前，低8位在后)
    uint16_t voltage_raw = (rxbuff[0] << 8) | rxbuff[1];
    measure->voltage = (float) voltage_raw / 100.0f;

    // 2. 解析电流 (高8位在前，低8位在后)
    uint16_t current_raw = (rxbuff[2] << 8) | rxbuff[3];
    measure->current = (float) current_raw / 100.0f;

    // 3. 解析功率 (高8位在前，低8位在后)
    // 既然协议里直接发了功率，我们就直接读硬件算好的，比自己用 U*I 算更准
    uint16_t power_raw = (rxbuff[4] << 8) | rxbuff[5];
    measure->power = (float) power_raw / 100.0f;

    // 4. 计算累计能量 (功率 × 时间)
    measure->energy += measure->power * measure->dt * 0.001f; // dt单位是ms，转换为秒

    measure->update_cnt++;
}

/**
 * @brief 功率计丢失回调函数
 * @param power_meter_ptr 功率计实例指针
 */
void XidiPowerMeterLostCallback(void *power_meter_ptr)
{
    XidiPowerMeterInstance *power_meter = (XidiPowerMeterInstance *) power_meter_ptr;
}

/**
 * @brief 功率计初始化
 * @param XidiPowerMeter_config 功率计初始化配置结构体指针 (必须确保其生命周期是全局或静态的)
 * @return XidiPowerMeterInstance* 功率计实例指针
 */
XidiPowerMeterInstance *XidiPowerMeterInit(XidiPowerMeter_Init_Config_s *XidiPowerMeter_config)
{
    // 1. 检查是否已经初始化
    if (power_meter_instance != NULL)
    {
        return power_meter_instance;
    }

    // 2. 分配内存并清零
    power_meter_instance = (XidiPowerMeterInstance *) malloc(sizeof(XidiPowerMeterInstance));
    if (power_meter_instance == NULL)
    {
        return NULL;
    }
    memset(power_meter_instance, 0, sizeof(XidiPowerMeterInstance));

    // 3. 配置并注册 FDCAN 实例
    // (rx_id, tx_id, can_handle 等参数由外部 config 提供，这里只绑定回调和私有指针)
    XidiPowerMeter_config->can_config.can_module_callback = XidiPowerMeterDecode;
    XidiPowerMeter_config->can_config.id = power_meter_instance;

    power_meter_instance->fdcan_ins = CANRegister(&XidiPowerMeter_config->can_config);
    if (power_meter_instance->fdcan_ins == NULL)
    {
        free(power_meter_instance);
        power_meter_instance = NULL;
        return NULL;
    }

    // 4. 配置并注册 守护进程
    // (reload_count 等参数由外部 config 提供，这里只绑定回调和私有指针)
    XidiPowerMeter_config->daemon_config.callback = XidiPowerMeterLostCallback;
    XidiPowerMeter_config->daemon_config.owner_id = power_meter_instance;

    power_meter_instance->daemon = DaemonRegister(&XidiPowerMeter_config->daemon_config);

    return power_meter_instance;
}

/**
 * @brief 获取电压值
 * @param power_meter 功率计实例指针
 * @return float 电压值 (V)
 */
float XidiPowerMeterGetVoltage(XidiPowerMeterInstance *power_meter)
{
    if (power_meter == NULL)
        return 0.0f;
    return power_meter->powermeter_msg.voltage;
}

/**
 * @brief 获取电流值
 * @param power_meter 功率计实例指针
 * @return float 电流值 (A)
 */
float XidiPowerMeterGetCurrent(XidiPowerMeterInstance *power_meter)
{
    if (power_meter == NULL)
        return 0.0f;
    return power_meter->powermeter_msg.current;
}

/**
 * @brief 获取功率值
 * @param power_meter 功率计实例指针
 * @return float 功率值 (W)
 */
float XidiPowerMeterGetPower(XidiPowerMeterInstance *power_meter)
{
    if (power_meter == NULL)
        return 0.0f;
    return power_meter->powermeter_msg.power;
}

/**
 * @brief 获取累计能量
 * @param power_meter 功率计实例指针
 * @return float 累计能量 (J)
 */
float XidiPowerMeterGetEnergy(XidiPowerMeterInstance *power_meter)
{
    if (power_meter == NULL)
        return 0.0f;
    return power_meter->powermeter_msg.energy;
}

/**
 * @brief 重置累计能量
 * @param power_meter 功率计实例指针
 */
void XidiPowerMeterResetEnergy(XidiPowerMeterInstance *power_meter)
{
    if (power_meter != NULL)
    {
        power_meter->powermeter_msg.energy = 0.0f;
    }
}

/**
 * @brief 获取功率计实例（用于外部访问）
 * @return PowerMeterInstance* 功率计实例指针
 */
XidiPowerMeterInstance *GetXidiPowerMeterInstance(void)
{
    return power_meter_instance;
}
