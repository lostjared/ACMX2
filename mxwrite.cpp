#include "mxwrite.hpp"
#include <cmath>
#include <cstdio>
#include <iostream>
#include <numeric>
#include <thread>
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

void Writer::calculateFPSFraction(float fps, int &fps_num, int &fps_den) {
    const float epsilon = 0.001f; 
    fps_den = 1001;
    if (std::fabs(fps - 29.97f) < epsilon) {
        fps_num = 30000;
        fps_den = 1001;
    } else if (std::fabs(fps - 59.94f) < epsilon) {
        fps_num = 60000;
        fps_den = 1001;
    } else {
        float precision = 1000.0f;
        fps_num = static_cast<int>(std::round(fps * precision));
        fps_den = static_cast<int>(precision);
        int gcd = std::gcd(fps_num, fps_den);
        fps_num /= gcd;
        fps_den /= gcd;
    }
}

bool Writer::open(const std::string& filename, int w, int h, float fps, int bitrate_kbps) {
    std::lock_guard<std::mutex> lock(writer_mutex);
    avformat_network_init();
    av_log_set_level(AV_LOG_INFO);
    opened = false;

    if (avformat_alloc_output_context2(&format_ctx, nullptr, "mp4", filename.c_str()) < 0) {
        std::cerr << "Could not allocate output context.\n";
        return false;
    }

    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Could not find H.264 encoder.\n";
        avformat_free_context(format_ctx);
        return false;
    }

    stream = avformat_new_stream(format_ctx, codec);
    if (!stream) {
        std::cerr << "Could not create new stream.\n";
        avformat_free_context(format_ctx);
        return false;
    }

    width = w;
    height = h;
    calculateFPSFraction(fps, fps_num, fps_den);

    AVRational tb = {fps_den, fps_num};
    stream->time_base = tb;

    codec_ctx = avcodec_alloc_context3(codec);
    if (!codec_ctx) {
        std::cerr << "Could not allocate codec context.\n";
        avformat_free_context(format_ctx);
        return false;
    }

    codec_ctx->width = width;
    codec_ctx->height = height;
    codec_ctx->time_base = stream->time_base;
    codec_ctx->framerate = AVRational{fps_num, fps_den};
    codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    codec_ctx->bit_rate = bitrate_kbps * 1000LL;
    codec_ctx->gop_size = 12;
    codec_ctx->max_b_frames = 0;
    codec_ctx->thread_count = std::thread::hardware_concurrency();
    codec_ctx->thread_type = FF_THREAD_FRAME;

    AVBufferRef *hw_device_ctx = nullptr;
    if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) == 0) {
        codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    } else {
        std::cerr << "Could not initialize hardware acceleration.\n";
    }

    codec_ctx->rc_max_rate = bitrate_kbps * 1000LL;
    codec_ctx->rc_min_rate = bitrate_kbps * 1000LL;
    codec_ctx->rc_buffer_size = bitrate_kbps * 2000LL;
    codec_ctx->rc_initial_buffer_occupancy = codec_ctx->rc_buffer_size * 3/4;

    if (format_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec.\n";
        avcodec_free_context(&codec_ctx);
        avformat_free_context(format_ctx);
        return false;
    }
    if (avcodec_parameters_from_context(stream->codecpar, codec_ctx) < 0) {
        std::cerr << "Could not copy codec parameters.\n";
        avcodec_free_context(&codec_ctx);
        avformat_free_context(format_ctx);
        return false;
    }
    if (!(format_ctx->flags & AVFMT_NOFILE)) {
        if (avio_open(&format_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file: " << filename << "\n";
            avcodec_free_context(&codec_ctx);
            avformat_free_context(format_ctx);
            return false;
        }
    }
    if (avformat_write_header(format_ctx, nullptr) < 0) {
        std::cerr << "Error writing MP4 header.\n";
        avio_closep(&format_ctx->pb);
        avcodec_free_context(&codec_ctx);
        avformat_free_context(format_ctx);
        return false;
    }
    frameRGBA = av_frame_alloc();
    if (!frameRGBA) {
        std::cerr << "Could not allocate frame.\n";
        avio_closep(&format_ctx->pb);
        avcodec_free_context(&codec_ctx);
        avformat_free_context(format_ctx);
        return false;
    }
    frameRGBA->format = AV_PIX_FMT_RGBA;
    frameRGBA->width = width;
    frameRGBA->height = height;
    if (av_frame_get_buffer(frameRGBA, 32) < 0) {
        std::cerr << "Could not allocate frame buffer for RGBA frame.\n";
        av_frame_free(&frameRGBA);
        avio_closep(&format_ctx->pb);
        avcodec_free_context(&codec_ctx);
        avformat_free_context(format_ctx);
        return false;
    }

    frameYUV = av_frame_alloc();
    if (!frameYUV) {
        std::cerr << "Could not allocate YUV frame.\n";
        av_frame_free(&frameRGBA);
        avio_closep(&format_ctx->pb);
        avcodec_free_context(&codec_ctx);
        avformat_free_context(format_ctx);
        return false;
    }
    frameYUV->format = AV_PIX_FMT_YUV420P;
    frameYUV->width = width;
    frameYUV->height = height;
    if (av_frame_get_buffer(frameYUV, 32) < 0) {
        std::cerr << "Could not allocate frame buffer for YUV frame.\n";
        av_frame_free(&frameRGBA);
        av_frame_free(&frameYUV);
        avio_closep(&format_ctx->pb);
        avcodec_free_context(&codec_ctx);
        avformat_free_context(format_ctx);
        return false;
    }

    sws_ctx = sws_getContext(width, height, AV_PIX_FMT_RGBA, width, height, AV_PIX_FMT_YUV420P, SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!sws_ctx) {
        std::cerr << "Could not initialize the conversion context.\n";
        av_frame_free(&frameRGBA);
        av_frame_free(&frameYUV);
        avio_closep(&format_ctx->pb);
        avcodec_free_context(&codec_ctx);
        avformat_free_context(format_ctx);
        return false;
    }

    opened = true; 
    recordingStart = std::chrono::steady_clock::now();
    return true;
}

void Writer::write(void* rgba_buffer)
{
    std::lock_guard<std::mutex> lock(writer_mutex);
    if (!opened) {
        return;
    }
    if (!rgba_buffer) {
        return;
    }
    int in_linesize = width * 4; 
    uint8_t* src_ptr = static_cast<uint8_t*>(rgba_buffer);

    std::lock_guard<std::mutex> frame_lock(frame_mutex);
    memcpy(frameRGBA->data[0], rgba_buffer, width * height * 4);
    sws_scale(sws_ctx, frameRGBA->data, frameRGBA->linesize, 0, height, frameYUV->data, frameYUV->linesize);
    frameYUV->pts = frame_count++;
    
    int ret = avcodec_send_frame(codec_ctx, frameYUV);
    if (ret < 0) {
        std::cerr << "Error sending frame to encoder: " << ret << std::endl;
        return;
    }
    AVPacket* pkt = av_packet_alloc();
    while (true) {
        ret = avcodec_receive_packet(codec_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break; 
        } else if (ret < 0) {
            std::cerr << "Error receiving packet: " << ret << std::endl;
            av_packet_free(&pkt);
            return;
        }

        av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
        pkt->stream_index = stream->index;

        if (av_interleaved_write_frame(format_ctx, pkt) < 0) {
            std::cerr << "Error writing frame.\n";
            av_packet_free(&pkt);
            return;
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
}

bool Writer::open_ts(const std::string& filename, int w, int h, float fps, int bitrate_kbps) {
    avformat_network_init();
    av_log_set_level(AV_LOG_INFO);
    if (avformat_alloc_output_context2(&format_ctx, nullptr, "mp4", filename.c_str()) < 0) {
        std::cerr << "Could not allocate output context.\n";
        return false;
    }
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Could not find H.264 encoder.\n";
        return false;
    }
    stream = avformat_new_stream(format_ctx, codec);
    if (!stream) {
        std::cerr << "Could not create new stream.\n";
        return false;
    }
    width  = w;
    height = h;
    calculateFPSFraction(fps, fps_num, fps_den);
    time_base = AVRational{1, 1000000}; 
    stream->time_base = time_base;
    codec_ctx = avcodec_alloc_context3(codec);
    codec_ctx->width       = width;
    codec_ctx->height      = height;
    codec_ctx->time_base   = stream->time_base; 
    codec_ctx->framerate   = AVRational{fps_num, fps_den}; 
    codec_ctx->pix_fmt     = AV_PIX_FMT_YUV420P;
    codec_ctx->bit_rate    = bitrate_kbps * 1000LL;
    codec_ctx->gop_size    = 12; 
    codec_ctx->thread_count = std::thread::hardware_concurrency();
    codec_ctx->thread_type = FF_THREAD_FRAME;
    codec_ctx->max_b_frames = 0;
    codec_ctx->delay = 0;
    codec_ctx->flags |= AV_CODEC_FLAG_LOW_DELAY;

    AVBufferRef *hw_device_ctx = nullptr;
    if (av_hwdevice_ctx_create(&hw_device_ctx, AV_HWDEVICE_TYPE_CUDA, nullptr, nullptr, 0) == 0) {
        codec_ctx->hw_device_ctx = av_buffer_ref(hw_device_ctx);
    } else {
        std::cerr << "Could not initialize hardware acceleration.\n";
    }
    
    if (format_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec.\n";
        return false;
    }
    if (avcodec_parameters_from_context(stream->codecpar, codec_ctx) < 0) {
        std::cerr << "Could not copy codec parameters.\n";
        return false;
    }
    if (!(format_ctx->flags & AVFMT_NOFILE)) {
        if (avio_open(&format_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file: " << filename << "\n";
            return false;
        }
    }
    if (avformat_write_header(format_ctx, nullptr) < 0) {
        std::cerr << "Error writing MP4 header.\n";
        return false;
    }
    frameRGBA = av_frame_alloc();
    frameRGBA->format = AV_PIX_FMT_RGBA;
    frameRGBA->width  = width;
    frameRGBA->height = height;
    av_frame_get_buffer(frameRGBA, 32);
    frameYUV = av_frame_alloc();
    frameYUV->format = AV_PIX_FMT_YUV420P;
    frameYUV->width  = width;
    frameYUV->height = height;
    av_frame_get_buffer(frameYUV, 32);
    sws_ctx = sws_getContext(
        width, height, AV_PIX_FMT_RGBA,
        width, height, AV_PIX_FMT_YUV420P,
        SWS_FAST_BILINEAR,
        nullptr, nullptr, nullptr
    );
    if (!sws_ctx) {
        std::cerr << "Could not create sws context.\n";
        return false;
    }

    opened = true;
    frame_count = 0;
    recordingStart = std::chrono::steady_clock::now();
    return true;
}

void Writer::write_ts(void* rgba_buffer) {
    if (!opened || !rgba_buffer) {
        return; 
    }
    auto current_time = std::chrono::steady_clock::now();
    {
        std::lock_guard<std::mutex> lock(queue_mutex); 
        if (frame_queue.size() >= MAX_QUEUE_SIZE) {
            std::cerr << "Warning: Dropping frame due to full queue\n";
            return; 
        }
        size_t frame_size = width * height * 4;
        auto frame_copy = std::make_unique<uint8_t[]>(frame_size);
        memcpy(frame_copy.get(), rgba_buffer, frame_size);
        frame_queue.push({frame_copy.release(), current_time});
    }
    while (true) {
        Frame_Data frame;
        {
            std::lock_guard<std::mutex> lock(queue_mutex); 
            if (frame_queue.empty()) {
                break; 
            }
            frame = frame_queue.front(); 
            frame_queue.pop(); 
        }

        int64_t elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(
            frame.capture_time - recordingStart).count();
        int64_t pts_val = av_rescale_q(elapsed_us, AVRational{1, 1000000}, stream->time_base);

        int in_linesize = width * 4;
        uint8_t* src_ptr = static_cast<uint8_t*>(frame.data);
        for (int y = 0; y < height; y++) {
            uint8_t* dst = frameRGBA->data[0] + y * frameRGBA->linesize[0];
            uint8_t* src = src_ptr + y * in_linesize;
            memcpy(dst, src, in_linesize);
        }

        sws_scale(
            sws_ctx,
            frameRGBA->data,
            frameRGBA->linesize,
            0,
            height,
            frameYUV->data,
            frameYUV->linesize
        );
        frameYUV->pts = pts_val;

        int ret = avcodec_send_frame(codec_ctx, frameYUV);
        if (ret < 0) {
            std::cerr << "Error sending frame to encoder\n";
            free(frame.data); 
            return;
        }

        
        AVPacket* pkt = av_packet_alloc();
        while (true) {
            ret = avcodec_receive_packet(codec_ctx, pkt);
            if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
                break; 
            } else if (ret < 0) {
                std::cerr << "Error receiving packet\n";
                av_packet_free(&pkt); 
                free(frame.data); 
                return;
            }

            
            av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
            pkt->stream_index = stream->index;
            if (av_interleaved_write_frame(format_ctx, pkt) < 0) {
                std::cerr << "Error writing frame.\n";
                av_packet_free(&pkt); 
                free(frame.data); 
                return;
            }

            av_packet_unref(pkt); 
        }

        av_packet_free(&pkt);
        free(frame.data); 
    }
}

void Writer::close()
{
    std::lock_guard<std::mutex> lock(writer_mutex);
    if (!opened) {
        return;
    }
    while (true) { 
        {
            std::lock_guard<std::mutex> queue_lock(queue_mutex);
            if (frame_queue.empty()) {
                break;
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    avcodec_send_frame(codec_ctx, nullptr);

    while (true) {
        AVPacket* pkt = av_packet_alloc();
        int ret = avcodec_receive_packet(codec_ctx, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            av_packet_free(&pkt);
            break;
        }
        av_packet_rescale_ts(pkt, codec_ctx->time_base, stream->time_base);
        pkt->stream_index = stream->index;
        av_interleaved_write_frame(format_ctx, pkt);
        av_packet_free(&pkt);
    }
    av_write_trailer(format_ctx);
    if (sws_ctx) {
        sws_freeContext(sws_ctx);
        sws_ctx = nullptr;
    }
    if (frameRGBA) {
        av_frame_free(&frameRGBA);
    }
    if (frameYUV) {
        av_frame_free(&frameYUV);
    }
    avcodec_free_context(&codec_ctx);
    if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&format_ctx->pb);
    }
    avformat_free_context(format_ctx);
    opened = false;
}