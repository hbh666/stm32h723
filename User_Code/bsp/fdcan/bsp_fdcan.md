# bsp_fdcan.c/bsp_fdcan.h

> 请注意使用FDCAN设备的时候务必保证总线只接入了2个终端电阻！开发板一般都有一个，6020电机、c620/c610电调、LK电机也都有终端电阻，注意把多于2个的全部断开（通过拨码）

## 使用说明

若你希望新增一个基于FDCAN的module，首先在该模块下应该有一个包含`FDCANInstance`指针的module结构体（或当功能简单的时候，可以是单独存在的
`FDCANInstance`，但不推荐这样做）。

## 代码结构

.h文件内包括了外部接口和类型定义,以及模块对应的宏。c文件内为私有函数和外部接口的定义。

## 类型定义

```c
#define CAN_MX_REGISTER_CNT 16     // 最大FDCAN设备注册数量
#define MX_CAN_FILTER_CNT (3 * 14) // 最多可以使用的CAN过滤器数量，支持3个CAN
#define DEVICE_CAN_CNT 3           // FDCAN1, FDCAN2, FDCAN3

#define CAN_CLASS   0              // 经典CAN模式
#define CAN_FD_BRS  1              // CAN FD模式

#define CAN_BR_125K 0              // 125Kbps
#define CAN_BR_200K 1              // 200Kbps
#define CAN_BR_250K 2              // 250Kbps
#define CAN_BR_500K 3              // 500Kbps
#define CAN_BR_1M   4              // 1Mbps
#define CAN_BR_2M   5              // 2Mbps
#define CAN_BR_2M5  6              // 2.5Mbps
#define CAN_BR_3M2  7              // 3.2Mbps
#define CAN_BR_4M   8              // 4Mbps
#define CAN_BR_5M   9              // 5Mbps

/* FDCAN实例结构体，每个注册到FDCAN的模块都应该有这个变量 */
typedef struct _FDCANInstance
{
    FDCAN_HandleTypeDef *can_handle; // FDCAN句柄
    FDCAN_TxHeaderTypeDef txconf;    // FDCAN报文发送配置
    uint32_t tx_id;                  // 发送id
    uint32_t tx_mailbox;             // FDCAN消息填入的邮箱号
    uint8_t tx_buff[64];             // 发送缓存，FDCAN最大支持64字节
    uint8_t rx_buff[64];             // 接收缓存，FDCAN最大支持64字节
    uint32_t rx_id;                  // 接收id
    uint8_t rx_len;                  // 接收长度
    // 接收的回调函数，用于解析接收到的数据
    void (*can_module_callback)(struct _FDCANInstance *); // 回调函数需要实例来区分注册的设备
    void *id;                        // 使用FDCAN外设的模块指针
} FDCANInstance;

/* FDCAN实例初始化结构体 */
typedef struct
{
    FDCAN_HandleTypeDef *can_handle;              // FDCAN句柄
    uint32_t tx_id;                               // 发送id
    uint32_t rx_id;                               // 接收id
    void (*can_module_callback)(FDCANInstance *); // 处理接收数据的回调函数
    void *id;                                     // 拥有FDCAN实例的模块地址
} FDCAN_Init_Config_s;

typedef void (*fdcan_callback)(FDCANInstance *);
```

- `CAN_MX_REGISTER_CNT`是最大的FDCAN设备注册数量，当每个设备的发送频率都较高时，设备过多会产生总线拥塞从而出现丢包和数据错误的情况。
- `MX_CAN_FILTER_CNT`是最大的FDCAN接收过滤器数量，三个FDCAN共享多个过滤器。当前为简单起见，每个过滤器只设置一组规则用于控制一个id的过滤。
- `DEVICE_CAN_CNT`是MCU拥有的FDCAN硬件数量（H7有3个）。

- `FDCANInstance`
  是一个FDCAN实例。注意，FDCAN作为一个总线设备，一条总线上可以挂载多个设备，因此多个设备可以共享同一个FDCAN硬件。其成员变量包括发送id，发送邮箱，发送buff以及接收buff，还有接收id和接收协议解析回调函数。
  **FDCAN支持最大64字节的数据帧**，因此收发buff长度设置为64字节。

- `FDCAN_Init_Config_s`是用于初始化FDCAN实例的结构，在调用FDCAN实例的初始化函数时传入。

- `can_module_callback()`是模块提供给FDCAN接收中断回调函数使用的协议解析函数指针。对于每个需要FDCAN的模块，需要定义一个这样的函数用于解包数据。
- 每个使用FDCAN外设的module，都需要在其内部定义一个`FDCANInstance*`。

## 外部接口

```c
FDCANInstance *FDCANRegister(FDCAN_Init_Config_s *config);
void FDCANSetDLC(FDCANInstance *_instance, uint8_t length);
uint8_t FDCANTransmit(FDCANInstance *_instance, float timeout);
void bsp_fdcan_set_baud(FDCAN_HandleTypeDef *hfdcan, uint8_t mode, uint8_t baud);
```

`FDCANRegister`是用于初始化FDCAN实例的接口，module层的模块对象（也应当为一个结构体）内要包含一个`FDCANInstance`
。调用时传入初始化配置的指针。`FDCANRegister`应当在module的初始化函数内被调用，推荐config采用以下的方式定义，更加直观明了：

```c
FDCAN_Init_Config_s config = {
    .can_handle = &hfdcan1,
    .tx_id = 0x005,
    .rx_id = 0x200,
    .can_module_callback = MotorCallback,
    .id = (void*)motor_ptr
};
```

`FDCANTransmit()`是通过模块通过其拥有的FDCAN实例发送数据的接口，调用时传入对应的instance。在发送之前，应当给instance内的
`tx_buff`赋值。

`bsp_fdcan_set_baud()`用于设置FDCAN的波特率和模式（经典CAN或CAN FD）。

## 私有函数和变量

在.c文件内设为static的函数和变量

```c
static FDCANInstance *fdcan_instance[CAN_MX_REGISTER_CNT] = {NULL};
```

这是bsp层管理所有FDCAN实例的入口。

```c
static void FDCANServiceInit()
static void FDCANAddFilter(FDCANInstance *_instance)
static void FDCANFIFOxCallback(FDCAN_HandleTypeDef *_hcan, uint32_t fifox)
static uint8_t FDCANDlcToLen(uint32_t dlc)
static uint32_t FDCANLenToDlc(uint8_t len)
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs)
void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs)
void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs)
```

- `FDCANServiceInit()`会被`FDCANRegister()`调用，对FDCAN外设进行硬件初始化并开启接收中断和消息提醒。

- `FDCANAddFilter()`在每次使用`FDCANRegister()`的时候被调用，用于给当前注册的实例添加过滤器规则并设定处理对应`rx_id`
  的接收FIFO。过滤器的作用是减小FDCAN收发器的压力，只接收符合过滤器规则的报文（否则不会产生接收中断）。

- `FDCANDlcToLen()`和`FDCANLenToDlc()`是FDCAN特有的数据长度码转换函数，用于在数据长度码和实际字节数之间转换。

- `HAL_FDCAN_RxFifo0Callback()`和`HAL_FDCAN_RxFifo1Callback()`都是对HAL的FDCAN回调函数的重定义（原本的callback是`__weak`
  修饰的弱定义），当发生FIFO0或FIFO1有新消息到达的时候，对应的callback会被调用。`FDCANFIFOxCallback()`
  随后被前两者调用，并根据接收id和硬件中断来源（哪一个FDCAN硬件）调用对应的instance的回调函数进行协议解析。

- `HAL_FDCAN_ErrorStatusCallback()`处理FDCAN的错误状态，包括总线关闭和错误被动状态的恢复。

- 当有一个模块注册了多个FDCAN实例时，通过`FDCANInstance.id`，使用强制类型转换将其转换成对应模块的实例指针，就可以对不同的模块实例进行回调处理了。

## 弱定义回调函数

```c
__weak void fdcan1_rx_callback(void);
__weak void fdcan2_rx_callback(void);
__weak void fdcan3_rx_callback(void);
```

这些是弱定义的FDCAN接收回调函数，用户可以根据需要重写这些函数来实现特定的功能。

## 注意事项

1. **终端电阻**：使用FDCAN设备时务必保证总线只接入了2个终端电阻！

2. **FDCAN特性**：
    - FDCAN支持经典CAN和CAN FD模式
    - CAN FD模式下支持最高64字节的数据帧
    - 支持更高的通信速率（最高5Mbps）

3. **发送超时**：由于FDCAN总线自带发送检测，如果总线上没有挂载目标设备，那么发送FIFO会被占满而无法发送。在`FDCANTransmit()`
   中会对发送FIFO是否已满进行检查。当超出`timeout`之后函数会返回零，说明发送失败。

4. **任务阻塞**：由于卡在`while(1)`处不断检查FIFO是否空闲，调用`FDCANTransmit()`
   的任务可能无法按时挂起，导致任务定时不精确。建议在没有连接FDCAN进行调试时，按需注释掉有关FDCAN发送的代码部分，或设定一个较小的
   `timeout`值，防止对其他需要精确定时的任务产生影响。

5. **模式选择**：使用`bsp_fdcan_set_baud()`函数可以在经典CAN模式和CAN FD模式之间切换，注意不同模式支持的波特率范围不同。

6. **数据长度**：FDCAN支持最大64字节的数据帧，使用`FDCANSetDLC()`函数设置数据长度时要注意不要超过64字节。