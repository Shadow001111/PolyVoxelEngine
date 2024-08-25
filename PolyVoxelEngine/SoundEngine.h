#pragma once
#include <AL/al.h>
#include <AL/alc.h>]
#include <iostream>
#include <vector>

class SoundSource
{
	ALuint alSource, alBuffer;
public:
	SoundSource();
	void load(const std::string& path);
	void play();
	void clean() const;
};

class SoundEngine
{
	static ALCdevice* device;
	static ALCcontext* context;
public:
	static int init();
	static void clean();
};

