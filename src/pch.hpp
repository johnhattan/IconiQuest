#pragma once

// pch — foundational shared engine header (adapted from bom project)
//
// Role:
// - Define common primitive aliases, math/geometry types, utility wrappers, and core helpers.
// - Aggregate low-level third-party headers (SDL3, pugixml) in one place.
//
// Public API:
// - Common aliases (u8/i32/f32, Point/Rectangle families).
// - Convenience helpers for Point/Rectangle operations and direction utilities.

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <float.h>
#include <fstream>
#include <iostream>
#include <limits.h>
#include <memory>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
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

template <typename T, typename Derived> struct _pair2D
{ // Base class for 2D pair types (e.g., Point, Size)
	_pair2D() = default;
	_pair2D(T first, T second) {}

	_pair2D(const _pair2D&) = default;
	_pair2D& operator=(const _pair2D&) = default;
	_pair2D(_pair2D&&) = default;
	_pair2D& operator=(_pair2D&&) = default;

	// Helper methods to access first/second via derived class
	T& first() { return static_cast<Derived&>(*this).first_impl(); }
	const T& first() const { return static_cast<const Derived&>(*this).first_impl(); }
	T& second() { return static_cast<Derived&>(*this).second_impl(); }
	const T& second() const { return static_cast<const Derived&>(*this).second_impl(); }

	auto operator<=>(const _pair2D& other) const = default;
	bool operator==(const _pair2D& other) const = default;

	Derived operator+(const _pair2D& other) const { return Derived(first() + other.first(), second() + other.second()); }
	Derived operator-(const _pair2D& other) const { return Derived(first() - other.first(), second() - other.second()); }
	Derived operator*(T scalar) const { return Derived(first() * scalar, second() * scalar); }
	Derived operator/(T scalar) const { return Derived(first() / scalar, second() / scalar); }

	Derived& operator+=(const _pair2D& other)
	{
		first() += other.first();
		second() += other.second();
		return static_cast<Derived&>(*this);
	}
	Derived& operator-=(const _pair2D& other)
	{
		first() -= other.first();
		second() -= other.second();
		return static_cast<Derived&>(*this);
	}
	Derived& operator*=(T scalar)
	{
		first() *= scalar;
		second() *= scalar;
		return static_cast<Derived&>(*this);
	}
	Derived& operator*=(const _pair2D& other)
	{
		first() *= other.first();
		second() *= other.second();
		return static_cast<Derived&>(*this);
	}
	Derived& operator/=(T scalar)
	{
		first() /= scalar;
		second() /= scalar;
		return static_cast<Derived&>(*this);
	}
	Derived& operator/=(const _pair2D& other)
	{
		first() /= other.first();
		second() /= other.second();
		return static_cast<Derived&>(*this);
	}

	Derived operator-() const { return Derived(-first(), -second()); }
	Derived operator+() const { return static_cast<const Derived&>(*this); }
};

template <typename T> struct Point : _pair2D<T, Point<T>>
{
	T x = 0, y = 0;

	Point() = default;
	Point(T x, T y) : x(x), y(y) {}

	T& first_impl() { return x; }
	const T& first_impl() const { return x; }
	T& second_impl() { return y; }
	const T& second_impl() const { return y; }

	T distanceTo(const Point& other) const requires(std::is_floating_point_v<T>)
	{
		T dx = x - other.x;
		T dy = y - other.y;
		return std::sqrt(dx * dx + dy * dy);
	}

	T distance(const Point& other) const requires(std::is_floating_point_v<T>)
	{
		return distanceTo(other);
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

template <typename T> struct Size : _pair2D<T, Size<T>>
{
	T width = 0, height = 0;

	Size() = default;
	Size(T width, T height) : width(width), height(height) {}

	T& first_impl() { return width; }
	const T& first_impl() const { return width; }
	T& second_impl() { return height; }
	const T& second_impl() const { return height; }

	T area() const { return width * height; }
};

// Type aliases for Size
typedef Size<u8> u8Size;
typedef Size<u16> u16Size;
typedef Size<u32> u32Size;
typedef Size<u64> u64Size;
typedef Size<i8> i8Size;
typedef Size<i16> i16Size;
typedef Size<i32> i32Size;
typedef Size<i64> i64Size;
typedef Size<f32> f32Size;
typedef Size<f64> f64Size;

template <typename T> struct Rectangle
{
	T x = 0, y = 0, w = 0, h = 0;

	Rectangle() = default;
	Rectangle(T x, T y, T w, T h) : x(x), y(y), w(w), h(h) {}

	Rectangle(const Point<T>& rPosition, const Size<T>& rSize) : x(rPosition.x), y(rPosition.y), w(rSize.width), h(rSize.height) {}

	Rectangle(const Point<T>& rPosition, T width, T height) : x(rPosition.x), y(rPosition.y), w(width), h(height) {}

	// Explicitly default copy and move semantics
	Rectangle(const Rectangle&) = default;
	Rectangle& operator=(const Rectangle&) = default;
	Rectangle(Rectangle&&) = default;
	Rectangle& operator=(Rectangle&&) = default;

	auto operator<=>(const Rectangle& other) const = default;
	bool operator==(const Rectangle& other) const = default;

	// Computed properties
	Point<T> getPosition() const { return { x, y }; }
	Size<T> getSize() const { return { w, h }; }
	void setPosition(const Point<T>& rPos)
	{
		x = rPos.x;
		y = rPos.y;
	}
	void setSize(const Size<T>& rSize)
	{
		w = rSize.width;
		h = rSize.height;
	}

	T Left() const { return x; }
	T Top() const { return y; }
	T Right() const { return x + w; }
	T Bottom() const { return y + h; }

	Point<T> LeftTop() const { return { Left(), Top() }; }
	Point<T> RightTop() const { return { Right(), Top() }; }
	Point<T> LeftBottom() const { return { Left(), Bottom() }; }
	Point<T> RightBottom() const { return { Right(), Bottom() }; }

	Point<T> Center() const { return { x + w * (T)0.5, y + h * (T)0.5 }; }

	Point<T> center() const { return Center(); }

	bool Contains(const Point<T>& rPoint) const
	{
		Rectangle normalized = Normalized();
		return (rPoint.x >= normalized.Left()) && (rPoint.x <= normalized.Right()) && (rPoint.y >= normalized.Top()) && (rPoint.y <= normalized.Bottom());
	}

	bool contains(const Point<T>& rPoint) const { return Contains(rPoint); }

	Rectangle Normalized() const
	{
		Rectangle retVal = *this;

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

	Rectangle& NormalizeInPlace()
	{
		*this = Normalized();
		return *this;
	}

	Rectangle Union(const Rectangle& rOther) const
	{
		Rectangle a = Normalized();
		Rectangle b = rOther.Normalized();

		return FromMinMax(std::min(a.Left(), b.Left()), std::min(a.Top(), b.Top()), std::max(a.Right(), b.Right()), std::max(a.Bottom(), b.Bottom()));
	}

	Rectangle Intersection(const Rectangle& rOther) const
	{
		Rectangle a = Normalized();
		Rectangle b = rOther.Normalized();

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

	static Rectangle FromMinMax(T left, T top, T right, T bottom)
	{
		return Rectangle(left, top, right - left, bottom - top);
	}
};

// Type aliases for Rectangle
typedef Rectangle<u8> u8Rectangle;
typedef Rectangle<u16> u16Rectangle;
typedef Rectangle<u32> u32Rectangle;
typedef Rectangle<u64> u64Rectangle;
typedef Rectangle<i8> i8Rectangle;
typedef Rectangle<i16> i16Rectangle;
typedef Rectangle<i32> i32Rectangle;
typedef Rectangle<i64> i64Rectangle;
typedef Rectangle<f32> f32Rectangle;
typedef Rectangle<f64> f64Rectangle;

struct Color
{
	u8 r = 0;
	u8 g = 0;
	u8 b = 0;
	u8 a = 255;

	static Color fromHex(const std::string& hexStr, Color fallback = { 0, 0, 0, 255 })
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

namespace Colors
{ // CSS Named Colors
	// Basic colors
	inline constexpr Color Black{ 0, 0, 0, 255 };
	inline constexpr Color White{ 255, 255, 255, 255 };
	inline constexpr Color Red{ 255, 0, 0, 255 };
	inline constexpr Color Green{ 0, 128, 0, 255 };
	inline constexpr Color Blue{ 0, 0, 255, 255 };
	inline constexpr Color Yellow{ 255, 255, 0, 255 };
	inline constexpr Color Cyan{ 0, 255, 255, 255 };
	inline constexpr Color Magenta{ 255, 0, 255, 255 };
	inline constexpr Color Gray{ 128, 128, 128, 255 };
	inline constexpr Color Silver{ 192, 192, 192, 255 };
	inline constexpr Color Maroon{ 128, 0, 0, 255 };
	inline constexpr Color Olive{ 128, 128, 0, 255 };
	inline constexpr Color Lime{ 0, 255, 0, 255 };
	inline constexpr Color Aqua{ 0, 255, 255, 255 };
	inline constexpr Color Teal{ 0, 128, 128, 255 };
	inline constexpr Color Navy{ 0, 0, 128, 255 };
	inline constexpr Color Purple{ 128, 0, 128, 255 };
	inline constexpr Color Fuchsia{ 255, 0, 255, 255 };

	// Extended CSS colors
	inline constexpr Color AliceBlue{ 240, 248, 255, 255 };
	inline constexpr Color AntiqueWhite{ 250, 235, 215, 255 };
	inline constexpr Color Aquamarine{ 127, 255, 212, 255 };
	inline constexpr Color Azure{ 240, 255, 255, 255 };
	inline constexpr Color Bisque{ 255, 228, 196, 255 };
	inline constexpr Color BlanchedAlmond{ 255, 235, 205, 255 };
	inline constexpr Color BlueViolet{ 138, 43, 226, 255 };
	inline constexpr Color Burlywood{ 222, 184, 135, 255 };
	inline constexpr Color CadetBlue{ 95, 158, 160, 255 };
	inline constexpr Color Chartreuse{ 127, 255, 0, 255 };
	inline constexpr Color Chocolate{ 210, 105, 30, 255 };
	inline constexpr Color Coral{ 255, 127, 80, 255 };
	inline constexpr Color CornflowerBlue{ 100, 149, 237, 255 };
	inline constexpr Color Cornsilk{ 255, 248, 220, 255 };
	inline constexpr Color Crimson{ 220, 20, 60, 255 };
	inline constexpr Color DarkBlue{ 0, 0, 139, 255 };
	inline constexpr Color DarkCyan{ 0, 139, 139, 255 };
	inline constexpr Color DarkGoldenrod{ 184, 134, 11, 255 };
	inline constexpr Color DarkGreen{ 0, 100, 0, 255 };
	inline constexpr Color DarkKhaki{ 189, 183, 107, 255 };
	inline constexpr Color DarkMagenta{ 139, 0, 139, 255 };
	inline constexpr Color DarkOliveGreen{ 85, 107, 47, 255 };
	inline constexpr Color DarkOrange{ 255, 140, 0, 255 };
	inline constexpr Color DarkOrchid{ 153, 50, 204, 255 };
	inline constexpr Color DarkRed{ 139, 0, 0, 255 };
	inline constexpr Color DarkSalmon{ 233, 150, 122, 255 };
	inline constexpr Color DarkSeaGreen{ 143, 188, 143, 255 };
	inline constexpr Color DarkSlateBlue{ 72, 61, 139, 255 };
	inline constexpr Color DarkSlateGray{ 47, 79, 79, 255 };
	inline constexpr Color DarkTurquoise{ 0, 206, 209, 255 };
	inline constexpr Color DarkViolet{ 148, 0, 211, 255 };
	inline constexpr Color DeepPink{ 255, 20, 147, 255 };
	inline constexpr Color DeepSkyBlue{ 0, 191, 255, 255 };
	inline constexpr Color DimGray{ 105, 105, 105, 255 };
	inline constexpr Color DodgerBlue{ 30, 144, 255, 255 };
	inline constexpr Color Firebrick{ 178, 34, 34, 255 };
	inline constexpr Color FloralWhite{ 255, 250, 240, 255 };
	inline constexpr Color ForestGreen{ 34, 139, 34, 255 };
	inline constexpr Color Gainsboro{ 220, 220, 220, 255 };
	inline constexpr Color GhostWhite{ 248, 248, 255, 255 };
	inline constexpr Color Goldenrod{ 218, 165, 32, 255 };
	inline constexpr Color GreenYellow{ 173, 255, 47, 255 };
	inline constexpr Color Honeydew{ 240, 255, 240, 255 };
	inline constexpr Color HotPink{ 255, 105, 180, 255 };
	inline constexpr Color IndianRed{ 205, 92, 92, 255 };
	inline constexpr Color Indigo{ 75, 0, 130, 255 };
	inline constexpr Color Ivory{ 255, 255, 240, 255 };
	inline constexpr Color Khaki{ 240, 230, 140, 255 };
	inline constexpr Color Lavender{ 230, 230, 250, 255 };
	inline constexpr Color LavenderBlush{ 255, 240, 245, 255 };
	inline constexpr Color LawnGreen{ 124, 252, 0, 255 };
	inline constexpr Color LemonChiffon{ 255, 250, 205, 255 };
	inline constexpr Color LightBlue{ 173, 216, 230, 255 };
	inline constexpr Color LightCoral{ 240, 128, 128, 255 };
	inline constexpr Color LightCyan{ 224, 255, 255, 255 };
	inline constexpr Color LightGoldenrodYellow{ 250, 250, 210, 255 };
	inline constexpr Color LightGreen{ 144, 238, 144, 255 };
	inline constexpr Color LightPink{ 255, 182, 193, 255 };
	inline constexpr Color LightSalmon{ 255, 160, 122, 255 };
	inline constexpr Color LightSeaGreen{ 32, 178, 170, 255 };
	inline constexpr Color LightSkyBlue{ 135, 206, 250, 255 };
	inline constexpr Color LightSlateGray{ 119, 136, 153, 255 };
	inline constexpr Color LightSteelBlue{ 176, 196, 222, 255 };
	inline constexpr Color LightYellow{ 255, 255, 224, 255 };
	inline constexpr Color LimeGreen{ 50, 205, 50, 255 };
	inline constexpr Color Linen{ 250, 240, 230, 255 };
	inline constexpr Color MediumAquamarine{ 102, 205, 170, 255 };
	inline constexpr Color MediumBlue{ 0, 0, 205, 255 };
	inline constexpr Color MediumOrchid{ 186, 85, 211, 255 };
	inline constexpr Color MediumPurple{ 147, 112, 219, 255 };
	inline constexpr Color MediumSeaGreen{ 60, 179, 113, 255 };
	inline constexpr Color MediumSlateBlue{ 123, 104, 238, 255 };
	inline constexpr Color MediumSpringGreen{ 0, 250, 154, 255 };
	inline constexpr Color MediumTurquoise{ 72, 209, 204, 255 };
	inline constexpr Color MediumVioletRed{ 199, 21, 133, 255 };
	inline constexpr Color MidnightBlue{ 25, 25, 112, 255 };
	inline constexpr Color MintCream{ 245, 255, 250, 255 };
	inline constexpr Color MistyRose{ 255, 228, 225, 255 };
	inline constexpr Color Moccasin{ 255, 228, 181, 255 };
	inline constexpr Color NavajoWhite{ 255, 222, 173, 255 };
	inline constexpr Color OldLace{ 253, 245, 230, 255 };
	inline constexpr Color OliveDrab{ 107, 142, 35, 255 };
	inline constexpr Color OrangeRed{ 255, 69, 0, 255 };
	inline constexpr Color Orchid{ 218, 112, 214, 255 };
	inline constexpr Color PaleGoldenrod{ 238, 232, 170, 255 };
	inline constexpr Color PaleGreen{ 152, 251, 152, 255 };
	inline constexpr Color PaleTurquoise{ 175, 238, 238, 255 };
	inline constexpr Color PaleVioletRed{ 219, 112, 147, 255 };
	inline constexpr Color PapayaWhip{ 255, 239, 213, 255 };
	inline constexpr Color PeachPuff{ 255, 218, 185, 255 };
	inline constexpr Color Peru{ 205, 133, 63, 255 };
	inline constexpr Color Plum{ 221, 160, 221, 255 };
	inline constexpr Color PowderBlue{ 176, 224, 230, 255 };
	inline constexpr Color RebeccaPurple{ 102, 51, 153, 255 };
	inline constexpr Color RosyBrown{ 188, 143, 143, 255 };
	inline constexpr Color RoyalBlue{ 65, 105, 225, 255 };
	inline constexpr Color SaddleBrown{ 139, 69, 19, 255 };
	inline constexpr Color Salmon{ 250, 128, 114, 255 };
	inline constexpr Color SandyBrown{ 244, 164, 96, 255 };
	inline constexpr Color SeaGreen{ 46, 139, 87, 255 };
	inline constexpr Color Seashell{ 255, 245, 238, 255 };
	inline constexpr Color Sienna{ 160, 82, 45, 255 };
	inline constexpr Color SkyBlue{ 135, 206, 235, 255 };
	inline constexpr Color SlateBlue{ 106, 90, 205, 255 };
	inline constexpr Color SlateGray{ 112, 128, 144, 255 };
	inline constexpr Color Snow{ 255, 250, 250, 255 };
	inline constexpr Color SpringGreen{ 0, 255, 127, 255 };
	inline constexpr Color SteelBlue{ 70, 130, 180, 255 };
	inline constexpr Color Tan{ 210, 180, 140, 255 };
	inline constexpr Color Thistle{ 216, 191, 216, 255 };
	inline constexpr Color Tomato{ 255, 99, 71, 255 };
	inline constexpr Color Turquoise{ 64, 224, 208, 255 };
	inline constexpr Color Violet{ 238, 130, 238, 255 };
	inline constexpr Color Wheat{ 245, 245, 220, 255 };
	inline constexpr Color WhiteSmoke{ 245, 245, 245, 255 };
	inline constexpr Color YellowGreen{ 154, 205, 50, 255 };
} // namespace Colors

// PropertyDictionary — key-value property storage with delta tracking
struct PropertyDictionary
{
	PropertyDictionary() = default;

	bool has(const std::string& key) const { return _deltas.find(key) != _deltas.end() || _org.find(key) != _org.end(); }

	std::string get(const std::string& key, const std::string& fallback = "") const
	{
		std::string retVal = fallback;
		auto itDelta = _deltas.find(key);
		if (itDelta != _deltas.end())
			retVal = itDelta->second;
		else
		{
			auto itOrg = _org.find(key);
			if (itOrg != _org.end())
				retVal = itOrg->second;
		}
		return retVal;
	}

	i32 getInt(const std::string& key, i32 fallback = 0) const
	{
		i32 retVal = fallback;
		std::string val = get(key);
		if (!val.empty())
		{
			try
			{
				retVal = static_cast<i32>(std::stoi(val));
			} catch (...)
			{
				retVal = fallback;
			}
		}
		return retVal;
	}

	f32 getFloat(const std::string& key, f32 fallback = 0.0f) const
	{
		f32 retVal = fallback;
		std::string val = get(key);
		if (!val.empty())
		{
			try
			{
				retVal = std::stof(val);
			} catch (...)
			{
				retVal = fallback;
			}
		}
		return retVal;
	}

	bool getBool(const std::string& key, bool fallback = false) const
	{
		bool retVal = fallback;
		std::string val = get(key);
		if (!val.empty())
		{
			std::string lower = val;
			std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
			if (lower == "true" || lower == "1")
				retVal = true;
			else if (lower == "false" || lower == "0")
				retVal = false;
		}
		return retVal;
	}

	std::vector<std::string> getList(const std::string& prefix) const
	{
		std::vector<std::string> list;
		if (has(prefix))
		{
			list.push_back(get(prefix));
			int index = 1;
			bool found = true;
			while (found)
			{
				std::string nextKey = prefix + std::to_string(index);
				if (has(nextKey))
				{
					list.push_back(get(nextKey));
					index++;
				}
				else
					found = false;
			}
		}
		return list;
	}

	void set(const std::string& key, const std::string& value)
	{
		auto itOrg = _org.find(key);
		if (itOrg != _org.end() && itOrg->second == value)
			_deltas.erase(key);
		else
			_deltas[key] = value;
	}

	void setInt(const std::string& key, int value) { set(key, std::to_string(value)); }
	void setFloat(const std::string& key, float value) { set(key, std::to_string(value)); }

	void setBool(const std::string& key, bool value) { set(key, value ? "true" : "false"); }
	bool flipBool(const std::string& key, bool fallback = false)
	{
		bool next = !getBool(key, fallback);
		setBool(key, next);
		return next;
	}

	void remove(const std::string& key)
	{
		_deltas.erase(key);
		_org.erase(key);
	}

	void setOrg(const std::string& key, const std::string& value) { _org[key] = value; }

	void revert() { _deltas.clear(); }

	void commit()
	{
		for (const auto& [k, v] : _deltas)
			_org[k] = v;
		_deltas.clear();
	}

	std::vector<std::string> allKeys() const
	{
		std::set<std::string> keys;
		for (const auto& [k, v] : _org)
			keys.insert(k);
		for (const auto& [k, v] : _deltas)
			keys.insert(k);
		return std::vector<std::string>(keys.begin(), keys.end());
	}

	const std::unordered_map<std::string, std::string>& getOrg() const { return _org; }
	const std::unordered_map<std::string, std::string>& getDeltas() const { return _deltas; }

private:
	std::unordered_map<std::string, std::string> _org;
	std::unordered_map<std::string, std::string> _deltas;
};
