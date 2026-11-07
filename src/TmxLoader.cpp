#include "TmxLoader.hpp"
#include <pugixml.hpp>
#include <iostream>

bool TmxLoader::loadWorld(World& world, const std::string& tmxPath) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(tmxPath.c_str());

    if (!result) {
        std::cerr << "[TmxLoader] Error reading " << tmxPath << ": " << result.description() << std::endl;
        return false;
    }

    pugi::xml_node mapNode = doc.child("map");
    if (!mapNode) {
        std::cerr << "[TmxLoader] Missing <map> tag in " << tmxPath << std::endl;
        return false;
    }

    float tileW = mapNode.attribute("tilewidth").as_float(48.0f);
    float tileH = mapNode.attribute("tileheight").as_float(48.0f);
    world.props().setFloat("tileWidth", tileW);
    world.props().setFloat("tileHeight", tileH);

    // Read map-level properties
    pugi::xml_node mapProps = mapNode.child("properties");
    if (mapProps) {
        for (pugi::xml_node prop : mapProps.children("property")) {
            std::string k = prop.attribute("name").as_string();
            std::string v = prop.attribute("value").as_string();
            world.props().setOrg(k, v);
        }
    }

    // Read objectgroups (Locations)
    for (pugi::xml_node groupNode : mapNode.children("objectgroup")) {
        std::string groupName = groupNode.attribute("name").as_string();
        if (groupName.empty()) continue;

        auto loc = std::make_unique<Location>(groupName, &world);

        // Read location properties
        pugi::xml_node groupProps = groupNode.child("properties");
        if (groupProps) {
            for (pugi::xml_node prop : groupProps.children("property")) {
                std::string k = prop.attribute("name").as_string();
                std::string v = prop.attribute("value").as_string();
                loc->props().setOrg(k, v);
            }
        }

        // Read objects (Actors or Bounds)
        for (pugi::xml_node objNode : groupNode.children("object")) {
            std::string objName = objNode.attribute("name").as_string();
            std::string objType = objNode.attribute("type").as_string();
            float ox = objNode.attribute("x").as_float();
            float oy = objNode.attribute("y").as_float();
            float ow = objNode.attribute("width").as_float(tileW);
            float oh = objNode.attribute("height").as_float(tileH);

            if (objType == "Bounds" || objName == "dimensions") {
                loc->setBounds({ox, oy, ow, oh});
            } else {
                // Tiled tile objects register at bottom-left
                float centerX = ox + ow * 0.5f;
                float centerY = oy - oh * 0.5f;

                auto actor = std::make_unique<Actor>(objName, loc.get(), centerX, centerY, ow, oh);
                actor->props().setOrg("id", std::to_string(objNode.attribute("id").as_int()));
                if (!objType.empty()) {
                    actor->props().setOrg("type", objType);
                }

                // Read object properties
                pugi::xml_node objProps = objNode.child("properties");
                if (objProps) {
                    for (pugi::xml_node prop : objProps.children("property")) {
                        std::string k = prop.attribute("name").as_string();
                        std::string v = prop.attribute("value").as_string();
                        actor->props().setOrg(k, v);
                    }
                }

                loc->addActor(std::move(actor));
            }
        }

        world.addLocation(std::move(loc));
    }

    return true;
}

bool TmxLoader::saveWorld(const World& world, const std::string& tmxPath) {
    pugi::xml_document doc;
    pugi::xml_parse_result result = doc.load_file(tmxPath.c_str());

    pugi::xml_node mapNode = doc.child("map");
    if (!mapNode) {
        mapNode = doc.append_child("map");
        mapNode.append_attribute("version") = "1.0";
        mapNode.append_attribute("orientation") = "orthogonal";
        mapNode.append_attribute("renderorder") = "right-down";
        mapNode.append_attribute("width") = "100";
        mapNode.append_attribute("height") = "100";
        mapNode.append_attribute("tilewidth") = "48";
        mapNode.append_attribute("tileheight") = "48";
    }

    for (const auto& loc : world.getLocations()) {
        if (!loc) continue;

        pugi::xml_node groupNode = mapNode.find_child_by_attribute("objectgroup", "name", loc->getName().c_str());
        if (!groupNode) {
            groupNode = mapNode.append_child("objectgroup");
            groupNode.append_attribute("name") = loc->getName().c_str();
        }

        // Update properties
        pugi::xml_node groupProps = groupNode.child("properties");
        if (!groupProps) groupProps = groupNode.append_child("properties");
        for (const auto& k : loc->props().allKeys()) {
            pugi::xml_node pNode = groupProps.find_child_by_attribute("property", "name", k.c_str());
            if (!pNode) {
                pNode = groupProps.append_child("property");
                pNode.append_attribute("name") = k.c_str();
            }
            pNode.attribute("value") = loc->props().get(k).c_str();
        }

        // Update Bounds
        Rect b = loc->getBounds();
        pugi::xml_node boundsNode = groupNode.find_child_by_attribute("object", "name", "dimensions");
        if (!boundsNode) boundsNode = groupNode.find_child_by_attribute("object", "type", "Bounds");
        if (!boundsNode) {
            boundsNode = groupNode.append_child("object");
            boundsNode.append_attribute("name") = "dimensions";
            boundsNode.append_attribute("type") = "Bounds";
        }
        boundsNode.attribute("x") = b.x;
        boundsNode.attribute("y") = b.y;
        boundsNode.attribute("width") = b.w;
        boundsNode.attribute("height") = b.h;

        // Update Actors
        for (const auto& actor : loc->getActors()) {
            if (!actor) continue;
            pugi::xml_node objNode = groupNode.find_child_by_attribute("object", "name", actor->getName().c_str());
            if (!objNode) {
                objNode = groupNode.append_child("object");
                objNode.append_attribute("name") = actor->getName().c_str();
                objNode.append_attribute("gid") = "1";
            }

            Vec2 p = actor->getPos();
            float w = actor->getWidth();
            float h = actor->getHeight();
            objNode.attribute("x") = p.x - w * 0.5f;
            objNode.attribute("y") = p.y + h * 0.5f;
            objNode.attribute("width") = w;
            objNode.attribute("height") = h;

            pugi::xml_node objProps = objNode.child("properties");
            if (!objProps) objProps = objNode.append_child("properties");
            for (const auto& k : actor->props().allKeys()) {
                if (k == "name" || k == "x" || k == "y" || k == "width" || k == "height" || k == "id") continue;
                pugi::xml_node pNode = objProps.find_child_by_attribute("property", "name", k.c_str());
                if (!pNode) {
                    pNode = objProps.append_child("property");
                    pNode.append_attribute("name") = k.c_str();
                }
                pNode.attribute("value") = actor->props().get(k).c_str();
            }
        }
    }

    return doc.save_file(tmxPath.c_str());
}
