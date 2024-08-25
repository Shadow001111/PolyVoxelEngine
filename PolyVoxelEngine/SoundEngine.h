#pragma once
#include <AL/al.h>
#include <AL/alc.h>]
#include <iostream>
#include <vector>

class SoundSource
{
	ALuint source, buffer;
public:
	float deleteTime;

	SoundSource(ALuint source, ALuint buffer, float duration);
	void clean() const;
};

class SoundEngine
{
	static ALCdevice* device;
	static ALCcontext* context;

	static std::vector<SoundSource> soundSources;
public:
	static int init();
	static void clean();

	static void playSound(const std::string& path);

	static void removeEndedSoundSources();
};

