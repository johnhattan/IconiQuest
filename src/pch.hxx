#pragma once

struct SDL_Renderer;
struct SDL_Texture;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using i8 = std::int8_t;
using i16 = std::int16_t;
using i32 = std::int32_t;
using i64 = std::int64_t;
using f32 = float;
using f64 = double;
using usize = std::size_t;

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    Vec2() = default;
    Vec2(float _x, float _y) : x(_x), y(_y) {}

    Vec2 operator+(const Vec2& o) const { return {x + o.x, y + o.y}; }
    Vec2 operator-(const Vec2& o) const { return {x - o.x, y - o.y}; }
    Vec2 operator*(float s) const { return {x * s, y * s}; }
    Vec2 operator/(float s) const { return {x / s, y / s}; }

    float distance(const Vec2& o) const {
        float dx = x - o.x;
        float dy = y - o.y;
        return std::sqrt(dx * dx + dy * dy);
    }
};

struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;

    Rect() = default;
    Rect(float _x, float _y, float _w, float _h) : x(_x), y(_y), w(_w), h(_h) {}

    bool contains(const Vec2& pt) const {
        return pt.x >= x && pt.x <= (x + w) && pt.y >= y && pt.y <= (y + h);
    }

    Vec2 center() const {
        return {x + w * 0.5f, y + h * 0.5f};
    }
};

struct ColorRGBA {
    u8 r = 0;
    u8 g = 0;
    u8 b = 0;
    u8 a = 255;

    static ColorRGBA fromHex(const std::string& hexStr, ColorRGBA fallback = {0, 0, 0, 255}) {
        if (hexStr.empty()) return fallback;

        std::string s = hexStr;
        if (s[0] == '#') s = s.substr(1);
        else if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) s = s.substr(2);

        try {
            u32 val = static_cast<u32>(std::stoul(s, nullptr, 16));
            if (s.length() <= 6) {
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
        } catch (...) {
            return fallback;
        }
    }
};
