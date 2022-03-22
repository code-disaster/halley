#include "halley/tools/packer/asset_packer_task.h"
#include "halley/tools/project/project.h"
#include "halley/support/logger.h"
#include "halley/concurrency/concurrent.h"

using namespace Halley;

AssetPackerTask::AssetPackerTask(Project& project, std::optional<std::set<String>> assetsToPack, Vector<String> deletedAssets)
	: Task("Packing assets", true, true)
	, project(project)
	, assetsToPack(std::move(assetsToPack))
	, deletedAssets(std::move(deletedAssets))
{
}

void AssetPackerTask::run()
{
	Logger::logInfo("Packing assets (" + toString(assetsToPack->size()) + " modified).");
	AssetPacker::pack(project, assetsToPack, deletedAssets, [=] (float p, const String& s) { setProgress(p * 0.95f, s); });
	Logger::logInfo("Done packing assets");

	if (!isCancelled()) {
		setProgress(1.0f, "");

		if (assetsToPack) {
			Concurrent::execute(Executors::getMainUpdateThread(), [project = &project, assets = std::move(assetsToPack)] () {
				project->reloadAssets(assets.value(), true);
			});
		}
	}
}
