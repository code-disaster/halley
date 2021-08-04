#pragma once
#include "halley/core/graphics/window.h"
#include <SDL.h>

namespace Halley
{
	class SDLWindow final : public Window
	{
	public:
		SDLWindow(SDL_Window* window);

		void update(const WindowDefinition& definition) override;
		void updateDefinition(const WindowDefinition& definition);

		void show() override;
		void hide() override;
		void setVsync(bool vsync) override;
		void swap() override;
		void present() override;
		Rect4i getWindowRect() const override;

		int getId() const;
		void resize(Rect4i size);

		SDL_Window* getSDLWindow() const { return window; }
		const WindowDefinition& getDefinition() const override { return *curDefinition; }

		void destroy();

		void* getNativeHandle() const override;
		String getNativeHandleType() const override;

	private:
		SDL_Window* window;
		std::unique_ptr<WindowDefinition> curDefinition;
	};
}
