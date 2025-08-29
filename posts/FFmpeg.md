---
title: FFmpeg 学习笔记
date: 2025-08-27
category: 渐入佳境
tags: [编程, C++, 学习笔记, FFmpeg]
---

FFmpeg 是一个开源的多媒体处理工具，支持音视频的录制、转换和流化。提供命令行工具和丰富的库（如 libavcodec、libavformat、libavfilter 等），支持多种协议、格式、编解码器。FFmpeg 不仅能播放和处理本地文件，还能作为流媒体服务器或客户端，实现推流（如 RTMP 推流到直播平台）、拉流（如 RTSP 拉取监控视频）等功能。

## 目录


## 1. 分流与解码

多媒体文件（如 MP4、MKV、FLV）或网络流（如 RTSP、HTTP）通常包含多个轨道（视频、音频、字幕等）。每个轨道称为一个“流”（AVStream），有独立的编码参数和时间基。FFmpeg 根据输入自动选择合适的分流器（demuxer），如 MP4、FLV、MPEG-TS 等。分流器负责解析文件/流的封装格式，提取各个轨道的数据包。

### 1.1 Demuxing（解复用）

1. 打开媒体源、读取流信息、查找目标流（视频/音频）、读取数据包（AVPacket）。

```cpp
// avformat_open_input()
avformat_open_input(&fmt_ctx, filename, NULL, NULL); // 打开输入文件/流

// avformat_find_stream_info()
avformat_find_stream_info(fmt_ctx, NULL); // 读取流信息

// av_find_best_stream()
int video_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0); // 查找视频流
int audio_stream_index = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0); // 查找音频流

// av_read_frame()
AVPacket pkt;
while (av_read_frame(fmt_ctx, &pkt) >= 0) { // 读取数据包
    if (pkt.stream_index == video_stream_index) {
        // 处理视频包
    } else if (pkt.stream_index == audio_stream_index) {
        // 处理音频包
    }
    av_packet_unref(&pkt); // 释放包
}
```

2. 获取时长、Seek 跳转

```cpp
int64_t Demuxer::getDuration() const {
    if (!format_ctx_) return 0;

    // If format context has a valid duration, use it
    if (format_ctx_->duration != AV_NOPTS_VALUE) {
        return format_ctx_->duration;
    }

    // Otherwise try to get duration from video stream
    if (video_stream_ && video_stream_->duration != AV_NOPTS_VALUE) {
        return av_rescale_q(video_stream_->duration, video_stream_->time_base,
                            AV_TIME_BASE_Q);
    }
    return 0;
}

bool Demuxer::seek(int64_t timestamp, int flags) {
    if (!format_ctx_) return false;

    // Convert timestamp to the appropriate time base
    int64_t seek_target = av_rescale_q(timestamp, AV_TIME_BASE_Q, video_stream_->time_base);

    // Seek to the target position
    if (av_seek_frame(format_ctx_, video_stream_->index, seek_target, flags) < 0) {
        return false;
    }

    // Flush the decoder buffers
    return true;
}
```

### 1.2 Decoding（解码）



