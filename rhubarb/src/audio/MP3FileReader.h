#pragma once

#include "AudioClip.h"
#include <boost/filesystem/path.hpp>

class MP3FileReader : public AudioClip
{
public:
	MP3FileReader(const boost::filesystem::path& filePath);
	std::unique_ptr<AudioClip> clone() const override;
	inline int getSampleRate() const override { return sampleRate; }
	inline size_type size() const override { return sampleCount; }

private:
	SampleReader createUnsafeSampleReader() const override;

	boost::filesystem::path filePath;
	int sampleRate;
	int channelCount;
	size_type sampleCount;
};