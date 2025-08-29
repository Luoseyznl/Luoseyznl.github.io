#include "decoder.hpp"

#include "logger.hpp"

extern "C" {
#include <libavutil/pixdesc.h>
#include <libavutil/pixfmt.h>
}

using namespace utils;

Decoder::Decoder(Type type)
    : type_(type), codec_ctx_(nullptr), swr_ctx_(nullptr) {
  LOG_INFO << "Initializing Decoder";
}

Decoder::~Decoder() {
  LOG_INFO << "Destroying Decoder";
  close();
}

bool Decoder::open(AVStream* stream) {
  if (!stream) {
    LOG_ERROR << "Invalid stream";
    return false;
  }

  // Check stream type matches decoder type
  bool is_video = stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO;
  bool is_audio = stream->codecpar->codec_type == AVMEDIA_TYPE_AUDIO;
  if ((type_ == Type::Video && !is_video) ||
      (type_ == Type::Audio && !is_audio)) {
    LOG_ERROR << "Stream type mismatch";
    return false;
  }

  // Store stream parameters in config
  config_.type = type_;
  if (type_ == Type::Audio) {
    config_.sample_rate = stream->codecpar->sample_rate;
    config_.channels = stream->codecpar->ch_layout.nb_channels;
    config_.sample_format =
        static_cast<AVSampleFormat>(stream->codecpar->format);
  } else {
    config_.width = stream->codecpar->width;
    config_.height = stream->codecpar->height;
    config_.pixel_format = static_cast<AVPixelFormat>(stream->codecpar->format);
  }

  return openCodec(stream);
}

bool Decoder::openCodec(AVStream* stream) {
  const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
  if (!codec) {
    LOG_ERROR << "Codec not found";
    return false;
  }

  codec_ctx_ = avcodec_alloc_context3(codec);
  if (!codec_ctx_) {
    LOG_ERROR << "Could not allocate codec context";
    return false;
  }

  if (avcodec_parameters_to_context(codec_ctx_, stream->codecpar) < 0) {
    LOG_ERROR << "Could not copy codec params to codec context";
    closeCodec();
    return false;
  }

  if (avcodec_open2(codec_ctx_, codec, nullptr) < 0) {
    LOG_ERROR << "Could not open codec";
    closeCodec();
    return false;
  }

  if (!configureCodec()) {
    LOG_ERROR << "Failed to configure codec";
    closeCodec();
    return false;
  }

  if (type_ == Type::Video) {
    LOG_INFO << "Video decoder initialized: " << codec_ctx_->width << "x"
             << codec_ctx_->height
             << ", Pixel format: " << av_get_pix_fmt_name(codec_ctx_->pix_fmt);
  } else {
    LOG_INFO << "Audio decoder initialized: " << codec_ctx_->sample_rate
             << "Hz, " << codec_ctx_->ch_layout.nb_channels << " channels, "
             << "Format: " << av_get_sample_fmt_name(codec_ctx_->sample_fmt);
  }
  return true;
}

void Decoder::close() {
  LOG_INFO << "Closing decoder";
  if (swr_ctx_) {
    swr_free(&swr_ctx_);
    swr_ctx_ = nullptr;
  }
  closeCodec();
}

void Decoder::closeCodec() {
  if (codec_ctx_) {
    avcodec_free_context(&codec_ctx_);
    codec_ctx_ = nullptr;
  }
}

int Decoder::sendPacket(AVPacket* packet) {
  if (!codec_ctx_) {
    LOG_ERROR << "Decoder not initialized";
    return AVERROR(EINVAL);
  }

  // Handle flush packet
  if (!packet) {
    int ret = avcodec_send_packet(codec_ctx_, nullptr);
    if (ret < 0) {
      char errbuf[AV_ERROR_MAX_STRING_SIZE];
      av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
      LOG_ERROR << "Error flushing decoder: " << errbuf;
      return ret;
    }
    LOG_DEBUG << "Successfully sent flush packet";
    return 0;
  }

  // Validate packet
  if (packet->size <= 0) {
    LOG_ERROR << "Invalid packet size: " << packet->size;
    return AVERROR(EINVAL);
  }

  // Send packet for decoding
  int ret = avcodec_send_packet(codec_ctx_, packet);
  if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
    LOG_ERROR << "Error sending packet for decoding: " << errbuf;
    return ret;
  }

  // LOG_DEBUG << "Successfully sent packet to decoder, size: " << packet->size;
  return 0;
}

int Decoder::receiveFrame(AVFrame* frame) {
  if (!codec_ctx_) {
    LOG_ERROR << "Decoder not initialized";
    return AVERROR(EINVAL);
  }

  if (!frame) {
    LOG_ERROR << "Invalid frame pointer";
    return AVERROR(EINVAL);
  }

  int ret = avcodec_receive_frame(codec_ctx_, frame);
  if (ret == AVERROR(EAGAIN)) {
    LOG_DEBUG << "No frame available yet";
    return ret;
  } else if (ret == AVERROR_EOF) {
    LOG_DEBUG << "End of stream";
    return ret;
  } else if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
    LOG_ERROR << "Error receiving frame: " << errbuf;
    return ret;
  }

  // LOG_DEBUG << "Successfully received frame: "
  //           << "pts: " << frame->pts
  //           << "，type:" << (int)type_
  //           << ", pic type: " << av_get_picture_type_char(frame->pict_type);
  return 0;
}

void Decoder::flush() {
  LOG_INFO << "Flushing decoder";
  if (codec_ctx_) {
    avcodec_flush_buffers(codec_ctx_);
  }
}

bool Decoder::configureCodec() {
  // Audio specific configuration
  if (type_ == Type::Audio && !swr_ctx_) {
    swr_ctx_ = swr_alloc();
    if (!swr_ctx_) {
      LOG_ERROR << "Could not allocate resampler context";
      return false;
    }

    // Configure resampler if needed
    // This is where you would set up audio resampling parameters
  }
  return true;
}
