#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
}

#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <thread>

#include "decoder.hpp"
#include "demuxer.hpp"

class StreamSource {
 public:
  struct Frame {
    std::shared_ptr<AVFrame> frame;
    int64_t pts;
    int64_t duration;

    Frame(std::shared_ptr<AVFrame> f, int64_t p, int64_t d)
        : frame(f), pts(p), duration(d) {}
  };

  enum class State { Stopped, Paused, Running };

  StreamSource(Type type);
  ~StreamSource();

  bool open(const std::string& filename);
  void close();

  void start();
  void pause();
  void resume();
  void stop();

  /**
   * Seek to the specified timestamp
   * @param timestamp Target position in seconds
   * @return true if seek was successful, false otherwise
   */
  bool seek(int64_t timestamp);

  bool isEOF() const { return eof_; }

  // Frame operations
  std::shared_ptr<Frame> getNextFrame();
  int64_t getCurrentTimestamp() const;

  // Video specific methods
  int getWidth() const { return width_; }
  int getHeight() const { return height_; }
  double getFrameRate() const { return frame_rate_; }
  AVPixelFormat getPixelFormat() const { return pixel_fmt_; }

  // Audio specific methods
  int getSampleRate() const { return sample_rate_; }
  int getChannels() const { return channels_; }
  AVSampleFormat getSampleFormat() const { return sample_fmt_; }
  int64_t getChannelLayout() const { return channel_layout_; }

  // Duration in us
  int64_t getDuration() const { return demuxer_ ? demuxer_->getDuration() : 0; }
  AVRational getTimeBase() const;

 private:
  void decodingThread();
  void processPacket(AVPacket* packet);
  void queueFrame(std::shared_ptr<Frame> frame);
  void clearQueue();
  bool initCodec(AVStream* stream);

  // Media type
  Type type_;

  // Media properties
  int width_;
  int height_;
  double frame_rate_;
  AVPixelFormat pixel_fmt_;

  int sample_rate_;
  int channels_;
  AVSampleFormat sample_fmt_;
  int64_t channel_layout_;

  // Common components
  std::unique_ptr<Decoder> decoder_;
  std::shared_ptr<Demuxer> demuxer_;

  // Thread management
  std::thread decoding_thread_;
  std::atomic<State> state_{State::Stopped};
  std::atomic<bool> eof_{false};

  // Frame queue
  std::queue<std::shared_ptr<Frame>> frame_queue_;
  mutable std::mutex queue_mutex_;
  std::condition_variable queue_cond_;
  const size_t MAX_QUEUE_SIZE;
};
