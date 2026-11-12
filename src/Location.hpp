#pragma once

#include "pch.hpp"

class Actor;
class World;

struct Location
{
	std::string name;
	observer_ptr<World> pWorld;
	PropertyDictionary props;
	f32Rectangle bounds;
	std::vector<std::unique_ptr<Actor>> actors;

	Location(const std::string& rName, observer_ptr<World> pWorld);
	~Location();

	std::string getName() const { return name; }
	observer_ptr<World> getWorld() const { return pWorld; }

	PropertyDictionary& getProps() { return props; }
	const PropertyDictionary& getProps() const { return props; }

	f32Rectangle getBounds() const { return bounds; }
	void setBounds(const f32Rectangle& rBounds) { bounds = rBounds; }

	const std::vector<std::unique_ptr<Actor>>& getActors() const { return actors; }
	std::vector<std::unique_ptr<Actor>>& getActors() { return actors; }

	void addActor(std::unique_ptr<Actor> pActor);
	observer_ptr<Actor> findActor(const std::string& rName) const;
	std::unique_ptr<Actor> removeActor(observer_ptr<Actor> pActor);
	void takeActor(observer_ptr<Actor> pActor);

	Color getBackgroundColor() const;
	void revert();
	void render(SDL_Renderer* pRenderer, f32 alpha = 1.0f);
};
