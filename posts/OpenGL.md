---
title: OpenGL 学习笔记
date: 2025-08-24
tags: [OpenGL, 计算机图形学]
---

OpenGL 是一个 跨平台、跨语言的图形 API（Application Programming Interface），主要用于 2D/3D 图形渲染。可以支持 Windows, Linux, macOS 等多个操作系统，由 GPU（图形处理单元）执行图形渲染任务（硬件加速）。
OpenGL 只负责渲染，不负责窗口管理（通常与 GLFW, GLUT, SDL 等库结合使用）。

## 目录


## 0. 图形学预备知识

### 1. **什么是渲染？**

渲染是将场景中的几何数据（顶点、纹理、光照等）转换为图像的过程。包括顶点处理、图元装配、光栅化、片段处理等多个阶段。
现实中的画面非常复杂，计算机需要把三维世界的信息一步步转换成二维屏幕上的像素。每一步都负责特定的任务：

1. **顶点处理**：把物体的几何信息（点、线、面）发送给 GPU
2. **坐标变换**：将物体从模型空间转换到世界空间、视图空间，最后投影到裁剪空间
3. **图元装配**：把点、线、面等基本图元组合成可渲染的基本单元（如三角形）
4. **光栅化**：把连续的图元转换为离散的片段（Fragment），对应屏幕上的像素点
5. **片段处理**：计算每个片段的颜色，进行纹理映射、光照计算等
6. **测试与混合**：对片段进行深度测试、模板测试等，决定是否将其写入帧缓冲，并进行颜色混合

> 坐标变换通过 MVP（Model-View-Projection）矩阵实现。本质是线性代数中的矩阵乘法，将顶点坐标从一个空间映射到另一个空间。
> 图元（Primitive）是图形的基本构建单元，如点（GL_POINTS）、线（GL_LINES）、三角形（GL_TRIANGLES）等。包含顶点数据和属性（颜色、纹理坐标等）。
> 光栅化阶段将这些连续的图元转换为离散的片段（Fragment），经过覆盖测试、插值计算、纹理采样的步骤后得到离散的像素数据。每个片段对应屏幕上的一个像素，包含颜色、深度等信息。
> 片段着色器（Fragment Shader）对每个片段进行处理，决定最终显示的颜色。可以实现纹理映射、光照计算和各种特效。

## 1. OpenGL 基础概念

### 1.1 图形渲染管线（Graphics Pipeline）

1. CPU 生成顶点数据（顶点坐标、颜色、纹理坐标等）。通过 VBO（Vertex Buffer Object）传输到 GPU。

2. 顶点着色器（Vertex Shader）处理顶点数据，进行坐标变换（模型、视图、投影矩阵）。

3. 图元装配（Primitive Assembly）将顶点组装成图元（点 GL_POINTS、线 GL_LINES、三角形 GL_TRIANGLES 等）。

4. 光栅化（Rasterization）将连续的图元转换为离散的片段（Fragment），对应屏幕上的像素。

5. 片段着色器（Fragment Shader）计算每个片段的最终颜色，进行纹理映射、光照计算等。写入帧缓冲（Frame Buffer）。

6. 测试与混合（Testing and Blending）对片段进行深度测试、模板测试等，决定是否将其写入帧缓冲，并进行颜色混合。

> 在渲染开始前，CPU 负责准备场景的几何数据，比如顶点坐标、颜色、纹理坐标等。这些数据通常以数组或结构体的形式存储在内存中。为了让 GPU 能够高效地访问这些数据，OpenGL 提供了顶点缓冲对象（VBO），用于将数据从 CPU 传输到 GPU 的显存。

> OpenGL 的着色器使用专为 GPU 并行计算设计的 GLSL 语言编写。着色器源码会在运行时以字符串形式传递给 OpenGL 驱动，由驱动编译为 GPU 可执行的代码。顶点着色器（Vertex Shader）是运行在 GPU 上的小程序，负责处理每个顶点，包括坐标变换（模型、视图、投影矩阵）、法线变换、纹理坐标传递等。

> GPU 会根据顶点数据和指定的绘制方式（如 GL_TRIANGLES、GL_LINES），将顶点组装成基本图元。光栅化阶段则将连续的图元（如三角形）转换为离散的片段（Fragment），每个片段对应屏幕上的一个像素。光栅化决定了哪些像素被覆盖，以及如何插值顶点属性（如颜色、纹理坐标）。片段着色器（Fragment Shader）对每个片段进行处理，决定最终显示的颜色，可以实现纹理映射、光照计算和各种特效。

### 1.2 基本对象

- **顶点缓冲对象（VBO）**：存储顶点数据的缓冲区对象，用于将顶点数据传输到 GPU。
  - VBO 顶点缓冲对象；用于存储顶点属性（位置、法线、纹理坐标等）的数据。
  - EBO 元素缓冲对象（索引缓冲对象）；用于存储顶点索引数据，减少重复顶点。
  - FBO 帧缓冲对象；用于离屏渲染，存储颜色、深度、模板等信息。
- **顶点数组对象（VAO）**：存储顶点属性配置的对象，绑定 VAO 后，后续的顶点属性配置都会存储在该 VAO 中。*
- **纹理对象（Texture Object）**：用于存储纹理数据的对象，可以绑定到不同的纹理单元（Texture Unit）进行采样。
- **着色器对象（Shader Object）**：用于存储和编译 GLSL 着色器代码的对象。
  - 顶点着色器（Vertex Shader）：处理顶点数据，进行坐标变换等。
  - 片段着色器（Fragment Shader）：处理片段数据，计算最终颜色。
  - 几何着色器（Geometry Shader）：可选，用于处理图元数据，生成新的图元。
  - 计算着色器（Compute Shader）：可选，用于通用计算任务。
- **程序对象（Program Object）**：用于链接多个着色器对象，形成一个完整的渲染程序。


## 2. OpenGL 渲染流程（可编程管线）

> OpenGL 2.0 以前是固定管线，很多渲染步骤（比如坐标变换、光照计算、纹理映射等）都是由 OpenGL 内部实现的，用户只能通过设置状态来控制渲染效果。OpenGL 2.0 引入了可编程管线，允许用户编写自定义的着色器程序，完全控制顶点和片段的处理过程。

1. 初始化 OpenGL 上下文（Context），创建窗口（使用 GLFW, GLUT, SDL 等库）。

    ```cpp
    // GLFW是跨平台的窗口和OpenGL上下文管理库，必须先初始化
    if (!glfwInit()) {
        return -1;
    }

    // 指定要用的OpenGL版本（这里是3.3）和核心模式，确保后续OpenGL代码能正常运行
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // 创建窗口和OpenGL上下文
    GLFWwindow* window =
        glfwCreateWindow(640, 480, "Simple OpenGL Demo", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window); // 让OpenGL上下文与当前线程绑定
    ```

    > OpenGL上下文（OpenGL Context）是OpenGL用来保存所有状态、资源和配置的环境。它包含了着色器、纹理、缓冲区等信息，是OpenGL渲染的“工作空间”。
    > OpenGL的所有操作都必须在一个激活了上下文的线程中进行。只有当前线程拥有（绑定了）上下文，才能调用OpenGL API，否则会出错或没有效果。

2. 加载 OpenGL 函数（使用 GLAD, GLEW 等库）。

    ```cpp
    // 加载 OpenGL 函数（设置可用的OpenGL函数指针）
    if (glewInit() != GLEW_OK) {
        std::cerr << "Failed to init GLEW\n";
        return -1;
    }
    ```

3. 创建顶点数据，配置顶点属性。

    ```cpp
    float vertices[] = { ... }; // 顶点数据，两个三角形组成一个矩形

    GLuint VAO, VBO;
    glGenVertexArrays(1, &VAO); // 创建VAO
    glGenBuffers(1, &VBO);      // 创建VBO

    glBindVertexArray(VAO);     // 绑定VAO
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // 绑定VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW); // 把顶点数据传到GPU

    // 设置顶点属性指针：位置属性，3个float，步长为3*sizeof(float)，偏移为0
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0); // 启用顶点属性
    ```

4. 创建、编译、链接、清理着色器（Vertex Shader 和 Fragment Shader）。

    ```cpp
    // 顶点着色器源码
    const char* vertexShaderSource = R"(
    #version 330 core
    layout (location = 0) in vec3 aPos;
    void main() {
        gl_Position = vec4(aPos, 1.0);          // 直接使用传入的顶点位置
    }
    )";

    // 片段着色器源码
    const char* fragmentShaderSource = R"(
    #version 330 core
    out vec4 FragColor;
    void main() {
        FragColor = vec4(0.2, 0.6, 0.9, 1.0);   // 蓝色
    }
    )";

    // 编译着色器（编译成GPU能执行的代码。顶点着色器负责处理每个顶点，片段着色器负责处理每个像素。）
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, nullptr);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, nullptr);
    glCompileShader(fragmentShader);

    // 链接着色器程序（把编译好的着色器合成一个完整的渲染程序，后续绘制时用。）
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    // 清理着色器对象（着色器已经链接到程序里，单独的着色器对象可以释放，节省资源。）
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    ```

5. 渲染循环

    ```cpp
    while (!glfwWindowShouldClose(window)) {
        // 设置并清除窗口背景色，准备新一帧的绘制。
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // 使用着色器程序, 绑定VAO，绘制两个三角形组成的矩形。
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // 交换前后缓冲区(显示新绘制的内容)，处理窗口事件(如键盘、鼠标输入)。
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    ```

6. 清理资源，退出程序。

    ```cpp
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwDestroyWindow(window);
    glfwTerminate();
    ```

## 3. OpenGL 关键技术点

1. GLFW 窗口 & 上下文管理

OpenGL 的渲染需要一个有效的上下文（Context），GLFW 负责创建和管理这个上下文。GLFW(Graphics Library Framework) 是一个开源的跨平台库，用于创建窗口、处理输入和管理 OpenGL 上下文。

2. GLEW / GLAD 函数加载

OpenGL 的函数指针需要在运行时加载，GLEW (OpenGL Extension Wrangler Library) 和 GLAD (GL Loader-Generator) 是两个常用的库，用于加载 OpenGL 函数指针。它们会根据系统支持的 OpenGL 版本和扩展，动态加载相应的函数。

3. OpenGL 状态机

    OpenGL 是一个状态机，所有的操作都是基于当前的状态进行的（比如绑定的 VAO、VBO、着色器程序等）。

    ```cpp
    glBindVertexArray(VAO1); // 切换到 VAO1
    glUseProgram(shaderA);   // 切换到着色器A
    glDrawArrays(...);       // 绘制时用的是 VAO1 和 shaderA 的状态

    glBindVertexArray(VAO2); // 切换到 VAO2
    glUseProgram(shaderB);   // 切换到着色器B
    glDrawArrays(...);       // 绘制时用的是 VAO2 和 shaderB 的状态
    ```


4. 着色器 GLSL

OpenGL 使用 GLSL (OpenGL Shading Language) 编写着色器程序。每次渲染时，OpenGL 会自动为每个顶点或片段调用对应的着色器（由 GPU 并行、批量自动执行）。着色器有顶点着色器（Vertex Shader）和片段着色器（Fragment Shader）。顶点着色器处理每个顶点，进行坐标变换等；片段着色器处理每个片段，计算最终颜色。此外还有几何着色器（Geometry Shader）和计算着色器（Compute Shader），用于更复杂的图形处理和通用计算任务。


