#ifndef FFWRITE_HPP
#define FFWRITE_HPP
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}
#include <string>
#include <chrono>
#include <thread>
#include <mutex>
#include <queue>
#include<string_view>

struct Frame_Data {
    void* data;
    std::chrono::steady_clock::time_point capture_time;
};

class Writer {
public:

    Writer() : opened(false), queue_mutex(), writer_mutex(), frame_mutex() {}

    bool open(const std::string& filename, int width, int height, float fps, const char *crf);
    void write(void* rgba_buffer);
    bool open_ts(const std::string& filename, int width, int height, float fps, const char *crf);
    void write_ts(void* rgba_buffer);
    void close();
    bool is_open() const { return opened; }
    int64_t get_frame_count() const { return frame_count; } 
    double get_duration() const;
    ~Writer() {
        if (is_open()) {
            close();
            opened = false;
        }
    }

private:
    bool opened {false};
    int width = 0;
    int height = 0;
    int fps_num = 0;
    int fps_den = 0;
    int64_t frame_count = 0;
    double last_duration = 0.0; 
    AVFormatContext* format_ctx = nullptr;
    AVCodecContext* codec_ctx = nullptr;
    AVStream* stream = nullptr;
    AVFrame* frameRGBA = nullptr;
    AVFrame* frameYUV = nullptr;
    SwsContext* sws_ctx = nullptr;
    AVRational time_base;
    void calculateFPSFraction(float fps, int &fps_num, int &fps_den);
    std::chrono::steady_clock::time_point recordingStart;
    std::queue<Frame_Data> frame_queue;
    const size_t MAX_QUEUE_SIZE = 30; 
    std::mutex queue_mutex{};
    std::mutex writer_mutex{};
    std::mutex frame_mutex{};
};

extern void transfer_audio(std::string_view sourceAudioFile, std::string_view destVideoFile);
extern void cleanup_contexts(AVFormatContext* source_ctx, AVFormatContext* dest_ctx, AVFormatContext* output_ctx);

#endif