#include "pch.hpp"
#include "IqXmlLoader.hpp"
#include <pugixml.hpp>

namespace
{
	std::string trim(std::string value)
	{
		auto itStart = std::find_if_not(value.begin(), value.end(), [](unsigned char ch) { return std::isspace(ch) != 0; });
		auto itEnd = std::find_if_not(value.rbegin(), value.rend(), [](unsigned char ch) { return std::isspace(ch) != 0; }).base();
		if (itStart >= itEnd)
			return "";
		return std::string(itStart, itEnd);
	}

	std::vector<std::string> splitActions(const std::string& rHandler)
	{
		std::vector<std::string> retVal;
		std::stringstream ss(rHandler);
		std::string segment;
		while (std::getline(ss, segment, ';'))
		{
			segment = trim(segment);
			if (!segment.empty())
				retVal.push_back(segment);
		}
		return retVal;
	}

	std::string stripGlyphExtension(const std::string& rGlyph)
	{
		std::filesystem::path glyphPath(rGlyph);
		bool hasExt = glyphPath.has_extension();
		if (!hasExt)
			return rGlyph;
		std::string ext = glyphPath.extension().string();
		if (ext == ".svg" || ext == ".png")
			return glyphPath.stem().string();
		return rGlyph;
	}

	void parseTapHandler(const std::string& rHandler, PropertyDictionary& rProps)
	{
		std::vector<std::string> revealTargets;
		std::vector<std::string> syncTargets;

		const auto& actions = splitActions(rHandler);
		for (const auto& rAction : actions)
		{
			std::stringstream actionStream(rAction);
			std::string command;
			actionStream >> command;

			if (command == "reveal")
			{
				std::string target;
				actionStream >> target;
				if (!target.empty())
					revealTargets.push_back(target);
			}
			else if (command == "go")
			{
				std::string destination;
				actionStream >> destination;
				if (!destination.empty())
					rProps.setOrg("relocaterDest", destination);
			}
			else if (command == "toggle")
			{
				rProps.setOrg("toggle", "true");
			}
			else if (command == "reveal_sync")
			{
				std::string target;
				actionStream >> target;
				if (!target.empty())
					syncTargets.push_back(target);
			}
		}

		if (!revealTargets.empty())
		{
			std::string value;
			for (uindex i = 0; i < revealTargets.size(); ++i)
			{
				if (i != 0)
					value += ",";
				value += revealTargets[i];
			}
			rProps.setOrg("revealActor", value);
		}

		if (!syncTargets.empty())
		{
			std::string value;
			for (uindex i = 0; i < syncTargets.size(); ++i)
			{
				if (i != 0)
					value += ",";
				value += syncTargets[i];
			}
			rProps.setOrg("revealActorSync", value);
		}
	}

	void parseDropHandler(const std::string& rHandler, PropertyDictionary& rProps)
	{
		const auto& actions = splitActions(rHandler);
		for (const auto& rAction : actions)
		{
			std::stringstream actionStream(rAction);
			std::string command;
			actionStream >> command;

			if (command == "place")
			{
				std::string triggerItem;
				std::string onKeyword;
				std::string target;
				actionStream >> triggerItem >> onKeyword >> target;
				if (!triggerItem.empty() && (onKeyword == "on" || onKeyword == "and"))
				{
					rProps.setOrg("toggleTrigger", triggerItem);
					rProps.setOrg("toggleTriggerKeep", "true");
				}
			}
			else if (command == "toggle")
			{
				rProps.setOrg("toggle", "true");
			}
		}
	}

	void copyNodeAttributes(pugi::xml_node node, PropertyDictionary& rProps, const std::set<std::string>& rReserved)
	{
		for (const auto& attr : node.attributes())
		{
			std::string key = attr.name();
			if (rReserved.find(key) == rReserved.end())
				rProps.setOrg(key, attr.value());
		}
	}

	void parseEntityRecursive(
	    pugi::xml_node node,
	    observer_ptr<Location> pLocation,
	    const f32Point& rParentPos,
	    const std::string& rContainerName)
	{
		const std::string nodeType = node.name();
		bool isEntityType = nodeType == "item" || nodeType == "glyph" || nodeType == "entity";
		if (!isEntityType || !pLocation)
			return;

		std::string name = node.attribute("name").as_string();
		if (name.empty())
			return;

		f32 relX = node.attribute("x").as_float(0.0f);
		f32 relY = node.attribute("y").as_float(0.0f);
		f32 width = node.attribute("w").as_float(node.attribute("width").as_float(48.0f));
		f32 height = node.attribute("h").as_float(node.attribute("height").as_float(48.0f));
		f32Point absolutePos = rParentPos + f32Point(relX, relY);

		auto pActor = std::make_unique<Actor>(name, pLocation, absolutePos, f32Size(width, height));
		pActor->props.setOrg("container", rContainerName);
		pActor->props.setOrg("location", pLocation->getName());
		pActor->props.setOrg("xRel", std::to_string(relX));
		pActor->props.setOrg("yRel", std::to_string(relY));
		pActor->props.setOrg("nodeType", nodeType);

		if (nodeType == "item")
		{
			pActor->props.setOrg("item", "true");
			pActor->props.setOrg("draggable", "true");
		}

		if (node.attribute("hidden"))
			pActor->setHidden(node.attribute("hidden").as_bool(false));

		if (node.attribute("glyph"))
			pActor->props.setOrg("glyph", stripGlyphExtension(node.attribute("glyph").as_string()));

		copyNodeAttributes(node, pActor->props, {
			"name", "x", "y", "w", "h", "width", "height", "hidden", "glyph", "onTap", "onDrop"
		});

		if (node.attribute("onTap"))
			parseTapHandler(node.attribute("onTap").as_string(), pActor->props);
		if (node.attribute("onDrop"))
			parseDropHandler(node.attribute("onDrop").as_string(), pActor->props);

		const std::string containerName = name;
		pLocation->addActor(std::move(pActor));

		for (auto child : node.children())
			parseEntityRecursive(child, pLocation, absolutePos, containerName);
	}

	f32Rectangle deriveBoundsFromActors(const Location& rLocation, const f32Point& rLocationOrigin)
	{
		bool hasActor = false;
		f32Rectangle bounds;
		for (const auto& pActor : rLocation.getActors())
		{
			if (pActor)
			{
				f32Rectangle actorBounds = pActor->getBounds();
				if (!hasActor)
				{
					bounds = actorBounds;
					hasActor = true;
				}
				else
					bounds = bounds.Union(actorBounds);
			}
		}

		if (!hasActor)
			bounds = f32Rectangle(rLocationOrigin.x - 400.0f, rLocationOrigin.y - 300.0f, 800.0f, 600.0f);
		else
		{
			f32 padding = 120.0f;
			bounds.x -= padding;
			bounds.y -= padding;
			bounds.w += padding * 2.0f;
			bounds.h += padding * 2.0f;
		}

		return bounds;
	}

	void parseLocationNode(World& rWorld, pugi::xml_node locationNode)
	{
		const std::string name = locationNode.attribute("name").as_string();
		if (name.empty())
			return;

		auto pLocation = std::make_unique<Location>(name, &rWorld);

		f32 locationX = locationNode.attribute("x").as_float(0.0f);
		f32 locationY = locationNode.attribute("y").as_float(0.0f);
		f32 locationW = locationNode.attribute("w").as_float(locationNode.attribute("width").as_float(0.0f));
		f32 locationH = locationNode.attribute("h").as_float(locationNode.attribute("height").as_float(0.0f));

		copyNodeAttributes(locationNode, pLocation->props, {
			"name", "x", "y", "w", "h", "width", "height", "startup"
		});

		pLocation->props.setOrg("x", std::to_string(locationX));
		pLocation->props.setOrg("y", std::to_string(locationY));
		if (locationNode.attribute("startup").as_bool(false))
			pLocation->props.setOrg("locationStart", "true");

		const f32Point locationOrigin(locationX, locationY);
		for (auto child : locationNode.children())
			parseEntityRecursive(child, pLocation.get(), locationOrigin, name);

		if (locationW > 0.0f && locationH > 0.0f)
			pLocation->setBounds(f32Rectangle(locationX, locationY, locationW, locationH));
		else
			pLocation->setBounds(deriveBoundsFromActors(*pLocation, locationOrigin));

		rWorld.addLocation(std::move(pLocation));
	}
} // namespace

bool IqXmlLoader::loadWorld(World& rWorld, const std::string& rXmlPath)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(rXmlPath.c_str());

	bool retVal = false;
	if (!result)
		std::cerr << "[IqXmlLoader] Error reading " << rXmlPath << ": " << result.description() << std::endl;
	else
	{
		pugi::xml_node gameNode = doc.child("game");
		if (!gameNode)
			std::cerr << "[IqXmlLoader] Missing <game> root in " << rXmlPath << std::endl;
		else
		{
			pugi::xml_node worldNode = gameNode.child("world");
			if (worldNode)
			{
				for (auto locationNode : worldNode.children())
				{
					std::string nodeName = locationNode.name();
					if (nodeName == "location" || nodeName == "room")
						parseLocationNode(rWorld, locationNode);
				}
			}
			else
			{
				for (auto roomNode : gameNode.children("room"))
					parseLocationNode(rWorld, roomNode);
			}

			retVal = !rWorld.getLocations().empty();
			if (!retVal)
				std::cerr << "[IqXmlLoader] No <location> or <room> nodes found in " << rXmlPath << std::endl;
		}
	}

	return retVal;
}

bool IqXmlLoader::saveWorld(const World& rWorld, const std::string& rXmlPath)
{
	(void)rWorld;
	(void)rXmlPath;
	std::cerr << "[IqXmlLoader] Save is not implemented for the new XML format yet." << std::endl;
	return false;
}
