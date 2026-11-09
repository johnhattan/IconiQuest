#pragma once

#include <string>
#include <vector>
#include <memory>
#include "pch.hxx"
#include "PropertyDictionary.hpp"

class Actor;
class World;

class Location {
public:
    Location(const std::string& name, World* world);
    ~Location();

    std::string getName() const { return m_name; }
    World* getWorld() const { return m_world; }

    PropertyDictionary& props() { return m_props; }
    const PropertyDictionary& props() const { return m_props; }

    Rect getBounds() const { return m_bounds; }
    void setBounds(const Rect& bounds) { m_bounds = bounds; }

    const std::vector<std::unique_ptr<Actor>>& getActors() const { return m_actors; }
    std::vector<std::unique_ptr<Actor>>& getActors() { return m_actors; }

    void addActor(std::unique_ptr<Actor> actor);
    Actor* findActor(const std::string& name) const;
    std::unique_ptr<Actor> removeActor(Actor* actor);
    void takeActor(Actor* actor);

    ColorRGBA getBackgroundColor() const;
    void revert();
    void render(SDL_Renderer* renderer, float alpha = 1.0f);

private:
    std::string m_name;
    World* m_world = nullptr;
    PropertyDictionary m_props;
    Rect m_bounds;
    std::vector<std::unique_ptr<Actor>> m_actors;
};
