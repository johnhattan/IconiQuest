#pragma once

#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
#include <cstdint>

struct SDL_Renderer;
struct SDL_Texture;

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
    uint8_t r = 0;
    uint8_t g = 0;
    uint8_t b = 0;
    uint8_t a = 255;

    static ColorRGBA fromHex(const std::string& hexStr, ColorRGBA fallback = {0, 0, 0, 255}) {
        if (hexStr.empty()) return fallback;
        std::string s = hexStr;
        if (s[0] == '#') s = s.substr(1);
        else if (s.rfind("0x", 0) == 0 || s.rfind("0X", 0) == 0) s = s.substr(2);

        try {
            uint32_t val = (uint32_t)std::stoul(s, nullptr, 16);
            if (s.length() <= 6) {
                return {
                    (uint8_t)((val >> 16) & 0xFF),
                    (uint8_t)((val >> 8) & 0xFF),
                    (uint8_t)(val & 0xFF),
                    255
                };
            } else {
                return {
                    (uint8_t)((val >> 24) & 0xFF),
                    (uint8_t)((val >> 16) & 0xFF),
                    (uint8_t)((val >> 8) & 0xFF),
                    (uint8_t)(val & 0xFF)
                };
            }
        } catch (...) {
            return fallback;
        }
    }
};
