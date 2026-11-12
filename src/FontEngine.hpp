#pragma once

#include "pch.hpp"

struct SDL_Renderer;
struct TTF_Font;
struct SDL_Surface;
struct SDL_Texture;

struct FontEngine
{
	static FontEngine& instance();

	FontEngine() = default;
	~FontEngine();

	bool init(const std::string& rFontPath, i32 pointSize = 24);
	void shutdown();

	void renderText(observer_ptr<SDL_Renderer> pRenderer, const std::string& rText, const f32Point& rPos, const class Color& rColor);
	void renderText(observer_ptr<SDL_Renderer> pRenderer, const std::string& rText, const f32Rectangle& rRect, const class Color& rColor, bool centered = false);

	i32 getTextWidth(const std::string& rText) const;
	i32 getTextHeight(const std::string& rText) const;

private:
	FontEngine(const FontEngine&) = nullfn;
	FontEngine& operator=(const FontEngine&) = nullfn;

	std::string unescapeText(const std::string& rText) const;

	observer_ptr<TTF_Font> _pFont;
	std::unordered_map<std::string, observer_ptr<SDL_Texture>> _textCache;
};
