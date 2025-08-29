#include "demuxer.hpp"

#include "logger.hpp"

using namespace utils;

Demuxer::Demuxer(Type type)
    : format_ctx_(nullptr),
      type_(type),
      video_stream_(nullptr),
      audio_stream_(nullptr),
      video_stream_index_(-1),
      audio_stream_index_(-1),
      eof_(false) {
  LOG_INFO << "Initializing Demuxer";
}

Demuxer::~Demuxer() {
  LOG_INFO << "Destroying Demuxer";
  close();
}

bool Demuxer::open(const std::string& filename) {
  LOG_INFO << "Opening media file: " << filename;

  // Open input file
  if (avformat_open_input(&format_ctx_, filename.c_str(), nullptr, nullptr) <
      0) {
    LOG_ERROR << "Could not open input file: " << filename;
    return false;
  }

  // Read stream information
  if (avformat_find_stream_info(format_ctx_, nullptr) < 0) {
    LOG_ERROR << "Could not find stream information";
    close();
    return false;
  }

  // Find video stream
  video_stream_index_ =
      av_find_best_stream(format_ctx_, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
  if (video_stream_index_ >= 0) {
    video_stream_ = format_ctx_->streams[video_stream_index_];
    LOG_INFO << "Found video stream at index " << video_stream_index_;
  }

  // Find audio stream
  audio_stream_index_ =
      av_find_best_stream(format_ctx_, AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
  if (audio_stream_index_ >= 0) {
    audio_stream_ = format_ctx_->streams[audio_stream_index_];
    LOG_INFO << "Found audio stream at index " << audio_stream_index_;
  }

  return true;
}

void Demuxer::close() {
  LOG_INFO << "Closing demuxer";
  if (format_ctx_) {
    avformat_close_input(&format_ctx_);
    format_ctx_ = nullptr;
  }

  video_stream_ = nullptr;
  audio_stream_ = nullptr;
  video_stream_index_ = -1;
  audio_stream_index_ = -1;
  eof_ = false;
}

AVPacket* Demuxer::readPacket() {
  if (!format_ctx_) {
    LOG_ERROR << "Demuxer not initialized";
    return nullptr;
  }
  int target_stream_index = getStreamIndex();
  if (target_stream_index < 0) {
    LOG_ERROR << "No valid target stream";
    return nullptr;
  }
  while (true) {
    AVPacket* packet = av_packet_alloc();
    int ret = av_read_frame(format_ctx_, packet);
    if (ret < 0) {
      if (ret == AVERROR_EOF) {
        eof_ = true;
        LOG_INFO << "Reached end of file";
      } else {
        LOG_ERROR << "Error reading packet: " << ret;
      }
      av_packet_free(&packet);
      return nullptr;
    }
    if (packet->stream_index == target_stream_index) {
      return packet;
    } else {
      // Skip packets from other streams
      av_packet_free(&packet);
    }
  }
}

// timestamp: 微秒(us)
bool Demuxer::seek(int64_t timestamp, int flags) {
  if (!format_ctx_) {
    LOG_ERROR << "Demuxer not initialized";
    return false;
  }

  // 获取目标流
  int stream_index = getStreamIndex();
  if (stream_index < 0) {
    LOG_ERROR << "No valid stream for seeking";
    return false;
  }

  AVStream* stream = format_ctx_->streams[stream_index];
  if (!stream) {
    LOG_ERROR << "Invalid stream for seeking";
    return false;
  }

  // 将微秒转换为流的时间基准单位
  int64_t seek_target =
      av_rescale_q(timestamp, AV_TIME_BASE_Q, stream->time_base);

  LOG_DEBUG << "Seeking to " << timestamp
            << "us (stream timebase: " << stream->time_base.num << "/"
            << stream->time_base.den << ", target: " << seek_target << ")";

  int ret = av_seek_frame(format_ctx_, stream_index, seek_target, flags);

  if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, errbuf, AV_ERROR_MAX_STRING_SIZE);
    LOG_ERROR << "Error seeking to position " << timestamp << "us: " << errbuf;
    return false;
  }

  eof_ = false;
  LOG_INFO << "Successfully seeked to " << timestamp << "us";
  return true;
}

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

int Demuxer::getStreamIndex() {
  return type_ == Type::Audio ? audio_stream_index_ : video_stream_index_;
}

AVStream* Demuxer::getAVStream() {
  return type_ == Type::Audio ? audio_stream_ : video_stream_;
}