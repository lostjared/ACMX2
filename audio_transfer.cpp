#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/opt.h>
    #include <libavutil/mathematics.h>
    #include <libavutil/timestamp.h>
}

void transfer_audio(std::string_view sourceAudioFile, std::string_view destVideoFile);

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <source_video_with_audio> <destination_video>\n";
        return EXIT_FAILURE;
    }
    transfer_audio(argv[1], argv[2]);
    return 0;
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
        std::cerr << "Failed to determine output format\n";
        cleanup_contexts(source_ctx, dest_ctx, output_ctx);
        return;
    }

    if (avformat_alloc_output_context2(&output_ctx, output_fmt, nullptr, temp_output.c_str()) < 0) {
        std::cerr << "Failed to create output context\n";
        cleanup_contexts(source_ctx, dest_ctx, output_ctx);
        return;
    }
    for (unsigned i = 0; i < dest_ctx->nb_streams; ++i) {
        AVStream *dest_stream = dest_ctx->streams[i];
        AVStream *out_stream = avformat_new_stream(output_ctx, nullptr);
        if (!out_stream) {
            std::cerr << "Failed to create output stream\n";
            cleanup_contexts(source_ctx, dest_ctx, output_ctx);
            return;
        }

        if (i == static_cast<unsigned>(dest_audio_idx)) {
     
            avcodec_parameters_copy(out_stream->codecpar, source_ctx->streams[source_audio_idx]->codecpar);
        } else {
            avcodec_parameters_copy(out_stream->codecpar, dest_stream->codecpar);
        }
        out_stream->codecpar->codec_tag = 0; 
        out_stream->time_base = dest_stream->time_base;
    }

    if (dest_audio_idx == -1) {
        AVStream *out_stream = avformat_new_stream(output_ctx, nullptr);
        if (!out_stream) {
            std::cerr << "Failed to create new audio stream\n";
            cleanup_contexts(source_ctx, dest_ctx, output_ctx);
            return;
        }
        avcodec_parameters_copy(out_stream->codecpar, source_ctx->streams[source_audio_idx]->codecpar);
        out_stream->codecpar->codec_tag = 0;
        out_stream->time_base = source_ctx->streams[source_audio_idx]->time_base;
        dest_audio_idx = out_stream->index;
    }
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