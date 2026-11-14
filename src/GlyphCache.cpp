#include "GlyphCache.hpp"
#include "FontAwesomeIcons.hpp"
#include "pch.hpp"
#include <SDL3_ttf/SDL_ttf.h>

GlyphCache& GlyphCache::instance()
{
	static GlyphCache inst;
	return inst;
}

GlyphCache::~GlyphCache()
{
	shutdown();
}

bool GlyphCache::init(SDL_Renderer* pRenderer, const std::string& rImagesDir)
{
	_pRenderer = observer_ptr<SDL_Renderer>(pRenderer);
	_imagesDir = rImagesDir;

	// Load FontAwesome font for glyph rendering
	// The font path is resolved relative to the _default images dir
	// Try both .ttf and .otf extensions
	std::filesystem::path fontsDir = std::filesystem::path(rImagesDir).parent_path() / "fonts";
	std::vector<std::string> fontExtensions = { "FontAwesome-Regular.ttf", "FontAwesome-Regular.otf" };

	std::filesystem::path fontPath;
	bool fontFound = false;
	for (const auto& ext : fontExtensions)
	{
		if (!fontFound)
		{
			std::filesystem::path candidate = fontsDir / ext;
			if (std::filesystem::exists(candidate))
			{
				fontPath = candidate;
				fontFound = true;
			}
		}
	}

	if (!fontPath.empty())
	{
		// Ensure TTF is initialized (safe to call multiple times)
		TTF_Init();

		observer_ptr<TTF_Font> pFont(TTF_OpenFont(fontPath.string().c_str(), 48));
		if (pFont)
		{
			_pFontAwesomeFont = pFont;
			std::cout << "[GlyphCache] Loaded FontAwesome font: " << fontPath.string() << std::endl;
		}
		else
			std::cerr << "[GlyphCache] Failed to load FontAwesome font: " << SDL_GetError() << std::endl;
	}
	else
		std::cerr << "[GlyphCache] FontAwesome font not found in: " << fontsDir.string() << std::endl;

	return true;
}

void GlyphCache::shutdown()
{
	clear();
	_pRenderer = nullptr;
	if (_pFontAwesomeFont)
	{
		TTF_CloseFont(_pFontAwesomeFont.get());
		_pFontAwesomeFont = nullptr;
	}
	TTF_Quit();
}

void GlyphCache::clear()
{
	for (auto& [name, pTex] : _textures)
		if (pTex)
			SDL_DestroyTexture(pTex.get());

	_textures.clear();
}

void GlyphCache::setImagesDir(const std::string& rDir)
{
	bool dirChanged = _imagesDir != rDir;
	if (dirChanged)
	{
		clear();
		_imagesDir = rDir;
	}
}

void GlyphCache::setGameAssetsRoot(const std::string& rRoot)
{
	_gameAssetsRoot = rRoot;
}

void GlyphCache::setCurrentGame(const std::string& rGame)
{
	_currentGame = rGame;
	clear();
}

std::string GlyphCache::resolvePath(const std::string& rGlyphName)
{
	std::string retVal;

	if (!rGlyphName.empty())
	{
		std::vector<std::string> exts = { ".svg", ".png" };
		bool isDefaultGame = _currentGame.empty() || _currentGame == "_default";

		// For non-_default games: check game-specific images folder first
		if (!isDefaultGame && !_gameAssetsRoot.empty() && retVal.empty())
		{
			for (const auto& ext : exts)
			{
				if (retVal.empty())
				{
					std::filesystem::path p = std::filesystem::path(_gameAssetsRoot) / _currentGame / "images" / (rGlyphName + ext);
					if (std::filesystem::exists(p))
						retVal = p.string();
				}
			}
		}

		// Second priority: check _default images folder (for all games)
		if (retVal.empty())
		{
			for (const auto& ext : exts)
			{
				if (retVal.empty())
				{
					std::filesystem::path p = std::filesystem::path(_imagesDir) / (rGlyphName + ext);
					if (std::filesystem::exists(p))
						retVal = p.string();
				}
			}
		}

		// Third priority: try direct file (no extension)
		if (retVal.empty())
		{
			std::filesystem::path direct = std::filesystem::path(_imagesDir) / rGlyphName;
			if (std::filesystem::exists(direct))
				retVal = direct.string();
		}

		// Fourth priority: check if it's a FontAwesome icon
		if (retVal.empty())
		{
			if (FontAwesome::iconMap.find(rGlyphName) != FontAwesome::iconMap.end())
				retVal = "fontawesome:" + rGlyphName;
		}
	}

	return retVal;
}

bool GlyphCache::isFromFontAwesome(const std::string& rGlyphName) const
{
	return FontAwesome::iconMap.find(rGlyphName) != FontAwesome::iconMap.end();
}

observer_ptr<SDL_Texture> GlyphCache::getTexture(const std::string& rGlyphName)
{
	observer_ptr<SDL_Texture> retVal;

	bool canLoad = _pRenderer && !rGlyphName.empty();
	if (canLoad)
	{
		auto it = _textures.find(rGlyphName);
		bool found = it != _textures.end();
		if (found)
			retVal = it->second;
		else
		{
			std::string path = resolvePath(rGlyphName);
			bool isFontAwesome = !path.empty() && path.substr(0, 12) == "fontawesome:";
			bool pathFound = !path.empty();
			if (!pathFound)
				_textures[rGlyphName] = nullptr;
			else if (isFontAwesome)
			{
				// FontAwesome icon - render it from the font
				observer_ptr<SDL_Texture> pFATex = renderFontAwesomeGlyph(rGlyphName);
				_textures[rGlyphName] = pFATex;
				retVal = pFATex;
			}
			else
			{
				observer_ptr<SDL_Surface> pSurface(IMG_Load(path.c_str()));
				if (!pSurface)
				{
					std::cerr << "[GlyphCache] Failed to load " << path << ": " << SDL_GetError() << std::endl;
					_textures[rGlyphName] = nullptr;
				}
				else
				{
					observer_ptr<SDL_Texture> pTex(SDL_CreateTextureFromSurface(_pRenderer.get(), pSurface.get()));
					SDL_DestroySurface(pSurface.get());

					if (!pTex)
						std::cerr << "[GlyphCache] Failed to create texture for " << path << ": " << SDL_GetError() << std::endl;

					_textures[rGlyphName] = pTex;
					retVal = pTex;
				}
			}
		}
	}

	return retVal;
}

observer_ptr<SDL_Texture> GlyphCache::renderFontAwesomeGlyph(const std::string& rGlyphName, i32 size)
{
	observer_ptr<SDL_Texture> retVal;

	bool canRender = _pFontAwesomeFont && _pRenderer;
	if (canRender)
	{
		// Get the Unicode codepoint for this icon
		auto it = FontAwesome::iconMap.find(rGlyphName);
		bool foundIcon = it != FontAwesome::iconMap.end();

		if (foundIcon)
		{
			u32 codepoint = it->second;

			// Convert codepoint to UTF-8 string
			std::string utf8Char;
			if (codepoint < 0x80)
				utf8Char = static_cast<char>(codepoint);
			else if (codepoint < 0x800)
			{
				utf8Char += static_cast<char>(0xC0 | (codepoint >> 6));
				utf8Char += static_cast<char>(0x80 | (codepoint & 0x3F));
			}
			else if (codepoint < 0x10000)
			{
				utf8Char += static_cast<char>(0xE0 | (codepoint >> 12));
				utf8Char += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
				utf8Char += static_cast<char>(0x80 | (codepoint & 0x3F));
			}
			else
			{
				utf8Char += static_cast<char>(0xF0 | (codepoint >> 18));
				utf8Char += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
				utf8Char += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
				utf8Char += static_cast<char>(0x80 | (codepoint & 0x3F));
			}

			// Render glyph to surface using the codepoint directly
			SDL_Color black = { 0, 0, 0, 255 };
			observer_ptr<SDL_Surface> pSurface(TTF_RenderGlyph_Blended(_pFontAwesomeFont.get(), codepoint, black));

			if (pSurface)
			{
				// Convert surface to texture
				observer_ptr<SDL_Texture> pTex(SDL_CreateTextureFromSurface(_pRenderer.get(), pSurface.get()));
				SDL_DestroySurface(pSurface.get());

				if (pTex)
					retVal = pTex;
				else
					std::cerr << "[GlyphCache] Failed to create texture for FontAwesome glyph '" << rGlyphName << "': " << SDL_GetError() << std::endl;
			}
			else
				std::cerr << "[GlyphCache] Failed to render FontAwesome glyph '" << rGlyphName << "': " << SDL_GetError() << std::endl;
		}
	}

	return retVal;
}

std::vector<std::string> GlyphCache::getAllGlyphs()
{
	std::vector<std::string> results;

	if (std::filesystem::exists(_imagesDir))
	{
		for (const auto& entry : std::filesystem::directory_iterator(_imagesDir))
		{
			if (entry.is_regular_file())
			{
				std::string ext = entry.path().extension().string();
				bool isSvgOrPng = ext == ".svg" || ext == ".png";
				if (isSvgOrPng)
					results.push_back(entry.path().stem().string());
			}
		}
		std::sort(results.begin(), results.end());
	}

	return results;
}

void GlyphCache::renderBlankBackground(const f32Rectangle& rDst, f32 alpha)
{
	observer_ptr<SDL_Texture> pTex = getTexture("_blank");
	if (pTex)
	{
		SDL_SetTextureAlphaModFloat(pTex.get(), alpha);
		SDL_FRect fdst = { rDst.x, rDst.y, rDst.w, rDst.h };
		SDL_RenderTexture(_pRenderer.get(), pTex.get(), NULL, &fdst);
	}
	else
	{
		SDL_SetRenderDrawColor(_pRenderer.get(), 180, 180, 180, (Uint8)(alpha * 200.0f));
		SDL_FRect fdst = { rDst.x, rDst.y, rDst.w, rDst.h };
		SDL_RenderFillRect(_pRenderer.get(), &fdst);
	}
}

void GlyphCache::renderGlyph(const std::string& rGlyphName, const f32Rectangle& rDst, f32 rotation, bool flipH, bool flipV, f32 alpha)
{
	bool canRender = !rGlyphName.empty() && rGlyphName != "blank" && rGlyphName != "_blank_ignore";
	if (canRender)
	{
		// Get texture (handles both file-based and FontAwesome glyphs)
		observer_ptr<SDL_Texture> pTex = getTexture(rGlyphName);
		if (pTex)
		{
			SDL_SetTextureAlphaModFloat(pTex.get(), alpha);

			// Check if this is a FontAwesome glyph and scale to 75%
			bool isFontAwesome = isFromFontAwesome(rGlyphName);
			SDL_FRect fdst = { rDst.x, rDst.y, rDst.w, rDst.h };
			if (isFontAwesome)
			{ // Scale to 75% and center within the destination
				f32 scale = 0.75f;
				f32 scaledW = fdst.w * scale;
				f32 scaledH = fdst.h * scale;
				f32 offsetX = (fdst.w - scaledW) * 0.5f;
				f32 offsetY = (fdst.h - scaledH) * 0.5f;
				fdst.x += offsetX;
				fdst.y += offsetY;
				fdst.w = scaledW;
				fdst.h = scaledH;
			}

			SDL_FlipMode flip = SDL_FLIP_NONE;
			bool flipBoth = flipH && flipV;
			bool flipHOnly = flipH && !flipV;
			bool flipVOnly = flipV && !flipH;
			if (flipBoth)
				flip = SDL_FLIP_HORIZONTAL_AND_VERTICAL;
			else if (flipHOnly)
				flip = SDL_FLIP_HORIZONTAL;
			else if (flipVOnly)
				flip = SDL_FLIP_VERTICAL;

			bool hasTransform = rotation != 0.0f || flip != SDL_FLIP_NONE;
			if (hasTransform)
				SDL_RenderTextureRotated(_pRenderer.get(), pTex.get(), NULL, &fdst, (double)rotation, NULL, flip);
			else
				SDL_RenderTexture(_pRenderer.get(), pTex.get(), NULL, &fdst);
		}
		else
		{
			// Failed to load texture; render placeholder rectangle
			SDL_SetRenderDrawColor(_pRenderer.get(), 100, 150, 200, (Uint8)(alpha * 255.0f));
			SDL_FRect fdst = { rDst.x, rDst.y, rDst.w, rDst.h };
			SDL_RenderFillRect(_pRenderer.get(), &fdst);
			SDL_SetRenderDrawColor(_pRenderer.get(), 255, 255, 255, (Uint8)(alpha * 255.0f));
			SDL_RenderRect(_pRenderer.get(), &fdst);
		}
	}
}
