#include "graphics/sprite/sprite_painter.h"
#include "graphics/sprite/sprite.h"
#include "graphics/painter.h"
#include "graphics/text/text_renderer.h"

using namespace Halley;

SpritePainterEntry::SpritePainterEntry(gsl::span<const Sprite> sprites, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip)
	: ptr(sprites.empty() ? nullptr : &sprites[0])
	, count(uint32_t(sprites.size()))
	, tieBreaker(tieBreaker)
{
	this->clip = clip.value_or(Rect4f {});
	packedData = packData(SpritePainterEntryType::SpriteRef, layer, insertOrder, clip.has_value());
}

SpritePainterEntry::SpritePainterEntry(gsl::span<const TextRenderer> texts, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip)
	: ptr(texts.empty() ? nullptr : &texts[0])
	, count(uint32_t(texts.size()))
	, tieBreaker(tieBreaker)
{
	this->clip = clip.value_or(Rect4f {});
	packedData = packData(SpritePainterEntryType::TextRef, layer, insertOrder, clip.has_value());
}

SpritePainterEntry::SpritePainterEntry(SpritePainterEntryType type, size_t spriteIdx, size_t count, int layer, float tieBreaker, size_t insertOrder, std::optional<Rect4f> clip)
	: count(uint32_t(count))
	, index(static_cast<int>(spriteIdx))
	, tieBreaker(tieBreaker)
{
	this->clip = clip.value_or(Rect4f {});
	packedData = packData(type, layer, insertOrder, clip.has_value());
}

bool SpritePainterEntry::operator<(const SpritePainterEntry& o) const
{
	const uint32_t l = packedData & 0xff00000;
	const uint32_t lo = o.packedData & 0xff00000;
	
	if (l != lo) {
		return l < lo;
	}
	
	if (tieBreaker != o.tieBreaker) {
		return tieBreaker < o.tieBreaker;
	}

	const uint32_t i = packedData & 0xfffff;
	const uint32_t io = o.packedData & 0xfffff;
	
	return i < io;
}

SpritePainterEntryType SpritePainterEntry::getType() const
{
	return static_cast<SpritePainterEntryType>(packedData >> 29u);
}

gsl::span<const Sprite> SpritePainterEntry::getSprites() const
{
	Expects(ptr != nullptr);
	Expects(getType() == SpritePainterEntryType::SpriteRef);
	return gsl::span<const Sprite>(static_cast<const Sprite*>(ptr), count);
}

gsl::span<const TextRenderer> SpritePainterEntry::getTexts() const
{
	Expects(ptr != nullptr);
	Expects(getType() == SpritePainterEntryType::TextRef);
	return gsl::span<const TextRenderer>(static_cast<const TextRenderer*>(ptr), count);
}

uint32_t SpritePainterEntry::getIndex() const
{
	Expects(ptr == nullptr);
	return index;
}

uint32_t SpritePainterEntry::getCount() const
{
	return count;
}

std::optional<Rect4f> SpritePainterEntry::getClip() const
{
	if ((packedData & 0x10000000) != 0) {
		return clip;
	} else {
		return std::nullopt;
	}
}

uint32_t SpritePainterEntry::packData(SpritePainterEntryType type, int layer, size_t insertOrder, bool clip)
{
	Expects(layer <= 0xff);
	Expects(insertOrder <= 0xfffff);

	uint32_t mask = static_cast<uint32_t>(type) << 29u;

	if (clip) {
		mask |= 0x10000000;
	}
	
	mask |= static_cast<uint32_t>(layer & 0xff) << 20u;
	mask |= static_cast<uint32_t>(insertOrder & 0xfffff);

	return mask;
}

void SpritePainterBucket::start()
{
	sprites.clear();
	cachedSprites.clear();
	cachedText.clear();
}

void SpritePainterBucket::add(const Sprite& sprite, int layer, float tieBreaker, std::optional<Rect4f>& clip)
{
	sprites.push_back(SpritePainterEntry(gsl::span<const Sprite>(&sprite, 1), layer, tieBreaker, sprites.size(), std::move(clip)));
	dirty = true;
}

void SpritePainterBucket::addCopy(const Sprite& sprite, int layer, float tieBreaker, std::optional<Rect4f>& clip)
{
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::SpriteCached, cachedSprites.size(), 1, layer, tieBreaker, sprites.size(), std::move(clip)));
	cachedSprites.push_back(sprite);
	dirty = true;
}

void SpritePainterBucket::add(gsl::span<const Sprite> sprites, int layer, float tieBreaker, std::optional<Rect4f>& clip)
{
	if (!sprites.empty()) {
		this->sprites.push_back(SpritePainterEntry(sprites, layer, tieBreaker, this->sprites.size(), std::move(clip)));
		dirty = true;
	}
}

void SpritePainterBucket::addCopy(gsl::span<const Sprite> sprites, int layer, float tieBreaker, std::optional<Rect4f>& clip)
{
	if (!sprites.empty()) {
		this->sprites.push_back(SpritePainterEntry(SpritePainterEntryType::SpriteCached, cachedSprites.size(), sprites.size(), layer, tieBreaker, this->sprites.size(), std::move(clip)));
		cachedSprites.insert(cachedSprites.end(), sprites.begin(), sprites.end());
		dirty = true;
	}
}

void SpritePainterBucket::add(const TextRenderer& text, int layer, float tieBreaker, std::optional<Rect4f>& clip)
{
	sprites.push_back(SpritePainterEntry(gsl::span<const TextRenderer>(&text, 1), layer, tieBreaker, sprites.size(), std::move(clip)));
	dirty = true;
}

void SpritePainterBucket::addCopy(const TextRenderer& text, int layer, float tieBreaker, std::optional<Rect4f>& clip)
{
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::TextCached, cachedText.size(), 1, layer, tieBreaker, sprites.size(), std::move(clip)));
	cachedText.push_back(text);
	dirty = true;
}

void SpritePainterBucket::add(SpritePainterEntry::Callback callback, int layer, float tieBreaker, std::optional<Rect4f>& clip)
{
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::Callback, callbacks.size(), 1, layer, tieBreaker, sprites.size(), std::move(clip)));
	callbacks.push_back(std::move(callback));
	dirty = true;
}

void SpritePainterBucket::draw(Painter& painter)
{
	if (dirty) {
		std::sort(sprites.begin(), sprites.end());
		dirty = false;
	}

	// View
	const auto& cam = painter.getCurrentCamera();
	const Rect4f view = cam.getClippingRectangle();

	// Draw!
	for (auto& s : sprites) {
		const auto type = s.getType();
			
		if (type == SpritePainterEntryType::SpriteRef) {
			draw(s.getSprites(), painter, view, s.getClip());
		} else if (type == SpritePainterEntryType::SpriteCached) {
			draw(gsl::span<const Sprite>(cachedSprites.data() + s.getIndex(), s.getCount()), painter, view, s.getClip());
		} else if (type == SpritePainterEntryType::TextRef) {
			draw(s.getTexts(), painter, view, s.getClip());
		} else if (type == SpritePainterEntryType::TextCached) {
			draw(gsl::span<const TextRenderer>(cachedText.data() + s.getIndex(), s.getCount()), painter, view, s.getClip());
		} else if (type == SpritePainterEntryType::Callback) {
			draw(callbacks.at(s.getIndex()), painter, s.getClip());
		}
	}
	painter.flush();
}

void SpritePainterBucket::draw(gsl::span<const Sprite> sprites, Painter& painter, const Rect4f& view, const std::optional<Rect4f>& clip)
{
	for (const auto& sprite: sprites) {
		if (sprite.isInView(view)) {
			sprite.draw(painter, clip);
		}
	}
}

void SpritePainterBucket::draw(gsl::span<const TextRenderer> texts, Painter& painter, const Rect4f& view, const std::optional<Rect4f>& clip)
{
	for (const auto& text: texts) {
		text.draw(painter, clip);
	}
}

void SpritePainterBucket::draw(const SpritePainterEntry::Callback& callback, Painter& painter, const std::optional<Rect4f>& clip)
{
	if (clip) {
		painter.setRelativeClip(clip.value());
	}
	callback(painter);
	if (clip) {
		painter.setClip();
	}
}

void SpritePainter::start()
{
	buckets.resize(32);
	for (auto& b : buckets) {
		b.start();
	}
}

void SpritePainter::start(size_t)
{
	start();
}

void SpritePainter::add(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	size_t idx = 0;
	while (mask != 0) {
		if ((mask & 1) != 0) {
			buckets[idx].add(sprite, layer, tieBreaker, clip);
		}
		++idx;
		mask >>= 1;
	}
}

void SpritePainter::addCopy(const Sprite& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	size_t idx = 0;
	while (mask != 0) {
		if ((mask & 1) != 0) {
			buckets[idx].addCopy(sprite, layer, tieBreaker, clip);
		}
		++idx;
		mask >>= 1;
	}
}

void SpritePainter::add(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	size_t idx = 0;
	while (mask != 0) {
		if ((mask & 1) != 0) {
			buckets[idx].add(sprites, layer, tieBreaker, clip);
		}
		++idx;
		mask >>= 1;
	}
}

void SpritePainter::addCopy(gsl::span<const Sprite> sprites, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	size_t idx = 0;
	while (mask != 0) {
		if ((mask & 1) != 0) {
			buckets[idx].addCopy(sprites, layer, tieBreaker, clip);
		}
		++idx;
		mask >>= 1;
	}
}

void SpritePainter::add(const TextRenderer& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	size_t idx = 0;
	while (mask != 0) {
		if ((mask & 1) != 0) {
			buckets[idx].add(text, layer, tieBreaker, clip);
		}
		++idx;
		mask >>= 1;
	}
}

void SpritePainter::addCopy(const TextRenderer& text, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	size_t idx = 0;
	while (mask != 0) {
		if ((mask & 1) != 0) {
			buckets[idx].addCopy(text, layer, tieBreaker, clip);
		}
		++idx;
		mask >>= 1;
	}
}

void SpritePainter::add(SpritePainterEntry::Callback callback, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
{
	size_t idx = 0;
	while (mask != 0) {
		if ((mask & 1) != 0) {
			buckets[idx].add(callback, layer, tieBreaker, clip);
		}
		++idx;
		mask >>= 1;
	}
}

void SpritePainter::draw(int mask, Painter& painter)
{
	size_t idx = 0;
	while (mask != 0) {
		if ((mask & 1) != 0) {
			buckets[idx].draw(painter);
		}
		++idx;
		mask >>= 1;
	}
}
