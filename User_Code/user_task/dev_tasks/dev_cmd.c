// ==================== 1. 包含必要的头文件 ====================
#include "dev_cmd.h"
#include "general_def.h"
#include "bsp_log.h"
// 引入你的功率计头文件
#include "cmsis_os2.h"
#include "xidipwmeter.h"


// ==================== 2. 定义局部指针 ====================
static XidiPowerMeterInstance *chassis_power_meter; // 功率计实例指针

// 存放读取到的功率数据（也可以直接放进发给底盘的结构体里）
static float current_power_w = 0.0f;
static float current_voltage_v = 0.0f;
static float current_energy_j = 0.0f;

// ==================== 3. 在初始化函数中注册 ====================
// 1. 定义为静态变量（static），确保它的内存空间常驻，防止底层指针悬垂！
static XidiPowerMeter_Init_Config_s xidi_pm_config = {
    .can_config = {
        .can_handle = &hfdcan1,  // 原来直接传的句柄，现在放进结构体里
        .rx_id = 0x213,          // 接收 ID
        .tx_id = 0x000,          // 发送 ID (不发送填0)
    },
    .daemon_config = {
        .reload_count = 5,       // 守护进程超时周期 (例如：5次没收到算离线)
    }
};

// 2. 初始化任务
void RobotCMDInit()
{
    // 传入配置结构体的地址，而不是直接传 hfdcan1
    chassis_power_meter = XidiPowerMeterInit(&xidi_pm_config);

    if (chassis_power_meter == NULL)
    {
        LOGERROR("[CMD] Power Meter Init Failed!");
    }
    else
    {
        // 建议加一句成功日志，方便上机调试时确认
        LOGINFO("[CMD] Power Meter Init Success!");
    }
}
// ==================== 4. 在控制任务中读取使用 ====================
void CmdTask(void *argument)
{
    RobotCMDInit();
    DaemonReload(chassis_power_meter->daemon);
    while (1)
    {
        if (chassis_power_meter != NULL)
        {
            current_power_w = XidiPowerMeterGetPower(chassis_power_meter);
            current_voltage_v = XidiPowerMeterGetVoltage(chassis_power_meter);
            current_energy_j = XidiPowerMeterGetEnergy(chassis_power_meter);

            // 你可以将获取到的功率写入要发送给底盘的 cmd 中，例如限制最大速度
            // chassis_cmd_send.real_power = current_power_w;
        }

        osDelay(10); // 根据实际需要调整读取频率
    }
}