#include "SoundEngine.h"

ALCdevice* SoundEngine::device = nullptr;
ALCcontext* SoundEngine::context = nullptr;

int SoundEngine::init()
{
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
    alcCloseDevice(device);
}
