#include "halley/net/session/session_multiplayer.h"

#include "halley/api/halley_api.h"
#include "halley/api/platform_api.h"
#include "halley/entity/components/transform_2d_component.h"
#include "halley/net/entity/entity_network_session.h"
#include "halley/net/session/network_session.h"

using namespace Halley;

SessionMultiplayer::SessionMultiplayer(const HalleyAPI& api, Resources& resources, ConnectionOptions options, SessionSettings settings)
	: host(options.mode == Mode::Host)
{
	playerName = api.platform->getPlayerName();

	service = api.platform->createNetworkService(6060);
	if (!service) {
		throw Exception("Unable to initialize Witchbrook multiplayer: platform has no network service implementation.", 0);
	}
	
	session = std::make_shared<NetworkSession>(*service, settings.networkVersion, playerName);
	entitySession = std::make_unique<EntityNetworkSession>(session, resources, std::move(settings.ignoreComponents), this);
	setupDictionary(entitySession->getSerializationDictionary(), std::move(settings.serializationDict));
	
	if (options.mode == Mode::Host) {
		Logger::logDev("Starting multiplayer session as the host.");
		session->host(options.maxPlayers);
		lobby = api.platform->makeMultiplayerLobby(session->getHostAddress());
		lobby->setPrivacy(MultiplayerPrivacy::FriendsOnly);
	} else if (options.mode == Mode::Join && options.clientConnectTo) {
		Logger::logDev("Starting multiplayer session as a client, connecting to " + options.clientConnectTo.value());
		session->join(options.clientConnectTo.value());
	} else if (options.mode == Mode::WaitForLobby) {
		Logger::logDev("Waiting for lobby callback...");
		api.platform->setJoinCallback([=] (PlatformJoinCallbackParameters params)
		{
			Logger::logDev("Starting multiplayer session as a client, connecting to " + params.param);
			session->join(params.param);
		});
		api.platform->setPreparingToJoinCallback([=]()
		{
			Logger::logDev("Preparing to join lobby...");
		});
		api.platform->setJoinErrorCallback([=]()
		{
			Logger::logError("Error joining lobby.");
		});
	}
}

bool SessionMultiplayer::isMultiplayer() const
{
	return true;
}

bool SessionMultiplayer::isHost() const
{
	return host;
}

bool SessionMultiplayer::hasLocalSave() const
{
	return host;
}

bool SessionMultiplayer::isReadyToStart() const
{
	return entitySession->isReadyToStart();
}

bool SessionMultiplayer::hasHostAuthority() const
{
	return host;
}

Vector<Rect4f> SessionMultiplayer::getRemoteViewPorts() const
{
	if (host) {
		const auto viewPorts = entitySession->getRemoteViewPorts();
		Vector<Rect4f> result;
		result.reserve(viewPorts.size());
		for (auto& v: viewPorts) {
			result.push_back(Rect4f(v));
		}
		return result;
	}
	return {};
}

bool SessionMultiplayer::isWaitingForInitialViewPort() const
{
	return waitingForViewPort;
}

void SessionMultiplayer::reportInitialViewPort(Rect4f viewPort)
{
	if (waitingForViewPort) {
		auto& sharedData = session->getMySharedData<EntityClientSharedData>();
		sharedData.viewRect = Rect4i(viewPort);
		sharedData.markModified();
		waitingForViewPort = false;
	}
}

EntityNetworkSession* SessionMultiplayer::getEntityNetworkSession()
{
	return entitySession.get();
}

NetworkSession* SessionMultiplayer::getNetworkSession()
{
	return session.get();
}

const String& SessionMultiplayer::getPlayerName() const
{
	return playerName;
}

void SessionMultiplayer::setNetworkQuality(NetworkService::Quality level)
{
	service->setSimulateQualityLevel(level);
}

void SessionMultiplayer::onStartSession(NetworkSession::PeerId myPeerId)
{
	if (!host) {
		waitingForViewPort = true;
	}
}

bool SessionMultiplayer::update()
{
	entitySession->receiveUpdates();

	return session->getStatus() != ConnectionStatus::Closed;
}

void SessionMultiplayer::onRemoteEntityCreated(EntityRef entity, NetworkSession::PeerId peerId)
{
}

void SessionMultiplayer::setupInterpolators(DataInterpolatorSet& interpolatorSet, EntityRef entity, bool remote)
{
}

bool SessionMultiplayer::isEntityInView(EntityRef entity, const EntityClientSharedData& clientData)
{
	const auto* transform = entity.tryGetComponent<Transform2DComponent>();

	if (!transform) {
		return true;
	}

	// Rect not defined, don't send
	if (!clientData.viewRect) {
		return false;
	}

	// Send if it's in an expanded rect
	return clientData.viewRect->grow(256).contains(Vector2i(transform->getGlobalPosition()));
}
void SessionMultiplayer::setupDictionary(SerializationDictionary& dict, std::shared_ptr<const ConfigFile> serializationDict)
{
	dict = SerializationDictionary(serializationDict->getRoot());
	//dict.setLogMissingStrings(true, 30, 100);
}
