# 设置目标系统为通用系统（无操作系统）
set(CMAKE_SYSTEM_NAME Generic)
# 设置目标处理器架构为ARM
set(CMAKE_SYSTEM_PROCESSOR arm)

# 明确指定编译器为Clang
set(CMAKE_C_COMPILER_ID Clang)
set(CMAKE_CXX_COMPILER_ID Clang)

# 设置工具链前缀为starm-（ST ARM工具链）
set(TOOLCHAIN_PREFIX starm-)

# 设置C编译器为starm-clang
set(CMAKE_C_COMPILER ${TOOLCHAIN_PREFIX}clang)
# 设置汇编器使用C编译器（clang也可以处理汇编文件）
set(CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER})
# 设置C++编译器为starm-clang++
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}clang++)
# 设置链接器为starm-clang
set(CMAKE_LINKER ${TOOLCHAIN_PREFIX}clang)
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

# STARM_TOOLCHAIN_CONFIG允许选择工具链配置，可能的取值有：
#  "STARM_HYBRID"   : 混合配置，使用starm-clang汇编器和编译器，但使用GNU链接器
#  "STARM_NEWLIB"   : 使用NEWLIB C库的starm-clang工具链
#  "STARM_PICOLIBC" : 使用PICOLIBC C库的starm-clang工具链
set(STARM_TOOLCHAIN_CONFIG "STARM_HYBRID")

# 根据选择的工具链配置设置多库配置选项
if (STARM_TOOLCHAIN_CONFIG STREQUAL "STARM_HYBRID")
    # 混合配置：使用GNU工具链的多库配置和GCC工具链路径
    set(TOOLCHAIN_MULTILIBS "--multi-lib-config=\"$ENV{CLANG_GCC_CMSIS_COMPILER}/multilib.gnu_tools_for_stm32.yaml\" --gcc-toolchain=\"$ENV{GCC_TOOLCHAIN_ROOT}/..\"")
elseif (STARM_TOOLCHAIN_CONFIG STREQUAL "STARM_NEWLIB")
    # Newlib配置：使用newlib.cfg配置文件
    set(TOOLCHAIN_MULTILIBS "--config=newlib.cfg")
endif ()

# 设置MCU特定的编译标志
set(TARGET_FLAGS "-mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard ${TOOLCHAIN_MULTILIBS}")

# 将MCU标志添加到C编译标志中
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${TARGET_FLAGS}")
# 设置汇编器标志：包含C标志，并添加汇编特定选项
set(CMAKE_ASM_FLAGS "${CMAKE_C_FLAGS} -x assembler-with-cpp -MP")
# 添加警告和优化相关标志到C编译标志
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fdata-sections -ffunction-sections")

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

# 设置C++标志：包含C标志，并禁用RTTI、异常和线程安全静态变量
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -fno-rtti -fno-exceptions -fno-threadsafe-statics")

# 设置可执行文件链接标志
set(CMAKE_EXE_LINKER_FLAGS "${TARGET_FLAGS}")

# 根据工具链配置添加特定的链接选项
if (STARM_TOOLCHAIN_CONFIG STREQUAL "STARM_HYBRID")
    # 混合配置：使用GCC的nano规格
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --gcc-specs=nano.specs")
    # 链接数学库
    set(TOOLCHAIN_LINK_LIBRARIES "m")
elseif (STARM_TOOLCHAIN_CONFIG STREQUAL "STARM_NEWLIB")
    # Newlib配置：使用无系统支持的C运行时
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lcrt0-nosys")
elseif (STARM_TOOLCHAIN_CONFIG STREQUAL "STARM_PICOLIBC")
    # Picolibc配置：使用托管环境的C运行时
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lcrt0-hosted")
endif ()

# 添加链接脚本文件
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -T \"${CMAKE_SOURCE_DIR}/STM32H723XG_FLASH.ld\"")
# 使用nano规格（精简C库）
# set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} --specs=nano.specs")
# 生成map文件并启用垃圾回收
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,-Map=${CMAKE_PROJECT_NAME}.map -Wl,--gc-sections")
# 链接C库和数学库（使用组解决循环依赖）
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lc -lm -Wl,--end-group")
# 打印内存使用情况
set(CMAKE_C_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--print-memory-usage")

# 设置C++链接标志：在C链接标志基础上添加C++标准库支持
set(CMAKE_CXX_LINK_FLAGS "${CMAKE_C_LINK_FLAGS} -Wl,--start-group -lstdc++ -lsupc++ -Wl,--end-group")