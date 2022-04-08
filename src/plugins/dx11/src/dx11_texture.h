#pragma once
#include "halley/core/graphics/texture.h"
#include <d3d11.h>

#include "halley/core/graphics/material/material_definition.h"
#undef min
#undef max

namespace Halley
{
	class DX11Video;

	class DX11Texture final : public Texture
	{
	public:
		DX11Texture(DX11Video& video, Vector2i size);
		~DX11Texture();

		DX11Texture& operator=(DX11Texture&& other) noexcept;

		void doLoad(TextureDescriptor& descriptor) override;
		void reload(Resource&& resource) override;
		void bind(DX11Video& video, int textureUnit, TextureSamplerType samplerType) const;

		DXGI_FORMAT getFormat() const;
		ID3D11Texture2D* getTexture() const;

	protected:
		size_t getVRamUsage() const override;
		void doCopyToTexture(Painter& painter, Texture& other) const override;
		void doCopyToImage(Painter& painter, Image& image) const override;

	private:
		DX11Video& video;
		ID3D11Texture2D* texture = nullptr;
		ID3D11ShaderResourceView* srv = nullptr;
		ID3D11ShaderResourceView* srvAlt = nullptr;
		ID3D11SamplerState* samplerState = nullptr;
		DXGI_FORMAT format;

		void copyToImageDirectly(Image& image) const;
	};
}
