#pragma once
#include "halley/plugin/iasset_importer.h"
#include <gsl/gsl>

namespace Halley
{
	class AudioImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::AudioClip; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

	private:
		Bytes encodeVorbis(int channels, int sampleRate, gsl::span<const Vector<float>> src);
		static Vector<float> resampleChannel(int from, int to, gsl::span<const float> src);
	};
}
