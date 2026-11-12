#include "GlyphCache.hpp"
#include "pch.hpp"

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
	return true;
}

void GlyphCache::shutdown()
{
	clear();
	_pRenderer = nullptr;
}

void GlyphCache::clear()
{
	for (auto& [name, pTex] : _textures)
	{
		if (pTex)
		{
			SDL_DestroyTexture(pTex.get());
		}
	}
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

		// First priority: try the game-specific images folder (if not using _default)
		bool tryGameSpecific = !_gameAssetsRoot.empty() && !_currentGame.empty() && _currentGame != "_default";
		if (tryGameSpecific)
		{
			for (const auto& ext : exts)
			{
				std::filesystem::path p = std::filesystem::path(_gameAssetsRoot) / _currentGame / "images" / (rGlyphName + ext);
				if (std::filesystem::exists(p))
				{
					retVal = p.string();
				}
			}
		}

		if (retVal.empty())
		{
			// Second priority: fall back to default images dir
			for (const auto& ext : exts)
			{
				std::filesystem::path p = std::filesystem::path(_imagesDir) / (rGlyphName + ext);
				if (std::filesystem::exists(p))
				{
					retVal = p.string();
				}
			}
		}

		if (retVal.empty())
		{
			// Third priority: try direct file (no extension)
			std::filesystem::path direct = std::filesystem::path(_imagesDir) / rGlyphName;
			if (std::filesystem::exists(direct))
			{
				retVal = direct.string();
			}
		}
	}

	return retVal;
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
		{
			retVal = it->second;
		}
		else
		{
			std::string path = resolvePath(rGlyphName);
			bool pathFound = !path.empty();
			if (!pathFound)
			{
				_textures[rGlyphName] = nullptr;
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
					{
						std::cerr << "[GlyphCache] Failed to create texture for " << path << ": " << SDL_GetError() << std::endl;
					}

					_textures[rGlyphName] = pTex;
					retVal = pTex;
				}
			}
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
				{
					results.push_back(entry.path().stem().string());
				}
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
		observer_ptr<SDL_Texture> pTex = getTexture(rGlyphName);
		if (pTex)
		{
			SDL_SetTextureAlphaModFloat(pTex.get(), alpha);

			SDL_FRect fdst = { rDst.x, rDst.y, rDst.w, rDst.h };
			SDL_FlipMode flip = SDL_FLIP_NONE;
			bool flipBoth = flipH && flipV;
			bool flipHOnly = flipH && !flipV;
			bool flipVOnly = flipV && !flipH;
			if (flipBoth)
			{
				flip = SDL_FLIP_HORIZONTAL_AND_VERTICAL;
			}
			else if (flipHOnly)
			{
				flip = SDL_FLIP_HORIZONTAL;
			}
			else if (flipVOnly)
			{
				flip = SDL_FLIP_VERTICAL;
			}

			bool hasTransform = rotation != 0.0f || flip != SDL_FLIP_NONE;
			if (hasTransform)
			{
				SDL_RenderTextureRotated(_pRenderer.get(), pTex.get(), NULL, &fdst, (double)rotation, NULL, flip);
			}
			else
			{
				SDL_RenderTexture(_pRenderer.get(), pTex.get(), NULL, &fdst);
			}
		}
	}
}
