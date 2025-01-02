#ifndef _FFWRITE_H_
#define _FFWRITE_H_

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
}
#include<opencv2/opencv.hpp>
#include<iostream>
#include<string>

class Writer {
public:
    bool open(const std::string &filename, int width, int height, float fps, int bit_rate); // bit_rate in Kbps
    void write(const cv::Mat &frame_mat);
    void close();
    ~Writer() {
        if(opened == true) {
            close();
            opened = false;
        }
    }
    bool is_open() const { return opened; }
private:
    AVFormatContext *format_context = nullptr;
    AVCodecContext *codec_context = nullptr;
    AVStream *stream = nullptr;
    AVFrame *frame = nullptr;
    int frame_count = 0;
    AVRational time_base;
    const AVPixelFormat pix_fmt = AV_PIX_FMT_YUV420P;
    bool opened = false;
};

#endif