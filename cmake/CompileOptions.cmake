add_compile_options(
        -pipe -Wall -fmessage-length=0 # basic options
        -ffunction-sections -fdata-sections -fno-common # optimize options
)
add_link_options(
        -pipe -Wl,--no-warn-rwx-segments # close RWX warning
        -lc -lstdc++ -lm -lnosys # lib options
        -Wl,--gc-sections -flto -specs=nano.specs -specs=nosys.specs # optimize options
)

# 添加宏定义
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
        -DUSE_HAL_DRIVER
        -DARM_MATH_CM7
        -DARM_MATH_MATRIX_CHECK
        -DARM_MATH_ROUNDING
        -DARM_MATH_LOOPUNROLL
        -DDISABLEFLOAT16
)
# 添加目标编译选项
target_compile_options(${CMAKE_PROJECT_NAME} PRIVATE
        -mthumb
        #        -mthumb-interwork
        -mfpu=fpv5-d16
        -mfloat-abi=hard
)