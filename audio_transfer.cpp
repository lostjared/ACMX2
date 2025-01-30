#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <mxwrite.hpp>

void transfer_audio(std::string_view sourceAudioFile, std::string_view destVideoFile);

int main(int argc, char **argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <source_video_with_audio> <destination_video>\n";
        return EXIT_FAILURE;
    }
    transfer_audio(argv[1], argv[2]);
    return 0;
}
