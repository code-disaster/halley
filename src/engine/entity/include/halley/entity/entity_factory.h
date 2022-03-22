#pragma once
#include <functional>

#include "create_functions.h"
#include "prefab.h"
#include "halley/file_formats/config_file.h"
#include "halley/data_structures/maybe.h"
#include "halley/entity/entity.h"

namespace Halley {
	class World;
	class Resources;
	class EntityScene;
	class EntityData;
	
	class EntityFactory {
	public:
		enum class UpdateMode {
			//TransformOnly,
			UpdateAll,
			UpdateAllDeleteOld
		};

		struct SerializationOptions {
			EntitySerialization::Type type = EntitySerialization::Type::Undefined;
			std::function<bool(EntityRef)> serializeAsStub;

			SerializationOptions() = default;
			explicit SerializationOptions(EntitySerialization::Type type, std::function<bool(EntityRef)> serializeAsStub = {})
				: type(type)
				, serializeAsStub(std::move(serializeAsStub))
			{}
		};

		explicit EntityFactory(World& world, Resources& resources);
		virtual ~EntityFactory();

		World& getWorld();
		
		EntityRef createEntity(const String& prefabName);
		EntityRef createEntity(const EntityData& data, EntityRef parent = EntityRef(), EntityScene* scene = nullptr);
		EntityScene createScene(const std::shared_ptr<const Prefab>& scene, bool allowReload, uint8_t worldPartition = 0);

		void updateEntity(EntityRef& entity, const IEntityData& data, int serializationMask, EntityScene* scene = nullptr, IDataInterpolatorSetRetriever* interpolators = nullptr);

		std::pair<EntityRef, std::optional<UUID>> loadEntityDelta(const EntityDataDelta& delta, const std::optional<UUID>& uuidSrc); // Returns entity and parent UUID
		std::tuple<EntityData, std::shared_ptr<const Prefab>, UUID> prefabDeltaToEntityData(const EntityDataDelta& delta);

		EntityData serializeEntity(EntityRef entity, const SerializationOptions& options, bool canStoreParent = true);
		EntityDataDelta serializeEntityAsDelta(EntityRef entity, const SerializationOptions& options, const EntityDataDelta::Options& deltaOptions, bool canStoreParent = true);
		EntityDataDelta entityDataToPrefabDelta(EntityData data, std::shared_ptr<const Prefab> prefab, const EntityDataDelta::Options& deltaOptions);
		
		std::shared_ptr<EntityFactoryContext> makeStandaloneContext();

	private:
		World& world;
		Resources& resources;

		void updateEntityNode(const IEntityData& iData, EntityRef entity, std::optional<EntityRef> parent, const std::shared_ptr<EntityFactoryContext>& context);
		void updateEntityComponents(EntityRef entity, const EntityData& data, const EntityFactoryContext& context);
		void updateEntityComponentsDelta(EntityRef entity, const EntityDataDelta& delta, const EntityFactoryContext& context);
		void updateEntityChildren(EntityRef entity, const EntityData& data, const std::shared_ptr<EntityFactoryContext>& context);
		void updateEntityChildrenDelta(EntityRef entity, const EntityDataDelta& delta, const std::shared_ptr<EntityFactoryContext>& context);
		std::optional<ConfigNode> getComponentsWithPrefabDefaults(EntityRef entity, const EntityFactoryContext& context, const ConfigNode& componentData, const String& componentName);

		EntityRef tryGetEntity(const UUID& instanceUUID, EntityFactoryContext& context, bool allowWorldLookup);
		EntityRef getEntity(const UUID& instanceUUID, EntityFactoryContext& context, bool allowWorldLookup);

		std::shared_ptr<EntityFactoryContext> makeContext(const IEntityData& data, std::optional<EntityRef> existing, EntityScene* scene, bool updateContext, int serializationMask, EntityFactoryContext* parent = nullptr, IDataInterpolatorSetRetriever* interpolators = nullptr);
		EntityRef instantiateEntity(const EntityData& data, EntityFactoryContext& context, bool allowWorldLookup);
		void preInstantiateEntities(const IEntityData& data, EntityFactoryContext& context, int depth);
		void collectExistingEntities(EntityRef entity, EntityFactoryContext& context);

		[[nodiscard]] std::shared_ptr<const Prefab> getPrefab(const String& id) const;
		[[nodiscard]] std::shared_ptr<const Prefab> getPrefab(std::optional<EntityRef> entity, const IEntityData& data) const;
	};

	class EntityFactoryContext {
	public:
		EntityFactoryContext(World& world, Resources& resources, int entitySerializationMask, bool update, std::shared_ptr<const Prefab> prefab = {}, const IEntityData* origEntityData = nullptr, EntityScene* scene = nullptr, EntityFactoryContext* parent = nullptr, IDataInterpolatorSetRetriever* interpolators = nullptr);
		
		template <typename T>
		CreateComponentFunctionResult createComponent(EntityRef& e, const ConfigNode& componentData) const
		{
			CreateComponentFunctionResult result;
			result.componentId = T::componentIndex;
			
			if (componentData.getType() == ConfigNodeType::Del) {
				e.removeComponent<T>();
			} else {
				auto* comp = e.tryGetComponent<T>();
				if (comp) {
					comp->deserialize(entitySerializationContext, componentData);
				} else {
					T component;
					component.deserialize(entitySerializationContext, componentData);
					e.addComponent<T>(std::move(component));
					result.created = true;
				}
			}

			return result;
		}

		const std::shared_ptr<const Prefab>& getPrefab() const { return prefab; }
		const EntitySerializationContext& getEntitySerializationContext() const { return entitySerializationContext; }
		World& getWorld() const { return *world; }
		EntityId getEntityIdFromUUID(const UUID& uuid) const;

		void addEntity(EntityRef entity);
		void notifyEntity(const EntityRef& entity) const;
		EntityRef getEntity(const UUID& uuid, bool allowPrefabUUID, bool allowWorldLookup) const;

		bool needsNewContextFor(const EntityData& value) const;
		bool isUpdateContext() const;

		const IEntityData& getRootEntityData() const;
		EntityScene* getScene() const;
		uint8_t getWorldPartition() const;
		void setWorldPartition(uint8_t partition);

		void setCurrentEntity(EntityRef entity);
		EntityRef getCurrentEntity() const;

	private:
		EntitySerializationContext entitySerializationContext;
		std::shared_ptr<const Prefab> prefab;
		World* world;
		EntityScene* scene;
		EntityFactoryContext* parent;
		Vector<EntityRef> entities;
		bool update = false;
		uint8_t worldPartition = 0;
		EntityRef curEntity;

		const IEntityData* entityData = nullptr;
		EntityData instancedEntityData;

		void setEntityData(const IEntityData& iData);
	};
}
