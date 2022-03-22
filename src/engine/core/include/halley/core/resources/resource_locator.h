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

#include <ctime>
#include <halley/text/halleystring.h>
#include <halley/resources/resource_data.h>
#include <halley/data_structures/hash_map.h>
#include <halley/data_structures/vector.h>
#include "halley/data_structures/maybe.h"

namespace Halley {
	enum class AssetType;
	class ResourceData;
	class SystemAPI;
	class AssetDatabase;

	class IResourceLocatorProvider {
	public:
		virtual ~IResourceLocatorProvider() {}
		virtual std::unique_ptr<ResourceData> getData(const String& path, AssetType type, bool stream) = 0;
		virtual const AssetDatabase& getAssetDatabase() = 0;
		virtual int getPriority() const { return 0; }
		virtual void purge(SystemAPI& system) = 0;
	};

	class ResourceLocator final : public IResourceLocator
	{
	public:
		explicit ResourceLocator(SystemAPI& system);
		void addFileSystem(const Path& path);
		void addPack(const Path& path, const String& encryptionKey = "", bool preLoad = false, bool allowFailure = false, std::optional<int> priority = {});
		Vector<String> getAssetsFromPack(const Path& path, const String& encryptionKey = "") const;
		void removePack(const Path& path);

		const Metadata* getMetaData(const String& resource, AssetType type) const override;

		std::unique_ptr<ResourceDataStatic> getStatic(const String& asset, AssetType type, bool throwOnFail) override;
		std::unique_ptr<ResourceDataStream> getStream(const String& asset, AssetType type, bool throwOnFail) override;

		void purge(const String& asset, AssetType type);
		void purgeAll();

		Vector<String> enumerate(AssetType type);
		bool exists(const String& asset, AssetType type);

		size_t getLocatorCount() const;

	private:
		SystemAPI& system;
		HashMap<String, IResourceLocatorProvider*> locatorPaths;
		HashMap<String, IResourceLocatorProvider*> assetToLocator;
		Vector<std::unique_ptr<IResourceLocatorProvider>> locators;
		static const Metadata dummyMetadata;

		void add(std::unique_ptr<IResourceLocatorProvider> locator, const Path& path);

		std::unique_ptr<ResourceData> getResource(const String& asset, AssetType type, bool stream, bool throwOnFail) const;
		void loadLocatorData(IResourceLocatorProvider& locator);
	};
}
