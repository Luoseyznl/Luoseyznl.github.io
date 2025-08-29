---
title: HTML & JavaScript & CSS & Node.js 学习笔记
date: 2025-08-23
category: 渐入佳境
tags: [HTML, JavaScript, CSS, Node.js]
---

HTML（超文本标记语言）是构建网页的基础，负责网页的结构和内容。JavaScript 是一种脚本语言，用于为网页添加交互和动态效果。CSS（层叠样式表）用于控制网页的外观和布局。Node.js 是一个基于 Chrome V8 引擎的 JavaScript 运行时，用于构建高性能的网络应用。

> HTML、CSS 和 JavaScript 是前端开发的三大核心技术，Node.js 则是后端开发的重要工具。此外还有比如 React、Vue.js 等前端框架，以及 Express、Koa 等 Node.js 框架，进一步提升了开发效率和应用性能。

## 目录

## 1. HTML 基础

HTML 使用标签（Tag）来定义文档的结构。常见的标签包括：

- `<!DOCTYPE html>`：声明文档类型
- `<html>`：根元素
- `<head>`：文档头部，包含元数据
- `<title>`：文档标题
- `<body>`：文档主体，包含可见内容

### 1.1 常用标签

- `<h1>` - `<h6>`：标题
- `<p>`：段落
- `<a>`：链接
- `<img>`：图片
- `<div>`：块级元素
- `<span>`：行内元素

## 2. JavaScript 基础

JavaScript 是一种动态类型的语言，支持面向对象和函数式编程。常用的 JavaScript 特性包括：

- 变量声明：`var`、`let`、`const`
- 数据类型：`Number`、`String`、`Boolean`、`Object`、`Array`
- 函数：普通函数、箭头函数
- 控制结构：`if`、`for`、`while`、`switch`

### 2.1 DOM 操作

JavaScript 可以通过 DOM（文档对象模型）操作 HTML 元素。常用的方法包括：

- `document.getElementById()`：根据 ID 获取元素
- `document.getElementsByClassName()`：根据类名获取元素
- `document.querySelector()`：根据 CSS 选择器获取元素
- `element.addEventListener()`：为元素添加事件监听器

## 3. CSS 基础

CSS 用于控制网页的外观和布局。常用的 CSS 属性包括：

- 颜色：`color`、`background-color`
- 字体：`font-size`、`font-family`
- 布局：`margin`、`padding`、`display`
- 选择器：`class`、`id`、`element`

### 3.1 响应式设计

响应式设计是指网页能够根据不同设备的屏幕大小和分辨率自动调整布局。常用的技术包括：

- 媒体查询（Media Query）
- 弹性盒子（Flexbox）
- 网格布局（Grid Layout）

## 4. Node.js 基础

Node.js 是一个基于 Chrome V8 引擎的 JavaScript 运行时，用于构建高性能的网络应用。常用的 Node.js 特性包括：

- 非阻塞 I/O
- 事件驱动架构
- 模块化开发

### 4.1 常用模块

- `http`：用于创建 HTTP 服务器
- `fs`：用于文件系统操作
- `path`：用于路径操作
- `express`：用于构建 Web 应用
