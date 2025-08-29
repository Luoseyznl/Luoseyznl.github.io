#include "stream_source.hpp"

#include "utils/logger.hpp"

extern "C" {
#include <libavutil/frame.h>
}

using namespace utils;

// Helper: get best pts for a frame without using
// av_frame_get_best_effort_timestamp. Prefer frame->pts, then packet->pts.
// Returns AV_NOPTS_VALUE if none.
static inline int64_t get_frame_pts(const AVFrame* frame,
                                    const AVPacket* packet) {
  if (!frame) return AV_NOPTS_VALUE;
  if (frame->pts != AV_NOPTS_VALUE) return frame->pts;
  if (packet && packet->pts != AV_NOPTS_VALUE) return packet->pts;
  return AV_NOPTS_VALUE;
}

StreamSource::StreamSource(Type type)
    : type_(type), MAX_QUEUE_SIZE(type == Type::Video ? 30 : 50) {
  // Initialize properties based on type
  if (type == Type::Video) {
    width_ = 0;
    height_ = 0;
    frame_rate_ = 0.0;
    pixel_fmt_ = AV_PIX_FMT_NONE;
  } else {
    sample_rate_ = 0;
    channels_ = 0;
    sample_fmt_ = AV_SAMPLE_FMT_NONE;
    channel_layout_ = 0;
  }

  // Create decoder
  decoder_ = std::make_unique<Decoder>(type == Type::Video ? Type::Video
                                                           : Type::Audio);
}

StreamSource::~StreamSource() { close(); }

bool StreamSource::open(const std::string& filename) {
  // Open demuxer
  demuxer_ = std::make_shared<Demuxer>(type_);
  if (!demuxer_->open(filename)) {
    LOG_ERROR << "Failed to open demuxer";
    return false;
  }

  // Get stream
  AVStream* stream = demuxer_->getAVStream();

  if (!stream) {
    LOG_ERROR << (type_ == Type::Video ? "No video" : "No audio")
              << " stream found";
    return false;
  }

  if (!initCodec(stream)) {
    LOG_ERROR << "Failed to initialize codec";
    return false;
  }

  state_.store(State::Stopped);
  eof_.store(false);

  LOG_INFO << "Opened " << (type_ == Type::Video ? "video" : "audio")
           << " file: " << filename;
  return true;
}

bool StreamSource::initCodec(AVStream* stream) {
  if (!decoder_->open(stream)) {
    LOG_ERROR << "Failed to open decoder";
    return false;
  }

  // Set media specific properties
  const auto& config = decoder_->getConfig();
  if (type_ == Type::Video) {
    width_ = config.width;
    height_ = config.height;
    pixel_fmt_ = config.pixel_format;
    frame_rate_ = av_q2d(stream->avg_frame_rate);
    LOG_INFO << "Video properties: " << width_ << "x" << height_
             << " fps=" << frame_rate_;
  } else {
    sample_rate_ = config.sample_rate;
    channels_ = config.channels;
    sample_fmt_ = config.sample_format;

    // Get channel layout from decoder context if available
    auto codec_ctx = decoder_->getCodecContext();
    if (codec_ctx) {
      if (codec_ctx->ch_layout.nb_channels > 0) {
        channel_layout_ = codec_ctx->ch_layout.u.mask;
      } else {
        // Fallback: set a simple default based on channels
        channel_layout_ =
            (channels_ == 1) ? AV_CH_LAYOUT_MONO : AV_CH_LAYOUT_STEREO;
      }
    }

    LOG_INFO << "Audio properties: "
             << "sample_rate=" << sample_rate_ << " channels=" << channels_
             << " format="
             << (sample_fmt_ != AV_SAMPLE_FMT_NONE
                     ? av_get_sample_fmt_name(sample_fmt_)
                     : "unknown")
             << " channel_layout=0x" << std::hex << channel_layout_ << std::dec;
  }

  return true;
}

void StreamSource::start() {
  LOG_INFO << (type_ == Type::Video ? "Video" : "Audio") << " reader starting";
  state_.store(State::Running);
  eof_.store(false);

  // Start decoding thread
  decoding_thread_ = std::thread(&StreamSource::decodingThread, this);
}

void StreamSource::decodingThread() {
  LOG_INFO << (type_ == Type::Video ? "Video" : "Audio")
           << " decoding thread started";
  int packet_count = 0;

  while (state_.load() != State::Stopped) {
    if (state_.load() == State::Paused) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    // Throttle if queue is full
    {
      std::unique_lock<std::mutex> lock(queue_mutex_);
      if (frame_queue_.size() >= MAX_QUEUE_SIZE) {
        LOG_DEBUG << (type_ == Type::Video ? "Video" : "Audio")
                  << " frame queue full (" << frame_queue_.size() << "/"
                  << MAX_QUEUE_SIZE << "), waiting...";
        queue_cond_.wait(lock, [this] {
          return frame_queue_.size() < MAX_QUEUE_SIZE ||
                 state_.load() != State::Running;
        });
        continue;
      }
    }

    // Read next packet from demuxer (demuxer already filters to desired stream)
    AVPacket* packet = demuxer_->readPacket();
    if (!packet) {
      if (demuxer_->isEOF()) {
        eof_.store(true);
        LOG_INFO << (type_ == Type::Video ? "Video" : "Audio")
                 << " reached EOF, flushing decoder...";
        processPacket(nullptr);  // flush decoder

        // Wait for remaining frames to be drained by the consumer before
        // stopping. Avoid stopping immediately so consumers (AudioPlayer) can
        // consume flushed frames.
        {
          std::unique_lock<std::mutex> lock(queue_mutex_);
          // wait up to 500ms or until queue becomes empty or state changes
          queue_cond_.wait_for(lock, std::chrono::milliseconds(500), [this] {
            return frame_queue_.empty() || state_.load() != State::Running;
          });

          if (frame_queue_.empty()) {
            LOG_INFO << (type_ == Type::Video ? "Video" : "Audio")
                     << " all flushed frames consumed, stopping reader";
            state_.store(State::Stopped);
            break;
          } else {
            LOG_INFO << (type_ == Type::Video ? "Video" : "Audio")
                     << " frames remain after flush: " << frame_queue_.size()
                     << " (will keep thread alive briefly)";
            // keep loop running to allow consumers to drain remaining frames
          }
        }
      }

      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    processPacket(packet);
    if (++packet_count % 30 == 0) {
      std::lock_guard<std::mutex> lock(queue_mutex_);
      LOG_DEBUG << (type_ == Type::Video ? "Video" : "Audio") << " processed "
                << packet_count << " packets, " << frame_queue_.size() << "/"
                << MAX_QUEUE_SIZE << " frames in queue";
    }
  }

  LOG_INFO << (type_ == Type::Video ? "Video" : "Audio")
           << " decoding thread exited";
}

void StreamSource::pause() {
  State expected = State::Running;
  if (state_.compare_exchange_strong(expected, State::Paused)) {
    LOG_INFO << (type_ == Type::Video ? "Video" : "Audio") << " reader paused";
  }
}

void StreamSource::resume() {
  State expected = State::Paused;
  if (state_.compare_exchange_strong(expected, State::Running)) {
    LOG_INFO << (type_ == Type::Video ? "Video" : "Audio") << " reader resumed";
  }
}

void StreamSource::stop() {
  state_.store(State::Stopped);
  queue_cond_.notify_all();
  LOG_INFO << (type_ == Type::Video ? "Video" : "Audio") << " reader stopped";
}

void StreamSource::close() {
  stop();

  if (decoding_thread_.joinable()) {
    decoding_thread_.join();
  }

  clearQueue();

  if (decoder_) {
    decoder_->close();
  }

  if (demuxer_) {
    demuxer_.reset();
  }

  state_.store(State::Stopped);
  eof_.store(false);

  LOG_INFO << (type_ == Type::Video ? "Video" : "Audio") << " reader closed";
}

std::shared_ptr<StreamSource::Frame> StreamSource::getNextFrame() {
  std::unique_lock<std::mutex> lock(queue_mutex_);

  if (frame_queue_.empty()) {
    // if (eof_.load() && state_.load() == State::Stopped) {
    //   LOG_INFO << (type_ == Type::Video ? "Video" : "Audio")
    //            << " no more frames, playback finished";
    // } else {
    //   LOG_DEBUG << (type_ == Type::Video ? "Video" : "Audio")
    //             << " waiting for frames... (queue empty, EOF=" << eof_.load()
    //             << ", state=" << static_cast<int>(state_.load()) << ")";
    // }
    return nullptr;
  }

  auto frame = frame_queue_.front();
  frame_queue_.pop();
  queue_cond_.notify_one();

  return frame;
}

void StreamSource::processPacket(AVPacket* packet) {
  // Send packet to decoder (packet==nullptr flushes)
  int send_ret = decoder_->sendPacket(packet);
  if (send_ret < 0) {
    LOG_ERROR << "Error sending packet to decoder: " << send_ret;
    if (packet) av_packet_free(&packet);
    return;
  }

  AVFrame* avframe = av_frame_alloc();
  if (!avframe) {
    LOG_ERROR << "Failed to alloc AVFrame";
    if (packet) av_packet_free(&packet);
    return;
  }

  // Pull all frames produced by this packet (or produced by flush)
  while (true) {
    int ret = decoder_->receiveFrame(avframe);
    if (ret == AVERROR(EAGAIN)) {
      break;  // no more frames now
    } else if (ret == AVERROR_EOF) {
      break;  // decoder drained
    } else if (ret < 0) {
      LOG_ERROR << "Error receiving frame: " << ret;
      break;
    }

    // Determine pts (source timebase is stream timebase)
    int64_t pts_src = get_frame_pts(avframe, packet);
    int64_t pts = AV_NOPTS_VALUE;
    if (pts_src != AV_NOPTS_VALUE && demuxer_) {
      pts = av_rescale_q(pts_src, getTimeBase(), AV_TIME_BASE_Q);
    }

    // Compute duration
    int64_t duration = 0;
    if (type_ == Type::Video) {
      if (avframe->duration > 0) {
        duration =
            av_rescale_q(avframe->duration, getTimeBase(), AV_TIME_BASE_Q);
      } else if (frame_rate_ > 0.0) {
        duration = static_cast<int64_t>(AV_TIME_BASE / frame_rate_);
      }
    } else {
      int sr =
          sample_rate_ > 0 ? sample_rate_ : decoder_->getConfig().sample_rate;
      if (sr > 0) {
        duration =
            (static_cast<int64_t>(avframe->nb_samples) * AV_TIME_BASE) / sr;
      }
    }

    // Fallback pts if still invalid
    if (pts == AV_NOPTS_VALUE) {
      static int64_t fallback_count = 0;
      pts = av_rescale_q(fallback_count++, av_make_q(1, 1), AV_TIME_BASE_Q);
    }

    // Clone frame and push to queue
    AVFrame* frame_clone = av_frame_clone(avframe);
    if (!frame_clone) {
      LOG_ERROR << "Failed to clone AVFrame";
      av_frame_unref(avframe);
      continue;
    }

    auto shared_frame = std::shared_ptr<AVFrame>(
        frame_clone, [](AVFrame* f) { av_frame_free(&f); });
    auto frame_wrapper = std::make_shared<Frame>(shared_frame, pts, duration);
    queueFrame(frame_wrapper);

    av_frame_unref(avframe);
  }

  av_frame_free(&avframe);
  if (packet) av_packet_free(&packet);
}

void StreamSource::queueFrame(std::shared_ptr<Frame> frame) {
  if (!frame) return;

  std::lock_guard<std::mutex> lock(queue_mutex_);
  if (frame_queue_.size() >= MAX_QUEUE_SIZE) {
    LOG_INFO << (type_ == Type::Video ? "Video" : "Audio")
             << " queue full, dropping oldest frame";
    frame_queue_.pop();
  }
  frame_queue_.push(frame);
  queue_cond_.notify_one();
}

void StreamSource::clearQueue() {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  while (!frame_queue_.empty()) {
    frame_queue_.pop();
  }
}

bool StreamSource::seek(int64_t timestamp) {
  LOG_INFO << "seek to " << timestamp;
  if (timestamp < 0 || timestamp > getDuration()) {
    LOG_ERROR << "Seek timestamp out of range: " << timestamp
              << ", duration: " << getDuration();
    return false;
  }

  if (!demuxer_) {
    LOG_ERROR << "No demuxer available";
    return false;
  }

  int seek_flags = AVSEEK_FLAG_BACKWARD;
  if (!demuxer_->seek(timestamp, seek_flags)) {
    LOG_ERROR << "Error seeking to position: " << timestamp;
    return false;
  }

  // Reset decoder and queues
  decoder_->flush();
  clearQueue();
  eof_.store(false);

  // Decode forward until we reach a frame at/after timestamp (timestamp unit:
  // AV_TIME_BASE)
  while (true) {
    AVPacket* packet = demuxer_->readPacket();
    if (!packet) {
      LOG_WARN << "seek: no packet post-seek";
      break;
    }

    if (packet->stream_index != demuxer_->getAVStream()->index) {
      av_packet_free(&packet);
      continue;
    }

    if (decoder_->sendPacket(packet) < 0) {
      LOG_ERROR << "Error sending packet to decoder during seek";
      av_packet_free(&packet);
      break;
    }

    AVFrame* avframe = av_frame_alloc();
    if (!avframe) {
      av_packet_free(&packet);
      LOG_ERROR << "alloc frame failed during seek";
      break;
    }

    while (true) {
      int ret = decoder_->receiveFrame(avframe);
      if (ret == AVERROR(EAGAIN)) {
        // need more packets
        break;
      } else if (ret == AVERROR_EOF) {
        av_frame_free(&avframe);
        av_packet_free(&packet);
        return true;
      } else if (ret < 0) {
        LOG_ERROR << "Error receiving frame during seek: " << ret;
        av_frame_free(&avframe);
        av_packet_free(&packet);
        return false;
      }

      int64_t pts_src = get_frame_pts(avframe, packet);
      int64_t pts = (pts_src != AV_NOPTS_VALUE)
                        ? av_rescale_q(pts_src, getTimeBase(), AV_TIME_BASE_Q)
                        : AV_NOPTS_VALUE;

      int64_t duration = 0;
      if (type_ == Type::Video) {
        if (avframe->duration > 0)
          duration =
              av_rescale_q(avframe->duration, getTimeBase(), AV_TIME_BASE_Q);
        else if (frame_rate_ > 0.0)
          duration = static_cast<int64_t>(AV_TIME_BASE / frame_rate_);
      } else {
        int sr =
            sample_rate_ > 0 ? sample_rate_ : decoder_->getConfig().sample_rate;
        if (sr > 0)
          duration =
              (static_cast<int64_t>(avframe->nb_samples) * AV_TIME_BASE) / sr;
      }

      if (pts == AV_NOPTS_VALUE && packet) {
        pts = (packet->pts != AV_NOPTS_VALUE)
                  ? av_rescale_q(packet->pts, getTimeBase(), AV_TIME_BASE_Q)
                  : AV_NOPTS_VALUE;
      }

      if (pts == AV_NOPTS_VALUE) {
        // can't determine, drop
        av_frame_unref(avframe);
        continue;
      }

      auto shared_frame = std::shared_ptr<AVFrame>(
          av_frame_clone(avframe), [](AVFrame* f) { av_frame_free(&f); });
      auto frame_wrapper = std::make_shared<Frame>(shared_frame, pts, duration);

      LOG_DEBUG << "Seek decoded frame pts: " << pts
                << ", target: " << timestamp;

      if (pts >= timestamp) {
        queueFrame(frame_wrapper);
        av_frame_free(&avframe);
        av_packet_free(&packet);
        LOG_INFO << "Seek completed, queued frame at pts: " << pts;
        return true;
      }

      // not yet at target
      av_frame_unref(avframe);
    }

    av_frame_free(&avframe);
    av_packet_free(&packet);
  }

  return true;
}

int64_t StreamSource::getCurrentTimestamp() const {
  std::lock_guard<std::mutex> lock(queue_mutex_);
  if (!frame_queue_.empty()) {
    return frame_queue_.front()->pts;
  }
  return 0;
}

AVRational StreamSource::getTimeBase() const {
  return demuxer_->getAVStream()->time_base;
}