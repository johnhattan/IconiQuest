#pragma once
/*
 * IqXmlLoader
 *
 * IconiQuest XML import/export interface for game data. IqXmlLoader reads
 * <game>/<world>/<location> hierarchies (with nested entities) into World,
 * Location, Actor, bounds, and property structures.
 */

#include "pch.hpp"
#include "World.hpp"

struct IqXmlLoader
{
	static bool loadWorld(World& rWorld, const std::string& rXmlPath);
	static bool saveWorld(const World& rWorld, const std::string& rXmlPath);
};
