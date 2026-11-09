#pragma once

#include <string>
#include <vector>
#include "pch.hxx"
#include "PropertyDictionary.hpp"

class Location;
class World;

class Actor {
public:
    Actor(
        const std::string& name,
        Location* location,
        float x = 0.0f,
        float y = 0.0f,
        float width = 48.0f,
        float height = 48.0f
    );
    virtual ~Actor() = default;

    std::string getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    Location* getLocation() const { return m_location; }
    void setLocation(Location* loc) { m_location = loc; }

    PropertyDictionary& props() { return m_props; }
    const PropertyDictionary& props() const { return m_props; }

    Vec2 getPos() const;
    void setPos(const Vec2& pos);
    Vec2 getOrgPos() const { return m_orgPos; }

    float getWidth() const { return m_width; }
    float getHeight() const { return m_height; }
    void setSize(float w, float h) { m_width = w; m_height = h; }

    f32Rect getBounds() const;
    bool isHidden() const;
    void setHidden(bool hidden);

    bool isDraggable() const;
    bool isDrawBackground() const;
    std::string getActiveGlyph() const;

    std::vector<std::string> getRevealActors() const;
    std::vector<std::string> getRevealActorsSync() const;
    std::vector<std::string> getAllRevealTargets() const;
    bool isRevealer() const;

    float getRotation() const;
    bool getFlipHoriz() const;
    bool getFlipVert() const;

    virtual void onPlace();
    virtual bool onTap(int numTaps = 1);
    virtual bool onDrop(Actor* droppedActor);

    void render(SDL_Renderer* renderer, float alpha = 1.0f);

protected:
    std::string m_name;
    Location* m_location = nullptr;
    PropertyDictionary m_props;

    Vec2 m_orgPos;
    float m_width = 48.0f;
    float m_height = 48.0f;
};
