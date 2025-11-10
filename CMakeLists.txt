cmake_minimum_required(VERSION 3.22)
project(TronOneH7_Scaffold)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# 启用编译命令以便于使用例如 clangd 进行索引
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
# 启用对 ASM 和 C 语言的 CMake 支持
enable_language(C CXX ASM)

# 设置编译器选项
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

# 创建可执行对象
add_executable(${CMAKE_PROJECT_NAME})

include(CompileOptions)

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# 添加链接库
target_link_libraries(${CMAKE_PROJECT_NAME}
        stm32cubemx
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        User_Code/bsp/fdcan/bsp_fdcan.c
        User_Code/bsp/spi/bsp_spi.c
        User_Code/bsp/delayticks/delayticks.c
        User_Code/module/periph/ws2812/ws2812.c
        User_Code/application/indicator_app/ws2812status.c
        # Add user sources here
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        User_Code/bsp/fdcan
        User_Code/bsp/spi
        User_Code/bsp/delayticks
        User_Code/module/periph/ws2812
        User_Code/module/paramdef
        User_Code/application/indicator_app
        # Add user defined include paths
)

# 添加全局宏定义
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        #        RM_REFEREE
)