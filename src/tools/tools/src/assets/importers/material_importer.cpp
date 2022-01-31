#include "material_importer.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/tools/file/filesystem.h"
#include "halley/text/string_converter.h"
#include "shader_importer.h"
#include "halley/core/graphics/shader.h"
#include "halley/file_formats/yaml_convert.h"

using namespace Halley;

void MaterialImporter::import(const ImportingAsset& asset, IAssetCollector& collector)
{
	Path basePath = asset.inputFiles.at(0).name.parentPath();
	auto material = parseMaterial(basePath, gsl::as_bytes(gsl::span<const Byte>(asset.inputFiles.at(0).data)), collector);
	collector.output(material.getName(), AssetType::MaterialDefinition, Serializer::toBytes(material));
}

MaterialDefinition MaterialImporter::parseMaterial(Path basePath, gsl::span<const gsl::byte> data, IAssetCollector& collector) const
{
	auto config = YAMLConvert::parseConfig(data);
	const auto& root = config.getRoot();

	// Load base material
	MaterialDefinition material;
	if (root.hasKey("base")) {
		String baseName = root["base"].asString();
		auto otherData = collector.readAdditionalFile(basePath / baseName);
		material = parseMaterial(basePath, gsl::as_bytes(gsl::span<Byte>(otherData)), collector);
	}
	material.load(root);

	// Load passes
	int passN = 0;
	if (root.hasKey("passes")) {
		for (auto& passNode: root["passes"].asSequence()) {
			loadPass(material, passNode, collector, passN++);
		}
	}

	return material;
}

void MaterialImporter::loadPass(MaterialDefinition& material, const ConfigNode& node, IAssetCollector& collector, int passN)
{
	const String passName = material.getName() + "_pass_" + toString(passN);
	const String shaderName = passName;

	const auto shaderTypes = { ShaderType::Pixel, ShaderType::Vertex, ShaderType::Geometry, ShaderType::Combined };
	const String languages[] = { "hlsl", "glsl", "metal", "spirv" };

	// Map languages to nodes
	std::map<String, const ConfigNode*> langToNode;
	const ConfigNode* defaultNode = nullptr;
	for (auto& shaderEntry: node["shader"]) {
		const auto language = shaderEntry["language"].asString();
		langToNode[language] = &shaderEntry;
		if (language == "hlsl") {
			defaultNode = &shaderEntry;
		}
	}

	// Generate each language
	for (auto& language: languages) {
		const auto iter = langToNode.find(language);
		const bool hasEntry = iter != langToNode.end();
		const ConfigNode* shaderEntryPtr = hasEntry ? iter->second : defaultNode;
		if (!shaderEntryPtr) {
			throw Exception("No shader for " + language + " in " + passName, HalleyExceptions::Tools);
		}
		const ConfigNode& shaderEntry = *shaderEntryPtr;
		
		ImportingAsset shaderAsset;
		shaderAsset.assetId = shaderName + ":" + language;
		shaderAsset.assetType = ImportAssetType::Shader;
		for (auto& curType: shaderTypes) {
			String curTypeName = toString(curType);
			if (shaderEntry.hasKey(curTypeName)) {
				auto data = loadShader(shaderEntry[curTypeName].asString(), collector);
				if (!hasEntry) {
					data = ShaderImporter::convertHLSL(shaderName, curType, data, language);
				}
				
				Metadata meta;
				meta.set("language", language);
				shaderAsset.inputFiles.emplace_back(ImportingAssetFile(shaderName + "." + curTypeName, std::move(data), meta));
			}
		}
		collector.addAdditionalAsset(std::move(shaderAsset));
	}

	material.addPass(MaterialPass(passName, node));
}

Bytes MaterialImporter::loadShader(const String& name, IAssetCollector& collector)
{
	std::set<String> loaded;
	return doLoadShader(name, collector, loaded);
}

Bytes MaterialImporter::doLoadShader(const String& name, IAssetCollector& collector, std::set<String>& loaded)
{
	Bytes finalResult;
	const auto appendLine = [&] (const void* data, size_t size)
	{
		const size_t curSize = finalResult.size();
		finalResult.resize(curSize + size + 1);
		memcpy(finalResult.data() + curSize, data, size);
		memcpy(finalResult.data() + curSize + size, "\n", 1);
	};

	Bytes rawData = collector.readAdditionalFile("shader/" + name);
	String strData = String(reinterpret_cast<const char*>(rawData.data()), rawData.size());
	auto lines = strData.split('\n');
	for (size_t i = 0; i < lines.size(); ++i) {
		const auto& curLine = lines[i];
		if (curLine.startsWith("#include")) {
			auto words = curLine.split(' ');
			auto quoted = words.at(1).trimBoth();
			if (quoted.startsWith("\"") && quoted.endsWith("\"")) {
				auto includeFile = quoted.mid(1, quoted.size() - 2);
				if (loaded.find(includeFile) == loaded.end()) {
					loaded.insert(includeFile);
					auto includeData = doLoadShader(includeFile, collector, loaded);
					appendLine(includeData.data(), includeData.size());
				}
			} else if (!(quoted.startsWith("<") && quoted.endsWith(">"))) {
				throw Exception("Invalid syntax in #include in shader", HalleyExceptions::Tools);
			} else {
				appendLine(curLine.c_str(), curLine.size());
			}
		} else {
			appendLine(curLine.c_str(), curLine.size());
		}
	}

	return finalResult;
}
