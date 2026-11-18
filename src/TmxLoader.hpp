#pragma once
/*
 * TmxLoader
 *
 * TMX import/export interface for IconiQuest map data.  TmxLoader translates
 * pugixml nodes into World, Location, Actor, bounds, and property structures,
 * and serializes runtime changes back to a TMX file when requested.
 *
 * Canonical runtime asset resolution belongs to the application and World;
 * this class operates on the map path and asset-independent XML data supplied
 * by them.
 */

#include "pch.hpp"
#include "World.hpp"

struct TmxLoader
{
	static bool loadWorld(World& rWorld, const std::string& rTmxPath);
	static bool saveWorld(const World& rWorld, const std::string& rTmxPath);
};
