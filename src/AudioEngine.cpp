#include "pch.hpp"
#include "AudioEngine.hpp"

#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>

struct AudioEngine::Impl {
    MIX_Mixer* mixer = nullptr;
    MIX_Track* themeTrack = nullptr;
    bool initialized = false;
};

AudioEngine& AudioEngine::instance() {
    static AudioEngine inst;
    return inst;
}

AudioEngine::AudioEngine() : m_impl(std::make_unique<Impl>()) {}

AudioEngine::~AudioEngine() {
    shutdown();
}

bool AudioEngine::init(const std::string& soundsDir) {
    m_soundsDir = soundsDir;
    if (m_impl->initialized) {
        return true;
    }

    if (!MIX_Init()) {
        std::cerr << "[AudioEngine] Failed to initialize SDL_mixer." << std::endl;
        return false;
    }

    SDL_AudioSpec spec{};
    spec.freq = 44100;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;

    m_impl->mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (!m_impl->mixer) {
        std::cerr << "[AudioEngine] Failed to create SDL_mixer device: " << SDL_GetError() << std::endl;
        MIX_Quit();
        return false;
    }

    m_impl->initialized = true;
    return true;
}

void AudioEngine::shutdown() {
    if (m_impl && m_impl->initialized) {
        stopTheme();
        if (m_impl->mixer) {
            MIX_DestroyMixer(m_impl->mixer);
            m_impl->mixer = nullptr;
        }
        MIX_Quit();
        m_impl->initialized = false;
    }
}

void AudioEngine::setSoundsDir(const std::string& dir) {
	m_soundsDir = dir;
}

void AudioEngine::setGameAssetsRoot(const std::string& root) {
	m_gameAssetsRoot = root;
}

void AudioEngine::setCurrentGame(const std::string& game) {
	m_currentGame = game;
	// Stop the current theme when switching games
	stopTheme();
}

std::string AudioEngine::resolveSoundPath(const std::string& name) {
	if (name.empty()) return "";

	std::vector<std::string> exts = {".ogg", ".mp3", ".wav", ".flac", ".mod"};
	
	// First priority: try the game-specific sounds folder (if not using _default)
	if (!m_gameAssetsRoot.empty() && !m_currentGame.empty() && m_currentGame != "_default") {
		for (const auto& ext : exts) {
			std::filesystem::path p = std::filesystem::path(m_gameAssetsRoot) / m_currentGame / "sounds" / (name + ext);
			if (std::filesystem::exists(p)) {
				return p.string();
			}
		}
	}

	// Second priority: fall back to default sounds dir
	for (const auto& ext : exts) {
		std::filesystem::path p = std::filesystem::path(m_soundsDir) / (name + ext);
		if (std::filesystem::exists(p)) {
			return p.string();
		}
	}

	// Third priority: try direct file (no extension)
	std::filesystem::path direct = std::filesystem::path(m_soundsDir) / name;
	if (std::filesystem::exists(direct)) {
		return direct.string();
	}

	return "";
}

void AudioEngine::playSfx(const std::string& name, float volume) {
    if (!m_soundEnabled || !m_impl->initialized || name.empty()) return;

    std::string path = resolveSoundPath(name);
    if (path.empty()) {
        std::cerr << "[AudioEngine] SFX not found: " << name << std::endl;
        return;
    }

    MIX_Audio* audio = MIX_LoadAudio(m_impl->mixer, path.c_str(), false);
    if (!audio) {
        std::cerr << "[AudioEngine] Failed to load SFX: " << path << std::endl;
        return;
    }

    MIX_Track* track = MIX_CreateTrack(m_impl->mixer);
    if (!track) {
        std::cerr << "[AudioEngine] Failed to create SFX track for: " << name << std::endl;
        return;
    }

    if (!MIX_SetTrackAudio(track, audio)) {
        std::cerr << "[AudioEngine] Failed to assign audio to SFX track: " << name << std::endl;
        MIX_DestroyTrack(track);
        return;
    }

    MIX_SetTrackGain(track, volume);
    MIX_PlayTrack(track, 0);
}

void AudioEngine::playTheme(const std::string& name, float volume) {
    if (!m_musicEnabled || !m_impl->initialized || name.empty()) return;
    if (m_currentTheme == name && m_impl->themeTrack) return;

    stopTheme();

    std::string path = resolveSoundPath(name);
    if (path.empty()) {
        std::cerr << "[AudioEngine] Theme not found: " << name << std::endl;
        return;
    }

    MIX_Audio* audio = MIX_LoadAudio(m_impl->mixer, path.c_str(), false);
    if (!audio) {
        std::cerr << "[AudioEngine] Failed to load theme: " << path << std::endl;
        return;
    }

    MIX_Track* track = MIX_CreateTrack(m_impl->mixer);
    if (!track) {
        std::cerr << "[AudioEngine] Failed to create theme track for: " << name << std::endl;
        return;
    }

    if (!MIX_SetTrackAudio(track, audio)) {
        std::cerr << "[AudioEngine] Failed to attach theme audio for: " << name << std::endl;
        MIX_DestroyTrack(track);
        return;
    }

    MIX_SetTrackGain(track, volume);
    MIX_SetTrackLoops(track, -1);
    if (MIX_PlayTrack(track, 0)) {
        m_impl->themeTrack = track;
        m_currentTheme = name;
    } else {
        MIX_DestroyTrack(track);
    }
}

void AudioEngine::stopTheme() {
    if (m_impl->initialized && m_impl->themeTrack) {
        MIX_StopTrack(m_impl->themeTrack, 0);
        MIX_DestroyTrack(m_impl->themeTrack);
        m_impl->themeTrack = nullptr;
        m_currentTheme.clear();
    }
}

void AudioEngine::toggleSound() {
    m_soundEnabled = !m_soundEnabled;
}

void AudioEngine::toggleMusic() {
    m_musicEnabled = !m_musicEnabled;
    if (!m_musicEnabled) {
        stopTheme();
    }
}
