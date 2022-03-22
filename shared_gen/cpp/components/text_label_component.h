// Halley codegen version 102
#pragma once

#ifndef DONT_INCLUDE_HALLEY_HPP
#include <halley.hpp>
#endif

class TextLabelComponent final : public Halley::Component {
public:
	static constexpr int componentIndex{ 2 };
	static const constexpr char* componentName{ "TextLabel" };

	Halley::TextRenderer text{};
	int layer{ 0 };
	Halley::OptionalLite<int> mask{};

	TextLabelComponent() {
	}

	TextLabelComponent(Halley::TextRenderer text, int layer, Halley::OptionalLite<int> mask)
		: text(std::move(text))
		, layer(std::move(layer))
		, mask(std::move(mask))
	{
	}

	Halley::ConfigNode serialize(const Halley::EntitySerializationContext& context) const {
		using namespace Halley::EntitySerialization;
		Halley::ConfigNode node = Halley::ConfigNode::MapType();
		Halley::EntityConfigNodeSerializer<decltype(text)>::serialize(text, Halley::TextRenderer{}, context, node, componentName, "text", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::serialize(layer, int{ 0 }, context, node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::serialize(mask, Halley::OptionalLite<int>{}, context, node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		return node;
	}

	void deserialize(const Halley::EntitySerializationContext& context, const Halley::ConfigNode& node) {
		using namespace Halley::EntitySerialization;
		Halley::EntityConfigNodeSerializer<decltype(text)>::deserialize(text, Halley::TextRenderer{}, context, node, componentName, "text", makeMask(Type::Prefab));
		Halley::EntityConfigNodeSerializer<decltype(layer)>::deserialize(layer, int{ 0 }, context, node, componentName, "layer", makeMask(Type::Prefab, Type::SaveData, Type::Network));
		Halley::EntityConfigNodeSerializer<decltype(mask)>::deserialize(mask, Halley::OptionalLite<int>{}, context, node, componentName, "mask", makeMask(Type::Prefab, Type::SaveData, Type::Network));
	}

};
