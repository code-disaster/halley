#pragma once
#include "halley/core/graphics/material/material.h"
#include "dx11_buffer.h"

namespace Halley
{
	class DX11MaterialConstantBuffer final : public MaterialConstantBuffer
	{
	public:
		DX11MaterialConstantBuffer(DX11Video& video);

		void update(gsl::span<const gsl::byte> data) override;
		DX11Buffer& getBuffer();

	private:
		DX11Buffer buffer;
	};
}
