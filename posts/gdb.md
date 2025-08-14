---
title: GDB 学习笔记
date: 2025-08-14
category: 渐入佳境
tags: [GDB, 调试, 开发]
---

## 目录


## 1. GDB 简介

GDB（GNU Debugger）是一个调试工具，允许开发者在程序运行时检查变量、控制执行流程、设置断点等。

### 1.1 安装

```bash
sudo apt-get install gdb  # Debian/Ubuntu 系统
sudo yum install gdb      # CentOS/RHEL 系统
```

### 1.2 启动 GDB

```bash
gdb ./your_program  # 启动 GDB 并加载可执行文件
```

### 1.3 基本命令

- `run`：运行程序。
- `break <line>`：在指定行设置断点。
- `next`：执行下一行代码，不进入函数。
- `step`：执行下一行代码，进入函数。
- `continue`：继续执行程序，直到下一个断点。
- `print <variable>`：打印变量的值。
- `backtrace`：显示函数调用栈。
- `info locals`：显示当前函数的所有局部变量。
