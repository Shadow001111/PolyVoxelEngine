#include "SoundEngine.h"
#include <sndfile.h>
#include <thread>
#include <chrono>
#include <GLFW/glfw3.h>


ALCdevice* SoundEngine::device = nullptr;
ALCcontext* SoundEngine::context = nullptr;
std::vector<SoundSource> SoundEngine::soundSources;

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

void SoundEngine::playSound(const std::string& path)
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

    ALuint alBuffer;
    alGenBuffers(1, &alBuffer);
    ALenum format = (sfInfo.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    alBufferData(alBuffer, format, buffer, bufferSize * sizeof(short), sfInfo.samplerate);
    delete[] buffer;

    ALuint alSource;
    alGenSources(1, &alSource);
    alSourcei(alSource, AL_BUFFER, alBuffer);

    float duration = (float)sfInfo.frames / (float)sfInfo.samplerate;
    soundSources.emplace_back(alSource, alBuffer, duration);
}

void SoundEngine::removeEndedSoundSources()
{
    float time = glfwGetTime();

    auto new_end = std::remove_if(soundSources.begin(), soundSources.end(), [time](SoundSource& obj) {
        if (time >= obj.deleteTime) 
        {
            obj.clean();
            return true;
        }
        return false;
        });

    soundSources.erase(new_end, soundSources.end());
}

SoundSource::SoundSource(ALuint source, ALuint buffer, float duration) : source(source), buffer(buffer)
{
    deleteTime = (float)glfwGetTime() + duration;
    alSourcePlay(source);
}

void SoundSource::clean() const
{
    alSourceStop(source);
    alDeleteSources(1, &source);
    alDeleteBuffers(1, &buffer);
}
