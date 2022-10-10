#include "graphics/sprite/sprite_painter.h"
#include "graphics/sprite/sprite.h"
#include "graphics/painter.h"

#include "graphics/material/material.h"
#include "graphics/text/text_renderer.h"
#include "halley/utils/algorithm.h"

using namespace Halley;

void MaterialRecycler::startFrame()
{
	for (auto& [k, v]: entries) {
		++v.age;
	}
	std_ex::erase_if_value(entries, [] (const Entry& e)
	{
		return e.age >= 3;
	});
}

std::shared_ptr<Material> MaterialRecycler::cloneMaterial(const Material& material)
{
	const auto iter = entries.find(material.getHash());
	if (iter == entries.end()) {
		auto m = material.clone();
		entries[material.getHash()] = Entry{ m, 0 };
		return m;
	} else {
		iter->second.age = 0;
		return iter->second.material;
	}
}

Sprite MaterialRecycler::clone(const Sprite& sprite)
{
	auto s2 = sprite.clone();
	if (sprite.hasMaterial()) {
		s2.setMaterial(cloneMaterial(sprite.getMaterial()));
	}
	return s2;
}

TextRenderer MaterialRecycler::clone(const TextRenderer& text)
{
	// TODO: clone materials
	return text.clone();
}

static uint16_t packTypeAndLayer(SpritePainterEntryType type, int layer)
{
	const uint16_t t = uint16_t(type);
	Ensures(t < 8);

	Ensures(layer >= 0 && layer < (1 << 13));

	const uint16_t typeAndLayer = (t << 13) | (layer & 0x1fff);
	return typeAndLayer;
}

static Rect2D<short> packClip(const std::optional<Rect4f>& clip)
{
	Rect2D<short> result = {0, 0, 0, 0};

	if (clip.has_value()) {
		const auto& p1 = clip->getTopLeft();
		const auto& p2 = clip->getBottomRight();

		result.set(
			Vector2D<short>(short(p1.x), short(p1.y)),
			Vector2D<short>(short(p2.x), short(p2.y)));

		constexpr int clipMin = std::numeric_limits<short>::min();
		constexpr int clipMax = std::numeric_limits<short>::max();

		Ensures(result.getLeft() >= clipMin && result.getRight() <= clipMax);
		Ensures(result.getTop() >= clipMin && result.getBottom() <= clipMax);
	}

	return result;
}

SpritePainterEntry::SpritePainterEntry(gsl::span<const Sprite> sprites, int layer, float tieBreaker, size_t insertOrder, const std::optional<Rect4f>& clip)
	: ptr(sprites.empty() ? nullptr : &sprites[0])
	, count(uint16_t(sprites.size()))
	, typeAndLayer(packTypeAndLayer(SpritePainterEntryType::SpriteRef, layer))
	, tieBreaker(tieBreaker)
	, insertOrder(uint32_t(insertOrder))
	, clip(packClip(clip))
{
	Ensures(sprites.size() < std::numeric_limits<uint16_t>::max());
}

SpritePainterEntry::SpritePainterEntry(gsl::span<const TextRenderer> texts, int layer, float tieBreaker, size_t insertOrder, const std::optional<Rect4f>& clip)
	: ptr(texts.empty() ? nullptr : &texts[0])
	, count(uint16_t(texts.size()))
	, typeAndLayer(packTypeAndLayer(SpritePainterEntryType::TextRef, layer))
	, tieBreaker(tieBreaker)
	, insertOrder(uint32_t(insertOrder))
	, clip(packClip(clip))
{
	Ensures(texts.size() < std::numeric_limits<uint16_t>::max());
}

SpritePainterEntry::SpritePainterEntry(SpritePainterEntryType type, size_t spriteIdx, size_t count, int layer, float tieBreaker, size_t insertOrder, const std::optional<Rect4f>& clip)
	: index(static_cast<int>(spriteIdx))
	, count(uint16_t(count))
	, typeAndLayer(packTypeAndLayer(type, layer))
	, tieBreaker(tieBreaker)
	, insertOrder(uint32_t(insertOrder))
	, clip(packClip(clip))
{
	Ensures(count < std::numeric_limits<uint16_t>::max());
}

bool SpritePainterEntry::operator<(const SpritePainterEntry& o) const
{
	const uint16_t l1 = typeAndLayer & 0x1fff;
	const uint16_t l2 = o.typeAndLayer & 0x1fff;
	if (l1 != l2) {
		return l1 < l2;
	}
	if (tieBreaker != o.tieBreaker) {
		return tieBreaker < o.tieBreaker;
	}
	return insertOrder < o.insertOrder;
}

SpritePainterEntryType SpritePainterEntry::getType() const
{
	uint16_t type = (typeAndLayer >> 13u) & 0x7;
	return (SpritePainterEntryType) (type);
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
	const auto& p1 = clip.getTopLeft();
	const auto& p2 = clip.getBottomRight();
	if ((p2.x - p1.x) * (p2.y - p1.y) != 0) {
		return Rect4f(Vector2f(p1.x, p1.y), Vector2f(p2.x, p2.y));
	}
	return std::nullopt;
}

void SpritePainterBucket::start(bool forceCopy)
{
	this->forceCopy = forceCopy;
	sprites.clear();
	cachedSprites.clear();
	cachedText.clear();
	materialRecycler.startFrame();
}

void SpritePainterBucket::add(const Sprite& sprite, int layer, float tieBreaker, const std::optional<Rect4f>& clip)
{
	if (forceCopy) {
		addCopy(sprite, layer, tieBreaker, clip);
	} else {
		sprites.push_back(SpritePainterEntry(gsl::span<const Sprite>(&sprite, 1), layer, tieBreaker, sprites.size(), std::move(clip)));
		dirty = true;
	}
}

void SpritePainterBucket::addCopy(const Sprite& sprite, int layer, float tieBreaker, const std::optional<Rect4f>& clip)
{
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::SpriteCached, cachedSprites.size(), 1, layer, tieBreaker, sprites.size(), std::move(clip)));
	cachedSprites.push_back(materialRecycler.clone(sprite));
	dirty = true;
}

void SpritePainterBucket::add(gsl::span<const Sprite> sprites, int layer, float tieBreaker, const std::optional<Rect4f>& clip)
{
	if (!sprites.empty()) {
		if (forceCopy) {
			addCopy(sprites, layer, tieBreaker, clip);
		} else {
			this->sprites.push_back(SpritePainterEntry(sprites, layer, tieBreaker, this->sprites.size(), std::move(clip)));
			dirty = true;
		}
	}
}

void SpritePainterBucket::addCopy(gsl::span<const Sprite> sprites, int layer, float tieBreaker, const std::optional<Rect4f>& clip)
{
	if (!sprites.empty()) {
		this->sprites.push_back(SpritePainterEntry(SpritePainterEntryType::SpriteCached, cachedSprites.size(), sprites.size(), layer, tieBreaker, this->sprites.size(), std::move(clip)));
		cachedSprites.reserve(cachedSprites.size() + sprites.size());
		for (auto& s: sprites) {
			cachedSprites.push_back(materialRecycler.clone(s));
		}
		dirty = true;
	}
}

void SpritePainterBucket::add(const TextRenderer& text, int layer, float tieBreaker, const std::optional<Rect4f>& clip)
{
	if (forceCopy) {
		addCopy(text, layer, tieBreaker, clip);
	} else {
		sprites.push_back(SpritePainterEntry(gsl::span<const TextRenderer>(&text, 1), layer, tieBreaker, sprites.size(), std::move(clip)));
		dirty = true;
	}
}

void SpritePainterBucket::addCopy(const TextRenderer& text, int layer, float tieBreaker, const std::optional<Rect4f>& clip)
{
	sprites.push_back(SpritePainterEntry(SpritePainterEntryType::TextCached, cachedText.size(), 1, layer, tieBreaker, sprites.size(), std::move(clip)));
	cachedText.push_back(materialRecycler.clone(text));
	dirty = true;
}

void SpritePainterBucket::add(SpritePainterEntry::Callback callback, int layer, float tieBreaker, const std::optional<Rect4f>& clip)
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

void SpritePainterBucket::draw(gsl::span<const Sprite> sprites, Painter& painter, Rect4f view, const std::optional<Rect4f>& clip) const
{
	for (const auto& sprite: sprites) {
		if (sprite.isInView(view)) {
			sprite.draw(painter, clip);
		}
	}
}

void SpritePainterBucket::draw(gsl::span<const TextRenderer> texts, Painter& painter, Rect4f view, const std::optional<Rect4f>& clip) const
{
	for (const auto& text: texts) {
		text.draw(painter, clip);
	}
}

void SpritePainterBucket::draw(const SpritePainterEntry::Callback& callback, Painter& painter, const std::optional<Rect4f>& clip) const
{
	if (clip) {
		painter.setRelativeClip(clip.value());
	}
	callback(painter);
	if (clip) {
		painter.setClip();
	}
}

void SpritePainter::start(bool forceCopy)
{
	buckets.resize(32);
	for (auto& b : buckets) {
		b.start(forceCopy);
	}
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

void SpritePainter::add(const TextRenderer& sprite, int mask, int layer, float tieBreaker, std::optional<Rect4f> clip)
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
