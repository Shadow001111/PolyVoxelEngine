#include "SoundEngine.h"
#include <sndfile.h>
#include <thread>
#include <chrono>
#include <GLFW/glfw3.h>

ALCdevice* SoundEngine::device = nullptr;
ALCcontext* SoundEngine::context = nullptr;

SoundSource::SoundSource() : alSource(0), alBuffer(0)
{}

void SoundSource::load(const std::string& path)
{
    SF_INFO sfInfo;
    SNDFILE* sndFile = sf_open(path.c_str(), SFM_READ, &sfInfo);
    if (!sndFile)
    {
        std::cerr << "Failed to open sound file: " << path << std::endl;
        return;
    }

    size_t bufferSize = sfInfo.frames * sfInfo.channels;
    short* buffer = new short[bufferSize];
    sf_read_short(sndFile, buffer, bufferSize);
    sf_close(sndFile);

    alGenBuffers(1, &alBuffer);
    ALenum format = (sfInfo.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    alBufferData(alBuffer, format, buffer, bufferSize * sizeof(short), sfInfo.samplerate);
    delete[] buffer;

    alGenSources(1, &alSource);
    alSourcei(alSource, AL_BUFFER, alBuffer);

    float duration = (float)sfInfo.frames / (float)sfInfo.samplerate;
}

void SoundSource::play()
{
    alSourcePlay(alSource);
}

void SoundSource::clean() const
{
    alSourceStop(alSource);
    alDeleteSources(1, &alSource);
    alDeleteBuffers(1, &alBuffer);
}


int SoundEngine::init()
{
    if (device)
    {
        std::cerr << "Audio device already opened" << std::endl;
        return -1;
    }

    device = alcOpenDevice(nullptr);
    if (!device) 
    {
        std::cerr << "Failed to open audio device" << std::endl;
        return -1;
    }

    context = alcCreateContext(device, nullptr);
    if (!context) 
    {
        std::cerr << "Failed to create audio context" << std::endl;
        alcCloseDevice(device);
        return -1;
    }

    alcMakeContextCurrent(context);
    return 0;
}

void SoundEngine::clean()
{
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device); device = nullptr;
}