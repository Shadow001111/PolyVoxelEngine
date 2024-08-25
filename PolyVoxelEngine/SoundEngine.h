#pragma once
#include <AL/al.h>
#include <AL/alc.h>]
#include <iostream>
#include <vector>

class SoundSource
{
	ALuint alSource = 0, alBuffer = 0;
public:
	SoundSource();
	void load(const std::string& path);
	void play() const;
	void stop() const;
	void pause() const;
	void rewind() const;
	void clean();
};

class SoundEngine
{
	static ALCdevice* device;
	static ALCcontext* context;
public:
	static int init();
	static void clean();
};

