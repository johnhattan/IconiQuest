#pragma once

#include "pch.hpp"

class Location;
class World;

struct Actor
{
	std::string name;
	observer_ptr<Location> pLocation;
	PropertyDictionary props;
	f32Rectangle bounds;

	Actor(const std::string& rName, observer_ptr<Location> pLocation, const f32Point& rPos, const f32Size& rSize);
	Actor(const std::string& rName, observer_ptr<Location> pLocation, const f32Point& rPos) : Actor(rName, pLocation, rPos, f32Size(48.0f, 48.0f)) {}
	Actor(const std::string& rName, observer_ptr<Location> pLocation) : Actor(rName, pLocation, f32Point(0.0f, 0.0f), f32Size(48.0f, 48.0f)) {}

	virtual ~Actor() = default;

	std::string getName() const { return name; }
	void setName(const std::string& rName) { name = rName; }

	observer_ptr<Location> getLocation() const { return pLocation; }
	void setLocation(observer_ptr<Location> pLoc) { pLocation = pLoc; }

	PropertyDictionary& getProps() { return props; }
	const PropertyDictionary& getProps() const { return props; }

	f32Point getPos() const;
	void setPos(const f32Point& rPos);
	f32Point getOrgPos() const { return bounds.getPosition(); }

	f32 getWidth() const { return bounds.w; }
	f32 getHeight() const { return bounds.h; }
	void setSize(f32 w, f32 h)
	{
		bounds.w = w;
		bounds.h = h;
	}

	f32Rectangle getBounds() const;
	bool isHidden() const;
	void setHidden(bool hidden);

	bool isDraggable() const;
	bool isDrawBackground() const;
	std::string getActiveGlyph() const;

	std::vector<std::string> getRevealActors() const;
	std::vector<std::string> getRevealActorsSync() const;
	std::vector<std::string> getAllRevealTargets() const;
	bool isRevealer() const;

	f32 getRotation() const;
	bool getFlipHoriz() const;
	bool getFlipVert() const;

	virtual void onPlace();
	virtual bool onTap(i32 numTaps = 1);
	virtual bool onDrop(observer_ptr<Actor> pDroppedActor);

	void render(SDL_Renderer* pRenderer, f32 alpha = 1.0f);
};
