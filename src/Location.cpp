#include "Location.hpp"
#include "Actor.hpp"
#include "World.hpp"
#include "pch.hpp"

Location::Location(const std::string& rName, observer_ptr<World> pWorld) :
    name(rName), pWorld(pWorld)
{
}

Location::~Location() = default;

void Location::addActor(std::unique_ptr<Actor> pActor)
{
	if (pActor)
	{
		pActor->setLocation(this);
		actors.push_back(std::move(pActor));
	}
}

observer_ptr<Actor> Location::findActor(const std::string& rName) const
{
	observer_ptr<Actor> retVal;
	bool nameEmpty = rName.empty();

	if (!nameEmpty)
	{ // Try exact match first
		for (const auto& pActor : actors)
			if (pActor && pActor->getName() == rName)
				retVal = pActor.get();

		if (!retVal)
		{ // Case-insensitive fallback if no exact match
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

std::unique_ptr<Actor> Location::removeActor(observer_ptr<Actor> pTarget)
{
	auto it = std::find_if(actors.begin(), actors.end(),
	    [pTarget](const std::unique_ptr<Actor>& pActor) { return pActor.get() == pTarget.get(); });

	std::unique_ptr<Actor> taken;
	bool found = it != actors.end();
	if (found)
	{
		taken = std::move(*it);
		actors.erase(it);
		taken->setLocation(nullptr);
	}

	return taken;
}

void Location::takeActor(observer_ptr<Actor> pActor)
{
	if (pActor)
	{
		observer_ptr<Location> pOldLoc = pActor->getLocation();
		bool differentLocation = pOldLoc != this;
		if (differentLocation)
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

Color Location::getBackgroundColor() const
{
	std::string rHex = props.get("colorBackground");
	Color retVal = { 15, 15, 20, 255 };

	if (!rHex.empty())
		retVal = Color::fromHex(rHex, { 15, 15, 20, 255 });

	return retVal;
}

void Location::revert()
{
	props.revert();

	// Two-pass revert so resetting one actor's deltas does not wipe another's onPlace state
	for (auto& pActor : actors)
		if (pActor)
			pActor->props.revert();

	for (auto& pActor : actors)
		if (pActor)
			pActor->onPlace();
}

void Location::render(SDL_Renderer* pRenderer, f32 alpha)
{
	for (auto& pActor : actors)
		if (pActor)
			pActor->render(pRenderer, alpha);
}
