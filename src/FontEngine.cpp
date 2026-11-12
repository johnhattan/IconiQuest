#include "pch.hpp"

#include "FontEngine.hpp"
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

FontEngine& FontEngine::instance()
{
	static FontEngine s_instance;
	return s_instance;
}

FontEngine::~FontEngine()
{
	shutdown();
}

bool FontEngine::init(const std::string& rFontPath, i32 pointSize)
{
	bool retVal = false;
	bool initialized = false;

	if (TTF_Init())
	{
		initialized = true;
	}
	else
	{
		std::cerr << "[FontEngine] Failed to init SDL_ttf: " << SDL_GetError() << std::endl;
	}

	if (initialized)
	{
		observer_ptr<TTF_Font> pFont(TTF_OpenFont(rFontPath.c_str(), (f32)pointSize));
		if (pFont)
		{
			_pFont = pFont;
			std::cout << "[FontEngine] Successfully loaded font: " << rFontPath << " at " << pointSize << "pt" << std::endl;
			retVal = true;
		}
		else
		{
			std::cerr << "[FontEngine] Failed to load font from " << rFontPath << ": " << SDL_GetError() << std::endl;
		}
	}

	return retVal;
}

void FontEngine::shutdown()
{
	for (auto& [key, pTexture] : _textCache)
	{
		if (pTexture)
		{
			SDL_DestroyTexture(pTexture.get());
		}
	}

	_textCache.clear();

	if (_pFont)
	{
		TTF_CloseFont(_pFont.get());
		_pFont = nullptr;
	}

	TTF_Quit();
}

void FontEngine::renderText(observer_ptr<SDL_Renderer> pRenderer, const std::string& rText, const f32Point& rPos, const class Color& rColor)
{
	bool canRender = _pFont && pRenderer && !rText.empty();

	if (canRender)
	{
		std::string unescaped = unescapeText(rText);

		SDL_Color color = { rColor.r, rColor.g, rColor.b, rColor.a };
		observer_ptr<SDL_Surface> pSurface(TTF_RenderText_Blended(_pFont.get(), unescaped.c_str(), unescaped.length(), color));

		if (pSurface)
		{
			observer_ptr<SDL_Texture> pTexture(SDL_CreateTextureFromSurface(pRenderer.get(), pSurface.get()));
			SDL_DestroySurface(pSurface.get());

			if (pTexture)
			{
				f32 w = 0.0f, h = 0.0f;
				SDL_GetTextureSize(pTexture.get(), &w, &h);
				SDL_FRect dstRect = { rPos.x, rPos.y, w, h };

				SDL_RenderTexture(pRenderer.get(), pTexture.get(), NULL, &dstRect);
				SDL_DestroyTexture(pTexture.get());
			}
			else
			{
				std::cerr << "[FontEngine] Failed to create text texture: " << SDL_GetError() << std::endl;
			}
		}
		else
		{
			std::cerr << "[FontEngine] Failed to render text surface: " << SDL_GetError() << std::endl;
		}
	}
}

void FontEngine::renderText(observer_ptr<SDL_Renderer> pRenderer, const std::string& rText, const f32Rectangle& rRect, const class Color& rColor, bool centered)
{
	bool canRender = _pFont && pRenderer && !rText.empty();

	if (canRender)
	{
		std::string unescaped = unescapeText(rText);

		// Split text by newlines
		std::vector<std::string> lines;
		std::stringstream ss(unescaped);
		std::string line;
		while (std::getline(ss, line))
		{
			lines.push_back(line);
		}

		// If text ends with newline, add a final blank line
		if (!unescaped.empty() && unescaped.back() == '\n')
		{
			lines.push_back("");
		}

		if (!lines.empty())
		{
			SDL_Color color = { rColor.r, rColor.g, rColor.b, rColor.a };
			i32 lineHeight = TTF_GetFontHeight(_pFont.get());

			// Calculate total height based on actual rendered lines
			f32 totalHeight = 0.0f;
			std::vector<f32> lineHeights;
			for (const auto& rLine : lines)
			{
				if (rLine.empty())
				{
					lineHeights.push_back((f32)lineHeight);
					totalHeight += lineHeight;
				}
				else
				{
					observer_ptr<SDL_Surface> pTestSurface(TTF_RenderText_Blended(_pFont.get(), rLine.c_str(), rLine.length(), color));
					if (pTestSurface)
					{
						f32 h = (f32)pTestSurface->h;
						lineHeights.push_back(h);
						totalHeight += h;
						SDL_DestroySurface(pTestSurface.get());
					}
					else
					{
						lineHeights.push_back((f32)lineHeight);
						totalHeight += lineHeight;
					}
				}
			}

			f32 currentY = rRect.y;
			if (centered)
			{
				currentY = rRect.y + (rRect.h - totalHeight) * 0.5f;
			}

			// Render each line
			for (uindex i = 0; i < lines.size(); ++i)
			{
				const auto& rLine = lines[i];
				// Render blank lines as a space so they maintain proper height
				const std::string& renderLine = rLine.empty() ? " " : rLine;

				observer_ptr<SDL_Surface> pSurface(TTF_RenderText_Blended(_pFont.get(), renderLine.c_str(), renderLine.length(), color));
				if (pSurface)
				{
					observer_ptr<SDL_Texture> pTexture(SDL_CreateTextureFromSurface(pRenderer.get(), pSurface.get()));
					SDL_DestroySurface(pSurface.get());

					if (pTexture)
					{
						f32 w = 0.0f, h = 0.0f;
						SDL_GetTextureSize(pTexture.get(), &w, &h);

						f32 x = rRect.x;
						if (centered)
						{
							x = rRect.x + (rRect.w - w) * 0.5f;
						}

						SDL_FRect dstRect = { x, currentY, w, h };
						SDL_RenderTexture(pRenderer.get(), pTexture.get(), NULL, &dstRect);
						SDL_DestroyTexture(pTexture.get());
					}
				}

				currentY += lineHeights[i];
			}
		}
	}
}

i32 FontEngine::getTextWidth(const std::string& rText) const
{
	i32 maxWidth = 0;

	if (_pFont && !rText.empty())
	{
		std::string unescaped = unescapeText(rText);
		std::stringstream ss(unescaped);
		std::string line;
		while (std::getline(ss, line))
		{
			if (!line.empty())
			{
				i32 w = 0;
				TTF_MeasureString(_pFont.get(), line.c_str(), line.length(), 0, &w, nullptr);
				maxWidth = std::max(maxWidth, w);
			}
		}
	}

	return maxWidth;
}

i32 FontEngine::getTextHeight(const std::string& rText) const
{
	i32 height = 0;

	if (_pFont && !rText.empty())
	{
		std::string unescaped = unescapeText(rText);
		i32 lineHeight = TTF_GetFontHeight(_pFont.get());

		// Count lines
		i32 lineCount = 1;
		for (char c : unescaped)
		{
			if (c == '\n')
				lineCount++;
		}

		height = lineHeight * lineCount;
	}

	return height;
}

std::string FontEngine::unescapeText(const std::string& rText) const
{
	std::string result;
	result.reserve(rText.length());

	for (uindex i = 0; i < rText.length(); ++i)
	{
		if (rText[i] == '\\' && i + 1 < rText.length() && rText[i + 1] == 'n')
		{
			result += '\n';
			++i;
		}
		else
		{
			result += rText[i];
		}
	}

	return result;
}
