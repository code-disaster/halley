#pragma once
#include "halley/editor_extensions/scene_editor_gizmo.h"

namespace Halley {
	enum class TranslateGizmoMode {
		Pivot,
		Centre
	};

	template <>
	struct EnumNames<TranslateGizmoMode> {
		constexpr std::array<const char*, 2> operator()() const {
			return{{
					"pivot",
					"centre"
				}};
		}
	};
	
	class TranslateGizmo final : public SceneEditorGizmo {
	public:
		explicit TranslateGizmo(SnapRules snapRules, UIFactory& factory, ISceneEditorWindow& sceneEditorWindow);
		void update(Time time, const ISceneEditor& sceneEditor, const SceneEditorInputState& inputState) override;
		void draw(Painter& painter, const ISceneEditor& sceneEditor) const override;
		bool isHighlighted() const override;
		std::shared_ptr<UIWidget> makeUI() override;
		Vector<String> getHighlightedComponents() const override;
		bool onKeyPress(KeyboardKeyPress key) override;
		bool canBoxSelectEntities() const override;

	protected:
		void onEntityChanged() override;

	private:
		UIFactory& factory;
		ISceneEditorWindow& sceneEditorWindow;

		std::optional<SceneEditorGizmoHandle> handle;
		Vector2f handleOffset;

		TranslateGizmoMode mode;
		std::shared_ptr<UIList> uiMode;
		Vector2f pendingMoveBy;

		Circle getHandleBounds(const SceneEditorGizmoHandle& handle) const;
		void updateEntityData(Vector2f pos, size_t idx);
		Vector2f getObjectOffset(size_t idx) const;
		void setMode(TranslateGizmoMode mode);
		void moveBy(Vector2i delta);
		void doMoveBy();
		void doMoveBy(Vector2f delta);
		void loadHandles();
		bool isDescendentOf(EntityRef e, gsl::span<const EntityRef> entities) const;
	};
}
