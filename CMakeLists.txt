cmake_minimum_required(VERSION 3.22)
include("cmake/gcc-arm-none-eabi.cmake")

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# 条件编译 所有车共用一套代码

# option(BALANCE "Enable balance" ON)
# option(STEER "Enable steer" ON)
option(OMNI "Enable omni" ON)
# option(INFANTRY_OMNI "Enable infantry moni" ON)
# option(HERO "Enable hero" ON)
# option(HERO_STEER "Enable hero_steer" ON)
# option(DART_STAND "Enable dart stand" ON)
# option(ENGINEER "Enable engineer" ON)

# Setup compiler settings
set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g")
elseif (CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS_RELEASE "-O3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
elseif (CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O2 -g")
    set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-O2 -g")
elseif (CMAKE_BUILD_TYPE STREQUAL "MinSizeRel")
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os")
    set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os")
endif ()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -Wno-volatile")

# 定义构建类型
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()
message("Build type: ${CMAKE_BUILD_TYPE}")

# Set the project name
set(CMAKE_PROJECT_NAME TronOneH7_Scaffold)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# 启用对 ASM 和 C 语言的 CMake 支持
enable_language(C CXX ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# 添加子目录
#add_subdirectory(cmake/stm32cubemx)
#add_subdirectory(User_Code/bsp)
#add_subdirectory(User_Code/module)
#if(STEER)
#    add_subdirectory(User_Code/user/steer)
#    message("You set STEER")
#elseif(BALANCE)
#    add_subdirectory(User_Code/user/balance)
#    message("You set BALANCE")
#elseif(OMNI)
#    add_subdirectory(User_Code/user/omni)
#    message("You set OMNI")
#elseif(INFANTRY_OMNI)
#    add_subdirectory(User_Code/user/infantry_omni)
#    message("You set INFANTRY_OMNI")
#elseif(DART_STAND)
#    add_subdirectory(User_Code/user/dart_stand)
#    message("You set DART_STAND")
#elseif(ENGINEER)
#    add_subdirectory(User_Code/user/engineer)
#    message("You set ENGINEER")
#elseif(HERO)
#    add_subdirectory(User_Code/user/hero)
#    message("You set HERO")
#elseif(HERO_STEER)
#    add_subdirectory(User_Code/user/hero_steer)
#    message("You set HERO_STEER")
#endif()

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
        stm32cubemx
        #    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/DSP/Lib/GCC/libarm_cortexM7lfdp_math.a
        # Add user defined libraries
)
#link_libraries("Drivers/CMSIS/DSP/Lib/GCC/libarm_cortexM7lfdp_math.a")
#
## 添加包含路径
#include_directories(
#        ./Drivers/CMSIS/Include
#        ./Drivers/CMSIS/DSP/Include/
#)
## 添加链接目录
#link_directories(
#        ./Drivers/CMSIS/DSP/Lib/GCC
#)
#
## 添加编译选项
#add_compile_options(-pipe -Wall -fmessage-length=0 # basic options
#        -ffunction-sections -fdata-sections -fno-common # optimize options
#)
#add_link_options(-pipe -Wl,--no-warn-rwx-segments # close RWX warning
#        -lc -lstdc++ -lm -lnosys # lib options
#        -Wl,--gc-sections -flto -specs=nano.specs -specs=nosys.specs # optimize options
#)
#
## 添加全局宏定义
#add_definitions(
#        -DUSE_HAL_DRIVER
#        -DARM_MATH_CM7
#        -DARM_MATH_MATRIX_CHECK
#        -DARM_MATH_ROUNDING
#        -DARM_MATH_LOOPUNROLL
#        -DDISABLEFLOAT16
#)
#
## 添加目标编译选项
#target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
#        -mthumb
#        -mthumb-interwork
#        -mfpu=fpv5-d16
#        -mfloat-abi=hard
#)
#
## 忽略 usbd_desc.c 文件中由于缺少字段初始化引起的警告
#set_source_files_properties(
#        ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App/usbd_desc.c
#        PROPERTIES COMPILE_FLAGS -Wno-missing-field-initializers
#)
## 忽略特定文件中的未使用参数警告
#set_source_files_properties(
#        ${CMAKE_CURRENT_SOURCE_DIR}/Core/Src/freertos.c
#        ${CMAKE_CURRENT_SOURCE_DIR}/USB_DEVICE/App/usbd_cdc_if.c
#        PROPERTIES COMPILE_FLAGS -Wno-unused-parameter
#)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        User_Code/bsp/fdcan/bsp_fdcan.c
        User_Code/bsp/spi/bsp_spi.c
        User_Code/bsp/delayticks/delayticks.c
        User_Code/module/periph/ws2812/ws2812.c
        # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        User_Code/bsp/fdcan
        User_Code/bsp/spi
        User_Code/bsp/delayticks
        User_Code/module/periph/ws2812
        User_Code/module/paramdef
        # Add user defined include paths
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        #        RM_REFEREE
        # Add user defined symbols
)

# 添加全局宏定义
#if(OMNI)
#    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
#            SENTRY
#            OMNI
#    )
#elseif(INFANTRY_OMNI)
#    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
#            # SENTRY
#            INFANTRY_OMNI
#    )
#elseif(STEER)
#    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
#            STEER
#    )
#elseif(BALANCE)
#    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
#            BALANCE
#    )
#elseif(HERO)
#    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
#            HERO
#    )
#elseif(HERO_STEER)
#    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
#            HERO_STEER
#    )
#elseif(DART_STAND)
#    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
#            DART_STAND
#    )
#elseif(ENGINEER)
#    target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
#            ENGINEER
#    )
#endif()

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)
