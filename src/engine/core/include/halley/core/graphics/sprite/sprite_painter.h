#pragma once

#include <halley/data_structures/vector.h>
#include <cstddef>
#include "halley/maths/rect.h"
#include <limits>
#include <optional>

namespace Halley
{
	class TextRenderer;
	class String;
	class Sprite;
	class Painter;

	enum class SpritePainterEntryType
	{
		SpriteRef,
		SpriteCached,
		TextRef,
		TextCached,
		Callback
	};

	class SpritePainterEntry
	{
	public:
		using Callback = std::function<void(Painter&)>;
		
		SpritePainterEntry(gsl::span<const Sprite> sprites, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip);
		SpritePainterEntry(gsl::span<const TextRenderer> texts, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip);
		SpritePainterEntry(SpritePainterEntryType type, size_t spriteIdx, size_t count, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip);

		bool operator<(const SpritePainterEntry& o) const;
		SpritePainterEntryType getType() const;
		gsl::span<const Sprite> getSprites() const;
		gsl::span<const TextRenderer> getTexts() const;
		uint32_t getIndex() const;
		uint32_t getCount() const;
		std::optional<Rect4f> getClip() const;

		static uint32_t packData(SpritePainterEntryType type, int layer, size_t insertOrder, bool clip);
	
	private:
		const void* ptr = nullptr;
		Rect4f clip;
		uint32_t count = 0;
		uint32_t index = std::numeric_limits<uint32_t>::max();
		// bits		desc
		// 31::29	type
		//   28		if set, clip is valid/used
		// 27::20	layer
		// 19::00	insertOrder
		uint32_t packedData;
		float tieBreaker;
	};

	class SpritePainterBucket
	{
	public:
		void start();

		void add(const Sprite& sprite, int layer, float tieBreaker, std::optional<Rect4f>& clip);
		void addCopy(const Sprite& sprite, int layer, float tieBreaker, std::optional<Rect4f>& clip);
		void add(gsl::span<const Sprite> sprites, int layer, float tieBreaker, std::optional<Rect4f>& clip);
		void addCopy(gsl::span<const Sprite> sprites, int layer, float tieBreaker, std::optional<Rect4f>& clip);
		void add(const TextRenderer& sprite, int layer, float tieBreaker, std::optional<Rect4f>& clip);
		void addCopy(const TextRenderer& text, int layer, float tieBreaker, std::optional<Rect4f>& clip);
		void add(SpritePainterEntry::Callback callback, int layer, float tieBreaker, std::optional<Rect4f>& clip);

		void draw(Painter& painter);

	private:
		Vector<SpritePainterEntry> sprites;
		Vector<Sprite> cachedSprites;
		Vector<TextRenderer> cachedText;
		Vector<SpritePainterEntry::Callback> callbacks;
		bool dirty = false;

		static void draw(gsl::span<const Sprite> sprite, Painter& painter, const Rect4f& view, const std::optional<Rect4f>& clip);
		static void draw(gsl::span<const TextRenderer> text, Painter& painter, const Rect4f& view, const std::optional<Rect4f>& clip);
		static void draw(const SpritePainterEntry::Callback& callback, Painter& painter, const std::optional<Rect4f>& clip);
	};

	class SpritePainter
	{
	public:
		void start();
		[[deprecated]] void start(size_t nSprites);
		
		void add(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void addCopy(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void addCopy(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(const TextRenderer& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void addCopy(const TextRenderer& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		void add(SpritePainterEntry::Callback callback, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip = {});
		
		void draw(int mask, Painter& painter);

	private:
		Vector<SpritePainterBucket> buckets;
	};
}
