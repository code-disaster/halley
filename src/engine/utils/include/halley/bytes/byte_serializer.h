#pragma once

#include <halley/utils/utils.h>
#include "halley/data_structures/vector.h"
#include <gsl/gsl>
#include "halley/data_structures/flat_map.h"
#include "halley/maths/vector2.h"
#include "halley/maths/rect.h"
#include "halley/file/path.h"
#include <map>
#include <cstdint>
#include <utility>
#include <set>

#include "halley/data_structures/hash_map.h"
#include "halley/data_structures/maybe.h"
#include "halley/maths/colour.h"
#include "halley/maths/vector4.h"
#include "iserialization_dictionary.h"

namespace Halley {
	class World;
	class String;

	class SerializerOptions {
	public:
		constexpr static int maxVersion = 1;
		
		int version = 0;
		bool exhaustiveDictionary = false;
		ISerializationDictionary* dictionary = nullptr;
		World* world = nullptr;

		SerializerOptions() = default;
		SerializerOptions(int version)
			: version(version)
		{}
	};

	class SerializerState {};

	class ByteSerializationBase {
	public:
		ByteSerializationBase(SerializerOptions options)
			: options(std::move(options))
		{}
		
		SerializerState* setState(SerializerState* state);
		
		template <typename T>
		T* getState() const
		{
			return static_cast<T*>(state);
		}

		int getVersion() const { return version; }
		void setVersion(int v) { version = v; }
		const SerializerOptions& getOptions() const { return options; }

	protected:
		SerializerOptions options;
		
	private:
		SerializerState* state = nullptr;
		int version = 0;
	};

	class Serializer : public ByteSerializationBase {
	public:
		Serializer(SerializerOptions options);
		explicit Serializer(gsl::span<gsl::byte> dst, SerializerOptions options);

		template <typename T, typename std::enable_if<std::is_convertible<T, std::function<void(Serializer&)>>::value, int>::type = 0>
		static Bytes toBytes(const T& f, SerializerOptions options = {})
		{
			auto dry = Serializer(options);
			f(dry);
			Bytes result(dry.getSize());
			auto s = Serializer(gsl::as_writable_bytes(gsl::span<Halley::Byte>(result)), options);
			f(s);
			return result;
		}

		template <typename T, typename std::enable_if<!std::is_convertible<T, std::function<void(Serializer&)>>::value, int>::type = 0>
		static Bytes toBytes(const T& value, SerializerOptions options = {})
		{
			return toBytes([&value](Serializer& s) { s << value; }, options);
		}

		size_t getSize() const { return size; }
		size_t getPosition() const { return size; }

		Serializer& operator<<(bool val) { return serializePod(val); }
		Serializer& operator<<(int8_t val) { return serializeInteger(val); }
		Serializer& operator<<(uint8_t val) { return serializeInteger(val); }
		Serializer& operator<<(int16_t val) { return serializeInteger(val); }
		Serializer& operator<<(uint16_t val) { return serializeInteger(val); }
		Serializer& operator<<(int32_t val) { return serializeInteger(val); }
		Serializer& operator<<(uint32_t val) { return serializeInteger(val); }
		Serializer& operator<<(int64_t val) { return serializeInteger(val); }
		Serializer& operator<<(uint64_t val) { return serializeInteger(val); }
		Serializer& operator<<(float val) { return serializePod(val); }
		Serializer& operator<<(double val) { return serializePod(val); }

		Serializer& operator<<(const std::string& str);
		Serializer& operator<<(const String& str);
		Serializer& operator<<(const StringUTF32& str);
		Serializer& operator<<(const Path& path);
		Serializer& operator<<(gsl::span<const gsl::byte> span);
		Serializer& operator<<(gsl::span<gsl::byte> span);
		Serializer& operator<<(const Bytes& bytes);

		template <typename T>
		Serializer& operator<<(const Vector<T>& val)
		{
			unsigned int sz = static_cast<unsigned int>(val.size());
			*this << sz;
			for (unsigned int i = 0; i < sz; i++) {
				*this << val[i];
			}
			return *this;
		}
		
		template <typename T>
		Serializer& operator<<(const std::list<T>& val)
		{
			unsigned int sz = static_cast<unsigned int>(val.size());
			*this << sz;
			for (const auto& v: val) {
				*this << v;
			}
			return *this;
		}

		template <typename T, typename U>
		Serializer& operator<<(const HashMap<T, U>& val)
		{
			// Convert to map first to make order deterministic
			std::map<T, U> m;
			for (auto& kv: val) {
				m[kv.first] = kv.second;
			}
			return (*this << m);
		}

		template <typename K, typename V, typename Cmp, typename Allocator>
		Serializer& operator<<(const std::map<K, V, Cmp, Allocator>& val)
		{
			*this << static_cast<unsigned int>(val.size());
			for (auto& kv : val) {
				*this << kv.first << kv.second;
			}
			return *this;
		}

		template <typename T>
		Serializer& operator<<(const std::set<T>& val)
		{
			unsigned int sz = static_cast<unsigned int>(val.size());
			*this << sz;
			for (auto& v: val) {
				*this << v;
			}
			return *this;
		}

		template <typename T>
		Serializer& operator<<(const Vector2D<T>& val)
		{
			return *this << val.x << val.y;
		}

		template <typename T>
		Serializer& operator<<(const Vector4D<T>& val)
		{
			return *this << val.x << val.y << val.z << val.w;
		}

		template <typename T>
		Serializer& operator<<(const Colour4<T>& val)
		{
			return *this << val.r << val.g << val.b << val.a;
		}

		template <typename T>
		Serializer& operator<<(const Rect2D<T>& val)
		{
			return *this << val.getTopLeft() << val.getBottomRight();
		}

		template <typename T, typename U>
		Serializer& operator<<(const std::pair<T, U>& p)
		{
			return *this << p.first << p.second;
		}
		
		template <typename T>
		Serializer& operator<<(const std::optional<T>& p)
		{
			if (p) {
				return *this << true << p.value();
			} else {
				return *this << false;
			}
		}

		template <typename T>
		Serializer& operator<<(const Range<T>& p)
		{
			return *this << p.start << p.end;
		}

		template <typename T, std::enable_if_t<std::is_enum<T>::value == true, int> = 0>
		Serializer& operator<<(const T& val)
		{
			using B = typename std::underlying_type<T>::type;
			return *this << B(val);
		}

		template <typename T, std::enable_if_t<std::is_enum<T>::value == false, int> = 0>
		Serializer& operator<<(const T& val)
		{
			val.serialize(*this);
			return *this;
		}

	private:
		size_t size = 0;
		gsl::span<gsl::byte> dst;
		bool dryRun;

		template <typename T>
		Serializer& serializePod(T val)
		{
			copyBytes(&val, sizeof(T));
			return *this;
		}

		template <typename T>
		Serializer& serializeInteger(T val)
		{
			if (options.version >= 1) {
				// Variable-length
				if constexpr (std::is_signed_v<T>) {
					serializeVariableInteger(static_cast<uint64_t>(val >= 0 ? val : -(val + 1)), val < 0);
				} else {
					serializeVariableInteger(val, {});
				}
				return *this;
			} else {
				// Fixed length
				return serializePod(val);
			}
		}

		void serializeVariableInteger(uint64_t val, std::optional<bool> sign);
		void copyBytes(const void* src, size_t size);
	};

	class Deserializer : public ByteSerializationBase {
	public:
		Deserializer(gsl::span<const gsl::byte> src, SerializerOptions options = {});
		Deserializer(const Bytes& src, SerializerOptions options = {});
		
		template <typename T>
		static T fromBytes(const Bytes& src, SerializerOptions options = {})
		{
			T result;
			Deserializer s(src, std::move(options));
			s >> result;
			return result;
		}

		template <typename T>
		static T fromBytes(gsl::span<const gsl::byte> src, SerializerOptions options = {})
		{
			T result;
			Deserializer s(src, std::move(options));
			s >> result;
			return result;
		}

		template <typename T>
		static void fromBytes(T& target, const Bytes& src, SerializerOptions options = {})
		{
			Deserializer s(src, std::move(options));
			s >> target;
		}

		template <typename T>
		static void fromBytes(T& target, gsl::span<const gsl::byte> src, SerializerOptions options = {})
		{
			Deserializer s(src, std::move(options));
			s >> target;
		}

		Deserializer& operator>>(bool& val) { return deserializePod(val); }
		Deserializer& operator>>(int8_t& val) { return deserializeInteger(val); }
		Deserializer& operator>>(uint8_t& val) { return deserializeInteger(val); }
		Deserializer& operator>>(int16_t& val) { return deserializeInteger(val); }
		Deserializer& operator>>(uint16_t& val) { return deserializeInteger(val); }
		Deserializer& operator>>(int32_t& val) { return deserializeInteger(val); }
		Deserializer& operator>>(uint32_t& val) { return deserializeInteger(val); }
		Deserializer& operator>>(int64_t& val) { return deserializeInteger(val); }
		Deserializer& operator>>(uint64_t& val) { return deserializeInteger(val); }
		Deserializer& operator>>(float& val) { return deserializePod(val); }
		Deserializer& operator>>(double& val) { return deserializePod(val); }

		Deserializer& operator>>(std::string& str);
		Deserializer& operator>>(String& str);
		Deserializer& operator>>(StringUTF32& str);
		Deserializer& operator>>(Path& p);
		Deserializer& operator>>(gsl::span<gsl::byte> span);
		Deserializer& operator>>(Bytes& bytes);

		template <typename T>
		Deserializer& operator>>(Vector<T>& val)
		{
			unsigned int sz;
			*this >> sz;
			ensureSufficientBytesRemaining(sz); // Expect at least one byte per vector entry

			val.clear();
			val.reserve(sz);
			for (unsigned int i = 0; i < sz; i++) {
				val.push_back(T());
				*this >> val[i];
			}
			return *this;
		}
		
		template <typename T>
		Deserializer& operator>>(std::list<T>& val)
		{
			unsigned int sz;
			*this >> sz;
			ensureSufficientBytesRemaining(sz); // Expect at least one byte per vector entry

			val.clear();
			for (unsigned int i = 0; i < sz; i++) {
				T v;
				*this >> v;
				val.push_back(std::move(v));
			}
			return *this;
		}

		template <typename T, typename U>
		Deserializer& operator>>(HashMap<T, U>& val)
		{
			unsigned int sz;
			*this >> sz;
			ensureSufficientBytesRemaining(sz * 2); // Expect at least two bytes per map entry

			val.clear();
			val.reserve(sz);
			for (unsigned int i = 0; i < sz; i++) {
				T key;
				U value;
				*this >> key >> value;
				val[key] = std::move(value);
			}
			return *this;
		}

		template <typename K, typename V, typename Cmp, typename Allocator>
		Deserializer& operator >> (std::map<K, V, Cmp, Allocator>& val)
		{
			unsigned int sz;
			*this >> sz;
			ensureSufficientBytesRemaining(size_t(sz) * 2); // Expect at least two bytes per map entry
			
			for (unsigned int i = 0; i < sz; i++) {
				K key;
				V value;
				*this >> key >> value;
				val[key] = std::move(value);
			}
			return *this;
		}

		template <typename T>
		Deserializer& operator>>(std::set<T>& val)
		{
			unsigned int sz;
			*this >> sz;
			ensureSufficientBytesRemaining(sz); // Expect at least one byte per set entry

			val.clear();
			for (unsigned int i = 0; i < sz; i++) {
				T v;
				*this >> v;
				val.insert(std::move(v));
			}
			return *this;
		}

		template <typename T>
		Deserializer& operator>>(Vector2D<T>& val)
		{
			*this >> val.x;
			*this >> val.y;
			return *this;
		}

		template <typename T>
		Deserializer& operator>>(Vector4D<T>& val)
		{
			*this >> val.x;
			*this >> val.y;
			*this >> val.z;
			*this >> val.w;
			return *this;
		}

		template <typename T>
		Deserializer& operator>>(Colour4<T>& val)
		{
			*this >> val.r;
			*this >> val.g;
			*this >> val.b;
			*this >> val.a;
			return *this;
		}

		template <typename T>
		Deserializer& operator>>(Rect2D<T>& val)
		{
			Vector2D<T> p1, p2;
			*this >> p1;
			*this >> p2;
			val = Rect2D<T>(p1, p2);
			return *this;
		}

		template <typename T, typename U>
		Deserializer& operator>>(std::pair<T, U>& p)
		{
			return *this >> p.first >> p.second;
		}

		template <typename T>
		Deserializer& operator>>(Range<T>& p)
		{
			return *this >> p.start >> p.end;
		}

		template <typename T>
		Deserializer& operator>>(std::optional<T>& p)
		{
			bool present;
			*this >> present;
			if (present) {
				T tmp;
				*this >> tmp;
				p = tmp;
			} else {
				p = std::optional<T>();
			}
			return *this;
		}

		template <typename T, std::enable_if_t<std::is_enum<T>::value == true, int> = 0>
		Deserializer& operator>>(T& val)
		{
			typename std::underlying_type<T>::type tmp;
			*this >> tmp;
			val = T(tmp);
			return *this;
		}

		template <typename T, std::enable_if_t<std::is_enum<T>::value == false, int> = 0>
		Deserializer& operator>>(T& val)
		{
			val.deserialize(*this);
			return *this;
		}

		template <typename T>
		void peek(T& val)
		{
			const auto oldPos = pos;
			*this >> val;
			pos = oldPos;
		}

		size_t getPosition() const { return pos; }
		size_t getBytesLeft() const { return src.size() - pos; }

	private:
		size_t pos = 0;
		gsl::span<const gsl::byte> src;

		template <typename T>
		Deserializer& deserializePod(T& val)
		{
			ensureSufficientBytesRemaining(sizeof(T));
			memcpy(&val, src.data() + pos, sizeof(T));
			pos += sizeof(T);
			return *this;
		}

		template <typename T>
		Deserializer& deserializeInteger(T& val)
		{
			if (options.version >= 1) {
				// Variable-length
				bool sign;
				uint64_t temp;
				deserializeVariableInteger(temp, sign, std::is_signed_v<T>);
				if (sign) {
					int64_t signedTemp = -int64_t(temp) - 1;
					val = static_cast<T>(signedTemp);
				} else {
					val = static_cast<T>(temp);
				}
				return *this;
			} else {
				// Fixed length
				return deserializePod(val);
			}
		}

		void deserializeVariableInteger(uint64_t& val, bool& sign, bool isSigned);

		void ensureSufficientBytesRemaining(size_t bytes);
		size_t getBytesRemaining() const;
	};
}
