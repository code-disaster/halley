#pragma once
#include "halley/core/api/halley_api_internal.h"
#include "halley/core/input/input_device.h"

namespace Halley
{
	class WinRTKeyboard;
	class WinRTMouse;

	class WinRTInput : public InputAPIInternal
	{
	public:
		void init() override;
		void deInit() override;
		void beginEvents(Time t) override;

		size_t getNumberOfKeyboards() const override;
		std::shared_ptr<InputKeyboard> getKeyboard(int id) const override;

		size_t getNumberOfJoysticks() const override;
		std::shared_ptr<InputJoystick> getJoystick(int id) const override;

		size_t getNumberOfMice() const override;
		std::shared_ptr<InputDevice> getMouse(int id) const override;
		void setMouseRemapping(std::function<Vector2f(Vector2i)> remapFunction) override;

		Vector<std::shared_ptr<InputTouch>> getNewTouchEvents() override;
		Vector<std::shared_ptr<InputTouch>> getTouchEvents() override;

	private:
		Vector<std::shared_ptr<InputJoystick>> gamepads;
		std::shared_ptr<WinRTKeyboard> keyboard;
		std::shared_ptr<WinRTMouse> mouse;
	};
}
