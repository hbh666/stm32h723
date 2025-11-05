#ifndef __BSP_FDCAN_H__
#define __BSP_FDCAN_H__

#include "main.h"
#include "fdcan.h"
#include <stdint.h>

#define CAN_MX_REGISTER_CNT 16     // 最大FDCAN设备注册数量
#define MX_CAN_FILTER_CNT (3 * 14) // 最多可以使用的CAN过滤器数量，支持3个CAN
#define DEVICE_CAN_CNT 3           // FDCAN1, FDCAN2, FDCAN3

#define CAN_CLASS   0
#define CAN_FD_BRS  1

#define CAN_BR_125K 0
#define CAN_BR_200K 1
#define CAN_BR_250K 2
#define CAN_BR_500K 3
#define CAN_BR_1M   4
#define CAN_BR_2M   5
#define CAN_BR_2M5  6
#define CAN_BR_3M2  7
#define CAN_BR_4M   8
#define CAN_BR_5M   9

/* FDCAN实例结构体，每个注册到FDCAN的模块都应该有这个变量 */
#pragma pack(1)
typedef struct _FDCANInstance {
    FDCAN_HandleTypeDef *can_handle; // FDCAN句柄
    FDCAN_TxHeaderTypeDef txconf; // FDCAN报文发送配置
    uint32_t tx_id; // 发送id
    uint32_t tx_mailbox; // FDCAN消息填入的邮箱号
    uint8_t tx_buff[64]; // 发送缓存，FDCAN最大支持64字节
    uint8_t rx_buff[64]; // 接收缓存，FDCAN最大支持64字节
    uint32_t rx_id; // 接收id
    uint8_t rx_len; // 接收长度
    // 接收的回调函数，用于解析接收到的数据
    void (*can_module_callback)(struct _FDCANInstance *); // 回调函数需要实例来区分注册的设备
    void *id; // 使用FDCAN外设的模块指针
} FDCANInstance;
#pragma pack()

/* FDCAN实例初始化结构体，将此结构体指针传入注册函数 */
typedef struct {
    FDCAN_HandleTypeDef *can_handle; // FDCAN句柄
    uint32_t tx_id; // 发送id
    uint32_t rx_id; // 接收id
    void (*can_module_callback)(FDCANInstance *); // 处理接收数据的回调函数
    void *id; // 拥有FDCAN实例的模块地址
} FDCAN_Init_Config_s;

/**
 * @brief 注册一个模块到FDCAN服务，在使用FDCAN设备前调用
 * @param config 初始化配置
 * @return FDCANInstance* 模块拥有的FDCAN实例
 */
FDCANInstance *FDCANRegister(FDCAN_Init_Config_s *config);

/**
 * @brief 修改FDCAN发送报文的数据帧长度
 * @param _instance 要修改长度的FDCAN实例
 * @param length 设定长度，FDCAN最大支持64字节
 */
void FDCANSetDLC(FDCANInstance *_instance, uint8_t length);

/**
 * @brief 通过FDCAN实例发送消息
 *        发送前需要向FDCAN实例的tx_buff写入发送数据
 *
 * @attention 超时时间不应该超过调用此函数的任务的周期,否则会导致任务阻塞
 *
 * @param _instance 模块拥有的FDCAN实例
 * @param timeout 超时时间,单位为ms
 * @return uint8_t 发送成功返回1,失败返回0
 */
uint8_t FDCANTransmit(FDCANInstance *_instance, float timeout);

/**
 * @brief 设置FDCAN波特率
 * @param hfdcan FDCAN句柄
 * @param mode CAN模式：CAN_CLASS或CAN_FD_BRS
 * @param baud 波特率选择
 */
void bsp_fdcan_set_baud(FDCAN_HandleTypeDef *hfdcan, uint8_t mode, uint8_t baud);

// 弱定义的接收回调函数，用户可重写
__weak void fdcan1_rx_callback(void);

__weak void fdcan2_rx_callback(void);

__weak void fdcan3_rx_callback(void);

#endif /* __BSP_FDCAN_H_ */
