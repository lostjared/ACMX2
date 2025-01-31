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

std::mutex transfer_audio_mutex;

bool is_format_supported(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext) return false;
    return (strcmp(ext, ".mp4") == 0 || 
            strcmp(ext, ".mkv") == 0 ||
            strcmp(ext, ".avi") == 0 ||
            strcmp(ext, ".mov") == 0);
}

void cleanup_contexts(AVFormatContext* source_ctx, 
                     AVFormatContext* dest_ctx,
                     AVFormatContext* output_ctx) {
    if (source_ctx) avformat_close_input(&source_ctx);
    if (dest_ctx) avformat_close_input(&dest_ctx);
    if (output_ctx) {
        if (!(output_ctx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&output_ctx->pb);
        avformat_free_context(output_ctx);
    }
}

void transfer_audio(std::string_view sourceAudioFile, std::string_view destVideoFile) {
    std::lock_guard<std::mutex> lock(transfer_audio_mutex);
    if (!is_format_supported(destVideoFile.data())) {
        std::cerr << "Unsupported output format. Supported formats: .mp4, .mkv, .avi, .mov\n";
        return;
    }

    AVFormatContext *source_ctx = nullptr, *dest_ctx = nullptr, *output_ctx = nullptr;
    int source_audio_idx = -1, dest_video_idx = -1, dest_audio_idx = -1;
    std::string temp_output = std::string(destVideoFile) + ".tmp";

    if (avformat_open_input(&source_ctx, sourceAudioFile.data(), nullptr, nullptr) != 0 ||
        avformat_open_input(&dest_ctx, destVideoFile.data(), nullptr, nullptr) != 0) {
        std::cerr << "Failed to open input files\n";
        cleanup_contexts(source_ctx, dest_ctx, output_ctx);
        return;
    }

    if (avformat_find_stream_info(source_ctx, nullptr) < 0 ||
        avformat_find_stream_info(dest_ctx, nullptr) < 0) {
        std::cerr << "Failed to find stream info\n";
        cleanup_contexts(source_ctx, dest_ctx, output_ctx);
        return;
    }


    for (unsigned i = 0; i < source_ctx->nb_streams; ++i) {
        if (source_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            source_audio_idx = i;
            break;
        }
    }


    for (unsigned i = 0; i < dest_ctx->nb_streams; ++i) {
        AVMediaType type = dest_ctx->streams[i]->codecpar->codec_type;
        if (type == AVMEDIA_TYPE_VIDEO) dest_video_idx = i;
        else if (type == AVMEDIA_TYPE_AUDIO) dest_audio_idx = i;
    }

    if (source_audio_idx == -1 || dest_video_idx == -1) {
        std::cerr << "Required streams not found\n";
        cleanup_contexts(source_ctx, dest_ctx, output_ctx);
        return;
    }

    const AVOutputFormat *output_fmt = av_guess_format(nullptr, destVideoFile.data(), nullptr);
    if (!output_fmt) {
        output_fmt = av_guess_format("mp4", nullptr, nullptr);
        if (!output_fmt) {
            std::cerr << "Failed to determine output format\n";
            cleanup_contexts(source_ctx, dest_ctx, output_ctx);
            return;
        }
    }

    if (avformat_alloc_output_context2(&output_ctx, output_fmt, nullptr, temp_output.c_str()) < 0) {
        std::cerr << "Failed to create output context\n";
        cleanup_contexts(source_ctx, dest_ctx, output_ctx);
        return;
    }

 
    const AVCodec* audio_codec = avcodec_find_decoder(source_ctx->streams[source_audio_idx]->codecpar->codec_id);
    if (!audio_codec) {
        std::cerr << "Failed to find audio decoder\n";
        cleanup_contexts(source_ctx, dest_ctx, output_ctx);
        return;
    }

    
    for (unsigned i = 0; i < dest_ctx->nb_streams; ++i) {
        if (dest_ctx->streams[i]->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) {
            continue;  
        }
        
        AVStream *dest_stream = dest_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(output_ctx, nullptr);
        if (!out_stream) {
            std::cerr << "Failed to create output stream\n";
            cleanup_contexts(source_ctx, dest_ctx, output_ctx);
            return;
        }

        if (avcodec_parameters_copy(out_stream->codecpar, dest_stream->codecpar) < 0) {
            std::cerr << "Failed to copy video parameters\n";
            cleanup_contexts(source_ctx, dest_ctx, output_ctx);
            return;
        }
        
        out_stream->time_base = dest_stream->time_base;
        out_stream->codecpar->codec_tag = 0;
    }

    
    AVStream *out_stream = avformat_new_stream(output_ctx, audio_codec);
    if (!out_stream) {
        std::cerr << "Failed to create audio stream\n";
        cleanup_contexts(source_ctx, dest_ctx, output_ctx);
        return;
    }

    AVCodecParameters* source_params = source_ctx->streams[source_audio_idx]->codecpar;
    if (avcodec_parameters_copy(out_stream->codecpar, source_params) < 0) {
        std::cerr << "Failed to copy audio parameters\n";
        cleanup_contexts(source_ctx, dest_ctx, output_ctx);
        return;
    }

    if (source_params->frame_size == 0) {
        out_stream->codecpar->frame_size = 1024;
    } else {
        out_stream->codecpar->frame_size = source_params->frame_size;
    }
    
    out_stream->time_base = source_ctx->streams[source_audio_idx]->time_base;
    out_stream->codecpar->codec_tag = 0;
    dest_audio_idx = out_stream->index;

    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_ctx->pb, temp_output.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Failed to open output file\n";
            cleanup_contexts(source_ctx, dest_ctx, output_ctx);
            return;
        }
    }
    if (avformat_write_header(output_ctx, nullptr) < 0) {
        std::cerr << "Failed to write header\n";
        cleanup_contexts(source_ctx, dest_ctx, output_ctx);
        return;
    }
    AVPacket packet;
    while (av_read_frame(dest_ctx, &packet) >= 0) {
        if (packet.stream_index == dest_audio_idx) {
            av_packet_unref(&packet);
            continue;
        }

        AVStream *in_stream = dest_ctx->streams[packet.stream_index];
        AVStream *out_stream = output_ctx->streams[packet.stream_index];
        av_packet_rescale_ts(&packet, in_stream->time_base, out_stream->time_base);

        if (av_interleaved_write_frame(output_ctx, &packet) < 0) {
            std::cerr << "Failed to write packet\n";
            av_packet_unref(&packet);
            cleanup_contexts(source_ctx, dest_ctx, output_ctx);
            return;
        }
        av_packet_unref(&packet);
    }

    av_seek_frame(source_ctx, source_audio_idx, 0, AVSEEK_FLAG_BACKWARD);
    while (av_read_frame(source_ctx, &packet) >= 0) {
        if (packet.stream_index == source_audio_idx) {
            AVStream *in_stream = source_ctx->streams[packet.stream_index];
            AVStream *out_stream = output_ctx->streams[dest_audio_idx];
            av_packet_rescale_ts(&packet, in_stream->time_base, out_stream->time_base);
            packet.stream_index = dest_audio_idx;

            if (av_interleaved_write_frame(output_ctx, &packet) < 0) {
                std::cerr << "Failed to write audio packet\n";
                av_packet_unref(&packet);
                cleanup_contexts(source_ctx, dest_ctx, output_ctx);
                return;
            }
        }
        av_packet_unref(&packet);
    }
    av_write_trailer(output_ctx);
    cleanup_contexts(source_ctx, dest_ctx, output_ctx);
    std::remove(destVideoFile.data());
    std::rename(temp_output.c_str(), destVideoFile.data());
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
    codec_ctx->rc_buffer_size = bitrate_kbps * 1000LL;  
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

    sws_ctx = sws_getContext(width, height, AV_PIX_FMT_RGBA, width, height, AV_PIX_FMT_YUV420P, SWS_BICUBIC, nullptr, nullptr, nullptr); 
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

void Writer::write(void* rgba_buffer) {
    std::lock_guard<std::mutex> lock(writer_mutex);
    if (!opened || !rgba_buffer) {
        return;
    }

    
    while (frame_queue.size() >= MAX_QUEUE_SIZE) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

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
        SWS_BICUBIC,  
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
