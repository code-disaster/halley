#include "ui_widget_list.h"

#include "ui_editor.h"

using namespace Halley;

UIWidgetList::UIWidgetList(String id, UIFactory& factory)
	: UIWidget(std::move(id), Vector2f(200, 100), UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "halley/ui_widget_list");
}

void UIWidgetList::onMakeUI()
{
	list = getWidgetAs<UITreeList>("widgetsList");
	populateList();

	setHandle(UIEventType::TreeItemReparented, "widgetsList", [=] (const UIEvent& event)
	{
		const auto& src = event.getConfigData().asSequence();
		Vector<MoveOperation> changes;
		changes.reserve(src.size());
		for (const auto& e: src) {
			changes.emplace_back(MoveOperation{ e["itemId"].asString(), e["parentId"].asString(), e["childIdx"].asInt() });
		}

		moveItems(changes);
	});
}

void UIWidgetList::setDefinition(std::shared_ptr<UIDefinition> def)
{
	definition = std::move(def);
	populateList();
}

void UIWidgetList::setUIEditor(UIEditor& editor)
{
	uiEditor = &editor;
}

UITreeList& UIWidgetList::getList()
{
	return *list;
}

void UIWidgetList::populateList()
{
	if (list && definition) {
		addWidget(definition->getRoot(), "");
	}
}

void UIWidgetList::addWidget(const ConfigNode& curNode, const String& parentId, size_t childIdx)
{
	doAddWidget(curNode, parentId, childIdx);

	list->sortItems();
	layout();
	list->setSelectedOptionId(curNode["uuid"].asString());
}

void UIWidgetList::doAddWidget(const ConfigNode& curNode, const String& parentId, size_t childIdx)
{
	const String id = curNode["uuid"].asString();
	const auto info = getEntryInfo(curNode);

	list->addTreeItem(id, parentId, childIdx, LocalisedString::fromUserString(info.label), "label", info.icon, !info.canHaveChildren);

	if (curNode.hasKey("children")) {
		for (const auto& c: curNode["children"].asSequence()) {
			doAddWidget(c, id);
		}
	}
}

void UIWidgetList::onWidgetModified(const String& id, const ConfigNode& data)
{
	const auto info = getEntryInfo(data);
	list->setLabel(id, LocalisedString::fromUserString(info.label), info.icon);
	list->setForceLeaf(id, info.canHaveChildren);
}

void UIWidgetList::moveItems(gsl::span<const MoveOperation> changes)
{
	for (auto& c: changes) {
		const auto result = definition->findUUID(c.itemId);
		auto* widgetPtr = result.result;
		auto* oldParent = result.parent;
		auto* newParent = definition->findUUID(c.parentId).result;
		if (widgetPtr && oldParent && newParent) {
			auto widget = ConfigNode(std::move(*widgetPtr));
			(*newParent)["children"].ensureType(ConfigNodeType::Sequence);
			auto& oldChildren = (*oldParent)["children"].asSequence();
			auto& newChildren = (*newParent)["children"].asSequence();
			std_ex::erase_if(oldChildren, [=] (const ConfigNode& n) { return &n == widgetPtr; });

			const int newPos = std::min(c.childIdx, static_cast<int>(newChildren.size()));
			newChildren.insert(newChildren.begin() + newPos, std::move(widget));
		}
	}

	uiEditor->markModified();
}

UIWidgetList::EntryInfo UIWidgetList::getEntryInfo(const ConfigNode& data) const
{
	EntryInfo result;

	String className;
	String id;
	String iconName;
	
	if (data.hasKey("widget")) {
		const auto& widgetNode = data["widget"];
		id = widgetNode["id"].asString("");

		const auto properties = uiEditor->getGameFactory().getPropertiesForWidget(widgetNode["class"].asString());
		className = properties.name;
		iconName = properties.iconName;

		result.canHaveChildren = properties.canHaveChildren;
	} else if (data.hasKey("spacer")) {
		className = "Spacer";
		iconName = "widget_icons/spacer.png";
		result.canHaveChildren = false;
	} else if (data.hasKey("stretchSpacer")) {
		className = "Stretch Spacer";
		iconName = "widget_icons/spacer.png";
		result.canHaveChildren = false;
	} else {
		className = "Sizer";
		String sizerType = "horizontal";

		if (data.hasKey("sizer")) {
			sizerType = data["sizer"]["type"].asString("horizontal");
		}

		iconName = "widget_icons/sizer_" + sizerType + ".png";
		result.canHaveChildren = true;
	} 

	result.label = className;
	if (!id.isEmpty()) {
		result.label += " \"" + id + "\"";
	}

	if (!iconName.isEmpty()) {
		result.icon = Sprite().setImage(uiEditor->getGameFactory().getResources(), iconName);
	}

	return result;
}
