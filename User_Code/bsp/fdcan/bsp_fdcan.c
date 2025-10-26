#include "bsp_fdcan.h"
#include "fdcan.h"
#include "memory.h"
#include "stdlib.h"

/* FDCAN实例指针存储，用于接收回调 */
static FDCANInstance *fdcan_instance[CAN_MX_REGISTER_CNT] = {NULL};
static uint8_t idx = 0; // 全局FDCAN实例索引

/* ---------------- 静态函数，由FDCANRegister()调用 -------------------- */

/**
 * @brief 添加过滤器以实现对特定id的报文的接收
 * @param _instance 特定模块拥有的FDCAN实例
 */
static void FDCANAddFilter(FDCANInstance *_instance)
{
    FDCAN_FilterTypeDef can_filter_conf;
    static uint8_t filter_idx[3] = {0}; // 每个CAN的过滤器索引

    uint8_t can_num = (_instance->can_handle == &hfdcan1 ? 0 :
                      _instance->can_handle == &hfdcan2 ? 1 : 2);

    can_filter_conf.IdType = FDCAN_STANDARD_ID;      // 标准ID
    can_filter_conf.FilterIndex = filter_idx[can_num]++; // 滤波器索引自增
    can_filter_conf.FilterType = FDCAN_FILTER_MASK;  // 掩码模式
    can_filter_conf.FilterConfig = (_instance->rx_id & 1) ? FDCAN_FILTER_TO_RXFIFO0 : FDCAN_FILTER_TO_RXFIFO1;
    can_filter_conf.FilterID1 = _instance->rx_id << 5; // 过滤器ID1
    can_filter_conf.FilterID2 = 0x7FF << 5;           // 掩码，匹配所有标准ID

    if (HAL_FDCAN_ConfigFilter(_instance->can_handle, &can_filter_conf) != HAL_OK)
    {
        Error_Handler();
    }
}

/**
 * @brief 在第一个FDCAN实例初始化时自动调用此函数，启动FDCAN服务
 */
static void FDCANServiceInit()
{
    // 配置全局过滤器
    for (int i = 0; i < 3; i++)
    {
        FDCAN_HandleTypeDef *hfdcan = (i == 0) ? &hfdcan1 : (i == 1) ? &hfdcan2 : &hfdcan3;

        HAL_FDCAN_ConfigGlobalFilter(hfdcan, FDCAN_REJECT, FDCAN_REJECT,
                                   FDCAN_FILTER_REMOTE, FDCAN_FILTER_REMOTE);

        // 启动FDCAN
        HAL_FDCAN_Start(hfdcan);

        // 激活通知
        if (i == 0) // FDCAN1使用FIFO0
        {
            HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO0_NEW_MESSAGE, 0);
        }
        else // FDCAN2和FDCAN3使用FIFO1
        {
            HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_RX_FIFO1_NEW_MESSAGE, 0);
        }

        HAL_FDCAN_ActivateNotification(hfdcan, FDCAN_IT_BUS_OFF, 0);
    }
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
        FDCANServiceInit(); // 第一次注册，先进行硬件初始化
    }

    if (idx >= CAN_MX_REGISTER_CNT) // 超过最大实例数
    {
        while (1); // 错误处理
    }

    for (size_t i = 0; i < idx; i++)
    { // 重复注册检查
        if (fdcan_instance[i]->rx_id == config->rx_id &&
            fdcan_instance[i]->can_handle == config->can_handle)
        {
            while (1); // ID冲突错误处理
        }
    }

    FDCANInstance *instance = (FDCANInstance *)malloc(sizeof(FDCANInstance));
    memset(instance, 0, sizeof(FDCANInstance));

    // 进行发送报文的配置
    instance->txconf.Identifier = config->tx_id;
    instance->txconf.IdType = FDCAN_STANDARD_ID;
    instance->txconf.TxFrameType = FDCAN_DATA_FRAME;
    instance->txconf.DataLength = FDCAN_DLC_BYTES_8;
    instance->txconf.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    instance->txconf.BitRateSwitch = FDCAN_BRS_OFF;
    instance->txconf.FDFormat = FDCAN_CLASSIC_CAN;
    instance->txconf.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    instance->txconf.MessageMarker = 0;

    // 设置回调函数和接收发送id
    instance->can_handle = config->can_handle;
    instance->tx_id = config->tx_id;
    instance->rx_id = config->rx_id;
    instance->can_module_callback = config->can_module_callback;
    instance->id = config->id;

    FDCANAddFilter(instance);           // 添加FDCAN过滤器规则
    fdcan_instance[idx++] = instance;   // 将实例保存到fdcan_instance中

    return instance; // 返回FDCAN实例指针
}

uint8_t FDCANTransmit(FDCANInstance *_instance, float timeout)
{
    uint32_t start_time = HAL_GetTick();

    // 等待发送FIFO有空闲位置
    while (HAL_FDCAN_GetTxFifoFreeLevel(_instance->can_handle) == 0)
    {
        if (HAL_GetTick() - start_time > timeout)
        {
            return 0; // 超时
        }
    }

    // 发送消息
    if (HAL_FDCAN_AddMessageToTxFifoQ(_instance->can_handle, &_instance->txconf, _instance->tx_buff) != HAL_OK)
    {
        return 0; // 发送失败
    }

    return 1; // 发送成功
}

void FDCANSetDLC(FDCANInstance *_instance, uint8_t length)
{
    if (length > 64) // FDCAN最大支持64字节
    {
        while (1); // 错误处理
    }
    _instance->txconf.DataLength = FDCANLenToDlc(length);
}

void bsp_fdcan_set_baud(hcan_t *hfdcan, uint8_t mode, uint8_t baud)
{
    uint32_t nom_brp=0, nom_seg1=0, nom_seg2=0, nom_sjw=0;
    uint32_t dat_brp=0, dat_seg1=0, dat_seg2=0, dat_sjw=0;

    if(mode == CAN_CLASS)
    {
        switch (baud)
        {
            case CAN_BR_125K: 	nom_brp=4 ; nom_seg1=139; nom_seg2=20; nom_sjw=20; break;
            case CAN_BR_200K: 	nom_brp=2 ; nom_seg1=174; nom_seg2=25; nom_sjw=25; break;
            case CAN_BR_250K: 	nom_brp=2 ; nom_seg1=139; nom_seg2=20; nom_sjw=20; break;
            case CAN_BR_500K: 	nom_brp=1 ; nom_seg1=139; nom_seg2=20; nom_sjw=20; break;
            case CAN_BR_1M:		nom_brp=1 ; nom_seg1=59 ; nom_seg2=20; nom_sjw=20; break;
        }
        dat_brp=1 ; dat_seg1=29; dat_seg2=10; dat_sjw=10;
        hfdcan->Init.FrameFormat = FDCAN_FRAME_CLASSIC;
    }

    if(mode == CAN_FD_BRS)
    {
        switch (baud)
        {
            case CAN_BR_2M: 	dat_brp=1 ; dat_seg1=29; dat_seg2=10; dat_sjw=10; break;
            case CAN_BR_2M5: 	dat_brp=1 ; dat_seg1=25; dat_seg2=6 ; dat_sjw=6 ; break;
            case CAN_BR_3M2: 	dat_brp=1 ; dat_seg1=19; dat_seg2=5 ; dat_sjw=5 ; break;
            case CAN_BR_4M: 	dat_brp=1 ; dat_seg1=14; dat_seg2=5 ; dat_sjw=5 ; break;
            case CAN_BR_5M:		dat_brp=1 ; dat_seg1=13; dat_seg2=2 ; dat_sjw=2 ; break;
        }
        nom_brp=1 ; nom_seg1=59 ; nom_seg2=20; nom_sjw=20;
        hfdcan->Init.FrameFormat = FDCAN_FRAME_FD_BRS;
    }

    HAL_FDCAN_DeInit(hfdcan);

    hfdcan->Init.NominalPrescaler = nom_brp;
    hfdcan->Init.NominalTimeSeg1  = nom_seg1;
    hfdcan->Init.NominalTimeSeg2  = nom_seg2;
    hfdcan->Init.NominalSyncJumpWidth = nom_sjw;

    hfdcan->Init.DataPrescaler = dat_brp;
    hfdcan->Init.DataTimeSeg1  = dat_seg1;
    hfdcan->Init.DataTimeSeg2  = dat_seg2;
    hfdcan->Init.DataSyncJumpWidth = dat_sjw;

    HAL_FDCAN_Init(hfdcan);
}

/* ----------------------- 回调函数定义 --------------------------*/

/**
 * @brief 处理FIFO0和FIFO1接收中断
 * @param _hcan FDCAN句柄
 * @param fifox 传递给HAL_FDCAN_GetRxMessage()以从特定FIFO获取消息
 */
static void FDCANFIFOxCallback(FDCAN_HandleTypeDef *_hcan, uint32_t fifox)
{
    FDCAN_RxHeaderTypeDef rxconf;
    uint8_t can_rx_buff[64];

    while (HAL_FDCAN_GetRxFifoFillLevel(_hcan, fifox)) // FIFO不为空
    {
        HAL_FDCAN_GetRxMessage(_hcan, fifox, &rxconf, can_rx_buff);

        for (size_t i = 0; i < idx; ++i)
        {
            if (_hcan == fdcan_instance[i]->can_handle &&
                rxconf.Identifier == fdcan_instance[i]->rx_id)
            {
                if (fdcan_instance[i]->can_module_callback != NULL)
                {
                    fdcan_instance[i]->rx_len = FDCANDlcToLen(rxconf.DataLength);
                    memcpy(fdcan_instance[i]->rx_buff, can_rx_buff, fdcan_instance[i]->rx_len);
                    fdcan_instance[i]->can_module_callback(fdcan_instance[i]);
                }
                return;
            }
        }
    }
}

// 弱定义的接收回调函数
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

/**
 * @brief FIFO0接收回调函数
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
{
    if ((RxFifo0ITs & FDCAN_IT_RX_FIFO0_NEW_MESSAGE) != RESET)
    {
        FDCANFIFOxCallback(hfdcan, FDCAN_RX_FIFO0);

        // 调用用户回调
        if (hfdcan == &hfdcan1)
        {
            fdcan1_rx_callback();
        }
    }
}

/**
 * @brief FIFO1接收回调函数
 */
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
{
    if ((RxFifo1ITs & FDCAN_IT_RX_FIFO1_NEW_MESSAGE) != RESET)
    {
        FDCANFIFOxCallback(hfdcan, FDCAN_RX_FIFO1);

        // 调用用户回调
        if (hfdcan == &hfdcan2)
        {
            fdcan2_rx_callback();
        }
        else if (hfdcan == &hfdcan3)
        {
            fdcan3_rx_callback();
        }
    }
}

/**
 * @brief 错误状态回调函数
 */
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
{
    if (ErrorStatusITs & FDCAN_IR_BO)
    {
        // 总线关闭恢复
        CLEAR_BIT(hfdcan->Instance->CCCR, FDCAN_CCCR_INIT);
        hfdcan->ErrorCode = 0;
    }
    if (ErrorStatusITs & FDCAN_IR_EP)
    {
        // 错误被动状态处理，重新初始化FDCAN
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

        // 重新配置过滤器并启动
        HAL_FDCAN_Start(hfdcan);
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