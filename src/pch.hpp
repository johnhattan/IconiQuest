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

struct Color
{
	u8 r = 0;
	u8 g = 0;
	u8 b = 0;
	u8 a = 255;

	static Color fromHex(const std::string& hexStr, Color fallback = {0, 0, 0, 255})
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
class PropertyDictionary {
public:
	PropertyDictionary() = default;

	bool has(const std::string& key) const {
		return m_deltas.find(key) != m_deltas.end() || m_org.find(key) != m_org.end();
	}

	std::string get(const std::string& key, const std::string& fallback = "") const {
		auto itDelta = m_deltas.find(key);
		if (itDelta != m_deltas.end()) {
			return itDelta->second;
		}
		auto itOrg = m_org.find(key);
		if (itOrg != m_org.end()) {
			return itOrg->second;
		}
		return fallback;
	}

	int getInt(const std::string& key, int fallback = 0) const {
		std::string val = get(key);
		if (val.empty()) return fallback;
		try {
			return std::stoi(val);
		} catch (...) {
			return fallback;
		}
	}

	float getFloat(const std::string& key, float fallback = 0.0f) const {
		std::string val = get(key);
		if (val.empty()) return fallback;
		try {
			return std::stof(val);
		} catch (...) {
			return fallback;
		}
	}

	bool getBool(const std::string& key, bool fallback = false) const {
		std::string val = get(key);
		if (val.empty()) return fallback;
		std::string lower = val;
		std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
		if (lower == "true" || lower == "1") return true;
		if (lower == "false" || lower == "0") return false;
		return fallback;
	}

	std::vector<std::string> getList(const std::string& prefix) const {
		std::vector<std::string> list;
		if (!has(prefix)) {
			return list;
		}

		list.push_back(get(prefix));
		int index = 1;
		while (true) {
			std::string nextKey = prefix + std::to_string(index);
			if (has(nextKey)) {
				list.push_back(get(nextKey));
				index++;
			} else {
				break;
			}
		}
		return list;
	}

	void set(const std::string& key, const std::string& value) {
		auto itOrg = m_org.find(key);
		if (itOrg != m_org.end() && itOrg->second == value) {
			m_deltas.erase(key);
		} else {
			m_deltas[key] = value;
		}
	}

	void setInt(const std::string& key, int value) {
		set(key, std::to_string(value));
	}

	void setFloat(const std::string& key, float value) {
		set(key, std::to_string(value));
	}

	void setBool(const std::string& key, bool value) {
		set(key, value ? "true" : "false");
	}

	bool flipBool(const std::string& key, bool fallback = false) {
		bool next = !getBool(key, fallback);
		setBool(key, next);
		return next;
	}

	void remove(const std::string& key) {
		m_deltas.erase(key);
		m_org.erase(key);
	}

	void setOrg(const std::string& key, const std::string& value) {
		m_org[key] = value;
	}

	void revert() {
		m_deltas.clear();
	}

	void commit() {
		for (const auto& [k, v] : m_deltas) {
			m_org[k] = v;
		}
		m_deltas.clear();
	}

	std::vector<std::string> allKeys() const {
		std::set<std::string> keys;
		for (const auto& [k, v] : m_org) keys.insert(k);
		for (const auto& [k, v] : m_deltas) keys.insert(k);
		return std::vector<std::string>(keys.begin(), keys.end());
	}

	const std::unordered_map<std::string, std::string>& getOrg() const { return m_org; }
	const std::unordered_map<std::string, std::string>& getDeltas() const { return m_deltas; }

private:
	std::unordered_map<std::string, std::string> m_org;
	std::unordered_map<std::string, std::string> m_deltas;
};
