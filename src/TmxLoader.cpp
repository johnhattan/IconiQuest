#include "TmxLoader.hpp"
#include "pch.hpp"
#include <pugixml.hpp>

bool TmxLoader::loadWorld(World& rWorld, const std::string& rTmxPath)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(rTmxPath.c_str());

	bool retVal = false;

	if (!result)
		std::cerr << "[TmxLoader] Error reading " << rTmxPath << ": " << result.description() << std::endl;
	else
	{
		pugi::xml_node mapNode = doc.child("map");
		if (!mapNode)
			std::cerr << "[TmxLoader] Missing <map> tag in " << rTmxPath << std::endl;
		else
		{
			f32 tileW = mapNode.attribute("tilewidth").as_float(48.0f);
			f32 tileH = mapNode.attribute("tileheight").as_float(48.0f);
			rWorld.props.setFloat("tileWidth", tileW);
			rWorld.props.setFloat("tileHeight", tileH);

			// Read map-level properties
			pugi::xml_node mapProps = mapNode.child("properties");
			if (mapProps)
			{
				for (pugi::xml_node prop : mapProps.children("property"))
				{
					std::string k = prop.attribute("name").as_string();
					std::string v = prop.attribute("value").as_string();
					rWorld.props.setOrg(k, v);
				}
			}

			// Read objectgroups (Locations)
			for (pugi::xml_node groupNode : mapNode.children("objectgroup"))
			{
				std::string groupName = groupNode.attribute("name").as_string();
				if (!groupName.empty())
				{
					auto loc = std::make_unique<Location>(groupName, &rWorld);

					// Read location properties
					pugi::xml_node groupProps = groupNode.child("properties");
					if (groupProps)
					{
						for (pugi::xml_node prop : groupProps.children("property"))
						{
							std::string k = prop.attribute("name").as_string();
							std::string v = prop.attribute("value").as_string();
							loc->props.setOrg(k, v);
						}
					}

					// Read objects (Actors or Bounds)
					for (pugi::xml_node objNode : groupNode.children("object"))
					{
						std::string objName = objNode.attribute("name").as_string();
						std::string objType = objNode.attribute("type").as_string();
						f32 ox = objNode.attribute("x").as_float();
						f32 oy = objNode.attribute("y").as_float();
						f32 ow = objNode.attribute("width").as_float(tileW);
						f32 oh = objNode.attribute("height").as_float(tileH);

						if (objType == "Bounds" || objName == "dimensions")
							loc->setBounds({ ox, oy, ow, oh });
						else
						{
							// Tiled tile objects register at bottom-left
							f32 centerX = ox + ow * 0.5f;
							f32 centerY = oy - oh * 0.5f;

							auto actor = std::make_unique<Actor>(objName, loc.get(), f32Point(centerX, centerY), f32Size(ow, oh));
							actor->props.setOrg("id", std::to_string(objNode.attribute("id").as_int()));
							if (!objType.empty())
								actor->props.setOrg("type", objType);

							// Read object properties
							pugi::xml_node objProps = objNode.child("properties");
							if (objProps)
							{
								for (pugi::xml_node prop : objProps.children("property"))
								{
									std::string k = prop.attribute("name").as_string();
									std::string v = prop.attribute("value").as_string();
									actor->props.setOrg(k, v);
								}
							}

							loc->addActor(std::move(actor));
						}
					}

					rWorld.addLocation(std::move(loc));
				}
			}

			retVal = true;
		}
	}

	return retVal;
}

bool TmxLoader::saveWorld(const World& rWorld, const std::string& rTmxPath)
{
	pugi::xml_document doc;
	pugi::xml_parse_result result = doc.load_file(rTmxPath.c_str());

	pugi::xml_node mapNode = doc.child("map");
	if (!mapNode)
	{
		mapNode = doc.append_child("map");
		mapNode.append_attribute("version") = "1.0";
		mapNode.append_attribute("orientation") = "orthogonal";
		mapNode.append_attribute("renderorder") = "right-down";
		mapNode.append_attribute("width") = "100";
		mapNode.append_attribute("height") = "100";
		mapNode.append_attribute("tilewidth") = "48";
		mapNode.append_attribute("tileheight") = "48";
	}

	for (const auto& loc : rWorld.getLocations())
	{
		if (!loc)
		{ // Skip null locations
		}
		else
		{
			pugi::xml_node groupNode = mapNode.find_child_by_attribute("objectgroup", "name", loc->getName().c_str());
			if (!groupNode)
			{
				groupNode = mapNode.append_child("objectgroup");
				groupNode.append_attribute("name") = loc->getName().c_str();
			}

			// Update properties
			pugi::xml_node groupProps = groupNode.child("properties");
			if (!groupProps)
				groupProps = groupNode.append_child("properties");
			for (const auto& k : loc->props.allKeys())
			{
				pugi::xml_node pNode = groupProps.find_child_by_attribute("property", "name", k.c_str());
				if (!pNode)
				{
					pNode = groupProps.append_child("property");
					pNode.append_attribute("name") = k.c_str();
				}
				pNode.attribute("value") = loc->props.get(k).c_str();
			}

			// Update Bounds
			f32Rectangle b = loc->getBounds();
			pugi::xml_node boundsNode = groupNode.find_child_by_attribute("object", "name", "dimensions");
			if (!boundsNode)
				boundsNode = groupNode.find_child_by_attribute("object", "type", "Bounds");
			if (!boundsNode)
			{
				boundsNode = groupNode.append_child("object");
				boundsNode.append_attribute("name") = "dimensions";
				boundsNode.append_attribute("type") = "Bounds";
			}
			boundsNode.attribute("x") = b.x;
			boundsNode.attribute("y") = b.y;
			boundsNode.attribute("width") = b.w;
			boundsNode.attribute("height") = b.h;

			// Update Actors
			for (const auto& actor : loc->getActors())
			{
				if (!actor)
				{ // Skip null actors
				}
				else
				{
					pugi::xml_node objNode = groupNode.find_child_by_attribute("object", "name", actor->getName().c_str());
					if (!objNode)
					{
						objNode = groupNode.append_child("object");
						objNode.append_attribute("name") = actor->getName().c_str();
						objNode.append_attribute("gid") = "1";
					}

					f32Point p = actor->getPos();
					f32 w = actor->getWidth();
					f32 h = actor->getHeight();
					objNode.attribute("x") = p.x - w * 0.5f;
					objNode.attribute("y") = p.y + h * 0.5f;
					objNode.attribute("width") = w;
					objNode.attribute("height") = h;

					pugi::xml_node objProps = objNode.child("properties");
					if (!objProps)
						objProps = objNode.append_child("properties");
					for (const auto& k : actor->props.allKeys())
					{
						if (k == "name" || k == "x" || k == "y" || k == "width" || k == "height" || k == "id")
						{ // Skip reserved properties
						}
						else
						{
							pugi::xml_node pNode = objProps.find_child_by_attribute("property", "name", k.c_str());
							if (!pNode)
							{
								pNode = objProps.append_child("property");
								pNode.append_attribute("name") = k.c_str();
							}
							pNode.attribute("value") = actor->props.get(k).c_str();
						}
					}
				}
			}
		}
	}

	return doc.save_file(rTmxPath.c_str());
}
