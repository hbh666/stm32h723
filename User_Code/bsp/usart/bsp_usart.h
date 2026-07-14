#ifndef BSP_USART_H
#define BSP_USART_H

#include <stdint.h>
#include "main.h"

#define DEVICE_USART_CNT 5     // 喵板至多分配5个串口
#define USART_RXBUFF_LIMIT 256 // 如果协议需要更大的buff,请修改这里

typedef struct usart_instance USART_Instance;

// 模块回调函数,用于解析协议
typedef void (*usart_module_callback)(USART_Instance *instance, const uint8_t *recv_data, uint16_t recv_size);

/* 发送模式枚举 */
typedef enum
{
    USART_TRANSFER_NONE = 0,
    USART_TRANSFER_BLOCKING,
    USART_TRANSFER_IT,
    USART_TRANSFER_DMA,
} USART_TRANSFER_MODE;

// 串口实例结构体,每个module都要包含一个实例.
// 由于串口是独占的点对点通信,所以不需要考虑多个module同时使用一个串口的情况,因此不用加入id;当然也可以选择加入,这样在bsp层可以访问到module的其他信息
struct usart_instance
{
    uint8_t recv_buff[USART_RXBUFF_LIMIT] __attribute__((aligned(32))); // DMA接收buffer
    uint16_t recv_buff_size; // 模块接收一包数据的大小
    UART_HandleTypeDef *usart_handle; // 实例对应的usart_handle
    usart_module_callback module_callback; // 解析收到的数据的回调函数
    volatile uint32_t rx_event_count;
    volatile uint32_t uart_error_count;
    volatile uint32_t rx_restart_error_count;
    volatile uint32_t last_uart_error;
    volatile uint16_t last_rx_size;
    volatile uint8_t rx_restart_pending;
};

/* usart 初始化配置结构体 */
typedef struct
{
    uint16_t recv_buff_size; // 模块接收一包数据的大小
    UART_HandleTypeDef *usart_handle; // 实例对应的usart_handle
    usart_module_callback module_callback; // 解析收到的数据的回调函数
} USART_Init_Config_s;

/**
 * @brief 注册一个串口实例,返回一个串口实例指针
 *
 * @param init_config 传入串口初始化结构体
 */
USART_Instance *USARTRegister(USART_Init_Config_s *init_config);

/**
 * @brief 启动串口DMA接收服务,需要传入一个已注册的usart实例
 *
 * @param _instance
 */
HAL_StatusTypeDef USARTServiceInit(USART_Instance *_instance);

/**
 * @brief 在任务上下文中恢复中断回调里启动失败的串口接收
 */
void USARTServiceTask(void);


/**
 * @brief 通过调用该函数可以发送一帧数据,需要传入一个usart实例,发送buff以及这一帧的长度
 * @note 在短时间内连续调用此接口,若采用IT/DMA会导致上一次的发送未完成而新的发送取消.
 * @note 若希望连续使用DMA/IT进行发送,请配合USARTIsReady()使用,或自行为你的module实现一个发送队列和任务.
 * @todo 是否考虑为USARTInstance增加发送队列以进行连续发送?
 * 
 * @param _instance 串口实例
 * @param send_buf 待发送数据的buffer
 * @param send_size how many bytes to send
 */
void USARTSend(USART_Instance *_instance, uint8_t *send_buf, uint16_t send_size, USART_TRANSFER_MODE mode);

/**
 * @brief 判断串口是否准备好,用于连续或异步的IT/DMA发送
 *
 * @param _instance 要判断的串口实例
 * @return uint8_t ready 1, busy 0
 */
uint8_t USARTIsReady(USART_Instance *_instance);

#endif
