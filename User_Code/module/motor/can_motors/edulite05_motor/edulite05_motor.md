# EL05 关节电机驱动使用说明

灵足时代(RobStride)EL05 关节电机驱动,基于 CAN 扩展帧运控(MIT)协议。
协议细节见同目录 `EL05_MIT模式协议速查.md`。

## 一、文件结构

| 文件 | 内容 |
|---|---|
| `edulite05_motor.h` | 量程宏、ID 掩码、测量/发送/实例/配置结构体、对外 API 声明 |
| `edulite05_motor.c` | 驱动实现:编码换算、组帧/解析、实例注册、控制任务 |
| `EL05_MIT模式协议速查.md` | 协议参考(帧格式逐字段) |

## 二、内部函数(static,外部不可见)

| 函数 | 作用 |
|---|---|
| `float_to_uint(x, x_min, x_max, bits)` | 物理量 → CAN 整数编码(发送用) |
| `uint_to_float(x_int, x_min, x_max, bits)` | CAN 整数 → 物理量解码(接收用) |
| `EduLite05SetMode(cmd, motor)` | 发一次性命令帧:类型 3 使能 / 类型 4 停止 / 类型 6 标零(命令在扩展帧 ID bit28~24) |
| `EduLite05Decode(motor_can)` | CAN 反馈回调(类型 2):解析角度/速度/力矩/温度 + ID 里的故障位/模式状态,并喂守护狗 |
| `EduLite05LostCallback(motor_ptr)` | daemon 失联回调:置 STOP + 发停止帧(安全保护) |
| `EduLite05Task(argument)` | **每电机一个控制任务**,1kHz 纯力控下发(p=v=Kp=Kd=0,力矩 t_ff 进扩展帧 ID bit23~8) |

## 三、对外 API(头文件声明,供上层调用)

| 函数 | 作用 | 何时调用 |
|---|---|---|
| `EDUMotorInit(config)` | 注册一个电机:分配实例、CAN 注册、守护注册、发使能帧。返回实例指针(失败 NULL) | 上电初始化阶段(可放 RobotInit 后的任务里) |
| `EDUMotorSetRef(motor, ref)` | 设定目标力矩 Nm(纯力控) | 控制循环里,每周期一次 |
| `EDUMotorSetMITParam(motor, p, v, kp, kd, t_ff)` | 直通运控五参数(电机端自带 PD) | 预留,任务尚未消费 |
| `EDUMotorOuterLoop(motor, type)` | 切换板端最外层闭环 | 预留 |
| `EDUMotorEnable(motor)` | 使能:置标志 + 发类型 3 帧 | 失联恢复/重新使能时 |
| `EDUMotorStop(motor)` | 停止:置标志 + 发类型 4 帧 | 保护逻辑/停机时 |
| `EDUMotorCaliEncoder(motor)` | 当前位置设为机械零位(发类型 6) | **标定流程显式调用**(Init 不会自动标零) |
| `EDUMotorSetCtrlMode(motor, mode)` | 切换控制模式 | 预留(以后切电流/位置模式) |
| `EDUMotorControlInit()` | 为所有已注册实例创建控制任务 | **RTOS 调度器启动后**,所有 Init 完成后调一次 |

## 四、外部调用示例

### 1. 初始化(约定: `tx_id` = 电机 CAN_ID)

```c
#include "edulite05_motor.h"

static Motor_Init_Config_s leg_joint_config = {
    .fdcan_init_config = {
        .can_handle = &hfdcan1,   // CAN1
        .tx_id = 1,               // 电机 CAN_ID(扩展帧 ID bit7~0)
        // rx_id / filter_mask / id_type 不用填,驱动内部计算
    },
    .controller_setting_init_config = {
        .motor_reverse_flag = MOTOR_DIRECTION_NORMAL,
    },
    // .controller_param_init_config = { 三环 PID,板端闭环时用 }
};

EDUMotorInstance *leg_joint;

void WheelLegInit(void)   // 在 RTOS 任务里调用(如 ChassisTask 开头)
{
    leg_joint = EDUMotorInit(&leg_joint_config);
    if (leg_joint == NULL) { /* 注册失败处理 */ }

    EDUMotorControlInit(); // 所有电机 Init 完,统一创建控制任务(只调一次)
}
```

### 2. 控制循环(纯力控)

```c
void ChassisTask(void *argument)
{
    WheelLegInit();
    for (;;)
    {
        float joint_torque = BalanceUpdate();   // 你的平衡环输出(Nm)
        EDUMotorSetRef(leg_joint, joint_torque); // 驱动任务 1kHz 自动下发
        osDelay(1);
    }
}
```

### 3. 标定零位(标定流程中)

```c
EDUMotorCaliEncoder(leg_joint);   // 当前位置 = 机械零位
```

### 4. 失联恢复(可选,手动重新使能)

```c
EDUMotorEnable(leg_joint);   // daemon 失联后电机已停,恢复时重新使能
```

## 五、运行时数据流

```
上层(你的轮腿控制)
  │ EDUMotorSetRef(motor, τ)   ← 写目标力矩
  ▼
EduLite05Task(每电机,1kHz)
  │ 组帧: 数据区 p/v/Kp/Kd 零位, t_ff 进扩展帧 ID bit23~8
  ▼
CAN1 → 电机(运控公式: t_ref = Kd·Δv + Kp·Δp + t_ff = τ)
  │ 反馈帧(类型2)
  ▼
EduLite05Decode(中断回调)
  │ 解析 position/velocity/torque/temperature/fault/mode_status
  │ DaemonReload 喂狗
  ▼
daemon 超时(10ms 无反馈)→ EduLite05LostCallback → 发停止帧 + STOP
```

## 六、注意事项

1. **bsp 依赖**:本驱动依赖 bsp_fdcan 的扩展帧支持(过滤器 MASK 模式、分发放行扩展帧)。CubeMX 重新生成代码会把 FDCAN1 的 `ExtFiltersNbr` 重置为 0,需重设(当前已设为 8)
2. **DWT_Delay 单位是秒**:驱动内部延时一律用 `DWT_Delay(0.001)`(1ms)
3. **标零不在 Init 里做**:EL05 标零覆盖机械零点,自动标零会丢参考,必须上层显式调用
4. **力矩进 ID,不在数据区**:扩展帧 ID bit23~8 每周期都在变(等于实时 t_ff),发送前必须现拼 ID 并设 `FDCAN_EXTENDED_ID`
5. **温度反馈单位 ℃×10**,驱动已 ÷10
6. **EDUMotorControlInit 必须在调度器启动后**调用(内部 xTaskCreate);RobotInit(关中断阶段)里不能调
7. 一条 CAN1 挂多个 EL05 时,每个电机 `tx_id`(电机 CAN_ID)不同即可,过滤分发按电机 ID 自动区分
