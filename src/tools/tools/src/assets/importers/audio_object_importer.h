#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	class AudioObjectImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::AudioObject; }

		void import(const ImportingAsset& asset, IAssetCollector& collector) override;
	};
}
