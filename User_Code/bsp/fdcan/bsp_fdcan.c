#include "bsp_fdcan.h"
#include "main.h"
#include "memory.h"
#include "stdlib.h"
// #include "bsp_dwt.h"
// #include "bsp_log.h"

/* FDCAN实例指针存储，用于接收回调 */
// 在FDCAN产生接收中断会遍历数组,选出hfdcan和rxid与发生中断的实例相同的那个,调用其回调函数
static FDCANInstance *fdcan_instance[CAN_MX_REGISTER_CNT] = {NULL};
static uint8_t idx = 0; // 全局FDCAN实例索引,每次有新的模块注册会自增

/* ---------------- 静态函数，由FDCANRegister()调用 -------------------- */

/**
 * @brief 添加过滤器以实现对特定id的报文的接收,会被FDCANRegister()调用
 *        给FDCAN添加过滤器后,FDCAN会根据接收到的报文的id进行消息过滤,符合规则的id会被填入FIFO触发中断
 *
 * @note H7的FDCAN有多个过滤器,这里为每个FDCAN分配独立的过滤器索引
 *       初始化时,奇数id的模块会被分配到FIFO0,偶数id的模块会被分配到FIFO1
 *
 * @attention FDCAN的过滤器配置与标准CAN有所不同,使用掩码模式进行过滤
 *
 * @param _instance FDCAN实例
 */
static void FDCANAddFilter(FDCANInstance *_instance)
{
    FDCAN_FilterTypeDef can_filter_conf;
    static uint8_t fdcan1_filter_idx = 0, fdcan2_filter_idx = 0, fdcan3_filter_idx = 0;

    can_filter_conf.IdType = FDCAN_STANDARD_ID; // 标准ID
    can_filter_conf.FilterType = FDCAN_FILTER_MASK; // 使用掩码模式

    // 根据can_handle判断是哪个FDCAN,然后分配过滤器索引
    if (_instance->can_handle == &hfdcan1)
    {
        can_filter_conf.FilterIndex = fdcan1_filter_idx++;
    }
    else if (_instance->can_handle == &hfdcan2)
    {
        can_filter_conf.FilterIndex = fdcan2_filter_idx++;
    }
    else if (_instance->can_handle == &hfdcan3)
    {
        can_filter_conf.FilterIndex = fdcan3_filter_idx++;
    }

    can_filter_conf.FilterConfig = (_instance->rx_id & 1) ? FDCAN_FILTER_TO_RXFIFO0 : FDCAN_FILTER_TO_RXFIFO1;
    can_filter_conf.FilterID1 = _instance->rx_id << 5; // 标准ID左移5位
    can_filter_conf.FilterID2 = 0x7FF << 5; // 掩码: 匹配所有标准ID

    if (HAL_FDCAN_ConfigFilter(_instance->can_handle, &can_filter_conf) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief 在第一个FDCAN实例初始化的时候会自动调用此函数,启动FDCAN服务
 *
 * @note 此函数会启动FDCAN1、FDCAN2和FDCAN3,开启相应的FIFO中断通知
 *
 */
static void FDCANServiceInit()
{
    // 配置全局过滤器 - 拒绝所有不匹配的帧
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan1, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan2, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);
    HAL_FDCAN_ConfigGlobalFilter(&hfdcan3, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

    // 启动FDCAN
    HAL_FDCAN_Start(&hfdcan1);
    HAL_FDCAN_Start(&hfdcan2);
    HAL_FDCAN_Start(&hfdcan3);

    // 激活接收中断 - FDCAN1使用FIFO0, FDCAN2和FDCAN3使用FIFO1
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);
    HAL_FDCAN_ActivateNotification(&hfdcan3, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);

    // 激活错误中断
    HAL_FDCAN_ActivateNotification(&hfdcan1, FDCAN_IT_BUS_OFF, 0);
    HAL_FDCAN_ActivateNotification(&hfdcan2, FDCAN_IT_BUS_OFF, 0);
    HAL_FDCAN_ActivateNotification(&hfdcan3, FDCAN_IT_BUS_OFF, 0);

    // LOGINFO("[bsp_fdcan] FDCAN Service Init");
}

/**
 * @brief 从FDCAN数据长度码转换为实际数据长度
 * @param dlc 数据长度码
 * @return uint8_t 实际数据长度
 */
static uint8_t FDCANDlcToLen(uint32_t dlc)
{
    if (dlc <= FDCAN_DLC_BYTES_8)
        return dlc;
    else if (dlc == FDCAN_DLC_BYTES_12)
        return 12;
    else if (dlc == FDCAN_DLC_BYTES_16)
        return 16;
    else if (dlc == FDCAN_DLC_BYTES_20)
        return 20;
    else if (dlc == FDCAN_DLC_BYTES_24)
        return 24;
    else if (dlc == FDCAN_DLC_BYTES_32)
        return 32;
    else if (dlc == FDCAN_DLC_BYTES_48)
        return 48;
    else if (dlc == FDCAN_DLC_BYTES_64)
        return 64;
    return 0;
}

/**
 * @brief 从实际数据长度转换为FDCAN数据长度码
 * @param len 实际数据长度
 * @return uint32_t 数据长度码
 */
static uint32_t FDCANLenToDlc(uint8_t len)
{
    if (len <= 8)
        return len;
    else if (len == 12)
        return FDCAN_DLC_BYTES_12;
    else if (len == 16)
        return FDCAN_DLC_BYTES_16;
    else if (len == 20)
        return FDCAN_DLC_BYTES_20;
    else if (len == 24)
        return FDCAN_DLC_BYTES_24;
    else if (len == 32)
        return FDCAN_DLC_BYTES_32;
    else if (len == 48)
        return FDCAN_DLC_BYTES_48;
    else if (len == 64)
        return FDCAN_DLC_BYTES_64;
    return FDCAN_DLC_BYTES_8;
}

/* ----------------------- 外部可调用函数 -----------------------*/

FDCANInstance *FDCANRegister(FDCAN_Init_Config_s *config)
{
    if (!idx)
    {
        FDCANServiceInit(); // 第一次注册,先进行硬件初始化
    }
    if (idx >= CAN_MX_REGISTER_CNT) // 超过最大实例数
    {
        while (1)
        {
            // LOGERROR("[bsp_fdcan] FDCAN instance exceeded MAX num, consider balance the load of FDCAN bus");
        }
    }
    for (size_t i = 0; i < idx; i++)
    {
        // 重复注册 | id重复
        if (fdcan_instance[i]->rx_id == config->rx_id && fdcan_instance[i]->can_handle == config->can_handle)
        {
            while (1)
            {
                // LOGERROR("[bsp_fdcan] FDCAN id crash ,tx [%d] or rx [%d] already registered", config->tx_id, config->rx_id);
            }
        }
    }

    FDCANInstance *instance = (FDCANInstance *) malloc(sizeof(FDCANInstance)); // 分配空间
    memset(instance, 0, sizeof(FDCANInstance)); // 分配的空间未必是0,所以要先清空

    // 进行发送报文的配置
    instance->txconf.Identifier = config->tx_id; // 发送id
    instance->txconf.IdType = FDCAN_STANDARD_ID; // 使用标准id
    instance->txconf.TxFrameType = FDCAN_DATA_FRAME; // 发送数据帧
    instance->txconf.DataLength = FDCAN_DLC_BYTES_8; // 默认发送长度为8
    instance->txconf.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    instance->txconf.BitRateSwitch = FDCAN_BRS_OFF; // 默认关闭比特率切换
    instance->txconf.FDFormat = FDCAN_CLASSIC_CAN; // 默认经典CAN模式
    instance->txconf.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    instance->txconf.MessageMarker = 0;

    // 设置回调函数和接收发送id
    instance->can_handle = config->can_handle;
    instance->tx_id = config->tx_id;
    instance->rx_id = config->rx_id;
    instance->can_module_callback = config->can_module_callback;
    instance->id = config->id;

    FDCANAddFilter(instance); // 添加FDCAN过滤器规则
    fdcan_instance[idx++] = instance; // 将实例保存到fdcan_instance中

    return instance; // 返回FDCAN实例指针
}

/**
 * @brief 通过FDCAN实例发送消息
 *        发送前需要向FDCAN实例的tx_buff写入发送数据
 *
 * @attention 超时时间不应该超过调用此函数的任务的周期,否则会导致任务阻塞
 *
 * @param _instance FDCAN实例
 * @param timeout 超时时间,单位为ms
 * @return uint8_t 发送成功返回1,失败返回0
 */
uint8_t FDCANTransmit(FDCANInstance *_instance, float timeout)
{
    // static uint32_t busy_count;
    static volatile float wait_time __attribute__((unused)); // for cancel warning
    // float dwt_start = DWT_GetTimeline_ms();

    // 等待发送FIFO有空闲位置
    while (HAL_FDCAN_GetTxFifoFreeLevel(_instance->can_handle) == 0)
    {
        // if (DWT_GetTimeline_ms() - dwt_start > timeout) // 超时
        // {
        //     LOGWARNING("[bsp_fdcan] FDCAN TX FIFO full! failed to add msg to FIFO. Cnt [%d]", busy_count);
        //     busy_count++;
        //     return 0;
        // }
    }
    // wait_time = DWT_GetTimeline_ms() - dwt_start;

    // 发送消息
    if (HAL_FDCAN_AddMessageToTxFifoQ(_instance->can_handle, &_instance->txconf, _instance->tx_buff) != HAL_OK)
    {
        // LOGWARNING("[bsp_fdcan] FDCAN bus BUS! cnt:%d", busy_count);
        // busy_count++;
        return 0;
    }
    return 1; // 发送成功
}

/**
 * @brief 修改FDCAN发送报文的数据帧长度
 *        注意FDCAN最大支持64字节,在没有进行修改的时候,默认长度为8
 *
 * @param _instance 要修改长度的FDCAN实例
 * @param length    设定长度
 */
void FDCANSetDLC(FDCANInstance *_instance, uint8_t length)
{
    // 发送长度错误!检查调用参数是否出错,或出现野指针/越界访问
    if (length > 64 || length == 0) // 安全检查
        while (1)
        {
            //LOGERROR("[bsp_fdcan] FDCAN DLC error! check your code or wild pointer");
        }
    _instance->txconf.DataLength = FDCANLenToDlc(length);
}

/**
 * @brief 设置FDCAN波特率
 * @param hfdcan FDCAN句柄
 * @param mode CAN模式：CAN_CLASS或CAN_FD_BRS
 * @param baud 波特率选择
 */
void bsp_fdcan_set_baud(FDCAN_HandleTypeDef *hfdcan, uint8_t mode, uint8_t baud)
{
    uint32_t nom_brp = 0, nom_seg1 = 0, nom_seg2 = 0, nom_sjw = 0;
    uint32_t dat_brp = 0, dat_seg1 = 0, dat_seg2 = 0, dat_sjw = 0;

    if (mode == CAN_CLASS)
    {
        switch (baud)
        {
            case CAN_BR_125K:
                nom_brp = 4;
                nom_seg1 = 139;
                nom_seg2 = 20;
                nom_sjw = 20;
                break;
            case CAN_BR_200K:
                nom_brp = 2;
                nom_seg1 = 174;
                nom_seg2 = 25;
                nom_sjw = 25;
                break;
            case CAN_BR_250K:
                nom_brp = 2;
                nom_seg1 = 139;
                nom_seg2 = 20;
                nom_sjw = 20;
                break;
            case CAN_BR_500K:
                nom_brp = 1;
                nom_seg1 = 139;
                nom_seg2 = 20;
                nom_sjw = 20;
                break;
            case CAN_BR_1M:
                nom_brp = 1;
                nom_seg1 = 59;
                nom_seg2 = 20;
                nom_sjw = 20;
                break;
            default:
                break;
        }
        dat_brp = 1;
        dat_seg1 = 29;
        dat_seg2 = 10;
        dat_sjw = 10; // 仲裁域默认1M
        hfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    }
    else if (mode == CAN_FD_BRS)
    {
        switch (baud)
        {
            case CAN_BR_2M:
                dat_brp = 1;
                dat_seg1 = 29;
                dat_seg2 = 10;
                dat_sjw = 10;
                break;
            case CAN_BR_2M5:
                dat_brp = 1;
                dat_seg1 = 25;
                dat_seg2 = 6;
                dat_sjw = 6;
                break;
            case CAN_BR_3M2:
                dat_brp = 1;
                dat_seg1 = 19;
                dat_seg2 = 5;
                dat_sjw = 5;
                break;
            case CAN_BR_4M:
                dat_brp = 1;
                dat_seg1 = 14;
                dat_seg2 = 5;
                dat_sjw = 5;
                break;
            case CAN_BR_5M:
                dat_brp = 1;
                dat_seg1 = 13;
                dat_seg2 = 2;
                dat_sjw = 2;
                break;
            default:
                break;
        }
        nom_brp = 1;
        nom_seg1 = 59;
        nom_seg2 = 20;
        nom_sjw = 20; // 数据域默认1M
        hfdcan->Init.FrameFormat = FDCAN_FRAME_FD_BRS;
    }

    HAL_FDCAN_Stop(hfdcan);

    hfdcan->Init.NominalPrescaler = nom_brp;
    hfdcan->Init.NominalTimeSeg1 = nom_seg1;
    hfdcan->Init.NominalTimeSeg2 = nom_seg2;
    hfdcan->Init.NominalSyncJumpWidth = nom_sjw;

    hfdcan->Init.DataPrescaler = dat_brp;
    hfdcan->Init.DataTimeSeg1 = dat_seg1;
    hfdcan->Init.DataTimeSeg2 = dat_seg2;
    hfdcan->Init.DataSyncJumpWidth = dat_sjw;

    if (HAL_FDCAN_Init(hfdcan) != HAL_OK)
    {
        Error_Handler();
    }

    // 重新启动FDCAN并配置过滤器
    HAL_FDCAN_Start(hfdcan);
    HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

    // 重新激活通知
    if (hfdcan == &hfdcan1)
    {
        HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
    }
    else
    {
        HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);
    }
    HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_BUS_OFF, 0);
}

/* ----------------------- 回调函数定义 --------------------------*/

/**
 * @brief 此函数会被下面两个函数调用,用于处理FIFO0和FIFO1接收中断(说明收到了新的数据)
 *        所有的实例都会被遍历,找到can_handle和rx_id相等的实例时,调用该实例的回调函数
 *
 * @param _hcan FDCAN句柄
 * @param fifox 传递给HAL_FDCAN_GetRxMessage()以从特定FIFO获取消息
 */
static void FDCANFIFOxCallback(FDCAN_HandleTypeDef *_hcan, uint32_t fifox)
{
    FDCAN_RxHeaderTypeDef rxconf;
    uint8_t can_rx_buff[64];

    while (HAL_FDCAN_GetRxFifoFillLevel(_hcan, fifox)) // FIFO不为空,有可能在其他中断时有多帧数据进入
    {
        HAL_FDCAN_GetRxMessage(_hcan, fifox, &rxconf, can_rx_buff); // 从FIFO中获取数据

        for (size_t i = 0; i < idx; ++i)
        {
            // 两者相等说明这是要找的实例
            if (_hcan == fdcan_instance[i]->can_handle && rxconf.Identifier == fdcan_instance[i]->rx_id)
            {
                if (fdcan_instance[i]->can_module_callback != NULL) // 回调函数不为空就调用
                {
                    fdcan_instance[i]->rx_len = FDCANDlcToLen(rxconf.DataLength); // 保存接收到的数据长度
                    memcpy(fdcan_instance[i]->rx_buff, can_rx_buff, fdcan_instance[i]->rx_len); // 消息拷贝到对应实例
                    fdcan_instance[i]->can_module_callback(fdcan_instance[i]); // 触发回调进行数据解析和处理
                }
                return;
            }
        }
    }
}

/**
 * @brief 注意,STM32的FDCAN设备有独立的FIFO
 * 下面两个函数是HAL库中的回调函数,他们被HAL声明为__weak,这里对他们进行重载(重写)
 * 当FIFO0或FIFO1有新消息时会调用这两个函数
 */

/**
 * @brief rx fifo callback. Once FIFO_0 has new message, this func would be called
 *
 * @param hfdcan FDCAN handle indicate which device the message in FIFO_0 comes from
 * @param RxFifo0ITs FIFO0中断标志
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
    {
        FDCANFIFOxCallback(hfdcan, FDCAN_RX_FIFO0); // 调用我们自己写的函数来处理消息
    }
}

/**
 * @brief rx fifo callback. Once FIFO_1 has new message, this func would be called
 *
 * @param hfdcan FDCAN handle indicate which device the message in FIFO_1 comes from
 * @param RxFifo1ITs FIFO1中断标志
 */
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
    if ((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != RESET)
    {
        FDCANFIFOxCallback(hfdcan, FDCAN_RX_FIFO1); // 调用我们自己写的函数来处理消息
    }
}

/**
 * @brief 错误状态回调函数
 * @param hfdcan FDCAN句柄
 * @param ErrorStatusITs 错误状态中断标志
 */
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
    if (ErrorStatusITs & FDCAN_IR_BO)
    {
        // 总线关闭恢复
        CLEAR_BIT(hfdcan->Instance->CCCR, FDCAN_CCCR_INIT);
        hfdcan->ErrorCode = 0;
        // LOGWARNING("[bsp_fdcan] FDCAN Bus Off recovered");
    }
    if (ErrorStatusITs & FDCAN_IR_EP)
    {
        // 错误被动状态处理，重新初始化FDCAN
        // LOGWARNING("[bsp_fdcan] FDCAN Error Passive, reinitializing...");

        if (hfdcan->Instance == FDCAN1)
        {
            MX_FDCAN1_Init();
        }
        else if (hfdcan->Instance == FDCAN2)
        {
            MX_FDCAN2_Init();
        }
        else if (hfdcan->Instance == FDCAN3)
        {
            MX_FDCAN3_Init();
        }

        // 重新配置并启动
        HAL_FDCAN_Start(hfdcan);
        HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT, FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

        if (hfdcan == &hfdcan1)
        {
            HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
        }
        else
        {
            HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);
        }
        HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_BUS_OFF, 0);

        hfdcan->ErrorCode = 0;
    }
}

// 弱定义的接收回调函数，用户可重写
__weak void fdcan1_rx_callback(void)
{
    // 用户可重写
}

__weak void fdcan2_rx_callback(void)
{
    // 用户可重写
}

__weak void fdcan3_rx_callback(void)
{
    // 用户可重写
}
