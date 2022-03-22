#include "dx11_shader.h"
#include "dx11_video.h"
#include "halley/core/graphics/material/material_definition.h"
#include "halley/support/logger.h"
using namespace Halley;

DX11Shader::DX11Shader(DX11Video& video, const ShaderDefinition& definition)
	: name(definition.name)
{
	for (auto& shader: definition.shaders) {
		loadShader(video, shader.first, shader.second);
	}
}

DX11Shader::~DX11Shader()
{
	if (vertexShader) {
		vertexShader->Release();
		vertexShader = nullptr;
	}
	if (pixelShader) {
		pixelShader->Release();
		pixelShader = nullptr;
	}
	if (geometryShader) {
		geometryShader->Release();
		geometryShader = nullptr;
	}
	if (layout) {
		layout->Release();
		layout = nullptr;
	}
}

void DX11Shader::loadShader(DX11Video& video, ShaderType type, const Bytes& bytes)
{
	HRESULT result = S_OK;

	switch (type) {
	case ShaderType::Vertex:
		result = video.getDevice().CreateVertexShader(bytes.data(), bytes.size(), nullptr, &vertexShader);
		vertexBlob = bytes;
		break;

	case ShaderType::Pixel:
		result = video.getDevice().CreatePixelShader(bytes.data(), bytes.size(), nullptr, &pixelShader);
		break;

	case ShaderType::Geometry:
		result = video.getDevice().CreateGeometryShader(bytes.data(), bytes.size(), nullptr, &geometryShader);
		break;

	default:
		break;
	}

	if (result != S_OK) {
		throw Exception("Unable to create shader " + name + " (" + toString(type) + ").", HalleyExceptions::VideoPlugin);
	}
}

int DX11Shader::getUniformLocation(const String& name, ShaderType stage)
{
	return -1;
}

int DX11Shader::getBlockLocation(const String& name, ShaderType stage)
{
	return -1;
}

void DX11Shader::bind(DX11Video& video)
{
	Expects(vertexShader);
	Expects(layout);

	auto& devCon = video.getDeviceContext();
	devCon.VSSetShader(vertexShader, nullptr, 0);
	devCon.GSSetShader(geometryShader, nullptr, 0);
	devCon.PSSetShader(pixelShader, nullptr, 0);

	devCon.IASetInputLayout(layout);
}

static DXGI_FORMAT getDX11Format(ShaderParameterType type)
{
	switch (type) {
	case ShaderParameterType::Float:
		return DXGI_FORMAT_R32_FLOAT;
	case ShaderParameterType::Float2:
		return DXGI_FORMAT_R32G32_FLOAT;
	case ShaderParameterType::Float3:
		return DXGI_FORMAT_R32G32B32_FLOAT;
	case ShaderParameterType::Float4:
		return DXGI_FORMAT_R32G32B32A32_FLOAT;
	case ShaderParameterType::Int:
		return DXGI_FORMAT_R32_SINT;
	case ShaderParameterType::Int2:
		return DXGI_FORMAT_R32G32_SINT;
	case ShaderParameterType::Int3:
		return DXGI_FORMAT_R32G32B32_SINT;
	case ShaderParameterType::Int4:
		return DXGI_FORMAT_R32G32B32A32_SINT;
	default:
		throw Exception("Unknown shader parameter type: " + toString(int(type)), HalleyExceptions::VideoPlugin);
	}
}

void DX11Shader::setMaterialLayout(DX11Video& video, const Vector<MaterialAttribute>& attributes)
{
	if (layout) {
		return;
	}

	Expects(!vertexBlob.empty());

	Vector<std::array<char, 64>> names(attributes.size());
	Vector<D3D11_INPUT_ELEMENT_DESC> desc(attributes.size());

	for (size_t i = 0; i < desc.size(); ++i) {
		const auto& a = attributes[i];
		
		const UINT semanticIndex = a.semanticIndex;
		const UINT inputSlot = 0;
		const DXGI_FORMAT format = getDX11Format(a.type);
		const UINT byteOffset = a.offset;
		strcpy_s(names[i].data(), 64, a.semantic.c_str());

		desc[i] = { names[i].data(), semanticIndex, format, inputSlot, byteOffset, D3D11_INPUT_PER_VERTEX_DATA, 0 };
	}

	HRESULT result = video.getDevice().CreateInputLayout(desc.data(), UINT(desc.size()), vertexBlob.data(), vertexBlob.size(), &layout);
	if (result != S_OK) {
		throw Exception("Unable to create input layout for shader " + name, HalleyExceptions::VideoPlugin);
	}
	vertexBlob.clear();
}
