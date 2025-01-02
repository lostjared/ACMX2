#include "ffwrite.hpp"
#include <cmath>
#include <cstdio>
#include <iostream>

bool Writer::open(const std::string& filename,
                  int w, int h,
                  float fps,
                  int bitrate_kbps)
{
    avformat_network_init();
    av_log_set_level(AV_LOG_INFO);

    // 1) Allocate an output context (MP4)
    if (avformat_alloc_output_context2(&format_ctx, nullptr, "mp4", filename.c_str()) < 0) {
        std::cerr << "Could not allocate output context.\n";
        return false;
    }

    // 2) Find H.264 encoder
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        std::cerr << "Could not find H.264 encoder.\n";
        return false;
    }

    // 3) Create a new stream
    stream = avformat_new_stream(format_ctx, codec);
    if (!stream) {
        std::cerr << "Could not create new stream.\n";
        return false;
    }

    width  = w;
    height = h;

    // 4) Determine a rational for fps
    int fps_num = 0;
    int fps_den = 0;
    if (std::fabs(fps - 29.97f) < 0.001f) {
        // standard NTSC fraction
        fps_num = 30000;
        fps_den = 1001;
    } else {
        // fallback: e.g. 30 => 30000/1000
        fps_num = static_cast<int>(std::round(fps * 1000.0));
        fps_den = 1000;
    }

    // time_base = 1/fps => (fps_den, fps_num)
    time_base = AVRational{fps_den, fps_num};
    stream->time_base = time_base;

    // 5) Create and configure codec context
    codec_ctx = avcodec_alloc_context3(codec);
    codec_ctx->width     = width;
    codec_ctx->height    = height;
    codec_ctx->time_base = stream->time_base; 
    codec_ctx->framerate = AVRational{ fps_num, fps_den };
    codec_ctx->pix_fmt   = AV_PIX_FMT_YUV420P;

    // bit_rate in bits per second
    codec_ctx->bit_rate  = bitrate_kbps * 1000LL; 
    codec_ctx->gop_size  = 12; // keyframe interval

    // If the container needs global headers
    if (format_ctx->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    // 6) Open codec
    if (avcodec_open2(codec_ctx, codec, nullptr) < 0) {
        std::cerr << "Could not open codec.\n";
        return false;
    }

    // 7) Copy parameters to the stream
    if (avcodec_parameters_from_context(stream->codecpar, codec_ctx) < 0) {
        std::cerr << "Could not copy codec parameters.\n";
        return false;
    }

    // 8) Open output file
    if (!(format_ctx->flags & AVFMT_NOFILE)) {
        if (avio_open(&format_ctx->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file: " << filename << "\n";
            return false;
        }
    }

    // 9) Write the MP4 header
    if (avformat_write_header(format_ctx, nullptr) < 0) {
        std::cerr << "Error writing MP4 header.\n";
        return false;
    }

    // 10) Allocate frames
    // RGBA
    frameRGBA = av_frame_alloc();
    frameRGBA->format = AV_PIX_FMT_RGBA;
    frameRGBA->width  = width;
    frameRGBA->height = height;
    av_frame_get_buffer(frameRGBA, 32);

    // YUV
    frameYUV = av_frame_alloc();
    frameYUV->format = AV_PIX_FMT_YUV420P;
    frameYUV->width  = width;
    frameYUV->height = height;
    av_frame_get_buffer(frameYUV, 32);

    // 11) Create sws context for RGBA -> YUV420
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
    return true;
}

void Writer::write(void* rgba_buffer)
{
    if (!opened) {
        return;
    }
    if (!rgba_buffer) {
        return;
    }

    // 1) Copy the RGBA data (width*height*4 bytes) into frameRGBA
    // respect linesize
    int in_linesize = width * 4; // 4 channels
    uint8_t* src_ptr = static_cast<uint8_t*>(rgba_buffer);

    for (int y = 0; y < height; y++) {
        uint8_t* dst = frameRGBA->data[0] + y * frameRGBA->linesize[0];
        uint8_t* src = src_ptr + y * in_linesize;
        memcpy(dst, src, in_linesize);
    }

    // 2) Convert RGBA -> YUV420
    sws_scale(
        sws_ctx,
        frameRGBA->data,
        frameRGBA->linesize,
        0,
        height,
        frameYUV->data,
        frameYUV->linesize
    );

    // 3) Set PTS
    frameYUV->pts = frame_count++;
    
    // 4) Send frame to encoder
    int ret = avcodec_send_frame(codec_ctx, frameYUV);
    if (ret < 0) {
        std::cerr << "Error sending frame to encoder: " << ret << std::endl;
        return;
    }

    // 5) Receive packets, write them
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

void Writer::close()
{
    if (!opened) {
        return;
    }

    // 1) Flush (send null frame)
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

    // 2) Write trailer -> ensures moov atom is written
    av_write_trailer(format_ctx);

    // 3) Free resources
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

    // Close the I/O if not done
    if (!(format_ctx->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&format_ctx->pb);
    }

    avformat_free_context(format_ctx);

    opened = false;
}