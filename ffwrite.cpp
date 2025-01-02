#include "ffwrite.hpp"
#include <cmath>   
#include <cstdio>  

bool Writer::open(const std::string& filename, int width, int height, float fps, int bit_rate) {
    avformat_network_init();
    av_log_set_level(AV_LOG_INFO);
    if (avformat_alloc_output_context2(&format_context, nullptr, "mp4", filename.c_str()) < 0) {
        fprintf(stderr, "Could not allocate output context\n");
        return false;
    }
    const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "Could not find H.264 encoder\n");
        return false;
    }
    stream = avformat_new_stream(format_context, codec);
    if (!stream) {
        fprintf(stderr, "Could not create new stream\n");
        return false;
    }

    int fps_num = 0;
    int fps_den = 0;
    if (std::fabs(fps - 29.97f) < 0.001f) {
        fps_num = 30000;
        fps_den = 1001;
    } else {
        fps_num = static_cast<int>(std::round(fps * 1000.0));
        fps_den = 1000;
    }
    stream->time_base = AVRational{ fps_den, fps_num };
    codec_context = avcodec_alloc_context3(codec);
    if (!codec_context) {
        fprintf(stderr, "Could not allocate codec context\n");
        return false;
    }

    codec_context->codec_id  = codec->id;
    codec_context->bit_rate = bit_rate * 1000;
    codec_context->width     = width;
    codec_context->height    = height;
    codec_context->time_base = stream->time_base; 
    codec_context->framerate = AVRational{ fps_num, fps_den };

    codec_context->gop_size  = 12;        
    codec_context->pix_fmt   = pix_fmt;   

    if (format_context->oformat->flags & AVFMT_GLOBALHEADER) {
        codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (avcodec_open2(codec_context, codec, nullptr) < 0) {
        fprintf(stderr, "Could not open the codec\n");
        return false;
    }

    if (avcodec_parameters_from_context(stream->codecpar, codec_context) < 0) {
        fprintf(stderr, "Could not copy codec parameters to stream\n");
        return false;
    }

    if (!(format_context->flags & AVFMT_NOFILE)) {
        if (avio_open(&format_context->pb, filename.c_str(), AVIO_FLAG_WRITE) < 0) {
            fprintf(stderr, "Could not open output file: %s\n", filename.c_str());
            return false;
        }
    }

    if (avformat_write_header(format_context, nullptr) < 0) {
        fprintf(stderr, "Error writing MP4 header\n");
        return false;
    }

    frame = av_frame_alloc();
    frame->format = codec_context->pix_fmt;
    frame->width  = codec_context->width;
    frame->height = codec_context->height;

    av_image_alloc(frame->data, frame->linesize,
                   frame->width, frame->height,
                   codec_context->pix_fmt, 32);

    opened = true;
    frame_count = 0;
    this->time_base = stream->time_base;
    return true;
}


void Writer::write(const cv::Mat &frame_mat) {
    if (!opened) return;  
    if (frame_mat.empty() ||
        frame_mat.cols != codec_context->width ||
        frame_mat.rows != codec_context->height)
    {
        return;
    }
    cv::Mat yuv_frame;
    cv::cvtColor(frame_mat, yuv_frame, cv::COLOR_BGR2YUV_I420);
    int y_size = codec_context->width * codec_context->height;
    memcpy(frame->data[0], yuv_frame.data, y_size);                   
    memcpy(frame->data[1], yuv_frame.data + y_size, y_size / 4);      
    memcpy(frame->data[2], yuv_frame.data + y_size + y_size / 4, y_size / 4); 
    frame->pts = frame_count;
    frame_count++;
    int ret = avcodec_send_frame(codec_context, frame);
    if (ret < 0) {
        return;
    }
    AVPacket *pkt = av_packet_alloc();
    while (true) {
        ret = avcodec_receive_packet(codec_context, pkt);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        }
        if (ret < 0) {
            av_packet_free(&pkt);
            return;
        }
        av_packet_rescale_ts(pkt, codec_context->time_base, stream->time_base);
        pkt->stream_index = stream->index;
        if (av_interleaved_write_frame(format_context, pkt) < 0) {
            av_packet_free(&pkt);
            return;
        }
        av_packet_unref(pkt);
    }
    av_packet_free(&pkt);
}

void Writer::close() {
    av_write_trailer(format_context);
    avcodec_free_context(&codec_context);
    av_frame_free(&frame);
    avformat_free_context(format_context);
    opened = false;
}
