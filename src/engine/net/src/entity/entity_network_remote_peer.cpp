#include "entity/entity_network_remote_peer.h"

#include "entity/entity_network_session.h"
#include "halley/entity/entity_factory.h"
#include "halley/entity/world.h"
#include "halley/support/logger.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

EntityNetworkRemotePeer::EntityNetworkRemotePeer(EntityNetworkSession& parent, NetworkSession::PeerId peerId)
	: parent(&parent)
	, peerId(peerId)
{}

NetworkSession::PeerId EntityNetworkRemotePeer::getPeerId() const
{
	return peerId;
}

void EntityNetworkRemotePeer::sendEntities(Time t, gsl::span<const std::pair<EntityId, uint8_t>> entityIds)
{
	Expects(isAlive());
	
	// Mark all as not alive
	for (auto& e: outboundEntities) {
		e.second.alive = false;
	}
	
	for (auto [entityId, ownerId]: entityIds) {
		if (ownerId == peerId) {
			// Don't send updates back to the owner
			continue;
		}

		const auto entity = parent->getWorld().getEntity(entityId);

		if (const auto iter = outboundEntities.find(entityId); iter == outboundEntities.end()) {
			createEntity(entity);
		} else {
			updateEntity(t, iter->second, entity);
		}
	}

	// Destroy dead entities
	for (auto& e: outboundEntities) {
		if (!e.second.alive) {
			destroyEntity(e.second);
		}
	}
	std_ex::erase_if_value(outboundEntities, [](const OutboundEntity& e) { return !e.alive; });
}

void EntityNetworkRemotePeer::receiveEntityPacket(NetworkSession::PeerId fromPeerId, EntityNetworkHeaderType type, InboundNetworkPacket packet)
{
	Expects(isAlive());

	EntityNetworkEntityHeader header;
	packet.extractHeader(header);
	const auto networkEntityId = header.entityId;

	if (type == EntityNetworkHeaderType::Create) {
		
	} else if (type == EntityNetworkHeaderType::Update) {
		
	} else if (type == EntityNetworkHeaderType::Destroy) {
		
	}
}

void EntityNetworkRemotePeer::destroy(World& world)
{
	if (alive) {
		for (const auto& [k, v]: inboundEntities) {
			world.destroyEntity(v.worldId);
		}
		inboundEntities.clear();
		alive = false;
	}
}

bool EntityNetworkRemotePeer::isAlive() const
{
	return alive;
}

void EntityNetworkRemotePeer::createEntity(EntityRef entity)
{
	OutboundEntity result;

	result.networkId = assignId();
	result.data = parent->getFactory().serializeEntity(entity, parent->getSerializationOptions());

	auto deltaData = parent->getFactory().entityDataToPrefabDelta(result.data, entity.getPrefab(), parent->getEntityDeltaOptions());
	const auto bytes = Serializer::toBytes(result.data);
	
	auto packet = OutboundNetworkPacket(bytes);
	packet.addHeader(EntityNetworkEntityHeader(result.networkId));
	packet.addHeader(EntityNetworkHeader(EntityNetworkHeaderType::Create));
	parent->getSession().sendToPeer(packet, peerId);
	
	outboundEntities[entity.getEntityId()] = std::move(result);
}

void EntityNetworkRemotePeer::updateEntity(Time t, OutboundEntity& remote, EntityRef entity)
{
	remote.timeSinceSend += t;
	if (remote.timeSinceSend < parent->getMinSendInterval()) {
		return;
	}
	
	auto newData = parent->getFactory().serializeEntity(entity, parent->getSerializationOptions());
	const auto deltaData = EntityDataDelta(remote.data, newData, parent->getEntityDeltaOptions());

	if (deltaData.hasChange()) {
		remote.data = std::move(newData);
		remote.timeSinceSend = t;

		const auto bytes = Serializer::toBytes(deltaData);
		
		auto packet = OutboundNetworkPacket(bytes);
		packet.addHeader(EntityNetworkEntityHeader(remote.networkId));
		packet.addHeader(EntityNetworkHeader(EntityNetworkHeaderType::Update));
		parent->getSession().sendToPeer(packet, peerId);
	}
}

void EntityNetworkRemotePeer::destroyEntity(OutboundEntity& remote)
{
	auto packet = OutboundNetworkPacket(Bytes());
	packet.addHeader(EntityNetworkEntityHeader(remote.networkId));
	packet.addHeader(EntityNetworkHeader(EntityNetworkHeaderType::Destroy));
	parent->getSession().sendToPeer(packet, peerId);
	allocatedOutboundIds.erase(remote.networkId);
}

uint16_t EntityNetworkRemotePeer::assignId()
{
	for (uint16_t i = 0; i < std::numeric_limits<uint16_t>::max() - 1; ++i) {
		const uint16_t id = i + nextId;
		if (!allocatedOutboundIds.contains(id)) {
			allocatedOutboundIds.insert(id);
			nextId = id + 1;
			return id;
		}
	}
	throw Exception("Unable to allocate network id for entity.", HalleyExceptions::Network);
}