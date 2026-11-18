#pragma once
/*
 * GlyphCache
 *
 * Singleton image and glyph resource service.  GlyphCache loads image assets,
 * resolves FontAwesome icon glyph names, caches SDL textures, and renders
 * glyphs or blank actor backgrounds into supplied destination rectangles.
 *
 * The asset root and current game bundle are set by the application or World
 * before rendering.  Callers own the destination geometry and renderer lifetime.
 */

#include "pch.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

struct TTF_Font;

struct GlyphCache
{
	static GlyphCache& instance();

	bool init(SDL_Renderer* pRenderer, const std::string& rImagesDir);
	void shutdown();

	void setImagesDir(const std::string& rDir);
	void setGameAssetsRoot(const std::string& rRoot);
	void setCurrentGame(const std::string& rGame);
	std::string getImagesDir() const { return _imagesDir; }

	observer_ptr<SDL_Texture> getTexture(const std::string& rGlyphName);
	std::vector<std::string> getAllGlyphs();
	bool isFromFontAwesome(const std::string& rGlyphName) const;

	void renderGlyph(
	    const std::string& rGlyphName,
	    const f32Rectangle& rDst,
	    f32 rotation = 0.0f,
	    bool flipH = false,
	    bool flipV = false,
	    f32 alpha = 1.0f);

	void renderBlankBackground(const f32Rectangle& rDst, f32 alpha = 1.0f);

	void clear();

private:
	GlyphCache() = default;
	~GlyphCache();

	std::string resolvePath(const std::string& rGlyphName);
	observer_ptr<SDL_Texture> renderFontAwesomeGlyph(const std::string& rGlyphName, i32 size = 48);

	observer_ptr<SDL_Renderer> _pRenderer;
	std::string _imagesDir;
	std::string _gameAssetsRoot;
	std::string _currentGame = "_default";
	std::unordered_map<std::string, observer_ptr<SDL_Texture>> _textures;
	observer_ptr<TTF_Font> _pFontAwesomeFont;
};
