#include "halley/core/graphics/mesh/mesh.h"
#include "halley/core/graphics/texture.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/core/graphics/material/material.h"
#include "halley/core/graphics/material/material_parameter.h"
#include "halley/bytes/byte_serializer.h"
#include "halley/resources/resource_data.h"
#include "halley/core/api/halley_api.h"

using namespace Halley;

Mesh::Mesh()
{
}

Mesh::Mesh(ResourceLoader& loader)
{
	auto data = loader.getStatic();
	Deserializer s(data->getSpan());
	deserialize(s);

	auto matDef = loader.getResources().get<MaterialDefinition>(materialName);
	material = std::make_unique<Material>(matDef);

	int i = 0;
	for (auto& t: textureNames) {
		auto texture = loader.getResources().get<Texture>(t);
		material->set("tex" + toString(i), texture);
		++i;
	}
}

std::unique_ptr<Mesh> Mesh::loadResource(ResourceLoader& loader)
{
	return std::make_unique<Mesh>(loader);
}

uint32_t Mesh::getNumVertices() const
{
	return numVertices;
}

gsl::span<const Byte> Mesh::getVertexData() const
{
	return gsl::span<const Byte>(vertexData.data(), vertexData.size());
}

gsl::span<const IndexType> Mesh::getIndices() const
{
	return indices;
}

std::shared_ptr<const Material> Mesh::getMaterial() const
{
	return material;
}

void Mesh::setVertices(size_t num, Bytes vertexData)
{
	numVertices = uint32_t(num);
	this->vertexData = std::move(vertexData);
}

void Mesh::setIndices(Vector<IndexType> indices)
{
	this->indices = std::move(indices);
}

void Mesh::setMaterialName(String name)
{
	this->materialName = std::move(name);
}

void Mesh::setTextureNames(Vector<String> textureNames)
{
	this->textureNames = std::move(textureNames);
}

void Mesh::serialize(Serializer& s) const
{
	s << numVertices;
	s << vertexData;
	s << indices;
	s << materialName;
	s << textureNames;
}

void Mesh::deserialize(Deserializer& s)
{
	s >> numVertices;
	s >> vertexData;
	s >> indices;
	s >> materialName;
	s >> textureNames;
}
