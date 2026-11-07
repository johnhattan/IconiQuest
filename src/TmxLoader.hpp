#pragma once

#include <string>
#include <memory>
#include "World.hpp"

class TmxLoader {
public:
    static bool loadWorld(World& world, const std::string& tmxPath);
    static bool saveWorld(const World& world, const std::string& tmxPath);
};
