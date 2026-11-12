#pragma once

#include "pch.hpp"

struct AudioEngine
{
	static AudioEngine& instance();

	bool init(const std::string& rSoundsDir);
	void shutdown();

	void setSoundsDir(const std::string& rDir);
	void setGameAssetsRoot(const std::string& rRoot);
	void setCurrentGame(const std::string& rGame);
	void playSfx(const std::string& rName, f32 volume = 1.0f);
	void playTheme(const std::string& rName, f32 volume = 0.6f);
	void stopTheme();

	void toggleSound();
	void toggleMusic();

	bool isSoundEnabled() const { return _soundEnabled; }
	bool isMusicEnabled() const { return _musicEnabled; }

private:
	AudioEngine();
	~AudioEngine();

	std::string resolveSoundPath(const std::string& rName);

	struct Impl;
	std::unique_ptr<Impl> _pImpl;
	std::string _soundsDir;
	std::string _gameAssetsRoot;
	std::string _currentGame = "_default";
	std::string _currentTheme;
	bool _soundEnabled = true;
	bool _musicEnabled = true;
};
