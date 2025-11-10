项目构建说明（CMake）
=====================

本文档概述本仓库使用的 CMake 构建系统、主要配置项，以及对用户最相关的自动包含规则（特别是 `User_Code`
目录中的源文件与头文件）。目标读者是开发者或维护者，希望快速了解如何配置、构建以及把新代码加入工程。

快速概览
--------

- 构建系统：CMake >= 3.22，生成器默认使用 Ninja（参见 `CMakePresets.json`）。
- 预设（presets）：仓库提供 `Debug` 和 `Release` 两个 configure/build preset（见 `CMakePresets.json`）。
- 目标可执行文件名：由顶层 CMake 项目名（`TronOneH7_Scaffold`）创建一个可执行目标。
- 工具链：默认通过 `cmake/starm-clang.cmake` 指定交叉/宿主工具链（在 presets 中通过 `toolchainFile` 引用）。
- 自动包含：`User_Code` 目录下的所有 `.c`、`.cpp` 源文件会被递归搜集并添加到目标；同目录下的 `.h`、`.hpp` 头文件所在目录也会被自动加入目标的
  include 路径。

文件和关键配置点
----------------

- 顶层配置：`CMakeLists.txt`
    - 设置了 C/C++ 标准（C23, C++23）并启用 ASM 支持。
    - 导出 `compile_commands.json` 以便 clangd / IDE 索引（`set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)`）。
    - 根据 `CMAKE_BUILD_TYPE` 设置编译器的优化/调试选项（Debug/Release/RelWithDebInfo/MinSizeRel）。
    - 通过 `add_subdirectory(cmake/stm32cubemx)` 引入由 STM32CubeMX 生成的构建片段（包括启动文件、链接脚本、HAL 库设置等）。
    - 将 `stm32cubemx` 静态/目标库和 `rpl::rpl`（项目中的第三方或子模块库）链接到可执行目标。

- 预设：`CMakePresets.json`
    - `default` preset 指明生成器（Ninja）、二进制输出目录（`build/${presetName}`）与工具链文件。
    - `Debug`/`Release` preset 分别继承 `default` 并设置 `CMAKE_BUILD_TYPE`。
    - 构建命令示例（在仓库根目录运行）：

```sh
cmake --preset Debug
cmake --build --preset Debug
```

（Windows 下的 shell 为 cmd.exe，上面命令同样适用；若使用 VS Developer Prompt 或 PowerShell，可在相应环境中运行）

User_Code 的自动包含行为
----------------------
顶层 `CMakeLists.txt` 中有两段与 `User_Code` 有关的逻辑：

1. 自动收集源文件：

```cmake
file(GLOB_RECURSE USER_SOURCES "${PROJECT_SOURCE_DIR}/User_Code/*.c" "${PROJECT_SOURCE_DIR}/User_Code/*.cpp")

target_sources(${CMAKE_PROJECT_NAME} PRIVATE
        ${USER_SOURCES}
)
```

该逻辑会递归查找 `User_Code` 下所有 `.c` 和 `.cpp` 文件，并把它们加入到可执行目标中。开发者只要把源文件放在 `User_Code`
子目录（或其子目录）下，CMake 在下一次 configure 时会将其纳入构建。

2. 自动加入头文件目录：

```cmake
file(GLOB_RECURSE USER_HEADERS "${CMAKE_SOURCE_DIR}/User_Code/*.h" "${CMAKE_SOURCE_DIR}/User_Code/*.hpp")

foreach (header ${USER_HEADERS})
    get_filename_component(dir ${header} DIRECTORY)
    target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${dir})
endforeach ()
```

这个片段会递归查找 `User_Code` 下的 `.h`/`.hpp` 文件，并将每个头文件所在的目录添加为目标的私有 include 目录。换句话说：

- 如果你的头文件位于 `User_Code/my_module/include/mymod.h`，该 `include` 目录会被自动添加到编译器的 include 路径中。
- 如果多个源/头文件位于同一目录，该目录只会被多次添加（可接受，但可通过改进避免重复，如需要我可以帮忙优化）。

如何添加/组织代码（建议）
------------------------

- 源文件：把 `.c`/`.cpp` 放在 `User_Code/<subdir>/` 下。CMake 会自动发现并编译。
- 头文件：把 `.h`/`.hpp` 放在与源文件同目录或子目录的 `include/` 目录中，CMake 会将头文件所在目录纳入 include 路径。
- 如果你想限制某些文件不被自动编译，可以：
    - 改名（例如添加后缀 `.inert`）或者
    - 将需要排除的文件放到项目外，或修改 `CMakeLists.txt` 来有选择性地添加源文件（我可以帮助你实现更精细的控制）。

拓展说明（链接库、工具链、CubeMX）
-----------------------------

- STM32CubeMX：`add_subdirectory(cmake/stm32cubemx)` 会把生成的 HAL/启动/链接脚本等加入工程，相关源通常在
  `cmake/stm32cubemx` 子项目中管理。
- 工具链：`CMakePresets.json` 中 `default` preset 指向 `cmake/starm-clang.cmake`。如果你使用不同编译器或调试器，请修改对应
  preset 的 `toolchainFile` 或在命令行提供 `-DCMAKE_TOOLCHAIN_FILE=...`。
- 第三方库：示例中 `rpl::rpl` 被链接到目标上；如果需要新增库，请在 `FindModules.cmake` 或 `cmake/Modules` 中添加查找逻辑，或直接使用
  `add_subdirectory` 引入并 `target_link_libraries`。

诊断与调试
---------

- 若 clangd / IDE 没有正确索引，请确认 `build/Debug/compile_commands.json`（或对应 preset 的 build 目录）存在。若不存在，请使用
  `cmake --preset Debug` 重新 configure。
- 若新加入的源文件未被编译：
    - 确认文件扩展名是 `.c` / `.cpp` 并放在 `User_Code` 子目录下；
    - 重新运行 `cmake --preset <preset>` 以刷新 CMake cache（Ninja incremental build 不会改变 configure 阶段的 glob 结果）；
    - 如果希望不依赖 glob，可以手动在 `CMakeLists.txt` 里使用 `target_sources` 显式列出文件列表。
