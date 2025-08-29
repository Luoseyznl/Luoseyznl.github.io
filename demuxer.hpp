#pragma once

extern "C" {
#include <libavformat/avformat.h>
}

#include <string>

#include "mediadefs.h"

class Demuxer {
 public:
  Demuxer(Type type);
  ~Demuxer();

  bool open(const std::string& filename);
  void close();

  AVPacket* readPacket();
  // timestamp: 微秒(us)
  bool seek(int64_t timestamp, int flags = 0);

  AVStream* getAVStream();
  int getStreamIndex();

  int64_t getDuration() const;
  bool isEOF() const { return eof_; }

  AVFormatContext* getFormatContext() const { return format_ctx_; }

 private:
  Type type_;
  AVFormatContext* format_ctx_;
  AVStream* video_stream_;
  AVStream* audio_stream_;
  int video_stream_index_;
  int audio_stream_index_;
  bool eof_;
};
