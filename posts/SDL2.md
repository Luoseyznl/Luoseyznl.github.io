---
title: SDL2 学习笔记
date: 2025-08-27
category: 渐入佳境
tags: [SDL2, 多媒体, 开发]
---

SDL2（Simple DirectMedia Layer 2）是一个跨平台的多媒体开发库，主要用于简化底层硬件的访问和管理。它为开发者提供了统一的接口，可以方便地操作窗口、图形、音频、输入设备等，广泛应用于游戏开发、模拟器、媒体播放器等领域。

## 目录

## 1. SDL2 音频播放

SDL2 提供了简单的音频播放功能，可以方便地加载和播放音频文件。音频的播放通常涉及以下几个步骤：

1. 初始化 SDL2 音频子系统。

    ```cpp
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0) {
        if (SDL_InitSubSystem(SDL_INIT_AUDIO) < 0) {
            // 错误处理
        }
    }
    ```

2. 打开音频设备。

    ```cpp
    SDL_AudioSpec want{};   // 配置期望的音频规格
    want.freq = sample_rate_;
    want.format = AUDIO_S16SYS;
    want.channels = static_cast<Uint8>(channels_);
    want.samples = 1024;
    want.callback = audioCallback;  // 音频回调函数（SDL2 会在需要更多音频数据时调用它）
    want.userdata = this;

    SDL_AudioSpec have{};   // 实际获得的音频规格
    device_id_ = SDL_OpenAudioDevice(nullptr, 0, &want, &have, 0);  // 打开默认音频设备
    ```

    > 音频播放有两个重要概念：**拉模式**和**推模式**。SDL2 使用的是拉模式，即音频设备会定期向应用程序请求更多的音频数据（通过回调函数）。应用程序需要在回调函数中提供足够的数据，以确保音频播放的连续性。

3. 加载音频数据（音频回调机制）。

    ```cpp
    void AudioPlayer::audioCallback(void* userdata, uint8_t* stream, int len) {
        auto* player = static_cast<AudioPlayer*>(userdata);
        if (!player || !player->is_initialized_) {
            SDL_memset(stream, 0, len);
            return;
        }
        player->fillAudioData(stream, len);
    }

    void AudioPlayer::fillAudioData(uint8_t* stream, int len) {
        // Real-time safe: only pop from ring buffer with minimal locking
        size_t got = popPCM(stream, static_cast<size_t>(len));
        if (got < static_cast<size_t>(len)) {
            // silence remaining（剩余部分用 0 填充（静音），避免杂音。）
            SDL_memset(stream + got, 0, static_cast<size_t>(len) - got);
        }

        // update consumed samples / audio clock（更新已消费样本/音频时钟）
        size_t bytes_per_sample =
            static_cast<size_t>(av_get_bytes_per_sample(AV_SAMPLE_FMT_S16));
        if (bytes_per_sample == 0 || channels_ == 0) return;
        size_t samples_consumed =
            got / (bytes_per_sample * static_cast<size_t>(channels_));
        if (samples_consumed > 0) {
            consumed_samples_.fetch_add(static_cast<int64_t>(samples_consumed), // 更新已消费样本（原子操作）
                                        std::memory_order_relaxed);
            int64_t base = base_pts_.load(std::memory_order_acquire);
            if (base != AV_NOPTS_VALUE && sample_rate_ > 0) {
            int64_t us = av_rescale_q(consumed_samples_.load(),               // FFmpeg 工具函数 秒数=样本数/采样率
                                        AVRational{1, sample_rate_}, AV_TIME_BASE_Q);
            audio_clock_.store(base + us, std::memory_order_release);
            }
        }

        // signal producer there is space (non-blocking)
        ring_not_full_.notify_one();  // 生产者-消费者架构
    }
    ```

    > `store(..., memory_order_release)`写入原子变量，并“释放”之前的修改，让其他线程能看到。`load(..., memory_order_acquire)`读取原子变量，并“获取”之前线程的修改，保证读取到的是最新的数据。C++11 标准库提供的原子操作，确保多线程环境下的数据一致性和可见性。（生产者-消费者模型）


4. 播放音频。

    ```cpp
    SDL_PauseAudioDevice(device_id_, 0); // 播放
    SDL_PauseAudioDevice(device_id_, 1); // 暂停
    SDL_CloseAudioDevice(device_id_);    // 关闭
    ```

5. 音量控制。

    ```cpp
    void AudioPlayer::setVolume(double norm) {
        volume_ = static_cast<int>(norm * SDL_MIX_MAXVOLUME);
    }
    ```
    

