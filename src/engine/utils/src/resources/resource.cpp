#include "halley/resources/resource.h"
#include "halley/support/exception.h"

using namespace Halley;

Resource::~Resource() = default;

void Resource::setMeta(Metadata m)
{
	// Only allow setting meta once to avoid a race condition
	// For example, resource_collection.cpp would try to set a meta on a Texture while texture.cpp is trying to load it and referencing its data
	if (!metaSet) {
		meta = std::move(m);
		metaSet = true;
	}
}

void Resource::setAssetId(String id)
{
	assetId = std::move(id);
}

void Resource::onLoaded(Resources& resources)
{
}

void Resource::increaseAssetVersion()
{
	++assetVersion;
}

void Resource::reloadResource(Resource&& resource)
{
	++assetVersion;
	resource.assetVersion = assetVersion;
	reload(std::move(resource));
}

void Resource::reload(Resource&& resource)
{
}

ResourceObserver::ResourceObserver()
{
}

ResourceObserver::ResourceObserver(const Resource& res)
{
	startObserving(res);
}

ResourceObserver::~ResourceObserver()
{
	stopObserving();
}

void ResourceObserver::startObserving(const Resource& r)
{
	res = &r;
	assetVersion = res->getAssetVersion();
}

void ResourceObserver::stopObserving()
{
	res = nullptr;
	assetVersion = 0;
}

bool ResourceObserver::needsUpdate() const
{
	return res && res->getAssetVersion() != assetVersion;
}

void ResourceObserver::update()
{
	assetVersion = res->getAssetVersion();
}

const Resource* ResourceObserver::getResourceBeingObserved() const
{
	return res;
}

AsyncResource::AsyncResource() 
	: failed(false)
	, loading(false)
{}

AsyncResource::~AsyncResource()
{
	waitForLoad(true);
}

AsyncResource::AsyncResource(const AsyncResource& other)
{
	other.waitForLoad(true);
	*this = other;
}

AsyncResource::AsyncResource(AsyncResource&& other) noexcept
{
	other.waitForLoad(true);
	*this = std::move(other);
}

AsyncResource& AsyncResource::operator=(const AsyncResource& other)
{
	other.waitForLoad(true);
	failed.store(other.failed);
	Resource::operator=(static_cast<const Resource&>(other));
	return *this;
}

AsyncResource& AsyncResource::operator=(AsyncResource&& other) noexcept
{
	other.waitForLoad(true);
	failed.store(other.failed);
	Resource::operator=(static_cast<Resource&&>(other));
	return *this;
}

void AsyncResource::startLoading()
{
	loading = true;
	failed = false;
}

void AsyncResource::doneLoading()
{
	{
		std::unique_lock<std::mutex> lock(loadMutex);
		loading = false;
	}
	loadWait.notify_all();
}

void AsyncResource::loadingFailed()
{
	failed = true;
	doneLoading();
}

void AsyncResource::waitForLoad(bool acceptFailed) const
{
	if (loading) {
		std::unique_lock<std::mutex> lock(loadMutex);
		while (loading) {
			loadWait.wait(lock);
		}
	}
	if (failed && !acceptFailed) {
		throw Exception("Resource failed to load.", HalleyExceptions::Resources);
	}
}

bool AsyncResource::isLoaded() const
{
	return !loading;
}

bool AsyncResource::hasSucceeded() const
{
	return !failed;
}

bool AsyncResource::hasFailed() const
{
	return failed;
}
