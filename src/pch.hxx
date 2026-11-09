#pragma once

// pch — foundational shared engine header (adapted from bom project)
//
// Role:
// - Define common primitive aliases, math/geometry types, utility wrappers, and core helpers.
// - Aggregate low-level third-party headers (SDL3, pugixml) in one place.
//
// Public API:
// - Common aliases (u8/i32/f32, Point/Rect families).
// - Convenience helpers for Point/Rect operations and direction utilities.

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <float.h>
#include <limits.h>
#include <string>
#include <string_view>
#include <vector>

struct SDL_Renderer;
struct SDL_Texture;

// Type aliases
using u8 = uint8_t;
[[maybe_unused]] constexpr u8 u8Min = 0;
[[maybe_unused]] constexpr u8 u8Max = UINT8_MAX;

using u16 = uint16_t;
[[maybe_unused]] constexpr u16 u16Min = 0;
[[maybe_unused]] constexpr u16 u16Max = UINT16_MAX;

using u32 = uint32_t;
[[maybe_unused]] constexpr u32 u32Min = 0;
[[maybe_unused]] constexpr u32 u32Max = UINT32_MAX;

using u64 = uint64_t;
[[maybe_unused]] constexpr u64 u64Min = 0;
[[maybe_unused]] constexpr u64 u64Max = UINT64_MAX;

using i8 = int8_t;
[[maybe_unused]] constexpr i8 i8Min = INT8_MIN;
[[maybe_unused]] constexpr i8 i8Max = INT8_MAX;

using i16 = int16_t;
[[maybe_unused]] constexpr i16 i16Min = INT16_MIN;
[[maybe_unused]] constexpr i16 i16Max = INT16_MAX;

using i32 = int32_t;
[[maybe_unused]] constexpr i32 i32Min = INT32_MIN;
[[maybe_unused]] constexpr i32 i32Max = INT32_MAX;

using i64 = int64_t;
[[maybe_unused]] constexpr i64 i64Min = INT64_MIN;
[[maybe_unused]] constexpr i64 i64Max = INT64_MAX;

using f32 = float;
[[maybe_unused]] constexpr f32 f32Min = FLT_MIN;
[[maybe_unused]] constexpr f32 f32Max = FLT_MAX;
[[maybe_unused]] constexpr f32 f32Epsilon = FLT_EPSILON;

using f64 = double;
[[maybe_unused]] constexpr f64 f64Min = DBL_MIN;
[[maybe_unused]] constexpr f64 f64Max = DBL_MAX;
[[maybe_unused]] constexpr f64 f64Epsilon = DBL_EPSILON;

using usize = size_t;
using uindex = size_t;

#define endcase break // switch statements should only use break, not goto-like fallthrough
#define nullfn delete // mark deleted functions more descriptively than = delete

// Non-owning pointer wrapper for observer relationships.
// - Does NOT own lifetime.
// - Never delete through this type.
// - Use for references into objects owned elsewhere (World, Location, caches, etc.).
template <typename T> struct observer_ptr
{
	using element_type = T;

	observer_ptr() = default;
	observer_ptr(std::nullptr_t) : _pointee(nullptr) {}
	observer_ptr(T* pPointee) : _pointee(pPointee) {}

	observer_ptr& operator=(std::nullptr_t)
	{
		_pointee = nullptr;
		return *this;
	}

	observer_ptr& operator=(T* pPointee)
	{
		_pointee = pPointee;
		return *this;
	}

	T* get() const { return _pointee; }
	void reset(T* pPointee = nullptr) { _pointee = pPointee; }

	T& operator*() const { return *_pointee; }
	T* operator->() const { return _pointee; }
	explicit operator bool() const { return _pointee != nullptr; }

	bool operator==(std::nullptr_t) const { return _pointee == nullptr; }
	bool operator==(const T* pOther) const { return _pointee == pOther; }
	friend bool operator==(const T* pLhs, const observer_ptr& rRhs) { return pLhs == rRhs._pointee; }
	bool operator==(const observer_ptr&) const = default;
	auto operator<=>(const observer_ptr&) const = default;

private:
	T* _pointee = nullptr;
};

template <typename T> struct Point
{
	T x = 0, y = 0;

	Point() = default;
	Point(T x, T y) : x(x), y(y)
	{
	}

	// Explicitly default copy and move semantics
	Point(const Point&) = default;
	Point& operator=(const Point&) = default;
	Point(Point&&) = default;
	Point& operator=(Point&&) = default;

	auto operator<=>(const Point& other) const = default; // all comparison operators
	bool operator==(const Point& other) const = default;

	Point operator+(const Point& other) const { return Point(x + other.x, y + other.y); }
	Point operator-(const Point& other) const { return Point(x - other.x, y - other.y); }
	Point operator*(T scalar) const { return Point(x * scalar, y * scalar); }
	Point operator/(T scalar) const { return Point(x / scalar, y / scalar); }

	// Compound assignment operators
	Point& operator+=(const Point& rOther)
	{
		x += rOther.x;
		y += rOther.y;
		return *this;
	}
	Point& operator+=(T scalar)
	{
		x += scalar;
		y += scalar;
		return *this;
	}
	Point& operator-=(const Point& rOther)
	{
		x -= rOther.x;
		y -= rOther.y;
		return *this;
	}
	Point& operator-=(T scalar)
	{
		x -= scalar;
		y -= scalar;
		return *this;
	}
	Point& operator*=(const Point& rOther)
	{
		x *= rOther.x;
		y *= rOther.y;
		return *this;
	}
	Point& operator*=(T scalar)
	{
		x *= scalar;
		y *= scalar;
		return *this;
	}
	Point& operator/=(const Point& rOther)
	{
		x /= rOther.x;
		y /= rOther.y;
		return *this;
	}
	Point& operator/=(T scalar)
	{
		x /= scalar;
		y /= scalar;
		return *this;
	}

	// Unary operators
	Point operator-() const { return {-x, -y}; }
	Point operator+() const { return *this; }

	// Distance calculation
	T distanceTo(const Point& other) const requires(std::is_floating_point_v<T>)
	{
		Point delta = *this - other;
		return std::sqrt(delta.x * delta.x + delta.y * delta.y);
	}

	T distance(const Point& other) const requires(std::is_floating_point_v<T>)
	{
		return distanceTo(other);  // backward compat
	}
};

// Type aliases for Point
typedef Point<u8> u8Point;
typedef Point<u16> u16Point;
typedef Point<u32> u32Point;
typedef Point<u64> u64Point;
typedef Point<i8> i8Point;
typedef Point<i16> i16Point;
typedef Point<i32> i32Point;
typedef Point<i64> i64Point;
typedef Point<f32> f32Point;
typedef Point<f64> f64Point;

// Backward compatibility alias
using Vec2 = f32Point;

template <typename T> struct Rect
{
	T x = 0, y = 0, w = 0, h = 0;

	Rect() = default;
	Rect(T x, T y, T w, T h) : x(x), y(y), w(w), h(h)
	{
	}

	Rect(const Point<T>& rPosition, const Point<T>& rSize) : x(rPosition.x), y(rPosition.y), w(rSize.x), h(rSize.y)
	{
	}

	Rect(const Point<T>& rPosition, T width, T height) : x(rPosition.x), y(rPosition.y), w(width), h(height)
	{
	}

	// Explicitly default copy and move semantics
	Rect(const Rect&) = default;
	Rect& operator=(const Rect&) = default;
	Rect(Rect&&) = default;
	Rect& operator=(Rect&&) = default;

	auto operator<=>(const Rect& other) const = default; // all comparison operators
	bool operator==(const Rect& other) const = default;

	// Convenience helpers
	Point<T> Size() const { return {w, h}; }

	T Left() const { return x; }
	T Top() const { return y; }
	T Right() const { return x + w; }
	T Bottom() const { return y + h; }

	Point<T> LeftTop() const { return {Left(), Top()}; }
	Point<T> RightTop() const { return {Right(), Top()}; }
	Point<T> LeftBottom() const { return {Left(), Bottom()}; }
	Point<T> RightBottom() const { return {Right(), Bottom()}; }

	Point<T> Center() const { return {x + w * (T)0.5, y + h * (T)0.5}; }

	Point<T> center() const { return Center(); }  // backward compat

	bool Contains(const Point<T>& rPoint) const
	{
		Rect normalized = Normalized();
		return (rPoint.x >= normalized.Left()) && (rPoint.x <= normalized.Right()) && (rPoint.y >= normalized.Top()) && (rPoint.y <= normalized.Bottom());
	}

	bool contains(const Point<T>& rPoint) const { return Contains(rPoint); }  // backward compat

	Rect Normalized() const
	{
		Rect retVal = *this;

		if (retVal.w < 0)
		{
			retVal.x += retVal.w;
			retVal.w = -retVal.w;
		}

		if (retVal.h < 0)
		{
			retVal.y += retVal.h;
			retVal.h = -retVal.h;
		}

		return retVal;
	}

	Rect& NormalizeInPlace()
	{
		*this = Normalized();
		return *this;
	}

	Rect Union(const Rect& rOther) const
	{
		Rect a = Normalized();
		Rect b = rOther.Normalized();

		return FromMinMax(std::min(a.Left(), b.Left()), std::min(a.Top(), b.Top()), std::max(a.Right(), b.Right()), std::max(a.Bottom(), b.Bottom()));
	}

	Rect Intersection(const Rect& rOther) const
	{
		Rect a = Normalized();
		Rect b = rOther.Normalized();

		T left = std::max(a.Left(), b.Left());
		T top = std::max(a.Top(), b.Top());
		T right = std::min(a.Right(), b.Right());
		T bottom = std::min(a.Bottom(), b.Bottom());

		if (right < left)
			right = left;

		if (bottom < top)
			bottom = top;

		return FromMinMax(left, top, right, bottom);
	}

	static Rect FromMinMax(T left, T top, T right, T bottom)
	{
		return Rect(left, top, right - left, bottom - top);
	}
};

// Type aliases for Rect
typedef Rect<u8> u8Rect;
typedef Rect<u16> u16Rect;
typedef Rect<u32> u32Rect;
typedef Rect<u64> u64Rect;
typedef Rect<i8> i8Rect;
typedef Rect<i16> i16Rect;
typedef Rect<i32> i32Rect;
typedef Rect<i64> i64Rect;
typedef Rect<f32> f32Rect;
typedef Rect<f64> f64Rect;

// Deduction guide for backward compat: Rect(x, y, w, h) deduces to Rect<f32>
template <typename T>
Rect(T, T, T, T) -> Rect<T>;

struct ColorRGBA
{
	u8 r = 0;
	u8 g = 0;
	u8 b = 0;
	u8 a = 255;

	static ColorRGBA fromHex(const std::string& hexStr, ColorRGBA fallback = {0, 0, 0, 255})
	{
		if (hexStr.empty())
			return fallback;

		std::string s = hexStr;
		if (s[0] == '#')
			s = s.substr(1);
		else if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0)
			s = s.substr(2);

		try
		{
			u32 val = static_cast<u32>(std::stoul(s, nullptr, 16));
			if (s.length() <= 6)
			{
				return {
					static_cast<u8>((val >> 16) & 0xFF),
					static_cast<u8>((val >> 8) & 0xFF),
					static_cast<u8>(val & 0xFF),
					255
				};
			}

			return {
				static_cast<u8>((val >> 24) & 0xFF),
				static_cast<u8>((val >> 16) & 0xFF),
				static_cast<u8>((val >> 8) & 0xFF),
				static_cast<u8>(val & 0xFF)
			};
		} catch (...)
		{
			return fallback;
		}
	}
};

// Backward compatibility: map old function names to new Point/Rect methods
namespace compat
{
	[[maybe_unused]] inline f32 distance(const Vec2& a, const Vec2& b)
	{
		Vec2 delta = a - b;
		return std::sqrt(delta.x * delta.x + delta.y * delta.y);
	}

	[[maybe_unused]] inline bool contains(const f32Rect& rect, const Vec2& pt)
	{
		return rect.Contains(pt);
	}

	[[maybe_unused]] inline Vec2 center(const f32Rect& rect)
	{
		return rect.Center();
	}
} // namespace compat
