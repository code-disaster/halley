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

#pragma once

#include "halley/api/audio_api.h"
#include "halley/data_structures/vector.h"

struct stb_vorbis;

namespace Halley {
	class ResourceData;
	class ResourceDataReader;

	class VorbisData {
	public:
		VorbisData(std::shared_ptr<ResourceData> resource, size_t numSamples, bool open);
		~VorbisData();

		size_t read(gsl::span<Vector<float>> dst);
		size_t read(AudioMultiChannelSamples dst, size_t nChannels);

		size_t getNumSamples() const; // Per channel
		int getSampleRate() const;
		int getNumChannels() const;

		void close();
		void reset();
		void seek(double t);
		void seek(size_t sample);
		size_t tell() const;

		size_t getSizeBytes() const;
		String getResourcePath() const;

	private:
		void open();
		bool vorbisOpen();
		bool vorbisGetNumSamples();
		void vorbisSeek(int pos);

		std::shared_ptr<ResourceData> resource;
		std::shared_ptr<ResourceDataReader> stream;

		stb_vorbis* file;
		size_t numSamples = 0;
		size_t samplePos = 0;

		uint8_t streamBuf[8192];
		int streamBufUsed = 0;
		int streamFirstFrameOffset = 0;

		float** pcmData;
		int pcmSamplesRead = 0;
		int pcmSamplesTotal = 0;

		bool streaming;
		bool error = false;
	};
}
