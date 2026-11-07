#pragma once

#include <string>
#include <unordered_map>
#include <memory>

class AudioEngine {
public:
    static AudioEngine& instance();

    bool init(const std::string& soundsDir);
    void shutdown();

    void setSoundsDir(const std::string& dir);
    void playSfx(const std::string& name, float volume = 1.0f);
    void playTheme(const std::string& name, float volume = 0.6f);
    void stopTheme();

    void toggleSound();
    void toggleMusic();

    bool isSoundEnabled() const { return m_soundEnabled; }
    bool isMusicEnabled() const { return m_musicEnabled; }

private:
    AudioEngine();
    ~AudioEngine();

    std::string resolveSoundPath(const std::string& name);

    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::string m_soundsDir;
    std::string m_currentTheme;
    bool m_soundEnabled = true;
    bool m_musicEnabled = true;
};
