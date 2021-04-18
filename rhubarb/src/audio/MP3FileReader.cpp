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
	if (code < 0) {
		switch (code)
		{
		case MP3D_E_PARAM:
			throw std::runtime_error("minimp3 error: MP3D_E_PARAM");
			break;
		case MP3D_E_MEMORY:
			throw std::runtime_error("minimp3 error: MP3D_E_MEMORY");
			break;
		case MP3D_E_IOERROR:
			throw std::runtime_error("minimp3 error: MP3D_E_IOERROR");
			break;
		case MP3D_E_USER:
			throw std::runtime_error("minimp3 error: MP3D_E_USER");
			break;
		case MP3D_E_DECODE:
			throw std::runtime_error("minimp3 error: MP3D_E_DECODE");
			break;
		}
	}
	return code;
}

class MP3File final 
{
public:
	MP3File::MP3File(const path& filePath)
	{
		throwOnError(mp3dec_ex_open_w(&dec, filePath.c_str(), MP3D_SEEK_TO_SAMPLE));
	}

	MP3File(const MP3File&) = delete;
	MP3File& operator=(const MP3File&) = delete;

	mp3dec_ex_t* get()
	{
		return &dec;
	}

	~MP3File()
	{
		
	}

private:
	mp3dec_ex_t dec;
};

MP3FileReader::MP3FileReader(const boost::filesystem::path& filePath)
	: filePath(filePath)
{
	MP3File file(filePath);

	channelCount = file.get()->info.channels;
	sampleRate = file.get()->info.hz;
	sampleCount = file.get()->samples / file.get()->info.channels;
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
			if (index < bufferStart || index >= bufferStart + bufferSize) {
				mp3dec_frame_info_t frame_info;

				throwOnError(mp3dec_ex_seek(file->get(), index * channelCount));

				constexpr int maxSize = 1024;

				bufferStart = index;
				bufferSize = mp3dec_ex_read_frame(file->get(), &buffer, &frame_info, maxSize) / channelCount;

				if (bufferSize == 0) {
					throw std::runtime_error("Unexpected end of file.");
				}
			}

			const size_type bufferIndex = index - bufferStart;

			mp3d_sample_t sum = 0.0f;
			for (int channel = 0; channel < channelCount; channel++) {
				sum += buffer[(bufferIndex * channelCount) + channel];
			}
			sum = sum / channelCount;

			return sum;

			return file->get()->buffer[index];
		};
}
