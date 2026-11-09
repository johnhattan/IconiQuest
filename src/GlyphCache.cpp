#include "GlyphCache.hpp"
#include <filesystem>
#include <iostream>

GlyphCache& GlyphCache::instance() {
    static GlyphCache inst;
    return inst;
}

GlyphCache::~GlyphCache() {
    shutdown();
}

bool GlyphCache::init(SDL_Renderer* renderer, const std::string& imagesDir) {
    m_renderer = renderer;
    m_imagesDir = imagesDir;
    return true;
}

void GlyphCache::shutdown() {
    clear();
    m_renderer = nullptr;
}

void GlyphCache::clear() {
    for (auto& [name, tex] : m_textures) {
        if (tex) {
            SDL_DestroyTexture(tex);
        }
    }
    m_textures.clear();
}

void GlyphCache::setImagesDir(const std::string& dir) {
    if (m_imagesDir != dir) {
        clear();
        m_imagesDir = dir;
    }
}

std::string GlyphCache::resolvePath(const std::string& glyphName) {
    if (glyphName.empty()) return "";

    std::vector<std::string> exts = {".svg", ".png"};
    for (const auto& ext : exts) {
        std::filesystem::path p = std::filesystem::path(m_imagesDir) / (glyphName + ext);
        if (std::filesystem::exists(p)) {
            return p.string();
        }
    }

    std::filesystem::path direct = std::filesystem::path(m_imagesDir) / glyphName;
    if (std::filesystem::exists(direct)) {
        return direct.string();
    }

    return "";
}

SDL_Texture* GlyphCache::getTexture(const std::string& glyphName) {
    if (!m_renderer || glyphName.empty()) return nullptr;

    auto it = m_textures.find(glyphName);
    if (it != m_textures.end()) {
        return it->second;
    }

    std::string path = resolvePath(glyphName);
    if (path.empty()) {
        m_textures[glyphName] = nullptr;
        return nullptr;
    }

    SDL_Surface* surface = IMG_Load(path.c_str());
    if (!surface) {
        std::cerr << "[GlyphCache] Failed to load " << path << ": " << SDL_GetError() << std::endl;
        m_textures[glyphName] = nullptr;
        return nullptr;
    }

    SDL_Texture* tex = SDL_CreateTextureFromSurface(m_renderer, surface);
    SDL_DestroySurface(surface);

    if (!tex) {
        std::cerr << "[GlyphCache] Failed to create texture for " << path << ": " << SDL_GetError() << std::endl;
    }

    m_textures[glyphName] = tex;
    return tex;
}

std::vector<std::string> GlyphCache::getAllGlyphs() {
    std::vector<std::string> results;
    if (!std::filesystem::exists(m_imagesDir)) return results;

    for (const auto& entry : std::filesystem::directory_iterator(m_imagesDir)) {
        if (entry.is_regular_file()) {
            std::string ext = entry.path().extension().string();
            if (ext == ".svg" || ext == ".png") {
                results.push_back(entry.path().stem().string());
            }
        }
    }
    std::sort(results.begin(), results.end());
    return results;
}

void GlyphCache::renderBlankBackground(const f32Rect& dst, float alpha) {
    SDL_Texture* tex = getTexture("_blank");
    if (tex) {
        SDL_SetTextureAlphaModFloat(tex, alpha);
        SDL_FRect fdst = {dst.x, dst.y, dst.w, dst.h};
        SDL_RenderTexture(m_renderer, tex, NULL, &fdst);
    } else {
        // Fallback: draw circular outline/fill
        SDL_SetRenderDrawColor(m_renderer, 180, 180, 180, (Uint8)(alpha * 200.0f));
        SDL_FRect fdst = {dst.x, dst.y, dst.w, dst.h};
        SDL_RenderFillRect(m_renderer, &fdst);
    }
}

void GlyphCache::renderGlyph(
    const std::string& glyphName,
    const f32Rect& dst,
    float rotation,
    bool flipH,
    bool flipV,
    float alpha
) {
    if (glyphName.empty() || glyphName == "blank" || glyphName == "_blank_ignore") {
        return;
    }

    SDL_Texture* tex = getTexture(glyphName);
    if (!tex) return;

    SDL_SetTextureAlphaModFloat(tex, alpha);

    SDL_FRect fdst = {dst.x, dst.y, dst.w, dst.h};
    SDL_FlipMode flip = SDL_FLIP_NONE;
    if (flipH && flipV) flip = SDL_FLIP_HORIZONTAL_AND_VERTICAL;
    else if (flipH) flip = SDL_FLIP_HORIZONTAL;
    else if (flipV) flip = SDL_FLIP_VERTICAL;

    if (rotation != 0.0f || flip != SDL_FLIP_NONE) {
        SDL_RenderTextureRotated(m_renderer, tex, NULL, &fdst, (double)rotation, NULL, flip);
    } else {
        SDL_RenderTexture(m_renderer, tex, NULL, &fdst);
    }
}
