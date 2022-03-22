#pragma once
#include <memory>
#include "halley/data_structures/vector.h"

#include "halley/core/input/text_input_capture.h"
#include "halley/text/halleystring.h"
#include "halley/maths/rect.h"
#include "halley/data_structures/maybe.h"

namespace Halley {
	enum class UIInputType;
	class UIEvent;
	class UIWidget;
	class UIRoot;

	class UIParent {
	public:
		virtual ~UIParent();

		virtual const String& getId() const = 0;
		virtual UIRoot* getRoot() = 0;
		virtual const UIRoot* getRoot() const = 0;

		virtual void sendEvent(UIEvent event) const = 0;
		virtual void receiveKeyPress(KeyboardKeyPress key) = 0;

		virtual Rect4f getRect() const = 0;
		virtual std::optional<float> getMaxChildWidth() const;

		void addChild(std::shared_ptr<UIWidget> widget);
		void removeChild(UIWidget& widget);
		virtual void clear();

		bool addNewChildren(UIInputType inputType);
		bool removeDeadChildren();
		bool isWaitingToSpawnChildren() const;

		virtual void markAsNeedingLayout();
		virtual void onChildrenAdded() {}
		virtual void onChildrenRemoved() {}
		virtual void onChildAdded(UIWidget& child) {}

		Vector<std::shared_ptr<UIWidget>>& getChildren();
		const Vector<std::shared_ptr<UIWidget>>& getChildren() const;
		Vector<std::shared_ptr<UIWidget>>& getChildrenWaiting();
		const Vector<std::shared_ptr<UIWidget>>& getChildrenWaiting() const;

		std::shared_ptr<UIWidget> getWidget(const String& id);
		std::shared_ptr<UIWidget> tryGetWidget(const String& id);
		
		template <typename T>
		std::shared_ptr<T> tryGetWidgetAs(const String& id)
		{
			return std::dynamic_pointer_cast<T>(tryGetWidget(id));
		}

		template <typename T>
		std::shared_ptr<T> getWidgetAs(const String& id)
		{
			auto widget = getWidget(id);
			if (widget) {
				auto w = std::dynamic_pointer_cast<T>(widget);
				if (!w) {
					throw Exception("Widget with id \"" + id + "\" was found, but it is not of type " + String(typeid(T).name()), HalleyExceptions::UI);
				}
				return w;
			} else {
				return {};
			}
		}

		virtual bool isDescendentOf(const UIWidget& ancestor) const;
		virtual bool isActiveInHierarchy() const;

		template <typename F>
		void descend(F f, bool includeInactive = false, bool includePending = false);

	private:
		Vector<std::shared_ptr<UIWidget>> children;
		Vector<std::shared_ptr<UIWidget>> childrenWaiting;

		std::shared_ptr<UIWidget> doGetWidget(const String& id) const;
	};
}
