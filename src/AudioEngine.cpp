#include "AudioEngine.hpp"
#include "pch.hpp"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

struct AudioEngine::Impl
{
	MIX_Mixer* pMixer = nullptr;
	MIX_Track* pThemeTrack = nullptr;
	bool initialized = false;
};

AudioEngine& AudioEngine::instance()
{
	static AudioEngine inst;
	return inst;
}

AudioEngine::AudioEngine() : _pImpl(std::make_unique<Impl>())
{
}

AudioEngine::~AudioEngine()
{
	shutdown();
}

bool AudioEngine::init(const std::string& rSoundsDir)
{
	_soundsDir = rSoundsDir;
	bool retVal = false;

	if (_pImpl->initialized)
	{
		retVal = true;
	}
	else
	{
		if (!MIX_Init())
		{
			std::cerr << "[AudioEngine] Failed to initialize SDL_mixer." << std::endl;
		}
		else
		{
			SDL_AudioSpec spec{};
			spec.freq = 44100;
			spec.format = SDL_AUDIO_S16;
			spec.channels = 2;

			_pImpl->pMixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
			if (!_pImpl->pMixer)
			{
				std::cerr << "[AudioEngine] Failed to create SDL_mixer device: " << SDL_GetError() << std::endl;
				MIX_Quit();
			}
			else
			{
				_pImpl->initialized = true;
				retVal = true;
			}
		}
	}

	return retVal;
}

void AudioEngine::shutdown()
{
	if (_pImpl && _pImpl->initialized)
	{
		stopTheme();
		if (_pImpl->pMixer)
		{
			MIX_DestroyMixer(_pImpl->pMixer);
			_pImpl->pMixer = nullptr;
		}
		MIX_Quit();
		_pImpl->initialized = false;
	}
}

void AudioEngine::setSoundsDir(const std::string& rDir)
{
	_soundsDir = rDir;
}

void AudioEngine::setGameAssetsRoot(const std::string& rRoot)
{
	_gameAssetsRoot = rRoot;
}

void AudioEngine::setCurrentGame(const std::string& rGame)
{
	_currentGame = rGame;
	// Stop the current theme when switching games
	stopTheme();
}

std::string AudioEngine::resolveSoundPath(const std::string& rName)
{
	std::string retVal;

	if (!rName.empty())
	{
		std::vector<std::string> exts = { ".ogg", ".mp3", ".wav", ".flac", ".mod" };

		if (!_gameAssetsRoot.empty() && !_currentGame.empty() && _currentGame != "_default")
		{ // First priority: try the game-specific sounds folder (if not using _default)
			for (const auto& ext : exts)
			{
				std::filesystem::path p = std::filesystem::path(_gameAssetsRoot) / _currentGame / "sounds" / (rName + ext);
				if (std::filesystem::exists(p))
					retVal = p.string();
			}
		}

		if (retVal.empty())
		{ // Second priority: fall back to default sounds dir
			for (const auto& ext : exts)
			{
				std::filesystem::path p = std::filesystem::path(_soundsDir) / (rName + ext);
				if (std::filesystem::exists(p))
					retVal = p.string();
			}
		}

		if (retVal.empty())
		{ // Third priority: try direct file (no extension)
			std::filesystem::path direct = std::filesystem::path(_soundsDir) / rName;
			if (std::filesystem::exists(direct))
				retVal = direct.string();
		}
	}

	return retVal;
}

void AudioEngine::playSfx(const std::string& rName, f32 volume)
{
	if (_soundEnabled && _pImpl->initialized && !rName.empty())
	{
		std::string path = resolveSoundPath(rName);
		if (!path.empty())
		{
			MIX_Audio* pAudio = MIX_LoadAudio(_pImpl->pMixer, path.c_str(), false);
			if (!pAudio)
				std::cerr << "[AudioEngine] Failed to load SFX: " << path << std::endl;
			else
			{
				MIX_Track* pTrack = MIX_CreateTrack(_pImpl->pMixer);
				if (!pTrack)
					std::cerr << "[AudioEngine] Failed to create SFX track for: " << rName << std::endl;
				else
				{
					if (!MIX_SetTrackAudio(pTrack, pAudio))
					{
						std::cerr << "[AudioEngine] Failed to assign audio to SFX track: " << rName << std::endl;
						MIX_DestroyTrack(pTrack);
					}
					else
					{
						MIX_SetTrackGain(pTrack, volume);
						MIX_PlayTrack(pTrack, 0);
					}
				}
			}
		}
		else
			std::cerr << "[AudioEngine] SFX not found: " << rName << std::endl;
	}
}

void AudioEngine::playTheme(const std::string& rName, f32 volume)
{
	if (_musicEnabled && _pImpl->initialized && !rName.empty())
	{
		if (_currentTheme == rName && _pImpl->pThemeTrack)
		{ // Already playing this theme
		}
		else
		{
			stopTheme();

			std::string path = resolveSoundPath(rName);
			if (path.empty())
			{
				std::cerr << "[AudioEngine] Theme not found: " << rName << std::endl;
			}
			else
			{
				MIX_Audio* pAudio = MIX_LoadAudio(_pImpl->pMixer, path.c_str(), false);
				if (!pAudio)
				{
					std::cerr << "[AudioEngine] Failed to load theme: " << path << std::endl;
				}
				else
				{
					MIX_Track* pTrack = MIX_CreateTrack(_pImpl->pMixer);
					if (!pTrack)
					{
						std::cerr << "[AudioEngine] Failed to create theme track for: " << rName << std::endl;
					}
					else
					{
						if (!MIX_SetTrackAudio(pTrack, pAudio))
						{
							std::cerr << "[AudioEngine] Failed to attach theme audio for: " << rName << std::endl;
							MIX_DestroyTrack(pTrack);
						}
						else
						{
							MIX_SetTrackGain(pTrack, volume);
							MIX_SetTrackLoops(pTrack, -1);
							if (MIX_PlayTrack(pTrack, 0))
							{
								_pImpl->pThemeTrack = pTrack;
								_currentTheme = rName;
							}
							else
							{
								MIX_DestroyTrack(pTrack);
							}
						}
					}
				}
			}
		}
	}
}

void AudioEngine::stopTheme()
{
	if (_pImpl->initialized && _pImpl->pThemeTrack)
	{
		MIX_StopTrack(_pImpl->pThemeTrack, 0);
		MIX_DestroyTrack(_pImpl->pThemeTrack);
		_pImpl->pThemeTrack = nullptr;
		_currentTheme.clear();
	}
}

void AudioEngine::toggleSound()
{
	_soundEnabled = !_soundEnabled;
}

void AudioEngine::toggleMusic()
{
	_musicEnabled = !_musicEnabled;
	if (!_musicEnabled)
	{
		stopTheme();
	}
}
