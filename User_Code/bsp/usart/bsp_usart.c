/**
 * @file bsp_usart.c
 * @author
 * @brief  串口bsp层的实现
 * @version beta
 * @date
 *
 * @copyright
 *
 */

#include "bsp_usart.h"
#include "bsp_log.h"
#include "memory.h"

/* usart service instance, modules' info would be recoreded here using USARTRegister() */
/* usart服务实例,所有注册了usart的模块信息会被保存在这里 */
static uint8_t idx;
static USART_Instance *usart_instance[DEVICE_USART_CNT] = {NULL};
static USART_Instance usart_instance_pool[DEVICE_USART_CNT]
    __attribute__((section(".dma_buffer"), aligned(32)));

static USART_Instance *USARTFindInstance(UART_HandleTypeDef *huart)
{
    for (uint8_t i = 0; i < idx; ++i)
    {
        if (usart_instance[i]->usart_handle == huart)
            return usart_instance[i];
    }
    return NULL;
}

/**
 * @brief 启动串口DMA接收服务,模块完成实例和回调初始化后显式调用
 *
 * @note 配合DMA_NORMAL和ReceiveToIdle使用,每次接收事件后由BSP重新启动
 *
 * @param _instance instance owned by module,模块拥有的串口实例
 */
HAL_StatusTypeDef USARTServiceInit(USART_Instance *_instance)
{
    if (_instance == NULL || _instance->usart_handle == NULL || _instance->usart_handle->hdmarx == NULL)
        return HAL_ERROR;

    HAL_StatusTypeDef status = HAL_UARTEx_ReceiveToIdle_DMA(
        _instance->usart_handle, _instance->recv_buff, _instance->recv_buff_size);

    // 关闭dma half transfer中断防止两次进入HAL_UARTEx_RxEventCallback()
    // 这是HAL库的一个设计失误,发生DMA传输完成/半完成以及串口IDLE中断都会触发HAL_UARTEx_RxEventCallback()
    // 我们只希望处理第一种和第三种情况,因此直接关闭DMA半传输中断
    if (status == HAL_OK)
    {
        __HAL_DMA_DISABLE_IT(_instance->usart_handle->hdmarx, DMA_IT_HT);
        _instance->rx_restart_pending = 0;
    }
    else
    {
        _instance->rx_restart_pending = 1;
    }

    return status;
}

static HAL_StatusTypeDef USARTRecoverRx(USART_Instance *instance)
{
    UART_HandleTypeDef *huart = instance->usart_handle;
    DMA_HandleTypeDef *hdma = huart->hdmarx;
    HAL_StatusTypeDef abort_status = HAL_UART_AbortReceive(huart);

    if (abort_status != HAL_OK || HAL_DMA_GetState(hdma) != HAL_DMA_STATE_READY)
    {
        if (HAL_DMA_DeInit(hdma) != HAL_OK || HAL_DMA_Init(hdma) != HAL_OK)
            return HAL_ERROR;

        /* An abort timeout returns before HAL restores the UART Rx state. */
        if (HAL_UART_AbortReceive(huart) != HAL_OK)
            return HAL_ERROR;
    }

    return USARTServiceInit(instance);
}

void USARTServiceTask(void)
{
    for (uint8_t i = 0; i < idx; ++i)
    {
        USART_Instance *instance = usart_instance[i];
        if (!instance->rx_restart_pending)
            continue;

        if (USARTRecoverRx(instance) != HAL_OK)
            instance->rx_restart_error_count++;
    }
}

USART_Instance *USARTRegister(USART_Init_Config_s *init_config)
{
    if (init_config == NULL || init_config->usart_handle == NULL || init_config->usart_handle->hdmarx == NULL ||
        init_config->recv_buff_size == 0 || init_config->recv_buff_size > USART_RXBUFF_LIMIT)
    {
        LOGERROR("[bsp_usart] invalid USART register config");
        return NULL;
    }
    if (init_config->usart_handle->hdmarx->Init.Mode != DMA_NORMAL)
    {
        LOGERROR("[bsp_usart] ReceiveToIdle service requires DMA_NORMAL");
        return NULL;
    }

    if (idx >= DEVICE_USART_CNT) // 超过最大实例数
    {
        LOGERROR("[bsp_usart] USART exceed max instance count!");
        return NULL;
    }

    for (uint8_t i = 0; i < idx; i++) // 检查是否已经注册过
        if (usart_instance[i]->usart_handle == init_config->usart_handle)
        {
            LOGERROR("[bsp_usart] USART instance already registered!");
            return NULL;
        }

    USART_Instance *instance = &usart_instance_pool[idx];
    memset(instance, 0, sizeof(USART_Instance));

    instance->usart_handle = init_config->usart_handle;
    instance->recv_buff_size = init_config->recv_buff_size;
    instance->module_callback = init_config->module_callback;

    usart_instance[idx++] = instance;
    return instance;
}

/* @todo 当前仅进行了形式上的封装,后续要进一步考虑是否将module的行为与bsp完全分离 */
void USARTSend(USART_Instance *_instance, uint8_t *send_buf, uint16_t send_size, USART_TRANSFER_MODE mode)
{
    switch (mode)
    {
        case USART_TRANSFER_BLOCKING:
            HAL_UART_Transmit(_instance->usart_handle, send_buf, send_size, 100);
            break;
        case USART_TRANSFER_IT:
            HAL_UART_Transmit_IT(_instance->usart_handle, send_buf, send_size);
            break;
        case USART_TRANSFER_DMA:
            HAL_UART_Transmit_DMA(_instance->usart_handle, send_buf, send_size);
            break;
        default:
            while (1); // illegal mode! check your code context! 检查定义instance的代码上下文,可能出现指针越界
            break;
    }
}

/* 串口发送时,gstate会被设为BUSY_TX */
uint8_t USARTIsReady(USART_Instance *_instance)
{
    return _instance != NULL && _instance->usart_handle != NULL &&
           _instance->usart_handle->gState == HAL_UART_STATE_READY;
}

/**
 * @brief 每次dma/idle中断发生时，都会调用此函数.对于每个uart实例会调用对应的回调进行进一步的处理
 *        例如:视觉协议解析/遥控器解析/裁判系统解析
 *
 * @note  通过__HAL_DMA_DISABLE_IT(huart->hdmarx,DMA_IT_HT)关闭dma half transfer中断防止两次进入HAL_UARTEx_RxEventCallback()
 *        这是HAL库的一个设计失误,发生DMA传输完成/半完成以及串口IDLE中断都会触发HAL_UARTEx_RxEventCallback()
 *        我们只希望处理，因此直接关闭DMA半传输中断第一种和第三种情况
 *
 * @param huart 发生中断的串口
 * @param Size 此次接收到的数据量
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    USART_Instance *instance = USARTFindInstance(huart);
    if (instance == NULL)
        return;

    instance->rx_event_count++;
    instance->last_rx_size = Size;

    if (Size > 0 && Size <= instance->recv_buff_size && instance->module_callback != NULL)
        instance->module_callback(instance, instance->recv_buff, Size);

    if (USARTServiceInit(instance) != HAL_OK)
        instance->rx_restart_error_count++;
}

/**
 * @brief 当串口发送/接收出现错误时,会调用此函数,此时这个函数要做的就是重新启动接收
 *
 * @note  最常见的错误:奇偶校验/溢出/帧错误
 *
 * @param huart 发生错误的串口
 */
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    USART_Instance *instance = USARTFindInstance(huart);
    if (instance == NULL)
        return;

    instance->uart_error_count++;
    instance->last_uart_error = HAL_UART_GetError(huart);

    if (USARTServiceInit(instance) != HAL_OK)
        instance->rx_restart_error_count++;
}


