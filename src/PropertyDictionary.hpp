#pragma once

#include "pch.hpp"

class PropertyDictionary {
public:
    PropertyDictionary() = default;

    bool has(const std::string& key) const {
        return m_deltas.find(key) != m_deltas.end() || m_org.find(key) != m_org.end();
    }

    std::string get(const std::string& key, const std::string& fallback = "") const {
        auto itDelta = m_deltas.find(key);
        if (itDelta != m_deltas.end()) {
            return itDelta->second;
        }
        auto itOrg = m_org.find(key);
        if (itOrg != m_org.end()) {
            return itOrg->second;
        }
        return fallback;
    }

    int getInt(const std::string& key, int fallback = 0) const {
        std::string val = get(key);
        if (val.empty()) return fallback;
        try {
            return std::stoi(val);
        } catch (...) {
            return fallback;
        }
    }

    float getFloat(const std::string& key, float fallback = 0.0f) const {
        std::string val = get(key);
        if (val.empty()) return fallback;
        try {
            return std::stof(val);
        } catch (...) {
            return fallback;
        }
    }

    bool getBool(const std::string& key, bool fallback = false) const {
        std::string val = get(key);
        if (val.empty()) return fallback;
        std::string lower = val;
        std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
        if (lower == "true" || lower == "1") return true;
        if (lower == "false" || lower == "0") return false;
        return fallback;
    }

    std::vector<std::string> getList(const std::string& prefix) const {
        std::vector<std::string> list;
        if (!has(prefix)) {
            return list;
        }

        list.push_back(get(prefix));
        int index = 1;
        while (true) {
            std::string nextKey = prefix + std::to_string(index);
            if (has(nextKey)) {
                list.push_back(get(nextKey));
                index++;
            } else {
                break;
            }
        }
        return list;
    }

    void set(const std::string& key, const std::string& value) {
        auto itOrg = m_org.find(key);
        if (itOrg != m_org.end() && itOrg->second == value) {
            m_deltas.erase(key);
        } else {
            m_deltas[key] = value;
        }
    }

    void setInt(const std::string& key, int value) {
        set(key, std::to_string(value));
    }

    void setFloat(const std::string& key, float value) {
        set(key, std::to_string(value));
    }

    void setBool(const std::string& key, bool value) {
        set(key, value ? "true" : "false");
    }

    bool flipBool(const std::string& key, bool fallback = false) {
        bool next = !getBool(key, fallback);
        setBool(key, next);
        return next;
    }

    void remove(const std::string& key) {
        m_deltas.erase(key);
        m_org.erase(key);
    }

    void setOrg(const std::string& key, const std::string& value) {
        m_org[key] = value;
    }

    void revert() {
        m_deltas.clear();
    }

    void commit() {
        for (const auto& [k, v] : m_deltas) {
            m_org[k] = v;
        }
        m_deltas.clear();
    }

    std::vector<std::string> allKeys() const {
        std::set<std::string> keys;
        for (const auto& [k, v] : m_org) keys.insert(k);
        for (const auto& [k, v] : m_deltas) keys.insert(k);
        return std::vector<std::string>(keys.begin(), keys.end());
    }

    const std::unordered_map<std::string, std::string>& getOrg() const { return m_org; }
    const std::unordered_map<std::string, std::string>& getDeltas() const { return m_deltas; }

private:
    std::unordered_map<std::string, std::string> m_org;
    std::unordered_map<std::string, std::string> m_deltas;
};
