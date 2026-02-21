//
// Created by ASUS on 2025/11/17.
//

#ifndef TRONONEH7_SCAFFOLD_BTB_H
#define TRONONEH7_SCAFFOLD_BTB_H

#include "bsp_fdcan.h"
#include "daemon.h"

#define BTB_MAX_INSTANCE 4 // 注意均衡负载,一条总线上不要挂载过多的外设

#define BTB_MAX_BUFFSIZE 60 // 最大发送/接收字节数,如果不够可以增加此数值
#define BTB_HEADER 's'      // 帧头
#define BTB_TAIL 'e'        // 帧尾
#define BTB_OFFSET_BYTES 4  // 's'+ datalen + 'e' + crc8

#pragma pack(1)
/* BTB 结构体, 拥有 BTB 的 app 应该包含一个 BTB 指针 */
typedef struct
{
    FDCANInstance *fdcan_ins;
    /* 发送部分 */
    uint8_t send_data_len; // 发送数据长度
    uint8_t send_buf_len;  // 发送缓冲区长度,为发送数据长度+帧头单包数据长度帧尾以及校验和(4)
    uint8_t raw_sendbuf[BTB_MAX_BUFFSIZE + BTB_OFFSET_BYTES]; // 额外4个bytes保存帧头帧尾和校验和
    /* 接收部分 */
    uint8_t recv_data_len; // 接收数据长度
    uint8_t recv_buf_len;  // 接收缓冲区长度,为接收数据长度+帧头单包数据长度帧尾以及校验和(4)
    uint8_t raw_recvbuf[BTB_MAX_BUFFSIZE + BTB_OFFSET_BYTES]; // 额外4个bytes保存帧头帧尾和校验和
    uint8_t unpacked_recv_data[BTB_MAX_BUFFSIZE];             // 解包后的数据,调用 btb_get() 后 cast 成对应的类型通过指针读取即可
    /* 接收和更新标志位*/
    uint8_t recv_state;   // 接收状态,
    uint8_t cur_recv_len; // 当前已经接收到的数据长度(包括帧头帧尾 datalen 和校验和)
    uint8_t update_flag;  // 数据更新标志位,当接收到新数据时,会将此标志位置1,调用 btb_get() 后会将此标志位置0

    Daemon_Instance *comm_daemon;
} btb_instance_t;
#pragma pack()

/* BTB 初始化结构体 */
typedef struct
{
    FDCAN_Init_Config_s can_config; // CAN初始化结构体
    uint8_t send_data_len;        // 发送数据长度
    uint8_t recv_data_len;        // 接收数据长度

    uint16_t daemon_count; // 守护进程计数,用于初始化守护进程
} btb_config_t;

/**
 * @brief 初始化 BTB
 *
 * @param config BTB 初始化结构体
 * @return btb_instance_t*
 */
btb_instance_t *btb_init(btb_config_t *config);

/**
 * @brief 通过 BTB 发送数据
 *
 * @param instance btb 实例
 * @param data 注意此地址的有效数据长度需要和初始化时传入的 datalen 相同
 */
void btb_send(btb_instance_t *instance, uint8_t *data);

/**
 * @brief 获取 BTB 接收的数据,需要自己使用强制类型转换将返回的 void 指针转换成指定类型
 *
 * @return void* 返回的数据指针
 * @attention 注意如果希望直接通过转换指针访问数据,如果数据是 union 或 struct,要检查是否使用了 pack(n)
 *            BTB 接收到的数据可以看作是 pack(1) 之后的,是连续存放的.
 *            如果使用了 pack(n) 可能会导致数据错乱,并且无法使用强制类型转换通过 memcpy 直接访问,转而需要手动解包.
 *            强烈建议通过 BTB 传输的数据使用 pack(1)
 */
void *btb_get(btb_instance_t *instance);

/**
 * @brief 检查 BTB 是否在线
 *
 * @param instance
 * @return uint8_t
 */
uint8_t btb_is_online(btb_instance_t *instance);

#endif //TRONONEH7_SCAFFOLD_BTB_H
