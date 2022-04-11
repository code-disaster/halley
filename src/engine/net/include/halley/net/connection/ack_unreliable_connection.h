#pragma once

#include "iconnection.h"
#include "network_packet.h"
#include <memory>
#include "halley/data_structures/vector.h"
#include <chrono>
#include <deque>
#include <limits>
#include <cstdint>
#include <optional>

namespace Halley
{
	class IAckUnreliableConnectionListener
	{
	public:
		virtual ~IAckUnreliableConnectionListener() {}

		virtual void onPacketAcked(int tag) = 0;
	};

	class IAckUnreliableConnectionStatsListener
	{
	public:
		virtual ~IAckUnreliableConnectionStatsListener() {}

		virtual void onPacketSent(uint16_t sequence, size_t size) = 0;
		virtual void onPacketResent(uint16_t sequence) = 0;
		virtual void onPacketAcked(uint16_t sequence) = 0;
		virtual void onPacketReceived(uint16_t sequence, size_t size, bool resend) = 0;
	};

	class AckUnreliableSubPacket
	{
	public:
		Vector<gsl::byte> data;
		int tag = -1;
		//bool reliable = false;
		bool resends = false;
		uint16_t seq = std::numeric_limits<uint16_t>::max();
		uint16_t resendSeq = 0;

		AckUnreliableSubPacket()
		{}

		AckUnreliableSubPacket(AckUnreliableSubPacket&& other) = default;

		AckUnreliableSubPacket(Vector<gsl::byte>&& data)
			: data(data)
			, resends(false)
		{}

		AckUnreliableSubPacket(Vector<gsl::byte>&& data, uint16_t resendSeq)
			: data(data)
			, resends(true)
			, resendSeq(resendSeq)
		{}
	};

	class AckUnreliableConnection : public IConnection
	{
		using Clock = std::chrono::steady_clock;

		struct SentPacketData
		{
			std::vector<int> tags;
			Clock::time_point timestamp = {};
			bool waiting = false;
		};

	public:
		AckUnreliableConnection(std::shared_ptr<IConnection> parent);

		void close() override;
		[[nodiscard]] ConnectionStatus getStatus() const override;
		[[nodiscard]] bool isSupported(TransmissionType type) const override;
		[[nodiscard]] bool receive(InboundNetworkPacket& packet) override;

		void send(TransmissionType type, OutboundNetworkPacket packet) override;
		[[nodiscard]] uint16_t sendTagged(gsl::span<const AckUnreliableSubPacket> subPackets);
		void sendAckPacketsIfNeeded();

		void addAckListener(IAckUnreliableConnectionListener& listener);
		void removeAckListener(IAckUnreliableConnectionListener& listener);

		[[nodiscard]] float getLatency() const { return lag; }
		[[nodiscard]] float getTimeSinceLastSend() const;
		[[nodiscard]] float getTimeSinceLastReceive() const;

		void setStatsListener(IAckUnreliableConnectionStatsListener* listener);

	private:
		std::shared_ptr<IConnection> parent;

		uint16_t nextSequenceToSend = 0;
		uint16_t highestReceived = 0xFFFF;

		Vector<char> receivedSeqs; // 0 = not received, 1 = received
		Vector<SentPacketData> sentPackets;
		std::deque<InboundNetworkPacket> pendingPackets;

		Vector<IAckUnreliableConnectionListener*> ackListeners;
		IAckUnreliableConnectionStatsListener* statsListener = nullptr;

		float lag = 1; // Start at 1 second
		float curLag = 0;

		Clock::time_point lastReceive;
		Clock::time_point lastSend;
		std::optional<Clock::time_point> earliestUnackedMsg;

		void processReceivedPacket(InboundNetworkPacket& packet);
		unsigned int generateAckBits();

		void processReceivedAcks(uint16_t ack, unsigned int ackBits);
		bool onSeqReceived(uint16_t sequence, bool hasSubPacket);
		void onAckReceived(uint16_t sequence);

		void startLatencyReport();
		void reportLatency(float lag);
		void endLatencyReport();

		void notifySend(uint16_t sequence, size_t size);
		void notifyResend(uint16_t sequence);
		void notifyAck(uint16_t sequence);
		void notifyReceive(uint16_t sequence, size_t size, bool resend);
	};
}
