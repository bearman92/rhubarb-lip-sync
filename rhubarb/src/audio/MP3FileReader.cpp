#include "MP3FileReader.h"

#define MINIMP3_ONLY_MP3
#define MINIMP3_FLOAT_OUTPUT
#define MINIMP3_IMPLEMENTATION
//#define MINIMP3_NO_STDIO

#include "minimp3_ex.h"
#include <format.h>
#include "tools/fileTools.h"

using boost::filesystem::path;
using std::vector;
using std::make_shared;
using std::ifstream;
using std::ios_base;

template<typename T>
T throwOnError(T code) {
	// OV_HOLE, though technically an error code, is only informational
	const bool error = code < 0;
	if (error) {
		const std::string message = fmt::format("minimp3 error {}", code);
		throw std::runtime_error(message);
	}
	return code;
}

class MP3File final 
{
public:
	MP3File::MP3File(const path& filePath)
	{
		int errCode = mp3dec_load_w(&mp3d, filePath.c_str(), &info, NULL, NULL);
		throwOnError(errCode);

		mp3d_sample_t* monoBuffer = nullptr;

		//Downmix to mono

		monoBuffer = (mp3d_sample_t*)malloc((info.samples / info.channels) * sizeof(mp3d_sample_t));
		mp3d_sample_t sum = 0.f;

		for (size_t i = 0; i < info.samples; i += info.channels)
		{
			sum = 0.f;

			for (int channel = 0; channel < info.channels; ++channel)
			{
				sum += info.buffer[i];
			}

			monoBuffer[i / info.channels] = sum / info.channels;
		}

		free(info.buffer);

		info.buffer = monoBuffer;
		info.samples = info.samples / info.channels;
		info.channels = 1;

		auto file = std::ofstream("C:\\tst\\pcm.pcm", std::ios::binary);
		file.write((char*)info.buffer, info.samples * sizeof(mp3d_sample_t));
		file.close();
	}

	MP3File(const MP3File&) = delete;
	MP3File& operator=(const MP3File&) = delete;

	mp3dec_file_info_t* get()
	{
		return &info;
	}

	~MP3File()
	{
		free(info.buffer);
	}

private:
	mp3dec_file_info_t info;
	mp3dec_t mp3d;
};

MP3FileReader::MP3FileReader(const boost::filesystem::path& filePath)
	: filePath(filePath)
{
	MP3File file(filePath);

	channelCount = file.get()->channels;
	sampleRate = file.get()->hz;
	sampleCount = file.get()->samples;
}

std::unique_ptr<AudioClip> MP3FileReader::clone() const
{
	return std::make_unique<MP3FileReader>(*this);
}

SampleReader MP3FileReader::createUnsafeSampleReader() const {
	return [
		channelCount = channelCount,
			file = make_shared<MP3File>(filePath),
			buffer = static_cast<mp3d_sample_t*>(nullptr),
			bufferStart = size_type(0),
			bufferSize = size_type(0)
	] (size_type index) mutable {
			/*if (index < bufferStart || index >= bufferStart + bufferSize) {
				mp3dec_frame_info_t frame_info;
				// Seek
				throwOnError(mp3dec_ex_seek(file->get(), index));

				constexpr int maxSize = 1024;

				bufferStart = index;
				bufferSize = mp3dec_ex_read_frame(file->get(), &buffer, &frame_info, maxSize);

				//file->pcmDebug.write((char*)buffer, bufferSize * sizeof(mp3d_sample_t));

				if (bufferSize == 0) {
					throw std::runtime_error("Unexpected end of file.");
				}
			}

			const size_type bufferIndex = index - bufferStart;

			mp3d_sample_t sum = 0.0f;
			for (int channel = 0; channel < channelCount; ++channel) {
				sum += buffer[bufferIndex + channel];
			}
			sum = sum / channelCount;

			return sum;*/

			return file->get()->buffer[index];
		};
}
