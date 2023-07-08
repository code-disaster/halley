#pragma once

namespace Halley {
	class ProjectWindow;

	class StatusBar : public UIWidget, public ILoggerSink
	{
	public:
		StatusBar(UIFactory& factory, ProjectWindow& projectWindow);
		~StatusBar() override;

		void notifyConsoleOpen();

	protected:
		void log(LoggerLevel level, std::string_view msg) override;

		void update(Time t, bool moved) override;
		void draw(UIPainter& painter) const override;
		void pressMouse(Vector2f mousePos, int button, KeyMods keyMods) override;

	private:
		UIFactory& factory;
		ProjectWindow& projectWindow;
		
		Sprite background;
		Sprite statusLED;
		TextRenderer statusText;

		std::mutex mutex;
		std::list<std::pair<LoggerLevel, String>> pendingStatus;

		void resetLED();
	};
}