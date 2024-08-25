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

	void setPosition(float x, float y, float z) const;
	void setVelocity(float x, float y, float z) const;
	void setDirection(float x, float y, float z) const;
	void setVolume(float volume) const;
	void setPitch(float pitch) const;
	void setLooping(bool looping) const;
	void setReferenceDistance(float distance) const;
	void setRolloffFactor(float factor) const;
	void setMaxDistance(float distance) const;
	void setMinGain(float gain) const;
	void setMaxGain(float gain) const;
	void setRelativeMode(bool relative) const;
	void setConeInnerAngle(float angle) const;
	void setConeOuterAngle(float angle) const;
	void setConeOuterGain(float gain) const;
	void setDopplerFactor(float factor) const;

	ALenum getState() const;
};

class SoundEngine
{
	static ALCdevice* device;
	static ALCcontext* context;
public:
	static int init();
	static void clean();
};

