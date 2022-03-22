/*****************************************************************\
           __
          / /
		 / /                     __  __
		/ /______    _______    / / / / ________   __       __
	   / ______  \  /_____  \  / / / / / _____  | / /      / /
	  / /      | / _______| / / / / / / /____/ / / /      / /
	 / /      / / / _____  / / / / / / _______/ / /      / /
	/ /      / / / /____/ / / / / / / |______  / |______/ /
   /_/      /_/ |________/ / / / /  \_______/  \_______  /
                          /_/ /_/                     / /
			                                         / /
		       High Level Game Framework            /_/

  ---------------------------------------------------------------

  Copyright (c) 2007-2011 - Rodrigo Braz Monteiro.
  This file is subject to the terms of halley_license.txt.

\*****************************************************************/

#pragma once

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <algorithm>
#include <limits>
#include <list>
#include <cmath>

#include <memory>
#include <array>
#include <functional>
#include <halley/data_structures/vector.h>
#include <gsl/gsl_assert>

#include "type_traits.h"

#ifdef _MSC_VER
#include <xmmintrin.h>
#endif


namespace Halley {
	// General aliases
	using Byte = unsigned char;
	using Bytes = Vector<Byte>;
	
	template <typename T>
	constexpr inline T clamp(T value, T minValue, T maxValue)
	{
		return std::min(std::max(minValue, value), maxValue);
	}

	template <typename T>
	constexpr inline T clamp2(T value, T minValue, T maxValue)
	{
		return std::max(minValue, std::min(value, maxValue));
	}

	template <typename T>
	constexpr inline T maxAbs(T a, T b)
	{
		return abs(a) > abs(b) ? a : b;
	}

	template <typename T>
	constexpr inline T minAbs(T a, T b)
	{
		return abs(a) < abs(b) ? a : b;
	}

	template <typename T>
	constexpr inline bool rangeIntersection(T s1, T e1, T s2, T e2)
	{
		return (s1 < e2) && (s2 < e1);
	}

	constexpr double pi()
	{
		return 3.1415926535897932384626433832795;
	}

	constexpr float pif()
	{
		return 3.1415926535897932384626433832795f;
	}

	// True modulo definition
	template <typename T> constexpr inline T modulo (T a, T b)
	{
		static_assert(std::is_signed<T>::value, "Must be signed to use modulo operation");
		T res = a % b;
		if (res < 0) res = b+res;
		return res;
	}

	template <typename T> constexpr inline T floatModulo (T a, T b)
	{
		if (b == 0) return a;
		return a - b * std::floor(a/b);
	}
	template <> constexpr inline float modulo (float a, float b) { return floatModulo(a, b); }
	template <> constexpr inline double modulo (double a, double b) { return floatModulo(a, b); }

	// Float floor division (e.g. -0.5 / 2 = -1.0)
	template <typename T>
	constexpr inline T floorDivFloat (T a, T b)
	{
		return floor(a / b);
	}

	// Int floor division (e.g. -2 / 3 = -1)
	template <typename T>
	constexpr inline T floorDivInt (T a, T b)
	{
		T result = a / b;
		if (a % b < 0) {
			--result;
		}
		return result;
	}

	// Generic floor divs
	constexpr inline float floorDiv (float a, float b) { return floorDivFloat(a, b); }
	constexpr inline double floorDiv (double a, double b) { return floorDivFloat(a, b); }
	constexpr inline long floorDiv (long a, long b) { return floorDivInt(a, b); }
	constexpr inline int floorDiv (int a, int b) { return floorDivInt(a, b); }
	constexpr inline short floorDiv (short a, short b) { return floorDivInt(a, b); }
	constexpr inline char floorDiv (char a, char b) { return floorDivInt(a, b); }

	// Interpolation
	template <typename T>
	[[nodiscard]] constexpr inline T interpolate(T a, T b, float factor)
	{
		return static_cast<T>(a * (1 - factor) + b * factor);
	}

	template <typename T>
	[[nodiscard]] constexpr inline T lerp(T a, T b, float factor)
	{
		return static_cast<T>(a * (1 - factor) + b * factor);
	}

	template <typename T>
	[[nodiscard]] constexpr inline T damp(T a, T b, float lambda, float dt)
	{
		return lerp<T>(a, b, 1.0f - std::exp(-lambda * dt));
	}

	namespace Detail {
		template<class T> using QuantizeMember = decltype(std::declval<T>().quantize(std::declval<float>()));
	}
	
	template <typename T>
	[[nodiscard]] constexpr inline T quantize(T a, float factor)
	{
		static_assert(is_detected_v<Detail::QuantizeMember, T> || std::is_scalar_v<T>, "Type does not support quantization.");
		
		if constexpr (is_detected_v<Detail::QuantizeMember, T>) {
			return a.quantize(factor);
		} else if constexpr (std::is_scalar_v<T>) {
			return std::round(a / factor) * factor;
		} else {
			// Will never be reached due to static_assert above
			return a;
		}
	}

	template <typename T>
	constexpr T sinRange(T angle, T min, T max)
	{
		return lerp(min, max, sin(angle) * T(0.5) + T(0.5));
	}

	template <typename T>
	constexpr T cosRange(T angle, T min, T max)
	{
		return lerp(min, max, cos(angle) * T(0.5) + T(0.5));
	}

	// Smoothing
	template <typename T>
	[[nodiscard]] constexpr inline T smoothCos(T a)
	{
		return (T(1) - T(std::cos(a * pi()))) * T(0.5);
	}

	template <typename T>
	[[nodiscard]] constexpr inline T overshootCurve(T x)
	{
		return T(pow(1.0f - x, 3.0f) * sin(2.5f * pi() * pow(x, 3.0f)) * 4.0f + (1.0f - pow(1.0f - x, 3.0f)) * sin(x * pi() * 0.5f));
	}

	// ASR (attack-sustain-release) envelope
	// Returns 0 at start of attack and end of release, 1 during sustain
	// Attack and release are linear interpolations
	template <typename T>
	[[nodiscard]] constexpr inline T asr(T x, T a, T s, T r) {
		if (x < a) return x/a;
		T as = a+s;
		if (x < as) return 1;
		return 1 - ((x-as)/r);
	}

	// Next power of 2
	template<typename T>
	[[nodiscard]] constexpr static T nextPowerOf2(T val)
	{
		--val;
		val = (val >> 1) | val;
		val = (val >> 2) | val;
		val = (val >> 4) | val;
		val = (val >> 8) | val;
		val = (val >> 16) | val;
		return val+1;
	}

	[[nodiscard]] constexpr inline int fastLog2Floor (uint32_t value) {
		// From https://stackoverflow.com/questions/11376288/fast-computing-of-log2-for-64-bit-integers
		constexpr char tab32[32] = { 0, 9, 1, 10, 13, 21, 2, 29, 11, 14, 16, 18, 22, 25, 3, 30, 8, 12, 20, 28, 15, 17, 24, 7, 19, 27, 23, 6, 26, 5, 4, 31 };

		Expects(value > 0);
		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;
		value |= value >> 8;
		value |= value >> 16;
		return static_cast<int>(tab32[uint32_t(value * 0x07C4ACDD) >> 27]);
	}

	[[nodiscard]] constexpr inline int fastLog2Floor (uint64_t value)
	{
		if (value == 0) {
			return 0;
		}
		
		// From https://stackoverflow.com/questions/11376288/fast-computing-of-log2-for-64-bit-integers
		constexpr char tab64[64] = { 63,  0, 58,  1, 59, 47, 53,  2, 60, 39, 48, 27, 54, 33, 42,  3, 61, 51, 37, 40, 49, 18, 28, 20,  55, 30, 34, 11, 43, 14, 22,  4, 62, 57, 46, 52, 38, 26, 32, 41, 50, 36, 17, 19, 29, 10, 13, 21, 56, 45, 25, 31, 35, 16,  9, 12, 44, 24, 15,  8, 23,  7,  6,  5};

		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;
		value |= value >> 8;
		value |= value >> 16;
		value |= value >> 32;
		return static_cast<int>(tab64[uint64_t((value - (value >> 1)) * 0x07EDD5E59A4E28C2) >> 58]);
	}

	[[nodiscard]] constexpr inline int fastLog2Ceil (uint32_t value)
	{
		if (value == 0) {
			return 0;
		}
		return fastLog2Floor(value - 1) + 1;
	}

	[[nodiscard]] constexpr inline int fastLog2Ceil (uint64_t value)
	{
		if (value == 0) {
			return 0;
		}
		return fastLog2Floor(value - 1) + 1;
	}

	// Advance a to b by up to inc
	template<typename T>
	[[nodiscard]] constexpr static T advance(T a, T b, T inc)
	{
		if (a < b) return std::min(a+inc, b);
		else return std::max(a-inc, b);
	}

	// Align address
	template <typename T>
	[[nodiscard]] constexpr T alignUp(T val, T align)
	{
		return val + (align - (val % align)) % align;
	}

	template <typename T>
	[[nodiscard]] constexpr T alignDown(T val, T align)
	{
		return (val / align) * align;
	}

	template <typename T>
	[[nodiscard]] constexpr T signOf(T val)
	{
		if (val == 0) {
			return 0;
		} else if (val > 0) {
			return 1;
		} else {
			return -1;
		}
	}

	// Prefetch data from memory
	static inline void prefetchL1(void* p) {
#ifdef _MSC_VER
		_mm_prefetch(static_cast<const char*>(p), _MM_HINT_T0);
#else
		__builtin_prefetch(p);
#endif
	}
	static inline void prefetchL2(void* p) {
#ifdef _MSC_VER
		_mm_prefetch(static_cast<const char*>(p), _MM_HINT_T1);
#else
		__builtin_prefetch(p);
#endif
	}
}
