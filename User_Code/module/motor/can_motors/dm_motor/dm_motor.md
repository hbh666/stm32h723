# 达妙电机

在 `gimbal.c` 中进行达妙电机的初始化配置并使用“一拖四”模式发送指令，你可以按照以下结构来组织代码。

### 1. 初始化配置与指定ID

在初始化阶段，你需要先配置好 `Motor_Init_Config_s`，并通过刚才新增的 `DMMotorSetCtrlMode` 函数将实例切换为一拖四模式。电机的 **ID** 是在 `can_init_config.tx_id` 中指定的。

```c
#include "DMmotor.h"

// 1. 定义电机配置结构体 (以ID为1的电机为例)
static Motor_Init_Config_s gimbal_dm_config_id1 = {
    .can_init_config = {
        .can_handle = &hcan1,      // 指定使用的CAN外设句柄，例如 hcan1 或 hcan2
        .tx_id      = 0x01,        // 【指定电机ID】这里填入电机的实际ID，如 1
    },
    .controller_param_init_config = {
        // 虽然一控四主要是直接下发电流，但为了结构完整性或外环计算，可配置相关PID
        .current_PID = {
            .Kp = 6.0f,
            .Ki = 0.0f,
            .Kd = 0.495f,
            .MaxOut = 45.0f,
        },
    },
    .controller_setting_init_config = {
        .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
    }
};

// 2. 声明电机实例指针
DMMotorInstance *gimbal_motor_1;
// 如果同一条总线上有另外三个电机，你需要分别为它们声明实例并配置 tx_id = 2, 3, 4

void Gimbal_Init(void)
{
    // 3. 调用Init函数完成底层初始化与实例分配
    gimbal_motor_1 = DMMotorInit(&gimbal_dm_config_id1);
    
    // 4. 【关键步骤】将该电机控制模式切换为一控四模式
    DMMotorSetCtrlMode(gimbal_motor_1, DM_CTRL_ONE_TO_FOUR);
    
    // (同理，对ID为2、3、4的电机执行相同的Init和SetCtrlMode操作)
}

```

### 2. 使用一拖四模式发送指令

在控制任务（例如 FreeRTOS 的 `Gimbal_Task`）中，你不再需要让每个电机单独发送报文，而是通过刚才新增的 `DMMotorSendOneToFourGroup` 函数**统一打包下发**。

```c
void Gimbal_Task(void const * argument)
{
    // 假设通过你的控制器（如LQR或MPC等）计算得出了4个电机的目标电流
    // 单位与你设定DM_T_MIN、DM_T_MAX的量纲一致
    float target_i1 = 1.5f;
    float target_i2 = -0.5f;
    float target_i3 = 2.0f;
    float target_i4 = 0.0f;

    while(1)
    {
        // ... (各种控制算法计算过程) ...

        // 调用一控四发送函数打包下发控制帧
        // 参数1: can_instance -> 传入挂载在该CAN总线上的任一电机实例的CAN指针即可
        // 参数2: group -> 1 表示控制电机 ID[1~4] (对应报文 0x3FE)
        //                 2 表示控制电机 ID[5~8] (对应报文 0x4FE)
        // 参数3~6: 分别对应这4个电机的电流值
        DMMotorSendOneToFourGroup(gimbal_motor_1->motor_can_instace, 1, 
                                  target_i1, target_i2, target_i3, target_i4);

        osDelay(2);
    }
}

```

### 3. 一拖四模式下的零点设置 (附加)

如果你在调试时需要将某台电机当前的位置设置为编码器零位，可以调用对应的零点校准函数，传入目标电机的ID：

```c
void Gimbal_Set_Zero(void)
{
    // 将总线上 ID = 1 的电机当前位置设为零点
    DMMotorSetZeroOneToFour(gimbal_motor_1->motor_can_instace, 0x01);
}

```

按照这种方式组织 `gimbal.c`，底层的CAN发送与接收解析就会被彻底隔离开，既保证了多电机联合控制的同步性，又能极大节省 CAN 总线的带宽。