#pragma once

#include "pch.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

class GlyphCache {
public:
    static GlyphCache& instance();

    bool init(SDL_Renderer* renderer, const std::string& imagesDir);
    void shutdown();

    void setImagesDir(const std::string& dir);
    std::string getImagesDir() const { return m_imagesDir; }

    SDL_Texture* getTexture(const std::string& glyphName);
    std::vector<std::string> getAllGlyphs();

    void renderGlyph(
        const std::string& glyphName,
        const f32Rect& dst,
        float rotation = 0.0f,
        bool flipH = false,
        bool flipV = false,
        float alpha = 1.0f
    );

    void renderBlankBackground(const f32Rect& dst, float alpha = 1.0f);

    void clear();

private:
    GlyphCache() = default;
    ~GlyphCache();

    std::string resolvePath(const std::string& glyphName);

    SDL_Renderer* m_renderer = nullptr;
    std::string m_imagesDir;
    std::unordered_map<std::string, SDL_Texture*> m_textures;
};
