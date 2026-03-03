#include "halfsteering_cmd.h"
#include "halfsteering_def.h"
#include "chassis_half_steer.h" // 包含底盘应用层结构体
#include "pid.h"
#include "rc.h"
#include "user_lib.h"
#include <string.h>
#include <math.h>

// --- 外部依赖 ---
extern Chassis_HalfSteer_t chassis_half_steer; // 假设在app层定义好的底盘实例

// --- 本地任务数据 ---
static HalfSteer_Task_Data_t task_data;
static const RC_ctrl_t *local_rc_ctrl;

// ================= PID 参数定义与实例化 =================
static PIDInstance chassis_follow_pid;
// 按你的格式直接在这里定义并初始化配置结构体
static PID_Init_Config_s chassis_follow_pid_conf = {
    .Kp = 6.0f,
    .Ki = 0.0f,
    .Kd = 0.495f,
    // .DeadBand          = 0.5f,
    //.CoefA             = 0.2f,
    //.CoefB             = 0.3f,
    //.Improve           = PID_Trapezoid_Intergral | PID_DerivativeFilter | PID_Derivative_On_Measurement | PID_Integral_Limit,
    //.IntegralLimit     = 50.0f,
    .MaxOut = 45.0f,
    //.Derivative_LPF_RC = 0.01f,
};

// --- 私有函数声明 ---
static void ModeSelection(void);

static void RemoteControlSet(Chassis_Ctrl_Cmd_s *cmd);

/**
 * @brief 任务初始化
 */
void RobotCMDInit(void)
{
    // 1. 获取遥控器指针
    local_rc_ctrl = RC_Get_RC_Pointer(); // 请替换为你实际获取遥控器数据的接口

    // 2. 初始化底盘跟随 PID
    PIDInit(&chassis_follow_pid, &chassis_follow_pid_conf);

    // 3. 任务数据清零
    memset(&task_data, 0, sizeof(HalfSteer_Task_Data_t));
    task_data.current_mode = MODE_RELAX;
    task_data.init_done = 1;
}

/**
 * @brief 任务核心循环 (放置于 RTOS Task 中)
 */
void RobotCMDTask(void)
{
    if (!task_data.init_done) return;

    Chassis_Ctrl_Cmd_s chassis_cmd_send;
    memset(&chassis_cmd_send, 0, sizeof(Chassis_Ctrl_Cmd_s));

    // 1. 状态机选择
    ModeSelection();

    // 2. 根据状态设定底层指令
    RemoteControlSet(&chassis_cmd_send);

    // 3. 将计算完成的指令发送给底盘应用层
    Chassis_HalfSteer_Update(&chassis_half_steer, &chassis_cmd_send);
}

/**
 * @brief 状态机模式选择 (复刻你的多拨杆判断逻辑)
 */
static void ModeSelection(void)
{
    // 左侧[中], 右侧[下] -> 视觉模式
    if (switch_is_mid(local_rc_ctrl->rc.s[1]) && switch_is_down(local_rc_ctrl->rc.s[0]))
    {
        task_data.current_mode = MODE_AUTO_VISION;
    }
    // 左侧[中], 右侧[中] -> 遥控器不跟随
    else if (switch_is_mid(local_rc_ctrl->rc.switch[1])
    &&
    switch_is_mid(local_rc_ctrl->rc.switch[0])
    ) {
        task_data.current_mode = MODE_NO_FOLLOW;
    }
    // 左侧[下], 右侧[下] -> 底盘小陀螺
    else
    if (switch_is_down(local_rc_ctrl->rc.s[1]) && switch_is_down(local_rc_ctrl->rc.s[0]))
    {
        task_data.current_mode = MODE_REMOTE_SPIN;
    }
    // 左侧[下], 右侧[中] -> 遥控器不跟随 (你原代码中的独立判断)
    else if (switch_is_down(local_rc_ctrl->rc.s[1]) && switch_is_mid(local_rc_ctrl->rc.s[0]))
    {
        task_data.current_mode = MODE_NO_FOLLOW;
    }
    // 左侧[下] (单边条件兜底) -> 默认跟随模式
    else if (switch_is_down(local_rc_ctrl->rc.s[1]))
    {
        task_data.current_mode = MODE_REMOTE_FOLLOW;
    }
    else
    {
        task_data.current_mode = MODE_RELAX;
    }
}

/**
 * @brief 遥控器映射与 PID 计算
 */
static void RemoteControlSet(Chassis_Ctrl_Cmd_s *cmd)
{
    if (task_data.current_mode == MODE_RELAX)
    {
        cmd->chassis_mode = CHASSIS_ZERO_FORCE;
        return;
    }

    // --- 1. 底盘基础平移 (水平和竖直方向) ---
    cmd->vx = RC_CHASSIS_SPEED_SCALE * ((float) local_rc_ctrl->rc.ch[1] / 660.0f); // 摇杆前进
    cmd->vy = RC_CHASSIS_SPEED_SCALE * ((float) local_rc_ctrl->rc.ch[0] / 660.0f); // 摇杆平移

    // 摇杆死区处理
    if (fabsf(local_rc_ctrl->rc.ch[1]) < RC_DEADBAND) cmd->vx = 0;
    if (fabsf(local_rc_ctrl->rc.ch[0]) < RC_DEADBAND) cmd->vy = 0;

    // --- 2. 旋转量与模式映射 ---
    switch (task_data.current_mode)
    {
        case MODE_REMOTE_FOLLOW:
            cmd->chassis_mode = CHASSIS_FOLLOW_GIMBAL_YAW;
            // 注意：此处需要你获取到底盘和云台的实际偏差角 (例如从 Gimbal 结构体或电机 feedback 中拿)
            // 假设获取到的夹角叫 angle_error，并已转换到 [-180, 180] 之间
            float angle_error = 0.0f; /* 替换为获取真实误差的代码 */

            // 使用在文件头部实例化的 chassis_follow_pid 计算 wz 输出
            cmd->wz = PIDCalculate(&chassis_follow_pid, angle_error, 0.0f) / 100.0f;
            break;

        case MODE_REMOTE_SPIN:
            cmd->chassis_mode = CHASSIS_ROTATE;
            cmd->wz = RC_ROTATE_SPEED_SCALE;
            break;

        case MODE_AUTO_VISION:
            // 在你给的代码中，24赛季检录视觉模式时让底盘转小陀螺
            cmd->chassis_mode = CHASSIS_ROTATE; // CHASSIS_RE_ROTATE 对应底层小陀螺逻辑
            cmd->wz = RC_ROTATE_SPEED_SCALE;
            break;

        case MODE_NO_FOLLOW:
            cmd->chassis_mode = CHASSIS_NO_FOLLOW;
            cmd->wz = 0.0f; // 如果需要拨杆控制旋转，可以在这里加入摇杆映射
            break;

        default:
            cmd->chassis_mode = CHASSIS_ZERO_FORCE;
            break;
    }
}
