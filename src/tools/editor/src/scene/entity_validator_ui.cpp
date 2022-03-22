#include "entity_validator_ui.h"

#include "entity_list.h"
#include "halley/editor_extensions/entity_validator.h"
using namespace Halley;

EntityValidatorUI::EntityValidatorUI(String id, UIFactory& factory)
	: UIWidget(std::move(id), {}, UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "halley/entity_validator");
	setActive(false);
}

void EntityValidatorUI::onMakeUI()
{
}

void EntityValidatorUI::setValidator(EntityValidator& v)
{
	validator = &v;
	refresh();
}

void EntityValidatorUI::setEntity(EntityData& e, IEntityEditor& editor, Resources& resources)
{
	curEntity = &e;
	entityEditor = &editor;
	gameResources = &resources;

	isPrefab = !curEntity->getPrefab().isEmpty() && gameResources->exists<Prefab>(curEntity->getPrefab());
	if (isPrefab) {
		const auto prefab = gameResources->get<Prefab>(curEntity->getPrefab());
		curEntityInstance = prefab->getEntityData().instantiateWithAsCopy(*curEntity);
	}
	
	refresh();
}

void EntityValidatorUI::refresh()
{
	if (!curEntity || !validator) {
		return;
	}
	
	Vector<IEntityValidator::Result> result;
	if (isPrefab) {
		result = validator->validateEntity(curEntityInstance, true);
	} else {
		result = validator->validateEntity(*curEntity, false);
	}
	
	if (result != curResultSet) {
		curResultSet = std::move(result);
		setActive(!curResultSet.empty());

		auto parent = getWidget("validationFields");
		parent->clear();

		bool first = true;
		auto severity = IEntityValidator::Severity::None;

		for (const auto& curResult: curResultSet) {
			severity = std::max(severity, curResult.severity);

			if (!first) {
				auto col = factory.getColourScheme()->getColour("ui_text");
				parent->add(std::make_shared<UIImage>(Sprite().setImage(factory.getResources(), "halley_ui/ui_separator.png").setColour(col)), 0, Vector4f(0, 4, 0, 4));
			}
			first = false;

			auto label = std::make_shared<UILabel>("", factory.getStyle("labelLight"), curResult.errorMessage);
			label->setMaxWidth(300.0f);
			parent->add(label);
			
			for (const auto& action: curResult.suggestedActions) {
				if (validator->canApplyAction(*entityEditor, *curEntity, action.actionData)) {
					auto button = std::make_shared<UIButton>("action", factory.getStyle("buttonThin"), action.label);
					button->setHandle(UIEventType::ButtonClicked, [this, actionData = ConfigNode(action.actionData)] (const UIEvent& event)
					{
						Concurrent::execute(Executors::getMainUpdateThread(), [=] () {
							validator->applyAction(*entityEditor, *curEntity, actionData);
							entityEditor->reloadEntity();
						});
					});
					parent->add(button);
				}
			}
		}

		setSeverity(*this, factory, severity);
	}
}

void EntityValidatorUI::setSeverity(UIWidget& widget, UIFactory& factory, IEntityValidator::Severity severity)
{
	auto capsule = widget.getWidgetAs<UIImage>("capsule");
	capsule->clearBehaviours();

	if (severity != IEntityValidator::Severity::None) {
		auto c0 = factory.getColourScheme()->getColour("ui_staticBox");
		auto c1 = factory.getColourScheme()->getColour(severity == IEntityValidator::Severity::Error ? "taskError" : "taskWarning");
		capsule->addBehaviour(std::make_shared<UIPulseSpriteBehaviour>(c0, c1, 2.0, 1.0));
	}
}

EntityValidatorListUI::EntityValidatorListUI(String id, UIFactory& factory)
	: UIWidget(std::move(id), {}, UISizer())
	, factory(factory)
{
	factory.loadUI(*this, "halley/entity_validator_list");
	setActive(false);
}

void EntityValidatorListUI::onMakeUI()
{
	setHandle(UIEventType::ButtonClicked, "prev", [=] (const UIEvent& event)
	{
		move(-1);
	});

	setHandle(UIEventType::ButtonClicked, "next", [=] (const UIEvent& event)
	{
		move(1);
	});

	description = getWidgetAs<UILabel>("description");
}

void EntityValidatorListUI::update(Time t, bool moved)
{
	const auto entityListStrong = entityList.lock();
	if (entityListStrong) {
		const auto& list = entityListStrong->getList();
		const auto curSel = list.getSelectedOption();
		const auto iter = std::find(invalidEntities.begin(), invalidEntities.end(), curSel);
		const int curPos = iter != invalidEntities.end() ? static_cast<int>(iter - invalidEntities.begin() + 1) : 0;
		description->setText(LocalisedString::fromHardcodedString("Entities have validation errors [" + (curPos > 0 ? toString(curPos) : "-") + "/" + toString(invalidEntities.size()) + "]"));
	}
}

void EntityValidatorListUI::setList(std::weak_ptr<EntityList> list)
{
	entityList = list;
}

void EntityValidatorListUI::setInvalidEntities(Vector<std::pair<int, IEntityValidator::Severity>> entities)
{
	auto severity = IEntityValidator::Severity::None;
	invalidEntities.resize(entities.size());
	for (size_t i = 0; i < entities.size(); ++i) {
		invalidEntities[i] = entities[i].first;
		severity = std::max(severity, entities[i].second);
	}
	setActive(!invalidEntities.empty());
	EntityValidatorUI::setSeverity(*this, factory, severity);
}

void EntityValidatorListUI::move(int delta)
{
	auto& list = entityList.lock()->getList();
	const auto curSel = list.getSelectedOption();
	int newSel = curSel;

	if (delta == 1) {
		newSel = invalidEntities.front();
		for (int e: invalidEntities) {
			if (e > curSel) {
				newSel = e;
				break;
			}
		}
	} else if (delta == -1) {
		newSel = invalidEntities.back();
		for (int i = int(invalidEntities.size()); --i >= 0;) {
			int e = invalidEntities[i];
			if (e < curSel) {
				newSel = e;
				break;
			}
		}
	}

	if (newSel != curSel) {
		list.setSelectedOption(newSel);
	}
}
