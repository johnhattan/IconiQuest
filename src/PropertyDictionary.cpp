#include "PropertyDictionary.hpp"
#include <algorithm>
#include <set>

bool PropertyDictionary::has(const std::string& key) const {
    return m_deltas.find(key) != m_deltas.end() || m_org.find(key) != m_org.end();
}

std::string PropertyDictionary::get(const std::string& key, const std::string& fallback) const {
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

int PropertyDictionary::getInt(const std::string& key, int fallback) const {
    std::string val = get(key);
    if (val.empty()) return fallback;
    try {
        return std::stoi(val);
    } catch (...) {
        return fallback;
    }
}

float PropertyDictionary::getFloat(const std::string& key, float fallback) const {
    std::string val = get(key);
    if (val.empty()) return fallback;
    try {
        return std::stof(val);
    } catch (...) {
        return fallback;
    }
}

bool PropertyDictionary::getBool(const std::string& key, bool fallback) const {
    std::string val = get(key);
    if (val.empty()) return fallback;
    std::string lower = val;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    if (lower == "true" || lower == "1") return true;
    if (lower == "false" || lower == "0") return false;
    return fallback;
}

std::vector<std::string> PropertyDictionary::getList(const std::string& prefix) const {
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

void PropertyDictionary::set(const std::string& key, const std::string& value) {
    auto itOrg = m_org.find(key);
    if (itOrg != m_org.end() && itOrg->second == value) {
        m_deltas.erase(key);
    } else {
        m_deltas[key] = value;
    }
}

void PropertyDictionary::setInt(const std::string& key, int value) {
    set(key, std::to_string(value));
}

void PropertyDictionary::setFloat(const std::string& key, float value) {
    set(key, std::to_string(value));
}

void PropertyDictionary::setBool(const std::string& key, bool value) {
    set(key, value ? "true" : "false");
}

bool PropertyDictionary::flipBool(const std::string& key, bool fallback) {
    bool next = !getBool(key, fallback);
    setBool(key, next);
    return next;
}

void PropertyDictionary::remove(const std::string& key) {
    m_deltas.erase(key);
    m_org.erase(key);
}

void PropertyDictionary::setOrg(const std::string& key, const std::string& value) {
    m_org[key] = value;
}

void PropertyDictionary::revert() {
    m_deltas.clear();
}

void PropertyDictionary::commit() {
    for (const auto& [k, v] : m_deltas) {
        m_org[k] = v;
    }
    m_deltas.clear();
}

std::vector<std::string> PropertyDictionary::allKeys() const {
    std::set<std::string> keys;
    for (const auto& [k, v] : m_org) keys.insert(k);
    for (const auto& [k, v] : m_deltas) keys.insert(k);
    return std::vector<std::string>(keys.begin(), keys.end());
}
