#pragma once
/*
 * Location
 *
 * Represents one room/location in a game world.  Location owns its Actor
 * objects, local property dictionary, bounds, background rendering state,
 * and room-local lifecycle operations.
 *
 * World owns the collection of locations and routes transitions, reversions,
 * and actor placement through this room abstraction.
 */

#include "pch.hpp"
#include "Actor.hpp"
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

inline Location::Location(const std::string& rName, observer_ptr<World> pWorld) :
    name(rName), pWorld(pWorld)
{
}

inline Location::~Location() = default;

inline void Location::addActor(std::unique_ptr<Actor> pActor)
{
	if (pActor)
	{
		pActor->setLocation(this);
		actors.push_back(std::move(pActor));
	}
}

inline observer_ptr<Actor> Location::findActor(const std::string& rName) const
{
	observer_ptr<Actor> retVal;
	if (!rName.empty())
	{
		for (const auto& pActor : actors)
		{
			if (pActor && pActor->getName() == rName)
				retVal = pActor.get();
		}

		if (!retVal)
		{
			for (const auto& pActor : actors)
			{
				if (pActor)
				{
					const std::string& rActorName = pActor->getName();
					bool sizeMatch = rActorName.size() == rName.size();
					bool caseInsensitiveMatch = sizeMatch && std::equal(rActorName.begin(), rActorName.end(), rName.begin(), [](char c1, char c2) {
						return std::tolower((unsigned char)c1) == std::tolower((unsigned char)c2);
					});

					if (caseInsensitiveMatch)
						retVal = pActor.get();
				}
			}
		}
	}
	return retVal;
}

inline std::unique_ptr<Actor> Location::removeActor(observer_ptr<Actor> pTarget)
{
	auto it = std::find_if(actors.begin(), actors.end(),
	    [pTarget](const std::unique_ptr<Actor>& pActor) { return pActor.get() == pTarget.get(); });

	std::unique_ptr<Actor> taken;
	if (it != actors.end())
	{
		taken = std::move(*it);
		actors.erase(it);
		taken->setLocation(nullptr);
	}
	return taken;
}

inline void Location::takeActor(observer_ptr<Actor> pActor)
{
	if (pActor)
	{
		observer_ptr<Location> pOldLoc = pActor->getLocation();
		if (pOldLoc != this)
		{
			if (pOldLoc)
			{
				std::unique_ptr<Actor> pPtr = pOldLoc->removeActor(pActor);
				if (pPtr)
					addActor(std::move(pPtr));
			}
		}
	}
}

inline Color Location::getBackgroundColor() const
{
	std::string rHex = props.get("colorBackground");
	Color retVal = { 15, 15, 20, 255 };
	if (!rHex.empty())
		retVal = Color::fromHex(rHex, { 15, 15, 20, 255 });
	return retVal;
}

inline void Location::revert()
{
	props.revert();
	for (auto& pActor : actors)
		if (pActor)
			pActor->props.revert();
	for (auto& pActor : actors)
		if (pActor)
			pActor->onPlace();
}

inline void Location::render(SDL_Renderer* pRenderer, f32 alpha)
{
	for (auto& pActor : actors)
		if (pActor)
			pActor->render(pRenderer, alpha);
}
