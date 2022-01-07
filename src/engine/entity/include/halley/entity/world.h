#pragma once

#include <memory>
#include <typeinfo>
#include <type_traits>
#include "entity_id.h"
#include "family_mask.h"
#include "family.h"
#include <halley/time/halleytime.h>
#include <halley/text/halleystring.h>
#include <halley/data_structures/mapped_pool.h>
#include <halley/time/stopwatch.h>
#include <halley/data_structures/vector.h>
#include <halley/data_structures/tree_map.h>
#include "service.h"
#include "create_functions.h"
#include "halley/utils/attributes.h"
#include <halley/data_structures/memory_pool.h>

namespace Halley {
	struct SystemMessageContext;
	class UUID;
	class ConfigNode;
	class RenderContext;
	class Entity;
	class System;
	class Painter;
	class HalleyAPI;

	class World
	{
	public:
		World(const HalleyAPI& api, Resources& resources, CreateComponentFunction createComponent);
		~World();

		static std::unique_ptr<World> make(const HalleyAPI& api, Resources& resources, const String& sceneName, bool devMode);

		void step(TimeLine timeline, Time elapsed);
		void render(RenderContext& rc);
		bool hasSystemsOnTimeLine(TimeLine timeline) const;
		
		System& addSystem(std::unique_ptr<System> system, TimeLine timeline);
		void removeSystem(System& system);
		Vector<System*> getSystems();
		System& getSystem(const String& name);
		Vector<std::unique_ptr<System>>& getSystems(TimeLine timeline);
		const Vector<std::unique_ptr<System>>& getSystems(TimeLine timeline) const;

		Service& addService(std::shared_ptr<Service> service);
		void loadSystems(const ConfigNode& config, std::function<std::unique_ptr<System>(String)> createFunction);

		template <typename T>
		T& getService()
		{
			return getService<T>("");
		}
		
		template <typename T>
		T& getService(const String& systemName)
		{
			static_assert(std::is_base_of<Service, T>::value, "Must extend Service");

			const auto serviceName = typeid(T).name();
			const auto rawService = tryGetService(serviceName);
			if (!rawService) {
				if constexpr (std::is_default_constructible_v<T>) {
					return dynamic_cast<T&>(addService(std::make_shared<T>()));
				} else {
					throw Exception(String("Service \"") + serviceName + "\" required by \"" + (systemName.isEmpty() ? "" : (systemName + "System")) + "\" not found, and it cannot be default constructed.", HalleyExceptions::Entity);
				}
			}
			return *dynamic_cast<T*>(rawService);
		}

		EntityRef createEntity(String name = "", std::optional<EntityRef> parent = {});
		EntityRef createEntity(String name, EntityId parentId);
		EntityRef createEntity(UUID uuid, String name, EntityId parentId);
		EntityRef createEntity(UUID uuid, String name = "", std::optional<EntityRef> parent = {}, uint8_t worldPartition = 0);

		void destroyEntity(EntityId id);
		void destroyEntity(EntityRef entity);

		EntityRef getEntity(EntityId id);
		ConstEntityRef getEntity(EntityId id) const;
		EntityRef tryGetEntity(EntityId id);
		ConstEntityRef tryGetEntity(EntityId id) const;
		Entity* tryGetRawEntity(EntityId id);
		const Entity* tryGetRawEntity(EntityId id) const;
		std::optional<EntityRef> findEntity(const UUID& id, bool includePending = false);

		size_t numEntities() const;
		std::vector<EntityRef> getEntities();
		std::vector<ConstEntityRef> getEntities() const;
		std::vector<EntityRef> getTopLevelEntities();
		std::vector<ConstEntityRef> getTopLevelEntities() const;

		void spawnPending(); // Warning: use with care, will invalidate entities

		void onEntityDirty();

		void setEntityReloaded();

		template <typename T>
		Family& getFamily() noexcept
		{
			// Disable re-using of families due to optional components messing them up
			/*
			FamilyMaskType mask = T::Type::readMask();
			auto iter = families.find(mask);
			if (iter != families.end()) {
				return *iter->second;
			}
			*/

			return addFamily(std::make_unique<FamilyImpl<T>>(*maskStorage));
		}

		const CreateComponentFunction& getCreateComponentFunction() const;

		MaskStorage& getMaskStorage() const noexcept;
		ComponentDeleterTable& getComponentDeleterTable();

		size_t sendSystemMessage(SystemMessageContext context, const String& targetSystem);

		bool isDevMode() const;

		void setEditor(bool isEditor);
		bool isEditor() const;

	private:
		const HalleyAPI& api;
		Resources& resources;
		std::array<Vector<std::unique_ptr<System>>, static_cast<int>(TimeLine::NUMBER_OF_TIMELINES)> systems;
		CreateComponentFunction createComponent;
		bool entityDirty = false;
		bool entityReloaded = false;
		bool editor = false;
		
		Vector<Entity*> entities;
		Vector<Entity*> entitiesPendingCreation;
		MappedPool<Entity*> entityMap;
		HashMap<UUID, Entity*> uuidMap;

		//TreeMap<FamilyMaskType, std::unique_ptr<Family>> families;
		Vector<std::unique_ptr<Family>> families;
		TreeMap<String, std::shared_ptr<Service>> services;

		TreeMap<FamilyMaskType, std::vector<Family*>> familyCache;

		std::shared_ptr<MaskStorage> maskStorage;
		std::shared_ptr<ComponentDeleterTable> componentDeleterTable;
		std::shared_ptr<PoolAllocator<Entity>> entityPool;

		std::list<SystemMessageContext> pendingSystemMessages;

		void allocateEntity(Entity* entity);
		void updateEntities();
		void initSystems(gsl::span<const TimeLine> timelines);

		void doDestroyEntity(EntityId id);
		void doDestroyEntity(Entity* entity);
		void deleteEntity(Entity* entity);

		void updateSystems(TimeLine timeline, Time elapsed);
		void renderSystems(RenderContext& rc) const;

		NOINLINE Family& addFamily(std::unique_ptr<Family> family) noexcept;
		void onAddFamily(Family& family) noexcept;

		Service* tryGetService(const String& name) const;

		const std::vector<Family*>& getFamiliesFor(const FamilyMaskType& mask);

		void processSystemMessages(TimeLine timeline);
	};
}
