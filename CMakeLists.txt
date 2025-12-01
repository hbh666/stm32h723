# 设置CMake最低版本要求为3.22
cmake_minimum_required(VERSION 3.22)

# 包含自定义的工具链检测脚本
include(cmake/DetectToolchain.cmake)

# 定义项目名称
project(TronOneH7_Scaffold)

# 将项目目录下的cmake文件夹添加到模块路径中
list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

# 启用编译命令导出，便于clangd等工具进行代码索引
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
# 启用对C、C++和汇编语言的支持
enable_language(C CXX ASM)

# 设置C语言标准为C23
set(CMAKE_C_STANDARD 23)
# 要求必须支持C23标准
set(CMAKE_C_STANDARD_REQUIRED ON)
# 启用GNU扩展（GCC/Clang特有的扩展功能）
set(CMAKE_C_EXTENSIONS ON)

# 设置C++语言标准为C++23
set(CMAKE_CXX_STANDARD 23)
# 要求必须支持C++23标准
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 启用GNU扩展
set(CMAKE_CXX_EXTENSIONS ON)

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

# 设置C++特定编译选项：启用RTTI，禁用volatile警告
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -frtti -Wno-volatile")

## 最严格的一集，Warning也是Error！
#set(CMAKE_CXX_FLAGS "$ENV{CXX_FLAGS} -Werror")  # 设置 C++ 编译选项
#set(CMAKE_C_FLAGS "$ENV{C_FLAGS} -Werror")      # 设置 C 编译选项

# 如果没有指定构建类型，默认为Debug模式
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif ()
# 打印当前构建类型
message("Build type: ${CMAKE_BUILD_TYPE}")

# 创建可执行文件目标
add_executable(${CMAKE_PROJECT_NAME})

# 包含编译选项配置文件
include(CompileOptions)
# 包含查找模块（当前被注释掉）
#include(FindModules)

# 添加STM32CubeMX生成的源码子目录
add_subdirectory(cmake/stm32cubemx)

# 为目标添加链接库
target_link_libraries(${CMAKE_PROJECT_NAME}
        stm32cubemx  # STM32CubeMX生成的库
        #        rpl::rpl     # 第三方库（当前被注释掉）
        ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/DSP/Lib/GCC/libarm_cortexM7lfdp_math.a
)

link_libraries("Drivers/CMSIS/DSP/Lib/GCC/libarm_cortexM7lfdp_math.a")
# 添加包含路径
include_directories(
        ./Drivers/CMSIS/Include
        ./Drivers/CMSIS/DSP/Include/
)
# 添加链接目录
link_directories(
        ./Drivers/CMSIS/DSP/Lib/GCC
)

# 递归查找User_Code目录下的所有C和C++源文件
file(GLOB_RECURSE USER_SOURCES "${PROJECT_SOURCE_DIR}/User_Code/*.c" "${PROJECT_SOURCE_DIR}/User_Code/*.cpp")

# 将用户源文件添加到可执行目标中
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${USER_SOURCES}
        # 可以在此处添加其他用户源文件
)

# 递归查找用户头文件
file(GLOB_RECURSE USER_HEADERS "${CMAKE_SOURCE_DIR}/User_Code/*.h" "${CMAKE_SOURCE_DIR}/User_Code/*.hpp")

# 为每个头文件所在目录添加包含路径
foreach (header ${USER_HEADERS})
    get_filename_component(dir ${header} DIRECTORY)  # 获取头文件所在目录
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${dir})  # 添加为私有包含目录
endforeach ()

# 添加其他包含路径（当前为空）
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
        # 可以在此处添加其他包含目录
)

# 添加全局宏定义
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        #        RM_REFEREE  # 裁判系统宏定义（当前被注释掉）
)