#pragma once
#include <AL/al.h>
#include <AL/alc.h>
#include <vector>

namespace Sound
{
	class SoundSource
	{
		friend class SoundEngine;

		static std::vector<SoundSource*> soundSources;

		ALuint alSource = 0, alBuffer = 0;
	public:
		SoundSource();
		~SoundSource();

		void load(const char* path);
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

		static void stopAllSounds();

		static void setListenerPosition(float x, float y, float z);
		static void setListenerVelocity(float x, float y, float z);
		static void setListenerOrientation(float forwardX, float forwardY, float forwardZ, float upX, float upY, float upZ);
	};
}

