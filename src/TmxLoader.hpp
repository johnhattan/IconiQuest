#pragma once

#include "World.hpp"
#include "pch.hpp"

struct TmxLoader
{
	static bool loadWorld(World& rWorld, const std::string& rTmxPath);
	static bool saveWorld(const World& rWorld, const std::string& rTmxPath);
};
