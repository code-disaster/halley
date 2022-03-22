#include "halley/file_formats/halley-yamlcpp.h"
#include <halley/tools/ecs/component_schema.h>

#include "halley/support/logger.h"
#include "halley/text/string_converter.h"

using namespace Halley;

ComponentSchema::ComponentSchema() {}

ComponentSchema::ComponentSchema(YAML::Node node, bool generate)
	: generate(generate)
{
	name = node["name"].as<std::string>();

	for (auto memberEntry : node["members"]) {
		for (auto m = memberEntry.begin(); m != memberEntry.end(); ++m) {
			String name = m->first.as<std::string>();
			if (m->second.IsScalar()) {
				// e.g. - value: int
				auto& field = members.emplace_back(TypeSchema(m->second.as<std::string>()), std::move(name));
				field.serializationTypes.push_back(EntitySerialization::Type::Prefab);
				field.serializationTypes.push_back(EntitySerialization::Type::SaveData);
				field.serializationTypes.push_back(EntitySerialization::Type::Network);
			} else {
				// e.g.
				// value:
				//   type: int
				//   access: protected
				//   initialValue: 5
				const YAML::Node& memberProperties = m->second;
				const String type = memberProperties["type"].as<std::string>();
				const String access = memberProperties["access"].as<std::string>("public");
				const String displayName = memberProperties["displayName"].as<std::string>("");
				const bool hideInEditor = memberProperties["hideInEditor"].as<bool>(false);
				const bool collapse = memberProperties["collapse"].as<bool>(false);

				std::set<EntitySerialization::Type> serializeTypes;
				if (memberProperties["canEdit"].as<bool>(true)) {
					serializeTypes.insert(EntitySerialization::Type::Prefab);
				}
				bool canSave = memberProperties["canSave"].as<bool>(true);
				if (canSave) {
					serializeTypes.insert(EntitySerialization::Type::SaveData);
				}
				if (memberProperties["canNetwork"].as<bool>(canSave)) {
					serializeTypes.insert(EntitySerialization::Type::Network);
				}

				std::optional<Range<float>> range;
				if (memberProperties["range"].IsDefined()) {
					Vector<float> vs;
					for (auto& r: memberProperties["range"]) {
						vs.push_back(r.as<float>());
					}
					if (vs.size() == 2) {
						range = Range<float>(vs[0], vs[1]);
					}
				}

				if (memberProperties["serializable"].IsDefined()) {
					throw Exception("serializable field is removed from ECS component definitions. Use canSave and canEdit instead.", HalleyExceptions::Entity);
				}

				Vector<String> defaultValue;
				const auto& defNode = memberProperties["defaultValue"];
				if (defNode.IsDefined()) {
					if (defNode.IsSequence()) {
						for (const auto& d: defNode) {
							defaultValue.emplace_back(d.as<std::string>());
						}
					} else {
						defaultValue.emplace_back(defNode.as<std::string>());
					}
				}

				auto& field = members.emplace_back(TypeSchema(type), std::move(name), std::move(defaultValue), fromString<MemberAccess>(access));
				field.collapse = collapse;
				field.serializationTypes = Vector<EntitySerialization::Type>(serializeTypes.begin(), serializeTypes.end());
				field.hideInEditor = hideInEditor;
				field.displayName = displayName;
				field.range = range;
			}
		}
	}

	if (node["customImplementation"].IsDefined()) {
		customImplementation = node["customImplementation"].as<std::string>();
	}

	const auto deps = node["componentDependencies"];
	if (deps.IsSequence()) {
		for (auto n = deps.begin(); n != deps.end(); ++n) {
			if (n->IsScalar()) {
				componentDependencies.push_back(n->as<std::string>());
			}
		}
	}
}

bool ComponentSchema::operator<(const ComponentSchema& other) const
{
	return id < other.id;
}
