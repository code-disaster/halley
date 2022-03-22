#pragma once

namespace Halley {
	class SceneEditorWindow;

	class UndoStack {
	public:
		UndoStack();
		
		void pushAdded(bool wasModified, gsl::span<const EntityChangeOperation> changes);
		void pushRemoved(bool wasModified, gsl::span<const String> entityIds, gsl::span<const std::pair<String, int>> parents, gsl::span<const EntityData> datas);
		void pushMoved(bool wasModified, gsl::span<const EntityChangeOperation> curState, gsl::span<const EntityChangeOperation> previousState);
		bool pushModified(bool wasModified, gsl::span<const String> entityIds, gsl::span<const EntityData*> before, gsl::span<const EntityData*> after);
		bool pushReplaced(bool wasModified, const String& entityId, const EntityData& before, const EntityData& after);

		void undo(SceneEditorWindow& sceneEditorWindow);
		void redo(SceneEditorWindow& sceneEditorWindow);

		void onSave();

		bool canUndo() const;
		bool canRedo() const;

	private:
		enum class Type {
			EntityAdded,
			EntityRemoved,
			EntityMoved,
			EntityModified,
			EntityReplaced
		};
		
		class Action {
		public:
			Type type;
			Vector<EntityChangeOperation> patches;
			bool clearModified = false;

			Action() = default;
			Action(Type type, EntityDataDelta delta, String entityId, String parent = "", int childIndex = -1);
			Action(Type type, Vector<EntityChangeOperation> patches);
		};

		struct ActionPair {
		public:
			Action forward;
			Action back;

			ActionPair() = default;
			ActionPair(Action forward, Action back) : forward(std::move(forward)), back(std::move(back)) {}

			bool isCompatibleWith(const Action& newForward) const;
			bool arePatchesCompatible(const EntityChangeOperation& a, const EntityChangeOperation& b, Type type) const;
		};

		Vector<std::unique_ptr<ActionPair>> stack;
		size_t stackPos = 0;
		const size_t maxSize;
		bool accepting;

		void addToStack(Action forward, Action back, bool wasModified);
		void runAction(const Action& action, SceneEditorWindow& sceneEditorWindow);
		void sortPatches(Vector<EntityChangeOperation>& patches);
	};
}
