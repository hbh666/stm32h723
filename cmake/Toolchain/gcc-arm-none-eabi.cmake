# 设置目标系统为通用系统（无操作系统）
set(CMAKE_SYSTEM_NAME Generic)
# 设置目标处理器架构为ARM
set(CMAKE_SYSTEM_PROCESSOR arm)

# 以下三行被注释掉的设置用于强制指定编译器信息
#set(CMAKE_C_COMPILER_FORCED TRUE)
#set(CMAKE_CXX_COMPILER_FORCED TRUE)
#set(CMAKE_C_COMPILER_ID GNU)
#set(CMAKE_CXX_COMPILER_ID GNU)

# 设置工具链前缀（需要在系统PATH环境变量中包含arm-none-eabi-工具链）
set(TOOLCHAIN_PREFIX arm-none-eabi-)

# 设置C编译器为arm-none-eabi-gcc
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}gcc)
# 设置汇编器使用C编译器（gcc也可以处理汇编文件）
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
# 设置C++编译器为arm-none-eabi-g++
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
# 设置链接器为arm-none-eabi-g++（使用C++编译器进行链接）
set(CMAKE_LINKER ${TOOLCHAIN_PREFIX}g++)
# 设置objcopy工具，用于生成二进制文件等
set(CMAKE_OBJCOPY ${TOOLCHAIN_PREFIX}objcopy)
# 设置size工具，用于查看程序内存占用
set(CMAKE_SIZE ${TOOLCHAIN_PREFIX}size)

# 设置各种源文件生成的可执行文件后缀为.elf
set(CMAKE_EXECUTABLE_SUFFIX_ASM ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_C ".elf")
set(CMAKE_EXECUTABLE_SUFFIX_CXX ".elf")

# 设置尝试编译的目标类型为静态库（用于工具链测试）
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# 设置MCU特定的编译标志
set(TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard ")

# 将MCU标志添加到C编译标志中
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
# 添加警告和优化相关标志
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wpedantic -fdata-sections -ffunction-sections")

# 根据构建类型设置不同的优化级别
if (CMAKE_BUILD_TYPE MATCHES "Debug")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")  # 调试模式：无优化，包含调试信息
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
elseif (CMAKE_BUILD_TYPE MATCHES "Release")
    set(CMAKE_C_FLAGS_RELEASE "-O3")   # 发布模式：最高级别优化
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
elseif (CMAKE_BUILD_TYPE MATCHES "RelWithDebInfo")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")  # 带调试信息的发布模式：中等优化，包含调试信息
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
elseif (CMAKE_BUILD_TYPE MATCHES "MinSizeRel")
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os")  # 最小体积模式：优化代码大小
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
endif ()

# 设置汇编器标志：包含C标志，并添加汇编特定选项
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MMD -MP")
# 设置C++标志：包含C标志，并禁用RTTI、异常和线程安全静态变量
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")

# 设置C链接标志
set(CMAKE_C_LINK_FLAGS "${TARGET_FLAGS}")
# 添加链接脚本文件
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32H723XG_FLASH.ld\"")
# 使用nano规格（精简C库）
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --specs=nano.specs")
# 生成map文件并启用垃圾回收
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
# 链接C库和数学库（使用组解决循环依赖）
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lc -lm -Wl,--end-group")
# 打印内存使用情况
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--print-memory-usage")

# 设置C++链接标志：在C链接标志基础上添加C++标准库支持
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group")

set(TOOLCHAIN_LINK_LIBRARIES "m")