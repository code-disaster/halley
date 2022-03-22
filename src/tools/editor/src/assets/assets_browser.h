#pragma once

#include "asset_browser_tabs.h"
#include "halley/core/api/halley_api.h"
#include "halley/ui/ui_factory.h"
#include "halley/ui/ui_widget.h"
#include "halley/ui/widgets/ui_list.h"
#include "halley/ui/widgets/ui_paged_pane.h"

namespace Halley {
	class EditorUIFactory;
	class ProjectWindow;
	class Project;
	class MetadataEditor;
	class AssetEditor;
	class AssetEditorWindow;
	
	class AssetsBrowser : public UIWidget {
    public:
        AssetsBrowser(EditorUIFactory& factory, Project& project, ProjectWindow& projectWindow);
        void openAsset(AssetType type, const String& assetId);
		void openFile(const Path& path);
        void replaceAssetTab(AssetType oldType, const String& oldId, AssetType newType, const String& newId);
        bool requestQuit(std::function<void()> callback);
        void saveTab();
		void saveAllTabs();
		void closeTab();
        void moveTabFocus(int delta);

    private:
		EditorUIFactory& factory;
		Project& project;
		ProjectWindow& projectWindow;

		std::map<AssetType, Path> curPaths;
		Path curSrcPath;
		AssetType curType = AssetType::Sprite;

		bool assetSrcMode = true;
		std::optional<Vector<String>> assetNames;
		std::optional<Path> pendingOpen;

		FuzzyTextMatcher fuzzyMatcher;
		String filter;
        
		std::shared_ptr<UIList> assetList;
		std::shared_ptr<AssetBrowserTabs> assetTabs;

        String lastClickedAsset;

		uint64_t curHash = 0;
		uint64_t curDirHash = 0;

		bool collapsed = false;

        void loadResources();
        void makeUI();
		void setAssetSrcMode(bool enabled);
		void updateAddRemoveButtons();

		void listAssetSources();
		void listAssets(AssetType type);
		void setListContents(Vector<String> files, const Path& curPath, bool flat);
		void refreshList();
		void setFilter(const String& filter);

		void clearAssetList();
		void addDirToList(const Path& curPath, const String& dir);
		void addFileToList(const Path& path);

		void loadAsset(const String& name, bool doubleClick);
		void refreshAssets(gsl::span<const String> assets);

		void addAsset();
		void addAsset(Path path, std::string_view data);
		void removeAsset();

		void setCollapsed(bool collapsed);
		void doSetCollapsed(bool collapsed);
	};
}
