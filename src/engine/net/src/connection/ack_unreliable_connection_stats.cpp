#include "connection/ack_unreliable_connection_stats.h"

using namespace Halley;

AckUnreliableConnectionStats::AckUnreliableConnectionStats(size_t capacity, size_t lineSize)
	: capacity(capacity)
	, lineSize(lineSize)
	, lineStart(lineSize)
{
	packetStats.resize(capacity);
}

void AckUnreliableConnectionStats::update(Time time)
{
}

void AckUnreliableConnectionStats::onPacketSent(uint16_t sequence, size_t size)
{
	addPacket(PacketStats{ sequence, State::Sent, true, size });
}

void AckUnreliableConnectionStats::onPacketReceived(uint16_t sequence, size_t size, bool resend)
{
	addPacket(PacketStats{ sequence, State::Received, false, size });
}

void AckUnreliableConnectionStats::onPacketResent(uint16_t sequence)
{
	for (auto& packet: packetStats) {
		if (packet.outbound && packet.seq == sequence) {
			packet.state = State::Resent;
			return;
		}
	}
}

void AckUnreliableConnectionStats::onPacketAcked(uint16_t sequence)
{
	for (auto& packet: packetStats) {
		if (packet.outbound && packet.seq == sequence) {
			packet.state = State::Acked;
			return;
		}
	}
}

gsl::span<const AckUnreliableConnectionStats::PacketStats> AckUnreliableConnectionStats::getPacketStats() const
{
	return packetStats;
}

size_t AckUnreliableConnectionStats::getLineStart() const
{
	return lineStart;
}

size_t AckUnreliableConnectionStats::getLineSize() const
{
	return lineSize;
}

void AckUnreliableConnectionStats::addPacket(PacketStats stats)
{
	packetStats[pos] = stats;
	pos = (pos + 1) % capacity;

	// Upon reaching a new line, clear it
	if (pos % lineSize == 0) {
		lineStart = (pos + lineSize) % capacity;
		for (size_t i = 0; i < lineSize; ++i) {
			packetStats[(pos + i) % capacity] = PacketStats();
		}
	}
}
