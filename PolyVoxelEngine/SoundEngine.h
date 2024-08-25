#include <AL/al.h>
#include <AL/alc.h>
#include <iostream>

#pragma once
class SoundEngine
{
	static ALCdevice* device;
	static ALCcontext* context;
public:
	static int init();
	static void clean();
};

