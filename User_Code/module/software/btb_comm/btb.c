#include "btb.h"
#include <string.h>
#include <stdlib.h>
// #include "crc8.h"
#include "bsp_dwt.h"
#include "bsp_fdcan.h"
#include "daemon.h"

// 内部函数声明
static void btb_reset_rx(btb_instance_t *ins);
static void btb_rx_callback(FDCANInstance *_instance);
static void btb_lost_callback(void *btb_ins);

/**
 * @brief 重置接收状态和缓冲区
 */
static void btb_reset_rx(btb_instance_t *ins)
{
    memset(ins->raw_recvbuf, 0, ins->cur_recv_len);
    ins->recv_state = 0;
    ins->cur_recv_len = 0;
}

/**
 * @brief CAN 接收回调（由底层驱动在收到数据时调用）
 */
static void btb_rx_callback(FDCANInstance *_instance)
{
    // 从 CANInstance 的 id 字段获取 btb_instance_t 指针
    btb_instance_t *comm = (btb_instance_t *)_instance->id;

    /* 接收状态机 */
    if (_instance->rx_buff[0] == BTB_HEADER && comm->recv_state == 0)
    {
        if (_instance->rx_buff[1] == comm->recv_data_len)
        {
            comm->recv_state = 1;   // 开始接收数据
        }
        else
            return; // 长度不匹配，忽略
    }

    if (comm->recv_state)
    {
        // 检查缓冲区是否溢出
        if (comm->cur_recv_len + _instance->rx_len > comm->recv_buf_len)
        {
            btb_reset_rx(comm);
            return;
        }

        // 拷贝数据
        memcpy(comm->raw_recvbuf + comm->cur_recv_len, _instance->rx_buff, _instance->rx_len);
        comm->cur_recv_len += _instance->rx_len;

        // 检查是否接收完整一帧
        if (comm->cur_recv_len == comm->recv_buf_len)
        {
            // 验证帧尾和 CRC
            if (comm->raw_recvbuf[comm->recv_buf_len - 1] == BTB_TAIL)
            {
                // uint8_t crc = crc_8(comm->raw_recvbuf + 2, comm->recv_data_len); //Todo:CRC
                // if (comm->raw_recvbuf[comm->recv_buf_len - 2] == crc)
                // {
                    memcpy(comm->unpacked_recv_data, comm->raw_recvbuf + 2, comm->recv_data_len);
                    comm->update_flag = 1;
                    DaemonReload(comm->comm_daemon);   // 喂看门狗
                // }
            }
            btb_reset_rx(comm);
        }
    }
}

/**
 * @brief 看门狗超时回调（通信丢失）
 */
static void btb_lost_callback(void *btb_ins)
{
    btb_instance_t *comm = (btb_instance_t *)btb_ins;
    btb_reset_rx(comm);
    LOGWARNING("[btb] can comm rx[%d] lost, reset rx state.", &comm->fdcan_ins->rx_id);
}

/**
 * @brief 初始化 BTB 实例
 */
btb_instance_t *btb_init(btb_config_t *config)
{
    btb_instance_t *ins = (btb_instance_t *)malloc(sizeof(btb_instance_t));
    memset(ins, 0, sizeof(btb_instance_t));

    ins->recv_data_len = config->recv_data_len;
    ins->recv_buf_len  = config->recv_data_len + BTB_OFFSET_BYTES;
    ins->send_data_len = config->send_data_len;
    ins->send_buf_len  = config->send_data_len + BTB_OFFSET_BYTES;

    // 预填充发送缓冲区固定字段
    ins->raw_sendbuf[0] = BTB_HEADER;
    ins->raw_sendbuf[1] = config->send_data_len;   // 数据长度
    ins->raw_sendbuf[config->send_data_len + BTB_OFFSET_BYTES - 1] = BTB_TAIL;

    // 注册 CAN 实例
    config->can_config.id = ins;
    config->can_config.can_module_callback = btb_rx_callback;
    ins->fdcan_ins = CANRegister(&config->can_config);   // 假设此函数存在

    // 注册看门狗
    Daemon_Init_Config_s daemon_config = {
        .callback = btb_lost_callback,
        .owner_id = (void *)ins,
        .reload_count = config->daemon_count,
    };
    ins->comm_daemon = DaemonRegister(&daemon_config);

    return ins;
}

/**
 * @brief 发送数据（自动分包）
 */
void btb_send(btb_instance_t *ins, uint8_t *data)
{
    // 计算 CRC 并填入发送缓冲区
    memcpy(ins->raw_sendbuf + 2, data, ins->send_data_len);//ToDo:CRC
    // uint8_t crc = crc_8(data, ins->send_data_len);
    // ins->raw_sendbuf[2 + ins->send_data_len] = crc;

    // 分包发送（CAN 单包最大 8 字节）
    uint8_t remain = ins->send_buf_len;
    uint8_t offset = 0;
    while (remain > 0)
    {
        uint8_t len = (remain >= 8) ? 8 : remain;
        CANSetDLC(ins->fdcan_ins, len);                     // 设置 DLC
        memcpy(ins->fdcan_ins->tx_buff, ins->raw_sendbuf + offset, len);
        CANTransmit(ins->fdcan_ins, 1);                      // 阻塞发送
        offset += len;
        remain -= len;
    }
}

/**
 * @brief 获取最新接收的数据（读取后清除更新标志）
 */
void *btb_get(btb_instance_t *ins)
{
    ins->update_flag = 0;
    return ins->unpacked_recv_data;
}

/**
 * @brief 检查通信是否在线（看门狗未超时）
 */
uint8_t btb_is_online(btb_instance_t *ins)
{
    return DaemonIsOnline(ins->comm_daemon);
}