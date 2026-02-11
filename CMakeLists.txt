# ==============================================================================
# 大连民族大学创一工作室 达妙开发板脚手架-CMake构建系统
# ==============================================================================
cmake_minimum_required(VERSION 3.22)

#欢迎提示
message(NOTICE ">> 欢迎使用创一工作室达妙开发板脚手架的CMake构建系统")
message(NOTICE ">> 如报错信息为未安装环境或找不到可用编译器，请先安装STM32CubeCLT，可以在ST官网找到下载链接 https://www.st.com/en/development-tools/stm32cubeclt.html")
message(NOTICE ">> 安装完STM32CubeCLT后记得重启刷新编译器环境变量或自己添加到环境变量PATH中")
message(NOTICE ">> 如使用Makefile，需要自己添加Make构建工具的环境变量，网上搜一搜教程，都有的")
message(NOTICE ">> 本项目默认使用arm-none-eabi-gcc，如需使用starm-clang请注意编译优化设置，截止发布前暂无法使用starm-clang构建的elf进行debug，无法打断点")

# ------------------------------------------------------------------------------
# 1. 机器人兵种选择开关 (Robot Configuration)
#    请将当前需要编译的机器人设置为 ON，其余保持 OFF
# ------------------------------------------------------------------------------

option(INFANTRY_MEC        "1. 麦轮步兵 (Mecanum Infantry)"           OFF)
option(INFANTRY_HALF_STEER "2. 半舵步兵 (Half-Steer Infantry)"        OFF)
option(HERO_MEC            "3. 麦轮英雄 (Mecanum Hero)"               OFF)
option(SENTRY_STEER        "4. 舵轮哨兵 (Steer Sentry)"               OFF)
option(INFANTRY_TRANSFORM  "5. 变形步兵 (Transformable Infantry)"     OFF)
option(BALANCE_SERIAL      "6. 串联腿平衡步兵 (Serial-Leg Balance)"    OFF)
option(BALANCE_PARALLEL    "7. 并联腿平衡步兵 (Parallel-Leg Balance)"  OFF)
option(DART                "8. 飞镖架 (Dart System)"                  OFF)

# ------------------------------------------------------------------------------
# 2. 安全互斥检查 (Safety Check)
# ------------------------------------------------------------------------------
set(ROBOT_ENABLE_COUNT 0)
if(INFANTRY_MEC)
    math(EXPR ROBOT_ENABLE_COUNT "${ROBOT_ENABLE_COUNT}+1")
endif()
if(INFANTRY_HALF_STEER)
    math(EXPR ROBOT_ENABLE_COUNT "${ROBOT_ENABLE_COUNT}+1")
endif()
if(HERO_MEC)
    math(EXPR ROBOT_ENABLE_COUNT "${ROBOT_ENABLE_COUNT}+1")
endif()
if(SENTRY_STEER)
    math(EXPR ROBOT_ENABLE_COUNT "${ROBOT_ENABLE_COUNT}+1")
endif()
if(INFANTRY_TRANSFORM)
    math(EXPR ROBOT_ENABLE_COUNT "${ROBOT_ENABLE_COUNT}+1")
endif()
if(BALANCE_SERIAL)
    math(EXPR ROBOT_ENABLE_COUNT "${ROBOT_ENABLE_COUNT}+1")
endif()
if(BALANCE_PARALLEL)
    math(EXPR ROBOT_ENABLE_COUNT "${ROBOT_ENABLE_COUNT}+1")
endif()
if(DART)
    math(EXPR ROBOT_ENABLE_COUNT "${ROBOT_ENABLE_COUNT}+1")
endif()

if(ROBOT_ENABLE_COUNT GREATER 1)
    message(FATAL_ERROR "【错误】检测到同时开启了 ${ROBOT_ENABLE_COUNT} 个兵种！请检查 option 设置，确保只有一个为 ON。")
elseif(ROBOT_ENABLE_COUNT EQUAL 0)
    message(WARNING "【警告】未选择任何兵种！将仅编译通用库，无法生成有效固件。")
endif()

# ------------------------------------------------------------------------------
# 3. 基础项目配置 (Project Setup)
# ------------------------------------------------------------------------------

include(cmake/DetectToolchain.cmake)
project(TronOneH7_Scaffold)
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
enable_language(C CXX ASM)

# 标准设置 (C23 / C++23)
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

#标准提示
message(NOTICE ">> 本项目使用 C ${CMAKE_C_STANDARD} 及 C++ ${CMAKE_CXX_STANDARD} 标准")

# 根据构建类型设置不同的编译选项
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")  # 调试模式：无优化，包含调试信息
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS_RELEASE "-O3")   # 发布模式：最高级别优化
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")  # 带调试信息的发布模式：中等优化，包含调试信息
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os")  # 最小体积模式：优化代码大小
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
endif ()

# 输出编译器路径
message(NOTICE ">> [Assembler] 找到的 C 编译器路径: ${CMAKE_C_COMPILER}")
message(NOTICE ">> [Assembler] 找到的 C++ 编译器路径: ${CMAKE_CXX_COMPILER}")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()

# 编译器标志
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -Wno-volatile")

message(NOTICE ">> [Build Type] 当前构建类型: ${CMAKE_BUILD_TYPE}")

# ------------------------------------------------------------------------------
# 4. 目标与库链接 (Target & Libraries)
# ------------------------------------------------------------------------------
add_executable(${CMAKE_PROJECT_NAME})

include(CompileOptions)

# FindModules
# 只有运行了这个，CMake 才会去执行 git clone 云端modules 并配置它的路径
include(FindModules)

# 添加 STM32CubeMX 底层库
add_subdirectory(cmake/stm32cubemx)

# 链接库
target_link_libraries(${CMAKE_PROJECT_NAME}
        stm32cubemx
        # 因为 ems 是通过 FindModules 下载的外部库，不在 User_Code 里。
        # 链接这个库会自动把 ems 的头文件路径加入到工程里。
        ems
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/DSP/Lib/GCC/libarm_cortexM7lfdp_math.a
)

link_libraries("Drivers/CMSIS/DSP/Lib/GCC/libarm_cortexM7lfdp_math.a")

include_directories(
        ./Drivers/CMSIS/Include
        ./Drivers/CMSIS/DSP/Include/
)
link_directories(
        ./Drivers/CMSIS/DSP/Lib/GCC
)

# ==============================================================================
# 5. 源码与头文件管理 (Source & Header Management)
# ==============================================================================

# ------------------------------------------------------------------------------
# A. 头文件管理：全自动模式，扫描 User_Code 下的所有头文件。
# ------------------------------------------------------------------------------
message(NOTICE ">> [Searching All Header] 正在全自动递归扫描 User_Code 下所有头文件路径...")
file(GLOB_RECURSE USER_HEADERS "${CMAKE_SOURCE_DIR}/User_Code/*.h" "${CMAKE_SOURCE_DIR}/User_Code/*.hpp")

foreach (header ${USER_HEADERS})
    get_filename_component(dir ${header} DIRECTORY)  # 获取头文件所在目录
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${dir})  # 添加为私有包含目录
endforeach ()


# ------------------------------------------------------------------------------
# B. 源文件管理：半自动模式 (防止兵种冲突)
#    通用部分全自动扫描，特定兵种部分根据开关选择性扫描。
# ------------------------------------------------------------------------------

message(NOTICE ">> [BSP Source] 正在全自动递归扫描 User_Code/bsp下所有源文件路径...")
message(NOTICE ">> [Module Source] 正在全自动递归扫描 User_Code/module下所有源文件路径...")
message(NOTICE ">> [Application Source] 正在全自动递归扫描 User_Code/application下所有通用APP模块的源文件路径...")

# 1. 添加通用代码 (BSP & Module & Application 通用框架)
#    包含 bsp, module 以及 application 下的通用组件
file(GLOB_RECURSE COMMON_SOURCES
        "${PROJECT_SOURCE_DIR}/User_Code/bsp/*.c"
        "${PROJECT_SOURCE_DIR}/User_Code/bsp/*.cpp"
        "${PROJECT_SOURCE_DIR}/User_Code/module/*.c"
        "${PROJECT_SOURCE_DIR}/User_Code/module/*.cpp"
        # 包含 application 根目录下的 robot.c 等文件 (不递归)
        "${PROJECT_SOURCE_DIR}/User_Code/application/*.c"
        "${PROJECT_SOURCE_DIR}/User_Code/application/*.cpp"
        # 包含 application 下的通用模块 (根据目录结构递归)
        "${PROJECT_SOURCE_DIR}/User_Code/application/chassis_app/*.c" "${PROJECT_SOURCE_DIR}/User_Code/application/chassis_app/*.cpp"
        "${PROJECT_SOURCE_DIR}/User_Code/application/gimbal_app/*.c"  "${PROJECT_SOURCE_DIR}/User_Code/application/gimbal_app/*.cpp"
        "${PROJECT_SOURCE_DIR}/User_Code/application/shoot_app/*.c"   "${PROJECT_SOURCE_DIR}/User_Code/application/shoot_app/*.cpp"
        "${PROJECT_SOURCE_DIR}/User_Code/application/indicator_app/*.c" "${PROJECT_SOURCE_DIR}/User_Code/application/indicator_app/*.cpp"
        "${PROJECT_SOURCE_DIR}/User_Code/application/vision_app/*.c"   "${PROJECT_SOURCE_DIR}/User_Code/application/vision_app/*.cpp"
)
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${COMMON_SOURCES})

list(LENGTH COMMON_SOURCES COMMON_SOURCES_COUNT)

# 输出统计结果
message(NOTICE ">> [General Source] 源文件扫描结束，在通用库中共扫描到 ${COMMON_SOURCES_COUNT} 个源文件")

# 2. 根据开关添加特定兵种代码 (Specific Code)

# 定义一个变量来存储当前激活的 App 路径
set(ACTIVE_APP_DIR "")

# 1. 麦轮步兵
if(INFANTRY_MEC)
    message(WARNING ">> 当前构建兵种: 麦轮步兵 (INFANTRY_MEC)")
    set(ACTIVE_APP_DIR "${PROJECT_SOURCE_DIR}/User_Code/application/infantry_mec")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE INFANTRY_MEC)
    message(NOTICE ">> [Specific Source] 开始扫描各兵种对应代码，当前选中的兵种为: 麦轮步兵 (INFANTRY_MEC)")

    # 2. 半舵步兵
elseif(INFANTRY_HALF_STEER)
    message(NOTICE ">> 当前构建兵种: 半舵步兵 (INFANTRY_HALF_STEER)")
    set(ACTIVE_APP_DIR "${PROJECT_SOURCE_DIR}/User_Code/application/infantry_half_steer")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE INFANTRY_HALF_STEER)
    message(NOTICE ">> [Specific Source] 开始扫描各兵种对应代码，当前选中的兵种为: 半舵步兵 (INFANTRY_HALF_STEER)")

    # 3. 麦轮英雄
elseif(HERO_MEC)
    message(NOTICE ">> 当前构建兵种: 麦轮英雄 (HERO_MEC)")
    set(ACTIVE_APP_DIR "${PROJECT_SOURCE_DIR}/User_Code/application/hero_mec")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE HERO_MEC)
    message(NOTICE ">> [Specific Source] 开始扫描各兵种对应代码，当前选中的兵种为: 麦轮英雄 (HERO_MEC)")

    # 4. 舵轮哨兵
elseif(SENTRY_STEER)
    message(NOTICE ">> 当前构建兵种: 舵轮哨兵 (SENTRY_STEER)")
    set(ACTIVE_APP_DIR "${PROJECT_SOURCE_DIR}/User_Code/application/sentry_steer")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE SENTRY_STEER SENTRY)
    message(NOTICE ">> [Specific Source] 开始扫描各兵种对应代码，当前选中的兵种为: 舵轮哨兵 (SENTRY_STEER)")

    # 5. 变形步兵
elseif(INFANTRY_TRANSFORM)
    message(NOTICE ">> 当前构建兵种: 变形步兵 (INFANTRY_TRANSFORM)")
    set(ACTIVE_APP_DIR "${PROJECT_SOURCE_DIR}/User_Code/application/infantry_transform")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE INFANTRY_TRANSFORM)
    message(NOTICE ">> [Specific Source] 开始扫描各兵种对应代码，当前选中的兵种为: 变形步兵 (INFANTRY_TRANSFORM)")

    # 6. 串联腿平衡步兵
elseif(BALANCE_SERIAL)
    message(NOTICE ">> 当前构建兵种: 串联腿平衡步兵 (BALANCE_SERIAL)")
    set(ACTIVE_APP_DIR "${PROJECT_SOURCE_DIR}/User_Code/application/balance_serial")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE BALANCE_SERIAL BALANCE)
    message(NOTICE ">> [Specific Source] 开始扫描各兵种对应代码，当前选中的兵种为: 串联腿平衡步兵 (BALANCE_SERIAL)")

    # 7. 并联腿平衡步兵
elseif(BALANCE_PARALLEL)
    message(NOTICE ">> 当前构建兵种: 并联腿平衡步兵 (BALANCE_PARALLEL)")
    set(ACTIVE_APP_DIR "${PROJECT_SOURCE_DIR}/User_Code/application/balance_parallel")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE BALANCE_PARALLEL BALANCE)
    message(NOTICE ">> [Specific Source] 开始扫描各兵种对应代码，当前选中的兵种为: 并联腿平衡步兵 (BALANCE_PARALLEL)")

    # 8. 飞镖架
elseif(DART)
    message(NOTICE ">> 当前构建兵种: 飞镖架 (DART)")
    set(ACTIVE_APP_DIR "${PROJECT_SOURCE_DIR}/User_Code/application/dart")
    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE DART)
    message(NOTICE ">> [Specific Source] 开始扫描各兵种对应代码，当前选中的兵种为: 飞镖架 (DART)")

endif()

# message(NOTICE ">> [Application Source] 正在全自动递归扫描 ${ACTIVE_APP_DIR} 下所有源文件路径...")

# 执行特定代码的添加
if(ACTIVE_APP_DIR)
    if(EXISTS "${ACTIVE_APP_DIR}")
        # 添加该兵种目录下的所有源码
        file(GLOB_RECURSE APP_SOURCES "${ACTIVE_APP_DIR}/*.c" "${ACTIVE_APP_DIR}/*.cpp")
        target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${APP_SOURCES})
    else()
        message(FATAL_ERROR ">> [ERROR] 选中的兵种目录不存在: ${ACTIVE_APP_DIR}")
    endif()
endif()

# ------------------------------------------------------------------------------
# 6. 其他设置
# ------------------------------------------------------------------------------

# 其他自定义头文件路径（如果还有不在 User_Code 里的）
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # 可以在此处添加其他包含目录
)

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        # RM_REFEREE
)