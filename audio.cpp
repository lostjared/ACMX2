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
        RtAudio::DeviceInfo info = audio.getDeviceInfo(i);
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
    }
}

int init_audio(unsigned int channels, float sense, int inputDeviceId, int outputDeviceId)  {
    input_channels = channels;
    amp_sense = sense;
    
    if (audio.getDeviceCount() < 1) {
        std::cerr << "acmx2: No audio devices found!" << std::endl;
        return 1;
    }
    else {
        std::cout << "acmx2: Audio device found...\n";
    }

    unsigned int sampleRate = 44100;
    unsigned int bufferFrames = 512;
    RtAudio::StreamParameters inputParams, outputParams;
    
    unsigned int inputDevice;
    unsigned int outputDevice;
    
    // Get input device
    if (inputDeviceId >= 0) {
        inputDevice = static_cast<unsigned int>(inputDeviceId);
        std::cout << "acmx2: Using specified input device: " << inputDevice << "\n";
    } else {
        inputDevice = audio.getDefaultInputDevice();
        std::cout << "acmx2: Using default input device: " << inputDevice << "\n";
    }
    
    // Get output device
    if (outputDeviceId >= 0) {
        outputDevice = static_cast<unsigned int>(outputDeviceId);
        std::cout << "acmx2: Using specified output device: " << outputDevice << "\n";
    } else {
        outputDevice = audio.getDefaultOutputDevice();
        std::cout << "acmx2: Using default output device: " << outputDevice << "\n";
    }

    if (inputDevice == 0) {
        std::cout << "acmx2: No Input device found...\n";
        return 1;
    }
    else if (outputDevice == 0) {
        std::cout << "acmx2: No Output device found...\n";
        return 1;
    }
    else {
        inputParams.deviceId = inputDevice;
        inputParams.nChannels = input_channels;
        inputParams.firstChannel = 0;
        outputParams.deviceId = outputDevice;
        outputParams.nChannels = 2;
        outputParams.firstChannel = 0;

        std::vector<unsigned int> sampleRates = audio.getDeviceInfo(inputDevice).sampleRates;
        if (std::find(sampleRates.begin(), sampleRates.end(), sampleRate) == sampleRates.end()) {
            sampleRate = 48000;
            if (std::find(sampleRates.begin(), sampleRates.end(), sampleRate) == sampleRates.end()) {
                sampleRate = sampleRates[0];
            }
        }
    }
    
    try {
        audio.openStream(&outputParams, &inputParams, RTAUDIO_FLOAT32, sampleRate, &bufferFrames, &audioCallback);
        audio.startStream();
        if (audio.isStreamOpen())
            std::cout << "acmx2: Audio stream opened...\n";
    }
    catch (std::exception& e) {
        std::cerr << "acmx2: Standard exception: " << e.what() << std::endl;
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