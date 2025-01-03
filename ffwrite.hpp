#ifndef WRITER_HPP
#define WRITER_HPP

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}
#include <string>

class Writer {
public:
    bool open(const std::string& filename, int width, int height, float fps, int bitrate_kbps);
    void write(void* rgba_buffer);
    void close();
    bool is_open() const { return opened; }
    ~Writer() {
        if (is_open()) {
            close();
            opened = false;
        }
    }

private:
    bool opened = false;
    int width = 0;
    int height = 0;
    int frame_count = 0;
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVStream* stream = nullptr;
    AVFrame* frameRGBA = nullptr;
    AVFrame* frameYUV = nullptr;
    SwsContext* sws_ctx = nullptr;
    AVRational time_base;
    void calculateFPSFraction(float fps, int &fps_num, int &fps_den);
};

#endif
