#pragma once
#include "halley/text/halleystring.h"
#include "halley/data_structures/maybe.h"
#include "halley/utils/utils.h"

namespace Halley {
	class ConfigNode;
	class ConfigFile;

	class AssetPackManifestEntry {
	public:
		AssetPackManifestEntry();
		AssetPackManifestEntry(const ConfigNode& node);

		const String& getName() const;
		bool checkMatch(const String& asset) const;
		bool isEncrypted() const;
		const String& getEncryptionKey() const;

	private:
		String name;
		String encryptionKey;
		Vector<String> matches;
	};

	class AssetPackManifest {
	public:
		explicit AssetPackManifest(const Bytes& data);
		explicit AssetPackManifest(const ConfigFile& file);

		void load(const ConfigFile& file);

		std::optional<std::reference_wrapper<const AssetPackManifestEntry>> getPack(const String& asset) const;

	private:
		Vector<String> exclude;
		Vector<AssetPackManifestEntry> packs;
	};
}
