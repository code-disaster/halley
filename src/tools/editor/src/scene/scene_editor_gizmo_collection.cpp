#include "scene_editor_gizmo_collection.h"

#include "gizmos/scripting/scripting_gizmo.h"
#include "gizmos/translate_gizmo.h"
#include "gizmos/selected_bounds_gizmo.h"
#include "gizmos/selection_box_gizmo.h"
#include "halley/core/graphics/camera.h"
using namespace Halley;

SceneEditorGizmoCollection::SceneEditorGizmoCollection(UIFactory& factory, Resources& resources, ISceneEditorWindow& sceneEditorWindow)
	: factory(factory)
	, resources(resources)
	, sceneEditorWindow(sceneEditorWindow)
{
	// TODO: read this elsewhere
	snapRules.grid = GridSnapMode::Pixel;
	snapRules.line = LineSnapMode::IsometricAxisAligned;
	
	selectedBoundsGizmo = std::make_unique<SelectedBoundsGizmo>(snapRules, resources);
	selectionBoxGizmo = std::make_unique<SelectionBoxGizmo>(snapRules, resources);

	resetTools();
}

bool SceneEditorGizmoCollection::update(Time time, const Camera& camera, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState, SceneEditorOutputState& outputState)
{
	selectedBoundsGizmo->setCamera(camera);
	selectedBoundsGizmo->update(time, sceneEditor, inputState);
	selectionBoxGizmo->setCamera(camera);
	selectionBoxGizmo->update(time, sceneEditor, inputState);
	
	if (activeGizmo) {
		activeGizmo->setCamera(camera);
		activeGizmo->setOutputState(&outputState);
		activeGizmo->update(time, sceneEditor, inputState);
		activeGizmo->setOutputState(nullptr);

		return activeGizmo->isHighlighted();
	}
	return false;
}

void SceneEditorGizmoCollection::draw(Painter& painter, const ISceneEditor& sceneEditor)
{
	selectedBoundsGizmo->draw(painter, sceneEditor);
	selectionBoxGizmo->draw(painter, sceneEditor);
	
	if (activeGizmo) {
		activeGizmo->draw(painter, sceneEditor);
	}
}

void SceneEditorGizmoCollection::setSelectedEntities(Vector<EntityRef> entities, Vector<EntityData*> datas)
{
	selectedEntities = std::move(entities);
	entityDatas = std::move(datas);
	
	selectedBoundsGizmo->setSelectedEntities(selectedEntities, entityDatas);
	
	if (activeGizmo) {
		activeGizmo->setSelectedEntities(selectedEntities, entityDatas);
	}
}

bool SceneEditorGizmoCollection::canBoxSelectEntities()
{
	if (activeGizmo) {
		return activeGizmo->canBoxSelectEntities();
	}
	return true;
}

void SceneEditorGizmoCollection::refreshEntity()
{
	selectedBoundsGizmo->refreshEntity();
	if (activeGizmo) {
		activeGizmo->refreshEntity();
	}
}

void SceneEditorGizmoCollection::onEntityModified(const UUID& uuid, const EntityData& oldData, const EntityData& newData)
{
	if (std_ex::contains_if(selectedEntities, [&] (const auto& e) { return e.getInstanceUUID() == uuid; })) {
		if (newData.getComponents().size() != oldData.getComponents().size()) {
			refreshEntity();
		}
	}
}

std::shared_ptr<UIWidget> SceneEditorGizmoCollection::setTool(const String& tool, const String& componentName, const String& fieldName)
{
	const bool changedTool = currentTool != tool;
	
	currentTool = tool;
	activeGizmo.reset();
	
	const auto iter = gizmoFactories.find(tool);
	if (iter != gizmoFactories.end()) {
		if (iter->second) {
			activeGizmo = iter->second(snapRules, componentName, fieldName);
		}
	} else {
		activeGizmo.reset();
	}

	if (changedTool) {
		sceneEditorWindow.setSetting(EditorSettingType::Temp, "tools.curTool", ConfigNode(tool));
		sceneEditorWindow.setHighlightedComponents(activeGizmo ? activeGizmo->getHighlightedComponents() : Vector<String>());
	}

	if (activeGizmo) {
		if (!selectedEntities.empty()) {
			activeGizmo->setSelectedEntities(selectedEntities, entityDatas);
		}
		return activeGizmo->makeUI();
	}

	return {};
}

void SceneEditorGizmoCollection::deselect()
{
	if (activeGizmo) {
		activeGizmo->deselect();
	}
}

void SceneEditorGizmoCollection::generateList(UIList& list)
{
	const auto iconCol = factory.getColourScheme()->getColour("ui_text");
	list.clear();
	for (const auto& tool: tools) {
		list.addImage(tool.id, std::make_shared<UIImage>(tool.icon.clone().setColour(iconCol)), 1, {}, UISizerAlignFlags::Centre)->setToolTip(tool.toolTip);
	}
	uiList = &list;
}

ISceneEditorWindow& SceneEditorGizmoCollection::getSceneEditorWindow()
{
	return sceneEditorWindow;
}

bool SceneEditorGizmoCollection::onKeyPress(KeyboardKeyPress key, UIList& list)
{
	if (activeGizmo) {
		if (activeGizmo->onKeyPress(key)) {
			return true;
		}
	}

	for (const auto& t: tools) {
		if (key == t.shortcut) {
			list.setSelectedOptionId(t.id);
			return true;
		}
	}

	return false;
}

void SceneEditorGizmoCollection::addTool(const Tool& tool, GizmoFactory gizmoFactory)
{
	tools.push_back(tool);
	gizmoFactories[tool.id] = std::move(gizmoFactory);
}

void SceneEditorGizmoCollection::resetTools()
{
	clear();
	
	addTool(Tool("drag", LocalisedString::fromHardcodedString("Hand [H]"), Sprite().setImage(resources, "ui/scene_editor_drag.png"), KeyCode::H),
		[this] (SnapRules snapRules, const String& componentName, const String& fieldName)
		{
			return std::unique_ptr<SceneEditorGizmo>{};
		}
	);
	addTool(Tool("translate", LocalisedString::fromHardcodedString("Move [V]"), Sprite().setImage(resources, "ui/scene_editor_move.png"), KeyCode::V),
		[this] (SnapRules snapRules, const String& componentName, const String& fieldName)
		{
			return std::make_unique<TranslateGizmo>(snapRules, factory, sceneEditorWindow);
		}
	);
	addTool(Tool("scripting", LocalisedString::fromHardcodedString("Scripting [S]"), Sprite().setImage(resources, "ui/scene_editor_scripting.png"), KeyCode::S),
		[this] (SnapRules snapRules, const String& componentName, const String& fieldName)
		{
			return std::make_unique<ScriptingGizmo>(snapRules, factory, sceneEditorWindow, sceneEditorWindow.getScriptNodeTypes());
		}
	);
}

void SceneEditorGizmoCollection::clear()
{
	tools.clear();
	gizmoFactories.clear();
	if (uiList) {
		uiList->clear();
		uiList = nullptr;
	}
	activeGizmo.reset();
}
