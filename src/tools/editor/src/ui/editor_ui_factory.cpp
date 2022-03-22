#include "editor_ui_factory.h"
#include "halley/core/resources/resources.h"
#include <halley/file_formats/config_file.h>
#include "scroll_background.h"
#include "select_asset_widget.h"
#include "src/assets/animation_editor.h"
#include "src/assets/asset_editor_window.h"
#include "src/assets/metadata_editor.h"
#include "src/assets/ui_editor/ui_editor_display.h"
#include "src/assets/ui_editor/ui_widget_editor.h"
#include "src/assets/ui_editor/ui_widget_list.h"
#include "src/scene/entity_editor.h"
#include "src/scene/entity_list.h"
#include "src/scene/entity_validator_ui.h"
#include "src/scene/scene_editor_canvas.h"
using namespace Halley;

EditorUIFactory::EditorUIFactory(const HalleyAPI& api, Resources& resources, I18N& i18n, const String& colourSchemeName)
	: UIFactory(api, resources, i18n)
{
	loadColourSchemes();
	setColourScheme(colourSchemeName);
	
	UIInputButtons listButtons;
	setInputButtons("list", listButtons);

	addFactory("scrollBackground", [=] (const ConfigNode& node) { return makeScrollBackground(node); });
	addFactory("animationEditorDisplay", [=] (const ConfigNode& node) { return makeAnimationEditorDisplay(node); });
	addFactory("metadataEditor", [=] (const ConfigNode& node) { return makeMetadataEditor(node); });
	addFactory("sceneEditorCanvas", [=](const ConfigNode& node) { return makeSceneEditorCanvas(node); });
	addFactory("entityList", [=](const ConfigNode& node) { return makeEntityList(node); });
	addFactory("entityValidator", [=](const ConfigNode& node) { return makeEntityValidator(node); });
	addFactory("entityValidatorList", [=](const ConfigNode& node) { return makeEntityValidatorList(node); });
	addFactory("entityEditor", [=](const ConfigNode& node) { return makeEntityEditor(node); });
	addFactory("selectAsset", [=](const ConfigNode& node) { return makeSelectAsset(node); });
	addFactory("uiWidgetList", [=](const ConfigNode& node) { return makeUIWidgetList(node); });
	addFactory("uiWidgetEditor", [=](const ConfigNode& node) { return makeUIWidgetEditor(node); });
	addFactory("uiEditorDisplay", [=](const ConfigNode& node) { return makeUIEditorDisplay(node); });
}

Sprite EditorUIFactory::makeAssetTypeIcon(AssetType type) const
{
	return Sprite()
		.setImage(getResources(), Path("ui") / "assetTypes" / toString(type) + ".png")
		.setColour(colourScheme->getColour("icon_" + toString(type)));
}

Sprite EditorUIFactory::makeImportAssetTypeIcon(ImportAssetType type) const
{
	return Sprite()
		.setImage(getResources(), Path("ui") / "assetTypes" / toString(type) + ".png")
		.setColour(colourScheme->getColour("icon_" + toString(type)));
}

Sprite EditorUIFactory::makeDirectoryIcon(bool up) const
{
	return Sprite()
		.setImage(getResources(), Path("ui") / "assetTypes" / (up ? "directoryUp" : "directory") + ".png")
		.setColour(colourScheme->getColour("icon_directory"));
}

std::shared_ptr<UIWidget> EditorUIFactory::makeScrollBackground(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	return std::make_shared<ScrollBackground>("scrollBackground", getStyle(node["style"].asString("scrollBackground")), makeSizerOrDefault(entryNode, UISizer(UISizerType::Vertical)));
}

std::shared_ptr<UIWidget> EditorUIFactory::makeAnimationEditorDisplay(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<AnimationEditorDisplay>(id, resources);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeMetadataEditor(const ConfigNode& entryNode)
{
	return std::make_shared<MetadataEditor>(*this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeSceneEditorCanvas(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<SceneEditorCanvas>(id, *this, resources, api, makeSizer(entryNode));
}

std::shared_ptr<UIWidget> EditorUIFactory::makeEntityList(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<EntityList>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeEntityValidator(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<EntityValidatorUI>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeEntityValidatorList(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<EntityValidatorListUI>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeEntityEditor(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<EntityEditor>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeSelectAsset(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<SelectAssetWidget>(id, *this, fromString<AssetType>(node["assetType"].asString()));
}

std::shared_ptr<UIWidget> EditorUIFactory::makeUIWidgetEditor(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<UIWidgetEditor>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeUIWidgetList(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<UIWidgetList>(id, *this);
}

std::shared_ptr<UIWidget> EditorUIFactory::makeUIEditorDisplay(const ConfigNode& entryNode)
{
	auto& node = entryNode["widget"];
	auto id = node["id"].asString();
	return std::make_shared<UIEditorDisplay>(id, Vector2f{}, makeSizer(entryNode).value_or(UISizer()));
}

void EditorUIFactory::loadColourSchemes()
{
	for (const auto& assetId: resources.enumerate<ConfigFile>()) {
		if (assetId.startsWith("colour_schemes/")) {
			auto& schemeConfig = resources.get<ConfigFile>(assetId)->getRoot();
			if (schemeConfig["enabled"].asBool(true)) {
				colourSchemes.emplace_back(assetId, schemeConfig["name"].asString());
			}
		}
	}
}

Vector<String> EditorUIFactory::getColourSchemeNames() const
{
	Vector<String> result;
	for (const auto& scheme: colourSchemes) {
		result.push_back(scheme.second);
	}
	std::sort(result.begin(), result.end());
	return result;
}

void EditorUIFactory::setColourScheme(const String& name)
{
	bool found = false;

	for (auto& scheme: colourSchemes) {
		if (scheme.second == name) {
			setColourSchemeByAssetId(scheme.first);
			found = true;
			break;
		}
	}

	if (!found && !colourSchemes.empty()) {
		setColourSchemeByAssetId(colourSchemes.front().first);
		found = true;
	}

	if (found) {
		reloadStyleSheet();
	}
}

void EditorUIFactory::setColourSchemeByAssetId(const String& assetId)
{
	colourScheme = std::make_shared<UIColourScheme>(getResources().get<ConfigFile>(assetId)->getRoot(), getResources());
}

void EditorUIFactory::reloadStyleSheet()
{
	const auto cur = getStyleSheet();
	
	if (true || !cur) {
		auto styleSheet = std::make_shared<UIStyleSheet>(resources);
		for (auto& style: resources.enumerate<ConfigFile>()) {
			if (style.startsWith("ui_style/")) {
				styleSheet->load(*resources.get<ConfigFile>(style), colourScheme);
			}
		}
		setStyleSheet(std::move(styleSheet));
	} else {
		// This doesn't work properly atm
		cur->reload(colourScheme);
	}
}
