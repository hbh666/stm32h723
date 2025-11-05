// #pragma once // 可以用#pragma once代替#ifndef ROBOT_DEF_H(header guard)
#ifndef ROBOT_DEF_H
#define ROBOT_DEF_H

typedef enum
{
    NORMAL_MODE = 0,            // 正常模式，对应数值0
    SYS_ERROR_OCCURRED = 1,     // 系统错误，对应数值1
    REMOTE_NOT_CONNECTED = 2,   // 遥控器未连接，对应数值2
    REMOTE_CONNECTED = 3,       // 遥控器已连接，对应数值3
    AUTO_SHOOTING_MODE = 4,      // 成功进入自动瞄准模式，对应数值4
    IMU_CALIBERATION_MODE = 5  // IMU校准模式，对应数值5
    // 添加其他模式
} RobotMode_t; //机器人控制模式

// 声明并初始化变量
RobotMode_t RobotMode = REMOTE_NOT_CONNECTED;  // 初始状态为遥控器未连接


// #pragma pack() // 开启字节对齐,结束前面的#pragma pack(1)

#endif