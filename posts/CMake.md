---
title: CMake 学习笔记
date: 2025-08-27
category: 渐入佳境
tags: [CMake, 构建工具, 学习笔记]
---

CMake 是一个跨平台的开源构建系统，主要用于管理 C/C++ 项目的编译过程。通过编写 `CMakeLists.txt` 文件，开发者可以定义项目的源文件、依赖库、编译选项等。CMake 会根据这些配置生成适合当前平台和编译器的构建文件（如 Makefile、Visual Studio 工程文件等），然后使用相应的构建工具进行编译。

## 目录


## 1. CMake 基础

### 1.1 CMake 语法糖

- `cmake_minimum_required(VERSION 3.10)`：指定所需的最低 CMake 版本。
- `project(MyProject VERSION 1.0 LANGUAGES CXX)`：定义项目名称、版本和使用的编程语言。
- `set(CMAKE_CXX_STANDARD 11)`：设置 C++ 标准为 C++11。
- `add_executable(MyApp main.cpp)`：定义一个可执行文件目标 `MyApp`，包含源文件 `main.cpp`。
- `add_library(MyLib STATIC lib.cpp)`：定义一个静态库目标 `MyLib`，包含源文件 `lib.cpp`。
- `target_link_libraries(MyApp PRIVATE MyLib)`：将库 `MyLib` 链接到可执行文件 `MyApp`，链接方式为私有（PRIVATE）。
- `include_directories(${CMAKE_SOURCE_DIR}/include)`：添加头文件搜索路径。
- `find_package(OpenGL REQUIRED)`：查找并引入 OpenGL 库，如果未找到则报错。
- `if(WIN32) ... elseif(UNIX) ... endif()`：根据操作系统条件执行不同的配置。
- `message(STATUS "Configuring for ${CMAKE_SYSTEM_NAME}")`：输出状态信息。
- `install(TARGETS MyApp DESTINATION bin)`：安装可执行文件到指定目录。
- `add_subdirectory(subdir)`：添加子目录，递归处理该目录下的 `CMakeLists.txt` 文件。
- `option(BUILD_TESTS "Build the tests" ON)`：定义一个选项变量 `BUILD_TESTS`，默认值为 ON。
- `set(SOURCES main.cpp lib.cpp)`：定义一个变量 `SOURCES`，包含多个源文件。
- `foreach(src ${SOURCES}) ... endforeach()`：遍历变量 `SOURCES` 中的每个元素，执行循环体内的命令。
- `configure_file(config.h.in config.h)`：根据模板文件 `config.h.in` 生成配置头文件 `config.h`，替换其中的变量。

### 1.2 常用命令

- `cmake .`：在当前目录生成 Makefile 或其他构建文件。
- `cmake --build .`：使用生成的构建文件编译项目。
- `cmake --install .`：安装编译生成的目标文件。
- `cmake --version`：查看 CMake 版本。
- `cmake --help`：查看 CMake 帮助信息。
- `cmake --find-package -DNAME=OpenGL -DCOMPILER_ID=GNU -DLANGUAGE=CXX -DMODE=EXIST`：检查是否能找到 OpenGL 包。
- `cmake -S . -B build`：指定源目录和构建目录。
- `cmake --build build --config Release`：在指定的构建目录中以 Release 配置编译项目。
- `cmake --build build --target clean`：清理构建目录。
- `cmake --build build --target MyApp`：编译指定的目标 `MyApp`。
- `cmake -DCMAKE_BUILD_TYPE=Debug .`：设置构建类型为 Debug。
- `cmake -DCMAKE_PREFIX_PATH=/path/to/deps .`：指定依赖库的搜索路径。
- `cmake -G "Unix Makefiles" .`：指定生成器为 Unix Makefiles。
- `cmake -G "Visual Studio 16 2019" .`：指定生成器为 Visual Studio 2019。
- `cmake -P script.cmake`：执行一个 CMake 脚本文件。
- `cmake --trace .`：启用跟踪模式，输出详细的执行过程。
- `cmake --build . -- -j4`：使用 4 个并行线程编译项目（适用于 Makefile 生成器）。

### 1.3 CMake 工作流程

1. 编写 `CMakeLists.txt` 文件，定义项目结构、源文件、依赖等。
2. 运行 `cmake` 命令生成构建文件（如 Makefile、Visual Studio 工程文件等）。
3. 使用生成的构建文件编译项目，生成可执行文件或库文件。
4. （可选）运行 `cmake --install` 命令安装生成的目标文件到指定目录。

### 1.4 CMake 构建类型

CMake 支持多种构建类型，常见的有：
- `Debug`：包含调试信息，未进行优化，适合调试阶段。
- `Release`：进行优化，去除调试信息，适合发布阶段。
- `RelWithDebInfo`：在 Release 基础上保留调试信息，适合调试发布版本。
- `MinSizeRel`：优化生成的二进制文件大小，适合嵌入式系统。

可以通过设置 `CMAKE_BUILD_TYPE` 变量来指定构建类型，例如：

```sh
cmake -DCMAKE_BUILD_TYPE=Release .
```

### 1.5 CMake 查找包

CMake 提供了 `find_package` 命令用于查找和引入外部库或包。常见的用法有：

```cmake
find_package(OpenGL REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Boost 1.65 REQUIRED COMPONENTS filesystem system)
```

找到包后，可以使用 `${OpenGL_LIBRARIES}`、`${SDL2_LIBRARIES}` 等变量链接库，使用 `${OpenGL_INCLUDE_DIRS}`、`${SDL2_INCLUDE_DIRS}` 等变量添加头文件路径。

## 2. Linux 平台编译原理

Linux 属于类 Unix 系统，使用 GCC 作为主要的编译器（也可以使用 Clang）。GNU Compiler Collection 支持多种编程语言（如 C、C++、Fortran 等）。GCC 的编译过程主要包括预处理、编译、汇编和链接四个阶段。CMake 在 Linux 平台上通常生成 Makefile，然后使用 `make` 命令调用 GCC 进行编译。


项目的三方库有源码依赖和二进制依赖两种方式：

1. 源码依赖：将第三方库的源代码直接包含在项目中，通常放在 `external` 或 `third_party` 目录下。CMake 可以通过 `add_subdirectory` 命令将这些库作为子目录添加到项目中。

2. 二进制依赖：使用系统或预编译的二进制库，通常通过包管理器安装（如 apt、brew 等）。CMake 可以通过 `find_package` 命令查找这些库，并链接到项目中。

> 二进制依赖的三方库通常安装在系统的标准路径（如 `/usr/lib`、`/usr/local/lib`）或自定义路径。CMake 也可以通过 `CMAKE_PREFIX_PATH` 变量指定额外的搜索路径。库名通常以 `lib` 开头，后缀为 `.so`（共享库）或 `.a`（静态库）。链接时使用 `-l` 选项指定库名（去掉 `lib` 前缀和后缀），使用 `-L` 选项指定库路径。比如 `gcc -o myapp main.o -L/usr/local/lib -lmylib`。可以通过 `ldd` 命令查看可执行文件依赖的共享库；通过 `nm` 命令查看库文件中的符号信息。比如 `nm -D /usr/local/lib/libmylib.so`。符号信息的结构为 `<地址> <类型> <符号名>@<版本>`。

比如在 Linux 上使用 CMake 构建一个音视频项目，主要涉及：FFmpeg（音视频编解码）、SDL2（音视频播放）、OpenGL（视频渲染）等库。CMake 会查找这些库的头文件和二进制文件，并生成相应的编译和链接命令。我们以 FFmpeg 为例，CMake 会执行类似以下命令：

```sh
gcc -I/usr/local/include -c main.cpp -o main.o
gcc -o myapp main.o -L/usr/local/lib -lavformat -lavcodec -lavutil -lswscale -lswresample -lpthread
```

我们可以在 CMakeLists.txt 中同时提供对 FFmpeg 的源码依赖和二进制依赖两种方式：

```cmake
set(FFMPEG_INCLUDE_DIR "")
set(FFMPEG_LIBRARY_DIR "")
set(FFMPEG_LIBRARIES "")

# 1. 提供源码 / 二进制依赖选项
option(USE_INTERNAL_FFMPEG "Build FFmpeg from third_party/ffmpeg (if ON)." OFF)

# 2. 根据选项选择依赖方式（源码依赖方式）
if(USE_INTERNAL_FFMPEG)
    set(FFMPEG_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ffmpeg)
    set(FFMPEG_INSTALL_DIR ${CMAKE_INSTALL_PREFIX})

    # 设置 FFmpeg 编译选项（./configure 脚本参数）
    set(FFMPEG_CONFIGURE_OPTIONS
        --prefix=${FFMPEG_INSTALL_DIR}  # 安装路径
        --disable-doc                   # 关闭文档生成
        --disable-programs              # 关闭程序生成（命令行工具）
        --enable-shared                 # 启用共享库
    )

    # 启用 ExternalProject 模块（自动管理源码依赖项目的下载、构建目录、配置命令、编译命令、安装命令等）
    include(ExternalProject) # 引入 ExternalProject 模块
    ExternalProject_Add(ffmpeg
        SOURCE_DIR ${FFMPEG_SOURCE_DIR}         # 源码目录
        BINARY_DIR ${FFMPEG_SOURCE_DIR}/build   # 构建目录
        CONFIGURE_COMMAND ${FFMPEG_SOURCE_DIR}/configure ${FFMPEG_CONFIGURE_OPTIONS} # 配置命令
        BUILD_COMMAND make -j${CMAKE_BUILD_PARALLEL_LEVEL} # 编译命令
        INSTALL_COMMAND make install            # 安装命令
        LOG_DOWNLOAD ON                         # 启用日志输出（下载、配置、编译、安装）
        LOG_CONFIGURE ON
        LOG_BUILD ON
        LOG_INSTALL ON
        BUILD_IN_SOURCE 0                       # 在 BINARY_DIR 中构建
    )

    # 设置 FFmpeg 头文件和库文件路径方便后续使用
    set(FFMPEG_INCLUDE_DIR ${FFMPEG_INSTALL_DIR}/include)
    set(FFMPEG_LIBRARY_DIR ${FFMPEG_INSTALL_DIR}/lib)
    set(FFMPEG_LIBRARIES
        ${FFMPEG_LIBRARY_DIR}/libavcodec.so
        ${FFMPEG_LIBRARY_DIR}/libavformat.so
        ${FFMPEG_LIBRARY_DIR}/libavutil.so
        ${FFMPEG_LIBRARY_DIR}/libswscale.so
        ${FFMPEG_LIBRARY_DIR}/libavdevice.so
        ${FFMPEG_LIBRARY_DIR}/libswresample.so
    )
# 3. 二进制依赖方式
else()
    find_package(PkgConfig REQUIRED)                    # 查找 PkgConfig 工具（获取已安装库的头文件、库文件路径、编译选项等）
    if(PkgConfig_FOUND)
        pkg_check_modules(AVFORMAT_PKG QUIET libavformat)
        pkg_check_modules(AVCODEC_PKG QUIET libavcodec)
        pkg_check_modules(AVUTIL_PKG QUIET libavutil)
        pkg_check_modules(SWRESAMPLE_PKG QUIET libswresample)
    endif()

    if(AVFORMAT_PKG_FOUND AND AVCODEC_PKG_FOUND AND AVUTIL_PKG_FOUND AND SWRESAMPLE_PKG_FOUND)
        message(STATUS "Using system FFmpeg via pkg-config")
        # 追加头文件和库文件路径
        list(APPEND FFMPEG_INCLUDE_DIR ${AVFORMAT_PKG_INCLUDE_DIRS} ${AVCODEC_PKG_INCLUDE_DIRS} ${AVUTIL_PKG_INCLUDE_DIRS} ${SWRESAMPLE_PKG_INCLUDE_DIRS})
        list(APPEND FFMPEG_LIBRARY_DIR ${AVFORMAT_PKG_LIBRARY_DIRS} ${AVCODEC_PKG_LIBRARY_DIRS} ${AVUTIL_PKG_LIBRARY_DIRS} ${SWRESAMPLE_PKG_LIBRARY_DIRS})
        list(APPEND FFMPEG_LIBRARIES ${AVFORMAT_PKG_LIBRARIES} ${AVCODEC_PKG_LIBRARIES} ${AVUTIL_PKG_LIBRARIES} ${SWRESAMPLE_PKG_LIBRARIES})
        # 添加库文件搜索路径
        link_directories(${AVFORMAT_PKG_LIBRARY_DIRS} ${AVCODEC_PKG_LIBRARY_DIRS} ${AVUTIL_PKG_LIBRARY_DIRS} ${SWRESAMPLE_PKG_LIBRARY_DIRS})
    else()
        message(STATUS "System FFmpeg not found or incomplete; falling back to internal build")
        set(USE_INTERNAL_FFMPEG ON)
    endif()
endif()

# 4. 添加头文件搜索路径
if(FFMPEG_INCLUDE_DIR)
    list(REMOVE_DUPLICATES FFMPEG_INCLUDE_DIR) # 去重
    include_directories(${FFMPEG_INCLUDE_DIR})
endif()

# 5. 指定项目目标的输出目录和运行时搜索路径
if(FFMPEG_LIBRARY_DIR)
    set_target_properties(${PROJECT_NAME} PROPERTIES                # 设置目标属性
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib    # 1. 动态库输出目录（build/lib）
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin    # 2. 可执行文件输出目录（build/bin）
        BUILD_RPATH "$ORIGIN/../lib;${FFMPEG_LIBRARY_DIR}"          # 3. 构建时运行时搜索路径（相对于可执行文件目录的相对路径）
        INSTALL_RPATH "$ORIGIN/../lib;${FFMPEG_LIBRARY_DIR}"
    )
else()
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/lib
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/bin
    )
endif()
```

## 3. Windows 平台编译原理

Windows 平台通常使用 MSVC (Microsoft Visual C++) 作为主要的编译器（或是 MinGW）。MSVC 是微软官方的编译器，集成在 Visual Studio IDE 中，使用微软自己的运行库和工具链，生成的程序依赖 Windows 原生 DLL 文件(如 `msvcp140.dll`、`vcruntime140.dll` 等)。MinGW 是一个开源的 GCC 编译器集合，提供类似于 Linux 的编译环境，生成的程序依赖 MinGW 提供的 DLL 文件（如 `libgcc_s_dw2-1.dll`、`libstdc++-6.dll` 等）。

> MSVC 属于闭源商业编译器（微软生态），MinGW 属于开源免费编译器（GNU 生态）。MSVC 的编译速度较快，优化能力强，支持最新的 C++ 标准和 Windows 特性，但生成的二进制文件通常较大。MinGW 的编译速度相对较慢，优化能力一般，但生成的二进制文件较小，且更易于移植到其他平台。

CMake 在 Windows 上既可以用 MSVC 也可以用 MinGW 作为编译器。CMake 会根据所选的生成器（如 Visual Studio、MinGW Makefiles 等）生成相应的构建文件，然后使用对应的编译器进行编译。比如：

```sh
# 使用 MinGW 编译
cmake -G "MinGW Makefiles" .
cmake --build . --config Release

# 使用 MSVC 编译
cmake -G "Visual Studio 16 2019" .
cmake --build . --config Release
```

windows 平台的三方库同样有源码依赖和二进制依赖两种方式。通常通过包管理器（如 vcpkg、Conan 等）安装预编译的二进制库，CMake 可以通过 `find_package` 命令查找这些库，并链接到项目中。所以 CMakeLists.txt 中对三方库的处理方式与 Linux 平台类似。仅需注意 Windows 上的库文件后缀通常为 `.lib`（静态库）和 `.dll`（动态库），链接时使用 `-l` 选项指定库名（去掉 `lib` 前缀和后缀），使用 `-L` 选项指定库路径。比如 `cl /Fe:myapp.exe main.obj /link /LIBPATH:C:\libs mylib.lib`。此外，Windows 上的路径分隔符为反斜杠 `\`，而 Linux 上为正斜杠 `/`。CMake 会自动处理这些差异，但在编写路径时建议使用正斜杠以提高跨平台兼容性。Windows 上的环境变量通常使用分号 `;` 作为分隔符，而 Linux 上使用冒号 `:`。`ExternalProject` 模块在 Windows 上同样适用，但要注意配置命令和编译命令可能与 Linux 不同。比如使用 `nmake` 或 `msbuild` 代替 `make`。
