/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#include "halley/utils/utils.h"
#include "halley/audio/vorbis_dec.h"

#include "halley/support/exception.h"
#include "halley/resources/resource_data.h"

#include "../../../../../contrib/stb_vorbis/stb_vorbis.h"

using namespace Halley;

static void onVorbisError(int error)
{
	String str;
	switch (error) {
	case VORBIS_outofmem: str = "Not enough memory."; break;
	case VORBIS_feature_not_supported: str = "Feature not supported."; break;
	case VORBIS_too_many_channels: str = "Too many channels."; break;
	case VORBIS_file_open_failure: str = "File open failed."; break;
	case VORBIS_seek_without_length: str = "Can't seek in unknown-length file."; break;
	case VORBIS_unexpected_eof: str = "Unexpected end of file."; break;
	case VORBIS_seek_invalid: str = "Seek past end of file."; break;
	default: str = "Unknown error.";
	}
	throw Exception("Error opening Ogg Vorbis: "+str, HalleyExceptions::Resources);
}

VorbisData::VorbisData(std::shared_ptr<ResourceData> resource, size_t numSamples, bool doOpen)
	: resource(resource)
	, file(nullptr)
	, numSamples(numSamples)
	, streaming(std::dynamic_pointer_cast<ResourceDataStream>(resource))
{
	if (doOpen) {
		open();
	}
}

VorbisData::~VorbisData()
{
	close();
}

void VorbisData::open()
{
	close();

	if (stream && !stream->isAvailable()) {
		error = true;
		return;
	}

	if (error) {
		return;
	}

	if (streaming) {
		stream = std::dynamic_pointer_cast<ResourceDataStream>(resource)->getReader();
	}

	if (!vorbisOpen() || !vorbisGetNumSamples()) {
		error = true;
		close();
	}
}

void VorbisData::close()
{
	if (file) {
		stb_vorbis_close(file);
		file = nullptr;
	}
}

void VorbisData::reset()
{
	close();
	open();
}

size_t VorbisData::read(gsl::span<Vector<float>> dst)
{
	AudioMultiChannelSamples samplesSpan;
	for (size_t i = 0; i < dst.size(); ++i) {
		samplesSpan[i] = dst[i];
	}
	return read(samplesSpan, dst.size());
}

size_t VorbisData::read(AudioMultiChannelSamples dst, size_t nChannels)
{
	if (!file) {
		open();
	}

	if (stream && !stream->isAvailable()) {
		error = true;
		close();
	}

	if (error) {
		return 0;
	}

	Expects(nChannels == getNumChannels());

	int totalRead = 0;
	int toReadLeft = int(dst[0].size());

	// Consume any left-over sample data from the previous call.
	if (pcmSamplesRead < pcmSamplesTotal) {
		const int samples = std::min(pcmSamplesTotal - pcmSamplesRead, toReadLeft);
		for (size_t i = 0; i < nChannels; ++i) {
			memcpy(dst[i].data(), &pcmData[i][pcmSamplesRead], samples * sizeof(float));
		}
		totalRead = samples;
		toReadLeft -=  samples;
		pcmSamplesRead += samples;
	}

	if (toReadLeft > 0) {
		Ensures(pcmSamplesRead == pcmSamplesTotal);
	}

	if (streaming) {
		while (toReadLeft > 0) {
			constexpr int streamBufLen = sizeof(streamBuf);
			int nRead = int(stream->read(gsl::as_writable_bytes(gsl::span(&streamBuf[streamBufUsed], streamBufLen - streamBufUsed))));

			const int numBytesConsumed = stb_vorbis_decode_frame_pushdata(file, streamBuf, streamBufUsed + nRead, nullptr, &pcmData, &pcmSamplesTotal);

			int e = stb_vorbis_get_error(file);
			if (e != VORBIS__no_error && e != VORBIS_need_more_data) {
				int x =1;
			}

			if (numBytesConsumed == 0) {
				break;
			} else {
				if (pcmSamplesTotal > 0) {
					const int samples = std::min(pcmSamplesTotal, toReadLeft);
					for (size_t i = 0; i < nChannels; ++i) {
						memcpy(dst[i].data() + totalRead, pcmData[i], samples * sizeof(float));
					}
					totalRead += samples;
					toReadLeft -= samples;
					pcmSamplesRead = samples;
				} else {
					
				}
				if (numBytesConsumed < streamBufUsed + nRead) {
					memmove(streamBuf, &streamBuf[numBytesConsumed], streamBufUsed + nRead - numBytesConsumed);
				}
				streamBufUsed = streamBufUsed + nRead - numBytesConsumed;
			}
		}
	} else {
		while (toReadLeft > 0) {
			pcmSamplesTotal = stb_vorbis_get_frame_float(file, nullptr, &pcmData);

			if (pcmSamplesTotal == 0) {
				break;
			} else {
				const int samples = std::min(pcmSamplesTotal, toReadLeft);
				for (size_t i = 0; i < nChannels; ++i) {
					memcpy(dst[i].data() + totalRead, pcmData[i], samples * sizeof(float));
				}
				totalRead += samples;
				toReadLeft -= samples;
				pcmSamplesRead = samples;
			}
		}
	}

	samplePos += totalRead;

	return totalRead;
}

size_t VorbisData::getNumSamples() const
{
	if (error || (stream && !stream->isAvailable())) {
		return 0;
	}

	Expects(file);
	return numSamples;
}

int VorbisData::getSampleRate() const
{
	if (error || (stream && !stream->isAvailable())) {
		return 0;
	}

	Expects(file);
	const stb_vorbis_info info = stb_vorbis_get_info(file);
	return info.sample_rate;
}

int VorbisData::getNumChannels() const
{
	if (error || (stream && !stream->isAvailable())) {
		return 0;
	}

	Expects(file);
	const stb_vorbis_info info = stb_vorbis_get_info(file);
	return info.channels;
}

void VorbisData::seek(double t)
{
	if (!file) {
		open();
	}
	if (error) {
		return;
	}
	vorbisSeek(int(t * getSampleRate()));
}

void VorbisData::seek(size_t sample)
{
	if (!file) {
		open();
	}
	if (error) {
		return;
	}
	vorbisSeek(int(sample));
}

size_t VorbisData::tell() const
{
	if (file) {
		return samplePos;
	} else {
		return 0;
	}
}

size_t VorbisData::getSizeBytes() const
{
	if (streaming) {
		return sizeof(ResourceDataStream);
	} else {
		auto res = std::dynamic_pointer_cast<ResourceDataStatic>(resource);
		return res->getSize() + sizeof(ResourceDataStatic);
	}
}

String VorbisData::getResourcePath() const
{
	return resource->getPath();
}

bool VorbisData::vorbisOpen()
{
	int e = VORBIS__no_error;

	if (streaming) {
		uint8_t dataBlock[8192];
		constexpr int dataBlockLen = sizeof(dataBlock);

		int nRead = int(stream->read(gsl::as_writable_bytes(gsl::span(dataBlock, dataBlockLen))));

		file = stb_vorbis_open_pushdata(dataBlock, nRead, &streamFirstFrameOffset, &e, nullptr);

		if (file != nullptr) {
			stream->seek(streamFirstFrameOffset, SEEK_SET);
		}
	} else {
		const auto res = std::dynamic_pointer_cast<ResourceDataStatic>(resource);

		file = stb_vorbis_open_memory(static_cast<const uint8_t*>(res->getData()), int(res->getSize()), &e, nullptr);

		if (file != nullptr) {
			stb_vorbis_seek_start(file);
		}
	}

	if (e != VORBIS__no_error) {
		onVorbisError(e);
		return false;
	}

	return true;
}

bool VorbisData::vorbisGetNumSamples()
{
	if (numSamples > 0) {
		return true;
	}

	int e = VORBIS__no_error;

	if (streaming) {
		throw Exception("Vorbis stream length should be queried during asset import.", HalleyExceptions::Resources);
#if 0
		// TODO: Unnecessary This should be done by the asset importer.
		uint8_t dataBlock[8192];
		constexpr int dataBlockLen = sizeof(dataBlock);
		int dataBlockUsed = 0;

		stb_vorbis_flush_pushdata(file);

		while ((e = stb_vorbis_get_error(file)) == VORBIS__no_error) {
			int nRead = int(stream->read(gsl::as_writable_bytes(gsl::span(&dataBlock[dataBlockUsed], dataBlockLen - dataBlockUsed))));
			dataBlockUsed += nRead;

			float **pcm;
			int samples;

			const int numBytesConsumed = stb_vorbis_decode_frame_pushdata(file, dataBlock, dataBlockUsed, nullptr, &pcm, &samples);

			if (numBytesConsumed == 0) {
				break;
			} else {
				if (samples > 0) {
					int offset = stb_vorbis_get_sample_offset(file);
					if (offset != -1) {
						numSamples = offset;
					}
				} else {
					
				}
				if (numBytesConsumed < dataBlockUsed) {
					memmove(dataBlock, &dataBlock[numBytesConsumed], dataBlockLen - numBytesConsumed);
				}
				dataBlockUsed -= numBytesConsumed;
			}
		}

		stream->seek(streamFirstFrameOffset, SEEK_SET);
		stb_vorbis_flush_pushdata(file);
#endif
	} else {
		// Non-streaming, simply ask the pulling API.
		numSamples = stb_vorbis_stream_length_in_samples(file);
	}

	if (e != VORBIS__no_error) {
		onVorbisError(e);
		return false;
	}

	return true;
}

void VorbisData::vorbisSeek(int pos)
{
	Ensures(streaming);

	if (samplePos > pos) {
		stream->seek(streamFirstFrameOffset, SEEK_SET);
		stb_vorbis_flush_pushdata(file);
		samplePos = 0;
		streamBufUsed = 0;
	}

	while (samplePos < pos) {
		constexpr int streamBufLen = sizeof(streamBuf);
		int nRead = int(stream->read(gsl::as_writable_bytes(gsl::span(&streamBuf[streamBufUsed], streamBufLen - streamBufUsed))));

		const int numBytesConsumed = stb_vorbis_decode_frame_pushdata(file, streamBuf, streamBufUsed + nRead, nullptr, &pcmData, &pcmSamplesTotal);

		if (numBytesConsumed == 0) {
			break;
		} else {
			if (numBytesConsumed < streamBufUsed + nRead) {
				memmove(streamBuf, &streamBuf[numBytesConsumed], streamBufUsed + nRead - numBytesConsumed);
			}
			streamBufUsed = streamBufUsed + nRead - numBytesConsumed;
		}

		samplePos += pcmSamplesTotal;
	}

	if (samplePos > pos) {
		pcmSamplesRead = int(samplePos) - pos;
		samplePos = pos;
	} else {
		pcmSamplesRead = 0;
	}
}
