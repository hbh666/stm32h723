# SBUS 遥控接收修复说明

这份记录用来说明这次从“遥控器收不到数据 / `rc_ctrl` 不变化”一路排查到当前稳定版本的主要修改点。写得偏简略，重点放在为什么改、DMA 怎么配、哪些文件动过，以及最后对当前 `git diff` 的复核结论。

## 1. 一开始的问题和定位思路

最开始的现象是：遥控器链路没有让 `rc_ctrl` 更新，后面又观察到 `rc_debug.rx_event_count` 也不增长，说明问题不只是协议解析，而是 UART/DMA 接收事件本身没有稳定进入。

排查时按链路分层看：

1. **初始化链路**：发现遥控器模块没有在 `RobotInit()` 里真正初始化，所以补了 `RemoteControlInit(&huart5)`。
2. **UART 协议配置**：用户确认 SBUS 硬件已经做了反向，所以保持 UART5 为 `100000 / 8E2`，也就是 HAL 里的 `UART_WORDLENGTH_9B + UART_PARITY_EVEN + UART_STOPBITS_2`。当前没有启用软件 `RXINV`。
3. **GPIO 电平配置**：UART5 RX 是 PD2，当前保持 `GPIO_NOPULL`，没有继续保留“内部上拉”的实验配置。
4. **DMA 内存可访问性**：H7 上 DMA1/DMA2 不能访问 DTCM。原来接收对象如果落到 DTCM，就可能导致 DMA 没法真正写入数据。现在把 USART 实例池和接收 buffer 放进 `.dma_buffer` 段，并链接到 RAM_D1。
5. **DMA 接收方式**：原来的 UART5 RX DMA 是 `DMA_CIRCULAR`，但这里使用的是 `HAL_UARTEx_ReceiveToIdle_DMA()`，为了按实际一帧 25 字节触发并重启，改成 `DMA_NORMAL + ReceiveToIdle`。
6. **异常恢复**：如果 UART/DMA 出错或重启失败，不能只在中断里死等。现在加了 `USARTServiceTask()`，在 daemon 任务上下文里做 stream 级恢复。
7. **SBUS 解析**：只接受完整 25 字节帧，检查帧头 `0x0F`，尾字节允许 `0x00/0x04/0x14/0x24/0x34`，并处理 failsafe/offline。

后面临时试过的 `RXINV`、PD2 上拉、50 字节接收、滑动同步、`rc_debug` 等实验代码已经撤掉，当前版本回到“硬件反相 + 正常 SBUS 帧解析”的方案。

## 2. DMA 当前怎么配置

UART5 RX 的 DMA 配置在 `Core/Src/usart.c` 和 `TronOneH7_Scaffold.ioc` 中保持一致：

- UART：`UART5`
- RX 引脚：`PD2`
- DMA stream：`DMA1_Stream5`
- Request：`DMA_REQUEST_UART5_RX`
- 方向：`DMA_PERIPH_TO_MEMORY`
- 外设地址不自增：`DMA_PINC_DISABLE`
- 内存地址自增：`DMA_MINC_ENABLE`
- 外设/内存数据宽度：`BYTE`
- 模式：`DMA_NORMAL`
- 优先级：`DMA_PRIORITY_VERY_HIGH`
- FIFO：关闭

接收 buffer 的关键点：

- `USART_Instance usart_instance_pool[DEVICE_USART_CNT]` 放在 `User_Code/bsp/usart/bsp_usart.c`。
- 这个池加了 `__attribute__((section(".dma_buffer"), aligned(32)))`。
- `STM32H723XG_FLASH.ld` 新增 `.dma_buffer (NOLOAD)` 段，并放到 `RAM_D1`，避免 DMA 访问 DTCM 失败。
- `USART_Instance.recv_buff` 也做了 32 字节对齐。

启动和重启方式：

- `USARTServiceInit()` 调用 `HAL_UARTEx_ReceiveToIdle_DMA()` 启动接收。
- 启动成功后关闭 DMA 半传输中断 `DMA_IT_HT`，避免半包回调干扰。
- `HAL_UARTEx_RxEventCallback()` 中记录 `rx_event_count`、`last_rx_size`，把实际收到的 `Size` 传给遥控器解析回调，然后重新启动接收。
- `HAL_UART_ErrorCallback()` 中记录 `uart_error_count`、`last_uart_error`，并尝试重启接收。
- 如果中断里重启失败，会置位/累计错误，后续由 `USARTServiceTask()` 在任务上下文里恢复 UART/DMA。

## 3. 主要加了什么，在哪里

- `User_Code/application/robot.c`
  - 定义全局 `volatile RobotMode_t RobotMode`。
  - 在 `RobotInit()` 中调用 `RemoteControlInit(&huart5)`。

- `Core/Src/usart.c`
  - UART5 保持 `100000 / 8E2`。
  - UART5 `AdvancedInit` 保持 `UART_ADVFEATURE_NO_INIT`，没有软件 RXINV。
  - PD2 保持 `GPIO_NOPULL`。
  - UART5 RX DMA 从 `DMA_CIRCULAR` 改为 `DMA_NORMAL`。

- `TronOneH7_Scaffold.ioc`
  - 同步把 `Dma.UART5_RX.13.Mode` 改为 `DMA_NORMAL`。

- `STM32H723XG_FLASH.ld`
  - 新增 `.dma_buffer` 段，放入 `RAM_D1`。

- `User_Code/bsp/usart/bsp_usart.c/.h`
  - USART 实例不再 `malloc`，改用静态实例池，并放入 `.dma_buffer`。
  - 模块回调改为携带 `(USART_Instance *instance, const uint8_t *recv_data, uint16_t recv_size)`。
  - 加入 `rx_event_count`、`last_rx_size`、`uart_error_count`、`last_uart_error`、`rx_restart_error_count` 等接收状态字段。
  - 加入 `USARTServiceTask()` 做 UART/DMA 恢复。
  - `USARTServiceInit()` 改为返回 `HAL_StatusTypeDef`。

- `User_Code/module/periph/remote_control/rc.c/.h`
  - `RemoteControlInit()` 注册 UART5，并显式启动 USART 接收服务。
  - SBUS 解析只接受完整 25 字节帧。
  - 加入帧头、尾字节、failsafe 校验。
  - 加入 `RemoteControlReadSnapshot()`，用于原子读取当前遥控器快照。
  - 修复 `RemoteControlIsOnline()` 和失控清零的竞态。
  - 修复 SWB/SWC 通道、开关边沿标志。

- `Core/Src/freertos.c`
  - daemon task 中调用 `USARTServiceTask()`，让 UART/DMA 异常恢复发生在任务上下文。

- `User_Code/module/software/daemon/daemon.c/.h`
  - 修正 `temp_count/init_count` 初始化逻辑。
  - `DaemonReload()`、`DaemonIsOnline()`、`Daemon_Update()` 加了简单临界区保护。
  - `temp_count` 改为 `volatile`。

- `User_Code/application/indicator_app/ws2812status.c`
  - 不再用局部 `RobotMode` 遮蔽全局状态。
  - LED 显示时结合 `RemoteControlIsOnline()` 判断遥控器离线。

- `User_Code/module/paramdef/robot_def.h`
  - `RobotMode` 声明改为 `extern volatile RobotMode_t RobotMode`。

## 4. 当前建议上板观察项

上板后重点看这些量：

- `rx_event_count` 是否持续增长。
- `last_rx_size` 是否稳定为 `25`。
- `rc_valid_frame_count` 是否持续增长。
- `uart_error_count`、`rx_restart_error_count` 是否不持续增长。
- 原始帧是否类似 `0F ... 00/04/14/24/34`。

注意：当前稳定版本已经没有 `rc_debug` 这个临时调试结构。`rx_event_count` 和 `last_rx_size` 在 `USART_Instance` 里，`rc_valid_frame_count` 等在 `rc.c` 内部是 `static volatile`。如果后续想长期在调试器里直接 watch 一个固定结构，可以再专门加一个轻量 getter 或 debug struct；当前为了回到干净版本，没有保留那套临时代码。

## 5. Git 变更复核和可疑点

已读取当前 `git status --short`、`git diff --stat`、`git diff --check` 和关键文件 diff。当前工作区共有 16 个已修改文件，其中 SBUS 接收链路相关的是：

- `Core/Src/freertos.c`
- `Core/Src/usart.c`
- `STM32H723XG_FLASH.ld`
- `TronOneH7_Scaffold.ioc`
- `User_Code/application/indicator_app/ws2812status.c`
- `User_Code/application/robot.c`
- `User_Code/bsp/usart/bsp_usart.c`
- `User_Code/bsp/usart/bsp_usart.h`
- `User_Code/module/paramdef/robot_def.h`
- `User_Code/module/periph/remote_control/rc.c`
- `User_Code/module/periph/remote_control/rc.h`
- `User_Code/module/software/daemon/daemon.c`
- `User_Code/module/software/daemon/daemon.h`

复核结论：

- `git diff --check` 没有发现空白错误，只提示这些文件下次被 Git 处理时 LF 可能转 CRLF。
- 没有发现 `rc_debug`、`rc_ctrl_debug`、`RemoteControlDebugPoll`、`RXINV`、`RxPinLevelInvert`、50 字节 buffer、滑动同步等实验代码残留。
- UART5 当前确实是 `100000 / 8E2`，没有软件 RXINV；PD2 也是 `GPIO_NOPULL`。
- UART5 RX DMA 在 `.ioc` 和生成代码里都已经是 `DMA_NORMAL`，没有一边改一边没同步的问题。
- `RobotMode` 目前只有一个定义，在 `robot.c`；其它地方通过 `extern volatile` 使用，没看到重复定义。

需要特别注意的可疑/无关变更：

- `User_Code/module/periph/buzzer/buzzer.cpp` 只改了两行注释空格，和 SBUS 修复无关，建议不要混进本次提交。
- `ozonedeb/windebnewestux.jdebug` 和 `ozonedeb/windebnewestux.jdebug.user` 是 Ozone/J-Link 调试器本地状态变化，包括探针序列号、打开窗口、布局、打开文件等，和代码逻辑无关，建议不要混进本次提交。
- `rc_valid_frame_count` 等计数是 `static volatile`，调试符号里一般能看到，但 C 代码外部不能直接引用；这不是接收链路问题，只是“是否方便 watch”的问题。

总体看，SBUS 主链路相关 diff 没看到明显可疑残留；最需要清理的是 `buzzer.cpp` 和 `ozonedeb/*.jdebug*` 这些无关 dirty 文件。
