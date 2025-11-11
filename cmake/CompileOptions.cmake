# 设置全局编译选项，适用于所有目标
add_compile_options(
        # 基础编译选项
        -pipe           # 使用管道而非临时文件进行编译，加快编译速度
        -Wall           # 启用所有常见的警告信息
        -fmessage-length=0 # 设置错误消息长度为0，确保完整显示长消息

        # 优化相关选项
        -ffunction-sections # 将每个函数放在独立的section中，便于链接时优化
        -fdata-sections     # 将每个数据放在独立的section中，便于链接时优化
        -fno-common         # 禁止将未初始化的全局变量放在common块，有助于发现重复定义
)

# 设置全局链接选项
add_link_options(
        -pipe           # 在链接阶段也使用管道

        # 链接器特定选项
        -Wl,--no-warn-rwx-segments # 关闭RWX段警告（可读可写可执行段）

        # 库链接选项
        -lc             # 链接C标准库
        -lstdc++        # 链接C++标准库
        -lm             # 链接数学库
        -lnosys         # 链接nosys库（为嵌入式系统提供空实现的系统调用）

        # 优化相关链接选项
        -Wl,--gc-sections   # 垃圾回收未使用的section，减小程序体积
        -flto               # 启用链接时优化
        -specs=nano.specs   # 使用nano规格（精简版C库，适用于嵌入式系统）
        -specs=nosys.specs  # 使用nosys规格（无操作系统支持）
)

# 为目标添加预处理器宏定义
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        -DUSE_HAL_DRIVER        # 使用HAL硬件抽象层驱动
        -DARM_MATH_CM7          # 使用Cortex-M7内核的数学库
        -DARM_MATH_MATRIX_CHECK # 启用矩阵运算边界检查
        -DARM_MATH_ROUNDING     # 启用数学运算舍入功能
        -DARM_MATH_LOOPUNROLL   # 启用循环展开优化
        -DDISABLEFLOAT16        # 禁用16位浮点数支持
)

# 为目标添加特定的编译选项
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
        -mthumb             # 生成thumb指令集代码（ARM的紧凑指令集）
        -mthumb-interwork   # 支持thumb和ARM指令集间互操作
        -mfpu=fpv5-d16      # 指定FPU类型：FPv5，16个双精度寄存器
        -mfloat-abi=hard    # 使用硬浮点ABI（浮点运算由硬件FPU处理）
)