#include<iostream>
#include<string>
#include<string_view>
#include<cstdlib>

extern "C" {
    #include <libavcodec/avcodec.h>
    #include <libavformat/avformat.h>
    #include <libavutil/opt.h>
    #include <libavutil/mathematics.h>
    #include <libavutil/timestamp.h>
}

void transfer_audio(std::string_view, std::string_view);

int main(int argc, char **argv)  {
    if(argc != 3) {
        std::cerr << "Error: usage audo_tranfser source_file dest_file\n";
        return EXIT_FAILURE;
    }
    transfer_audio(argv[1], argv[2]);
    std::cout << "Success...\n";
    return 0;
}

void transfer_audio(std::string_view sourceAudioFile, std::string_view destAudioFile) {
    AVFormatContext *input_ctx = nullptr;
    AVFormatContext *output_ctx = nullptr;
    int audio_stream_idx = -1;
    

    if (avformat_open_input(&input_ctx, sourceAudioFile.data(), nullptr, nullptr) < 0) {
        std::cerr << "Could not open input file\n";
        return;
    }
    

    if (avformat_find_stream_info(input_ctx, nullptr) < 0) {
        std::cerr << "Could not find stream info\n";
        avformat_close_input(&input_ctx);
        return;
    }
    

    for (unsigned int i = 0; i < input_ctx->nb_streams; i++) {
        if (input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }
    
    if (audio_stream_idx == -1) {
        std::cerr << "Could not find audio stream\n";
        avformat_close_input(&input_ctx);
        return;
    }
    
    if (avformat_alloc_output_context2(&output_ctx, nullptr, nullptr, destAudioFile.data()) < 0) {
        std::cerr << "Could not create output context\n";
        avformat_close_input(&input_ctx);
        return;
    }
    
    AVStream *out_stream = avformat_new_stream(output_ctx, nullptr);
    if (!out_stream) {
        std::cerr << "Failed to allocate output stream\n";
        avformat_close_input(&input_ctx);
        avformat_free_context(output_ctx);
        return;
    }
    
    avcodec_parameters_copy(out_stream->codecpar, input_ctx->streams[audio_stream_idx]->codecpar);
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE)) {
        if (avio_open(&output_ctx->pb, destAudioFile.data(), AVIO_FLAG_WRITE) < 0) {
            std::cerr << "Could not open output file\n";
            avformat_close_input(&input_ctx);
            avformat_free_context(output_ctx);
            return;
        }
    }
    
    if (avformat_write_header(output_ctx, nullptr) < 0) {
        std::cerr << "Error writing header\n";
        avformat_close_input(&input_ctx);
        if (!(output_ctx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&output_ctx->pb);
        avformat_free_context(output_ctx);
        return;
    }

    AVPacket packet;
    while (av_read_frame(input_ctx, &packet) >= 0) {
        if (packet.stream_index == audio_stream_idx) {
            packet.pts = av_rescale_q_rnd(packet.pts,
                                        input_ctx->streams[audio_stream_idx]->time_base,
                                        out_stream->time_base,
                                        static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.dts = av_rescale_q_rnd(packet.dts,
                                        input_ctx->streams[audio_stream_idx]->time_base,
                                        out_stream->time_base,
                                        static_cast<AVRounding>(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.duration = av_rescale_q(packet.duration,
                                         input_ctx->streams[audio_stream_idx]->time_base,
                                         out_stream->time_base);
            packet.stream_index = 0;
            
            if (av_interleaved_write_frame(output_ctx, &packet) < 0) {
                std::cerr << "Error writing packet\n";
                break;
            }
        }
        av_packet_unref(&packet);
    }
    
    av_write_trailer(output_ctx);
    avformat_close_input(&input_ctx);
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE))
        avio_closep(&output_ctx->pb);
    avformat_free_context(output_ctx);
}