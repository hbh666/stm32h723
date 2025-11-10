if(NOT CMAKE_TOOLCHAIN_FILE)
    # 检查 starm-clang 是否可用
    find_program(STARM_CLANG starm-clang)

    if(STARM_CLANG)
        set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Toolchain/starm-clang.cmake)
        message(STATUS "已找到 starm-clang, 使用 starm-clang 工具链")
    else()
        set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/Toolchain/gcc-arm-none-eabi.cmake)
        message(STATUS "未找到 starm-clang 工具链, 使用 gcc-arm-none-eabi 工具链")
    endif()
endif()