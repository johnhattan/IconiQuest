#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class PropertyDictionary {
public:
    PropertyDictionary() = default;

    bool has(const std::string& key) const;
    std::string get(const std::string& key, const std::string& fallback = "") const;
    int getInt(const std::string& key, int fallback = 0) const;
    float getFloat(const std::string& key, float fallback = 0.0f) const;
    bool getBool(const std::string& key, bool fallback = false) const;
    std::vector<std::string> getList(const std::string& prefix) const;

    void set(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setFloat(const std::string& key, float value);
    void setBool(const std::string& key, bool value);
    bool flipBool(const std::string& key, bool fallback = false);
    void remove(const std::string& key);

    void setOrg(const std::string& key, const std::string& value);
    void revert();
    void commit();

    std::vector<std::string> allKeys() const;
    const std::unordered_map<std::string, std::string>& getOrg() const { return m_org; }
    const std::unordered_map<std::string, std::string>& getDeltas() const { return m_deltas; }

private:
    std::unordered_map<std::string, std::string> m_org;
    std::unordered_map<std::string, std::string> m_deltas;
};
