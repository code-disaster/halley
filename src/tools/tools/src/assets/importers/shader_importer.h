#pragma once
#include "halley/plugin/iasset_importer.h"

namespace Halley
{
	enum class ShaderType;

	class ShaderImporter : public IAssetImporter
	{
	public:
		ImportAssetType getType() const override { return ImportAssetType::Shader; }
		void import(const ImportingAsset& asset, IAssetCollector& collector) override;

		static Bytes convertHLSL(const String& name, ShaderType type, const Bytes& data, const String& dstLanguage);
		static Bytes compileHLSL(const String& name, ShaderType type, const Bytes& data);

	private:
		static void patchGLSL(const String& name, ShaderType type, Bytes& data);
	};
}
