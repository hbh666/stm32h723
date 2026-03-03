#ifndef HALF_STEER_DEF_H
#define HALF_STEER_DEF_H

#include "stdint.h"

// ================= 遥控器映射系数 =================
#define RC_CHASSIS_SPEED_SCALE  80.0f  // 摇杆转平移速度比例
#define RC_ROTATE_SPEED_SCALE   1.0f   // 小陀螺模式固定自旋速度
#define RC_DEADBAND             10     // 摇杆死区

// ================= 机器人状态枚举 =================
typedef enum
{
    MODE_RELAX = 0, // 急停/掉线模式
    MODE_REMOTE_FOLLOW, // 纯遥控器：底盘跟随云台
    MODE_REMOTE_SPIN, // 纯遥控器：底盘小陀螺
    MODE_AUTO_VISION, // 视觉辅助模式
    MODE_NO_FOLLOW, // 云台底盘分离 (不跟随)
} Robot_State_e;

// ================= 内部控制对象结构体 =================
typedef struct
{
    Robot_State_e current_mode; // 当前状态机模式
    uint8_t init_done; // 初始化完成标志

    // 如果有视觉数据或云台数据需要跨函数传递，也可以加在这里
} HalfSteer_Task_Data_t;

#endif // HALF_STEER_DEF_H
