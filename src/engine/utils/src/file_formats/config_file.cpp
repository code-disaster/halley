#include "halley/file_formats/config_file.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/support/exception.h"
#include "halley/core/resources/resource_collection.h"
#include "halley/file_formats/yaml_convert.h"
#include "config_file_serialization_state.h"

using namespace Halley;

class ConfigFileSerializationState : public SerializerState {
public:
	bool storeFilePosition = false;
};

ConfigFile::ConfigFile()
{
}

ConfigFile::ConfigFile(const ConfigFile& other)
{
	root = ConfigNode(other.root);
	updateRoot();
}

ConfigFile::ConfigFile(ConfigNode root)
{
	this->root = std::move(root);
	updateRoot();
}

ConfigFile::ConfigFile(ConfigFile&& other) noexcept
{
	root = std::move(other.root);
	updateRoot();
}

ConfigFile& ConfigFile::operator=(ConfigFile&& other) noexcept
{
	root = std::move(other.root);
	updateRoot();
	return *this;
}

ConfigNode& ConfigFile::getRoot()
{
	return root;
}

const ConfigNode& ConfigFile::getRoot() const
{
	return root;
}

constexpr int curVersion = 3;

void ConfigFile::serialize(Serializer& s) const
{
	int version = curVersion;
	s << version;
	s << storeFilePosition;

	ConfigFileSerializationState state;
	state.storeFilePosition = storeFilePosition;
	const auto oldState = s.setState(&state);
	
	s << root;

	s.setState(oldState);
}

void ConfigFile::deserialize(Deserializer& s)
{
	int version;
	s >> version;

	if (version < 2) {
		storeFilePosition = false;
	} else if (version == 2) {
		storeFilePosition = true;
	} else {
		s >> storeFilePosition;
	}
	ConfigFileSerializationState state;
	state.storeFilePosition = storeFilePosition;
	const auto oldState = s.setState(&state);

	s >> root;

	s.setState(oldState);

	updateRoot();
}

size_t ConfigFile::getSizeBytes() const
{
	return root.getSizeBytes();
}

ResourceMemoryUsage ConfigFile::getMemoryUsage() const
{
	ResourceMemoryUsage result;
	result.ramUsage = getSizeBytes();
	return result;
}

std::unique_ptr<ConfigFile> ConfigFile::loadResource(ResourceLoader& loader)
{
	auto data = loader.getStatic(false);
	if (!data) {
		return {};
	}
	
	auto config = std::make_unique<ConfigFile>();
	Deserializer s(data->getSpan());
	s >> *config;

	return config;
}

void ConfigFile::reload(Resource&& resource)
{
	*this = std::move(dynamic_cast<ConfigFile&>(resource));
	updateRoot();
}

void ConfigFile::updateRoot()
{
	root.propagateParentingInformation(this);
}

ConfigObserver::ConfigObserver()
{
}

ConfigObserver::ConfigObserver(const ConfigNode& node)
	: node(&node)
{
}

ConfigObserver::ConfigObserver(const ConfigFile& file)
	: file(&file)
	, node(&file.getRoot())
{
}

const ConfigNode& ConfigObserver::getRoot() const
{
	Expects(node);
	return *node;
}

bool ConfigObserver::needsUpdate() const
{
	return file && assetVersion != file->getAssetVersion();
}

void ConfigObserver::update()
{
	if (file) {
		assetVersion = file->getAssetVersion();
		node = &file->getRoot();
	}
}

String ConfigObserver::getAssetId() const
{
	if (file) {
		return file->getAssetId();
	} else {
		return "";
	}
}

int ConfigObserver::getAssetVersion() const
{
	return assetVersion;
}


thread_local ConfigNode ConfigNode::undefinedConfigNode;
thread_local String ConfigNode::undefinedConfigNodeName;
