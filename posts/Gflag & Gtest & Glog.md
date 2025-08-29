---
title: Gflag & Gtest & Glog 学习笔记
date: 2025-08-23
category: 渐入佳境
tags: [Gflag, Gtest, Glog]
---

三个 Google 开源的 C++ 库，分别用于命令行参数解析、单元测试和日志记录。

## 目录



## 1. Gflag

Gflag 是 Google 开源的命令行参数解析库，提供了简单易用的 API 来定义和解析命令行 CLI 参数。它支持多种参数类型（如整数、浮点数、字符串等），并且可以自动生成帮助信息。

> 工程实践最好将 Gflag 的参数定义放在一个单独的头文件中，方便管理和维护。、

```sh
find_package(gflags REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE gflags)
target_include_directories(main PRIVATE ${gflags_INCLUDE_DIR})
```

### 1.1 定义参数

```cpp
#include <gflags/gflags.h>

// args 1: 参数名 args 2: 默认值 args 3: 参数描述
DEFINE_int32(port, 8080, "Server port");
DEFINE_string(host, "localhost", "Server host");
DEFINE_bool(verbose, false, "Enable verbose logging");
DEFINE_double(timeout, 30.0, "Request timeout in seconds");
DEFINE_float(threshold, 0.5, "Threshold value");
```

> `DEFINE_*` 宏用于定义参数，参数名、默认值和描述信息。参数名会自动转换为全局变量，可在整个程序中访问（链接性为 `extern`；可见性为 `static`）。Gflag 的参数定义和解析过程是**线程安全的**，适合多线程程序初始化阶段使用。

### 1.2 解析&使用参数

```cpp
#include <gflags/gflags.h>

int main(int argc, char* argv[]) {
    gflags::ParseCommandLineFlags(&argc, &argv, true);
    // 使用参数（全局变量）
    std::cout << "Server running on " << FLAGS_host << ":" << FLAGS_port << std::endl;
    if (FLAGS_verbose) {
        std::cout << "Verbose mode enabled" << std::endl;
    }
    return 0;
}
```

## 2. Gtest

Gtest 是 Google 开源的 C++ 单元测试框架，提供了丰富的断言和测试功能，支持测试用例的组织、运行和结果输出。工程实践中，建议将 Gtest 测试代码单独放在 `test` 目录，并用 CMake 或 Bazel 等构建工具进行统一管理：

- `test` 目录下的 `CMakeLists.txt` 只负责测试目标和 Gtest 依赖的编译；
- `src` 目录下的 `CMakeLists.txt` 只负责业务代码的编译规则；
- 项目根目录的 `CMakeLists.txt` 通过 `add_subdirectory(src)` 和 `add_subdirectory(test)` 引用各子目录，整体结构清晰，便于维护和持续集成。

> 补充：测试代码与业务代码分离，便于管理和自动化测试。推荐使用 `find_package(GTest REQUIRED)` 查找 Gtest 库，并用 `enable_testing()` 集成测试流程。测试用例建议命名规范，方便定位和维护。

```
src/
  CMakeLists.txt
  main.cpp
  utils.cpp
  utils.h
test/
  CMakeLists.txt
  test_utils.cpp
CMakeLists.txt
```

```sh
# 项目根目录 CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(MyProject)

add_subdirectory(src)
add_subdirectory(test)
```

```sh
# src 目录 CMakeLists.txt
add_library(utils utils.cpp utils.h)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE utils gflags)
```

```sh
# test 目录 CMakeLists.txt
find_package(GTest REQUIRED)
enable_testing()

add_executable(test_utils test_utils.cpp)
target_link_libraries(test_utils PRIVATE utils GTest::GTest GTest::Main)

add_test(NAME test_utils COMMAND test_utils)
```

### 2.1 编写测试用例

```cpp
#include <gtest/gtest.h>
#include "utils.h"

TEST(UtilsTest, Add) {
    EXPECT_EQ(add(2, 3), 5);
    EXPECT_EQ(add(-1, 1), 0);
}

TEST(UtilsTest, Subtract) {
    EXPECT_EQ(subtract(5, 3), 2);
    EXPECT_EQ(subtract(3, 5), -2);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

- `TEST` 宏用于定义测试用例，第一个参数是测试套件名，第二个参数是测试名称。
- 断言宏如 `EXPECT_EQ`、`ASSERT_TRUE` 等用于验证测试结果。
- `RUN_ALL_TESTS()` 会运行所有定义的测试用例，并返回测试结果。


## 3. Glog

Glog 是 Google 开源的日志记录库，提供了灵活的日志级别和输出方式，支持将日志输出到控制台、文件等多种目标。它还支持条件日志记录和堆栈跟踪功能。

作为典型的单例模式库，在程序入口处初始化后即可在任何地方使用日志功能。

```cpp
#include <glog/logging.h>

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);
    LOG(INFO) << "Hello, Glog!";
    return 0;
}
```

```sh
# 编译时链接 Glog 库
find_package(Glog REQUIRED)

add_executable(main main.cpp)
target_link_libraries(main PRIVATE Glog::Glog)
```

## 补充：find_package 与 C++链接过程说明

### 1. `find_package` 查找外部库

`find_package` 是 CMake 用于查找和配置外部库的命令。它会在系统中定位指定库，并设置相关变量和目标，方便后续编译和链接。例如：

```cmake
find_package(Glog REQUIRED)
```

查找到 Glog 后，可以通过 `target_link_libraries(main PRIVATE Glog::Glog)` 链接库。这里的 `Glog::Glog` 是 CMake 的**目标名**（target），不是实际文件名。采用“命名空间::库名”方式，CMake 能自动处理依赖和头文件路径，推荐使用。

如果找不到库，可以通过设置 `CMAKE_PREFIX_PATH` 或 `CMAKE_MODULE_PATH` 指定查找路径：

```cmake
set(CMAKE_PREFIX_PATH "/path/to/glog")
find_package(Glog REQUIRED)
```

### 2. C++的编译与链接过程

C++ 编译分为两个阶段：
- **编译阶段**：将源代码（.cpp）编译为目标文件（.o），只检查头文件和语法，不关心库的实现。
- **链接阶段**：将目标文件和库文件（如 .a、.so）合并，解决所有符号（函数、变量）引用，生成可执行文件。

传统链接方式是直接指定库文件名，如 `-lglog`，但这种方式不够灵活，且不支持复杂依赖。现代 CMake 推荐用 `find_package` 和 target 名称，自动处理依赖和路径。

### 3. pkg-config 辅助查找

`pkg-config` 是一个用于管理库编译和链接参数的工具，很多库会提供对应的 `.pc` 文件，方便集成到 CMake 或 Makefile。用法如：

```sh
pkg-config --cflags --libs glog
```

如果库提供 `.pc` 文件，会输出编译和链接参数。也可以在 CMake 中通过 `execute_process` 获取并使用：

```cmake
execute_process(COMMAND pkg-config --cflags --libs glog
                OUTPUT_VARIABLE GLOG_FLAGS
                OUTPUT_STRIP_TRAILING_WHITESPACE)
add_executable(main main.cpp)
target_compile_options(main PRIVATE ${GLOG_FLAGS})
```

> 注意：不是所有库都提供 `.pc` 文件，Google 的 gflags/glog/gtest 等通常推荐用 CMake 的 `find_package` 查找。
