#include"audio.hpp"
#include<vector>
#include<string>
#include<iostream>
#include<algorithm>

float gAmplitude = 0.0f;
float amp_sense = 25.0f;
unsigned int input_channels = 2;
bool output_buffer = false;

int audioCallback(void* outputBuffer, void* inputBuffer, unsigned int nBufferFrames,
    double streamTime, RtAudioStreamStatus status, void* userData) {
    if (status) {
        std::cerr << "Stream underflow detected!" << std::endl;
    }
    float* in = static_cast<float*>(inputBuffer);
    float* out = static_cast<float*>(outputBuffer);
    float sum = 0.0f;

    for (unsigned int i = 0; i < nBufferFrames; ++i) { 
        for (unsigned int ch = 0; ch < input_channels; ++ch) {
            unsigned int index = i * input_channels + ch;
            sum += std::abs(in[index]);
            if(output_buffer == true)
                out[index] = in[index]; 
        }
    }

    gAmplitude = sum / (nBufferFrames * input_channels);
    return 0;
}

float get_amp() { return gAmplitude; }
float get_sense() { return amp_sense; }

RtAudio audio;

void set_output(bool o) {
    output_buffer = o;
}


void list_audio_devices() {
    unsigned int devices = audio.getDeviceCount();
    std::cout << "acmx2: Found " << devices << " audio device(s):\n";
    
    for (unsigned int i = 0; i < devices; i++) {
        try {
            RtAudio::DeviceInfo info = audio.getDeviceInfo(i);
            if (info.probed == false) {
                std::cout << "  Device " << i << ": [Unable to probe device]\n";
                continue;
            }
            std::cout << "  Device " << i << ": " << info.name;
            if (info.isDefaultInput) std::cout << " [DEFAULT INPUT]";
            if (info.isDefaultOutput) std::cout << " [DEFAULT OUTPUT]";
            std::cout << "\n";
            std::cout << "    Input channels: " << info.inputChannels << "\n";
            std::cout << "    Output channels: " << info.outputChannels << "\n";
            std::cout << "    Sample rates: ";
            for (auto rate : info.sampleRates) {
                std::cout << rate << " ";
            }
            std::cout << "\n";
        } catch (std::exception &e) {
            std::cout << "  Device " << i << ": Error - " << e.what() << "\n";
        }
    }
}

int init_audio(unsigned int channels, float sense, int inputDeviceId, int outputDeviceId)  {
    input_channels = channels;
    amp_sense = sense;
    if (audio.getDeviceCount() < 1) {
        std::cerr << "acmx2: No audio devices found!" << std::endl;
        return 1;
    } else {
        std::cout << "acmx2: Audio device found...\n";
    }
    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 512;
    RtAudio::StreamParameters inputParams, outputParams;
    
    unsigned int inputDevice = audio.getDeviceCount();
    unsigned int outputDevice = audio.getDeviceCount();
    if (inputDeviceId < 0) {
        std::cout << "acmx2: Searching for valid input device...\n";
        for (unsigned int i = 0; i < audio.getDeviceCount(); i++) {
            try {
                RtAudio::DeviceInfo info = audio.getDeviceInfo(i);
                if (info.probed && info.inputChannels > 0) {
                    inputDevice = i;
                    std::cout << "acmx2: Found input device: " << i << " - " << info.name << "\n";
                    break;
                }
            } catch (std::exception &e) {
                continue;
            }
        }
        if (inputDevice >= audio.getDeviceCount()) {
            std::cerr << "acmx2: No valid input devices available!\n";
            return 1;
        }
    } else {
        inputDevice = static_cast<unsigned int>(inputDeviceId);
        std::cout << "acmx2: Using specified input device: " << inputDevice << "\n";
    }
    
    if (outputDeviceId < 0) {
        std::cout << "acmx2: Searching for valid output device...\n";
        for (unsigned int i = 0; i < audio.getDeviceCount(); i++) {
            try {
                RtAudio::DeviceInfo info = audio.getDeviceInfo(i);
                if (info.probed && info.outputChannels > 0) {
                    outputDevice = i;
                    std::cout << "acmx2: Found output device: " << i << " - " << info.name << "\n";
                    break;
                }
            } catch (std::exception &e) {
                continue;
            }
        }
        if (outputDevice >= audio.getDeviceCount()) {
            std::cerr << "acmx2: No valid output devices available!\n";
            return 1;
        }
    } else {
        outputDevice = static_cast<unsigned int>(outputDeviceId);
        std::cout << "acmx2: Using specified output device: " << outputDevice << "\n";
    }

    RtAudio::DeviceInfo inputInfo;
    RtAudio::DeviceInfo outputInfo;
    
    try {
        inputInfo = audio.getDeviceInfo(inputDevice);
        if (!inputInfo.probed || inputInfo.inputChannels == 0) {
            std::cerr << "acmx2: Invalid input device or no input channels!\n";
            return 1;
        }
    } catch (std::exception &e) {
        std::cerr << "acmx2: Error getting input device info: " << e.what() << "\n";
        return 1;
    }
    
    try {
        outputInfo = audio.getDeviceInfo(outputDevice);
        if (!outputInfo.probed || outputInfo.outputChannels == 0) {
            std::cerr << "acmx2: Invalid output device or no output channels!\n";
            return 1;
        }
    } catch (std::exception &e) {
        std::cerr << "acmx2: Error getting output device info: " << e.what() << "\n";
        return 1;
    }
    
    std::cout << "acmx2: Input device: " << inputInfo.name << "\n";
    std::cout << "acmx2: Output device: " << outputInfo.name << "\n";

    inputParams.deviceId = inputDevice;
    inputParams.nChannels = std::min(input_channels, inputInfo.inputChannels);
    inputParams.firstChannel = 0;
    
    outputParams.deviceId = outputDevice;
    outputParams.nChannels = std::min(2u, outputInfo.outputChannels);
    outputParams.firstChannel = 0;

    std::vector<unsigned int> sampleRates = inputInfo.sampleRates;
    if (sampleRates.empty() || std::find(sampleRates.begin(), sampleRates.end(), sampleRate) == sampleRates.end()) {
        if (std::find(sampleRates.begin(), sampleRates.end(), 48000) != sampleRates.end()) {
            sampleRate = 48000;
        } else if (!sampleRates.empty()) {
            sampleRate = sampleRates[0];
        } else {
            std::cerr << "acmx2: No supported sample rates found!\n";
            return 1;
        }
    }
    
    std::cout << "acmx2: Using sample rate: " << sampleRate << " Hz\n";
    std::cout << "acmx2: Using " << inputParams.nChannels << " input channel(s)\n";
    
    try {
        audio.openStream(&outputParams, &inputParams, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &audioCallback);
        audio.startStream();
        if (audio.isStreamOpen())
            std::cout << "acmx2: Audio stream opened...\n";
    }
    catch (std::exception& e) {
        std::cerr << "acmx2: RtAudio error: " << e.what() << std::endl;
        if (audio.isStreamOpen()) audio.closeStream();
        return 1;
    }
    catch (...) {
        std::cerr << "acmx2: Unknown error occurred!" << std::endl;
        if (audio.isStreamOpen()) audio.closeStream();
        return 1;
    }

    return 0;
}

void close_audio() {
    if (audio.isStreamOpen()) {
        audio.closeStream();
        std::cout << "acmx2: Audio stream closed.\n";
    }
}