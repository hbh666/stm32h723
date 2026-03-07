// ==================== 1. 包含必要的头文件 ====================
#include "dev_cmd.h"
#include "general_def.h"
#include "bsp_log.h"
// 引入你的功率计头文件
#include "cmsis_os2.h"
#include "xidipwmeter.h"


// ==================== 2. 定义局部指针 ====================
static PowerMeterInstance *chassis_power_meter; // 功率计实例指针

// 存放读取到的功率数据（也可以直接放进发给底盘的结构体里）
static float current_power_w = 0.0f;
static float current_voltage_v = 0.0f;
static float current_energy_j = 0.0f;

// ==================== 3. 在初始化函数中注册 ====================
void RobotCMDInit()
{
    chassis_power_meter = PowerMeterInit(&hfdcan1);
    if (chassis_power_meter == NULL)
    {
        LOGERROR("[CMD] Power Meter Init Failed!");
    }
}

// ==================== 4. 在控制任务中读取使用 ====================
void CmdTask()
{
    RobotCMDInit();
    while (1)
    {
        if (chassis_power_meter != NULL)
        {
            current_power_w = PowerMeterGetPower(chassis_power_meter);
            current_voltage_v = PowerMeterGetVoltage(chassis_power_meter);
            current_energy_j = PowerMeterGetEnergy(chassis_power_meter);

            // 你可以将获取到的功率写入要发送给底盘的 cmd 中，例如限制最大速度
            // chassis_cmd_send.real_power = current_power_w;
        }

        osDelay(10); // 根据实际需要调整读取频率
    }
}
