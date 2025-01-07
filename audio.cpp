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

int init_audio(unsigned int channels, float sense)  {
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
    unsigned int inputDeviceId = audio.getDefaultInputDevice();
    unsigned int outputDeviceId = audio.getDefaultOutputDevice();

    if (inputDeviceId == 0) {
        std::cout << "acmx2: No Input device found...\n";
        return 1;
    }
    else if (outputDeviceId == 0) {
        std::cout << "acmx2: No Output device found...\n";
        return 1;
    }
    else {
        inputParams.deviceId = audio.getDefaultInputDevice();
        inputParams.nChannels = input_channels;
        inputParams.firstChannel = 0;
        outputParams.deviceId = audio.getDefaultOutputDevice();
        outputParams.nChannels = 2;
        outputParams.firstChannel = 0;

        std::vector<unsigned int> sampleRates = audio.getDeviceInfo(inputDeviceId).sampleRates;
        if (std::find(sampleRates.begin(), sampleRates.end(), sampleRate) == sampleRates.end()) {
            sampleRate = 48000;
            if (std::find(sampleRates.begin(), sampleRates.end(), sampleRate) == sampleRates.end()) {
                sampleRate = sampleRates[0]; // Choose the first supported sample rate
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