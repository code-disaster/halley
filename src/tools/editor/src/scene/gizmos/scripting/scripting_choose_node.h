#pragma once

#include "src/scene/choose_window.h"

namespace Halley {
	class ScriptingChooseNode final : public ChooseAssetWindow {
	public:
		ScriptingChooseNode(Vector2f minSize, UIFactory& factory, Resources& resources, std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes, const Callback& callback);

	protected:
		std::shared_ptr<UISizer> makeItemSizer(std::shared_ptr<UIImage> icon, std::shared_ptr<UILabel> label, bool hasSearch) override;
		std::shared_ptr<UIImage> makeIcon(const String& id, bool hasSearch) override;
		void sortItems(Vector<std::pair<String, String>>& items) override;

		int getNumColumns(Vector2f scrollPaneSize) const override;
	
	private:
		Resources& resources;
		std::shared_ptr<ScriptNodeTypeCollection> scriptNodeTypes;
	};
}
