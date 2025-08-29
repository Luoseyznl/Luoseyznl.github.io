---
title: Bazel 学习笔记
date: 2025-08-23
category: 渐入佳境
tags: [Bazel]
---

Bazel 是 Google 开源的构建工具，支持多语言、多平台的构建和测试。它采用声明式的构建规则，基于依赖关系图进行增量构建，具有高效、可扩展和可重复的特点。Bazel 支持分布式构建和缓存，可以显著提升大型项目的构建速度。


> Bazel 的核心概念包括：**工作区（Workspace）**，包含所有源代码和构建文件的目录；**包（Package）**，工作区中的子目录，包含一个 `BUILD` 文件定义构建规则；**目标（Target）**，包中的具体构建单元，如库、可执行文件、测试等。Bazel 使用 `WORKSPACE` 文件定义外部依赖，使用 `BUILD` 文件定义包内的构建规则。

## 目录


## 1. Bazel 安装

```sh
# Ubuntu/Debian
sudo apt install bazel
# macOS (Homebrew)
brew install bazel
# Windows (Chocolatey)
choco install bazel
```

## 2. Bazel 基本使用

在实际工程中，项目既可以**本地编译运行**，也可以通过 CI（持续集成）系统自动拉取代码、编译和运行。需要注意配置文件路径和软链接的处理，保证本地和 CI 环境都能正确访问配置。

### 2.1 创建工作区

1. 在项目根目录创建 `WORKSPACE` 文件，标识这是一个 Bazel 工作区

    ```python
    # 空文件即可标识工作区
    # 如果项目有外部依赖（如 gflags），可用如下方式声明：

    load("@bazel_tools//tools/build_defs/repo:http.bzl", "http_archive")

    http_archive(
        name = "gflags",
        urls = ["https://github.com/gflags/gflags/archive/v2.2.2.tar.gz"],
        strip_prefix = "gflags-2.2.2",
    )
    ```
    > `@bazel_tools//tools/build_defs/repo:http.bzl` 是 Bazel 内置的规则库，提供了 `http_archive` 函数用于下载和解压外部依赖包。

    > `http_archive` 用于从远程下载和解压外部依赖包，并在 Bazel 构建中使用。`name` 是依赖的标识符，`urls` 是下载链接，`strip_prefix` 是解压后的目录前缀。使用时通过 `@gflags//:gflags` 引用（`@` 表示外部依赖，`//` 表示工作区根目录，`:` 表示包内目标）。

### 2.2 创建包和构建规则

1. 在项目根目录创建 `BUILD` 文件，定义构建规则。我们假设项目目录结构如下：

    ```
    my_project/
    ├── WORKSPACE
    ├── BUILD
    ├── config/
    │   └── config.yaml
    ├── include/
    │   └── hello.h
    ├── src/
    │   └── main.cpp
    └── tests/
        └── test_main.cpp
    ```

2. 在 `BUILD` 文件中添加如下内容：

    ```python
    load("@bazel_tools//tools/build_defs/pkg:pkg.bzl", "pkg")

    pkg(
        name = "my_project",
        srcs = glob(["src/*.cpp"]),
        hdrs = glob(["include/*.h"]),
        deps = [
            "@gflags//:gflags",
        ],
    )

    cc_binary(
        name = "main",
        srcs = ["src/main.cpp"],
        deps = [
            ":my_project",
            "@gflags//:gflags",
        ],
    )

    cc_test(
        name = "test_main",
        srcs = ["tests/test_main.cpp"],
        deps = [
            ":my_project",
            "@gflags//:gflags",
        ],
    )
    ```

    > `pkg` 定义一个库目标，包含源文件和头文件，并声明依赖。`cc_binary` 定义一个可执行文件目标，指定源文件和依赖。`cc_test` 定义一个测试目标，指定测试源文件和依赖。`cc_library` 定义一个 C++ 库目标，指定源文件和头文件，以及依赖的其他库。`cc_binary` 和 `cc_test` 目标可以依赖于 `cc_library` 目标，从而实现代码复用。

    > `glob` 用于匹配文件模式，返回符合条件的文件列表。`deps` 列表中可以包含其他目标或外部依赖。

### 2.3 添加配置文件支持

为了确保配置文件在运行时可访问，可以在 `BUILD` 文件中添加如下规则：

```python
filegroup(
    name = "config",
    srcs = ["config/config.yaml"],
)

cc_binary(
    name = "main",
    srcs = ["src/main.cpp"],
    deps = [
        ":my_project",
        ":config",  # 添加配置文件依赖
        "@gflags//:gflags",
    ],
)
```
> `filegroup` 定义一个文件组目标，包含配置文件。将其添加到 `cc_binary` 的 `deps` 中，确保在运行时可以访问配置文件。

在实际工程中，单纯的 filegroup 可能无法在运行时正确找到配置文件路径。我们可以在代码中使用相对路径或通过环境变量指定配置文件路径。为了兼容本地和 CI 环境，还可以在项目根目录下创建指向实际配置文件的软链接。

```
my_project/
├── WORKSPACE
├── BUILD
├── config/
│   └── config.yaml
├── config.yaml   # 指向 config/config.yaml 的软链接
├── include/
│   └── hello.h
├── src/
│   └── main.cpp
└── tests/
    └── test_main.cpp
```
```sh
ln -s config/config.yaml config.yaml
```
