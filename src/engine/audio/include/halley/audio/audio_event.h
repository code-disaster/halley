#pragma once
#include "halley/resources/resource.h"
#include "halley/maths/range.h"
#include "halley/data_structures/maybe.h"
#include "audio_clip.h"
#include "audio_dynamics_config.h"

namespace Halley
{
	class AudioPosition;
	class AudioEngine;
	class ConfigNode;
	class ConfigFile;
	class ResourceLoader;
	class IAudioEventAction;
	class Resources;
	class AudioDynamicsConfig;

	class AudioEvent final : public Resource
	{
	public:
		AudioEvent();
		explicit AudioEvent(const ConfigNode& config);

		size_t run(AudioEngine& engine, uint32_t id, const AudioPosition& position) const;

		void serialize(Serializer& s) const;
		void deserialize(Deserializer& s);

		void reload(Resource&& resource) override;
		static std::shared_ptr<AudioEvent> loadResource(ResourceLoader& loader);
		constexpr static AssetType getAssetType() { return AssetType::AudioEvent; }

	private:
		Vector<std::unique_ptr<IAudioEventAction>> actions;
		void loadDependencies(Resources& resources) const;
	};

	enum class AudioEventActionType
	{
		Play,
		Stop,
		Pause,
		Resume,
		SetSwitch,
		SetVariable
	};

	template <>
	struct EnumNames<AudioEventActionType> {
		constexpr std::array<const char*, 6> operator()() const {
			return{{
				"play",
				"stop",
				"pause",
				"resume",
				"setSwitch",
				"setVariable"
			}};
		}
	};

	class IAudioEventAction
	{
	public:
		virtual ~IAudioEventAction() {}
		virtual bool run(AudioEngine& engine, uint32_t id, const AudioPosition& position) const = 0;
		virtual AudioEventActionType getType() const = 0;

		virtual void serialize(Serializer& s) const = 0;
		virtual void deserialize(Deserializer& s) = 0;
		virtual void loadDependencies(const Resources& resources) {}
	};

	class AudioEventActionPlay final : public IAudioEventAction
	{
	public:
		explicit AudioEventActionPlay(AudioEvent& event);
		AudioEventActionPlay(AudioEvent& event, const ConfigNode& config);

		bool run(AudioEngine& engine, uint32_t id, const AudioPosition& position) const override;
		AudioEventActionType getType() const override;

		void serialize(Serializer& s) const override;
		void deserialize(Deserializer& s) override;

		void loadDependencies(const Resources& resources) override;

	private:
		AudioEvent& event;
		Vector<String> clips;
		Vector<std::shared_ptr<const AudioClip>> clipData;
		String group;
		Range<float> pitch;
		Range<float> volume;
		float delay = 0.0f;
		float minimumSpace = 0.0f;
		bool loop = false;
		std::optional<AudioDynamicsConfig> dynamics;
	};
}
