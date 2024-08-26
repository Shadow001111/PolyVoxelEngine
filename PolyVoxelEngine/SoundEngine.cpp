#include "SoundEngine.h"
#include <sndfile.h>
#include <thread>
#include <chrono>
#include <GLFW/glfw3.h>

std::vector<SoundSource*> SoundSource::soundSources;

SoundSource::SoundSource() : alSource(0), alBuffer(0)
{
    alGenSources(1, &alSource);
    soundSources.push_back(this);
}

SoundSource::~SoundSource()
{
    auto it = std::find(soundSources.begin(), soundSources.end(), this);
    if (it == soundSources.end())
    {
        std::cerr << "SoundSource wasn't in vector" << std::endl;
    }
    else
    {
        soundSources.erase(it);
    }
    clean();
}

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

    if (alBuffer != 0)
    {
        alDeleteBuffers(1, &alBuffer);
    }
    alGenBuffers(1, &alBuffer);
    ALenum format = (sfInfo.channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    alBufferData(alBuffer, format, buffer, bufferSize * sizeof(short), sfInfo.samplerate);
    delete[] buffer;

    if (alSource == 0)
    {
        alGenSources(1, &alSource);
    }
    alSourcei(alSource, AL_BUFFER, alBuffer);

    float duration = (float)sfInfo.frames / (float)sfInfo.samplerate;
}

void SoundSource::play() const
{
    alSourcePlay(alSource);
}

void SoundSource::stop() const
{
    alSourceStop(alSource);
}

void SoundSource::pause() const
{
    alSourcePause(alSource);
}

void SoundSource::rewind() const
{
    alSourceRewind(alSource);
}

void SoundSource::clean()
{
    alSourceStop(alSource);
    alDeleteSources(1, &alSource);
    alDeleteBuffers(1, &alBuffer);

    alSource = 0;
    alBuffer = 0;
}

void SoundSource::setPosition(float x, float y, float z) const
{
    alSource3f(alSource, AL_POSITION, x, y, z);
}

void SoundSource::setVelocity(float x, float y, float z) const
{
    alSource3f(alSource, AL_VELOCITY, x, y, z);
}

void SoundSource::setDirection(float x, float y, float z) const
{
    alSource3f(alSource, AL_DIRECTION, x, y, z);
}

void SoundSource::setVolume(float volume) const
{
    alSourcef(alSource, AL_GAIN, volume);
}

void SoundSource::setPitch(float pitch) const
{
    alSourcef(alSource, AL_PITCH, pitch);
}

void SoundSource::setLooping(bool looping) const
{
    alSourcei(alSource, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
}

void SoundSource::setReferenceDistance(float distance) const
{
    alSourcef(alSource, AL_REFERENCE_DISTANCE, distance);
}

void SoundSource::setRolloffFactor(float factor) const
{
    alSourcef(alSource, AL_ROLLOFF_FACTOR, factor);
}

void SoundSource::setMaxDistance(float distance) const
{
    alSourcef(alSource, AL_MAX_DISTANCE, distance);
}

void SoundSource::setMinGain(float gain) const
{
    alSourcef(alSource, AL_MIN_GAIN, gain);
}

void SoundSource::setMaxGain(float gain) const
{
    alSourcef(alSource, AL_MAX_GAIN, gain);
}

void SoundSource::setRelativeMode(bool relative) const
{
    alSourcei(alSource, AL_SOURCE_RELATIVE, relative ? AL_TRUE : AL_FALSE);
}

void SoundSource::setConeInnerAngle(float angle) const
{
    alSourcef(alSource, AL_CONE_INNER_ANGLE, angle);
}

void SoundSource::setConeOuterAngle(float angle) const
{
    alSourcef(alSource, AL_CONE_OUTER_ANGLE, angle);
}

void SoundSource::setConeOuterGain(float gain) const
{
    alSourcef(alSource, AL_CONE_OUTER_GAIN, gain);
}

void SoundSource::setDopplerFactor(float factor) const
{
    alSourcef(alSource, AL_DOPPLER_FACTOR, factor);
}

ALenum SoundSource::getState() const
{
    ALenum state;
    alGetSourcei(alSource, AL_SOURCE_STATE, &state);
    return state;
}


ALCdevice* SoundEngine::device = nullptr;
ALCcontext* SoundEngine::context = nullptr;

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

    // settings
    alSpeedOfSound(1000.0f);
    alDistanceModel(AL_LINEAR_DISTANCE_CLAMPED);

    return 0;
}

void SoundEngine::clean()
{
    alcMakeContextCurrent(nullptr);
    alcDestroyContext(context);
    alcCloseDevice(device); device = nullptr;
}

void SoundEngine::stopAllSounds()
{
    for (const SoundSource* soundSource : SoundSource::soundSources)
    {
        soundSource->stop();
    }
}

void SoundEngine::setListenerPosition(float x, float y, float z)
{
    alListener3f(AL_POSITION, x, y, z);
}

void SoundEngine::setListenerVelocity(float x, float y, float z)
{
    alListener3f(AL_VELOCITY, x, y, z);
}

void SoundEngine::setListenerOrientation(float forwardX, float forwardY, float forwardZ, float upX, float upY, float upZ)
{
    ALfloat orientation[] = { forwardX, forwardY, forwardZ, upX, upY, upZ };
    alListenerfv(AL_ORIENTATION, orientation);
}
