#pragma once

#include "prec.h"
#include "halley/file/path.h"

namespace Halley
{
	class Project;
	class ProjectLoader;
	class Preferences;

	class HalleyEditor final : public Game
	{
	public:
		HalleyEditor();
		~HalleyEditor();

		std::unique_ptr<Project> loadProject(Path path);
		std::unique_ptr<Project> createProject(Path path);

		Preferences& getPreferences();
		ProjectLoader& getProjectLoader();

	protected:
		void init(const Environment& environment, const Vector<String>& args) override;
		int initPlugins(IPluginRegistry &registry) override;
		ResourceOptions initResourceLocator(const Path& gamePath, const Path& assetsPath, const Path& unpackedAssetsPath, ResourceLocator& locator) override;
		std::unique_ptr<Stage> startGame() override;

		String getName() const override;
		String getDataPath() const override;
		bool isDevMode() const override;
		bool shouldCreateSeparateConsole() const override;

	private:
		std::unique_ptr<ProjectLoader> projectLoader;
		std::unique_ptr<Preferences> preferences;
		Path rootPath;

		String projectPath;
		bool gotProjectPath = false;

		void parseArguments(const Vector<String>& args);
	};
}