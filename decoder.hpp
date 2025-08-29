#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
}
#include "mediadefs.h"
class Decoder {
 public:
  struct Config {
    Type type;
    // Audio specific parameters
    int sample_rate = 0;
    int channels = 0;
    AVSampleFormat sample_format = AV_SAMPLE_FMT_NONE;
    // Video specific parameters
    int width = 0;
    int height = 0;
    AVPixelFormat pixel_format = AV_PIX_FMT_NONE;
  };

  Decoder(Type type);
  virtual ~Decoder();

  bool open(AVStream* stream);
  void close();
  int sendPacket(AVPacket* packet);
  int receiveFrame(AVFrame* frame);
  void flush();

  // Getters for stream properties
  const Config& getConfig() const { return config_; }
  AVCodecContext* getCodecContext() const { return codec_ctx_; }
  bool isOpen() const { return codec_ctx_ != nullptr; }

 protected:
  virtual bool configureCodec();

 private:
  bool openCodec(AVStream* stream);
  void closeCodec();

  Type type_;
  Config config_;
  AVCodecContext* codec_ctx_;
  SwrContext* swr_ctx_;  // Only used for audio
};
