#include "Actor.hpp"
#include "AudioEngine.hpp"
#include "GlyphCache.hpp"
#include "Location.hpp"
#include "World.hpp"
#include "pch.hpp"

Actor::Actor(const std::string& rName, observer_ptr<Location> pLocation, const f32Point& rPos, const f32Size& rSize) :
    name(rName),
    pLocation(pLocation),
    bounds(rPos, rSize)
{
	props.setOrg("name", rName);
	props.setOrg("x", std::to_string(rPos.x));
	props.setOrg("y", std::to_string(rPos.y));
	props.setOrg("width", std::to_string(rSize.width));
	props.setOrg("height", std::to_string(rSize.height));
}

f32Point Actor::getPos() const
{
	return { props.getFloat("x", bounds.x), props.getFloat("y", bounds.y) };
}

void Actor::setPos(const f32Point& rPos)
{
	props.setFloat("x", rPos.x);
	props.setFloat("y", rPos.y);
}

f32Rectangle Actor::getBounds() const
{
	f32Point p = getPos();
	return { p.x - bounds.w * 0.5f, p.y - bounds.h * 0.5f, bounds.w, bounds.h };
}

bool Actor::isHidden() const
{
	bool retVal = false;
	if (props.has("hiddenUnlessActor"))
	{
		std::string required = props.get("hiddenUnlessActor");
		if (pLocation && !pLocation->findActor(required))
			retVal = true;
	}
	return retVal || props.getBool("hidden", false);
}

void Actor::setHidden(bool hidden)
{
	props.setBool("hidden", hidden);
}

bool Actor::isDraggable() const
{
	return props.getBool("item", false) || props.getBool("draggable", false);
}

bool Actor::isDrawBackground() const
{
	return props.getBool("drawBackground", true);
}

std::string Actor::getActiveGlyph() const
{
	std::string retVal(name);

	if (props.getBool("toggled", false))
	{
		if (props.has("glyphToggled"))
			retVal = props.get("glyphToggled");
		else
			retVal = name + "Toggled";
	}

	if (props.has("glyphRevealed") && props.getBool("revealed", false))
		retVal = props.get("glyphRevealed");

	if (props.has("glyph"))
		retVal = props.get("glyph");

	return retVal;
}

static bool iequals(const std::string& a, const std::string& b)
{
	bool retVal = true;
	if (a.size() != b.size())
		retVal = false;
	else
	{
		for (uindex i = 0; (i < a.size()) && retVal; ++i)
			if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i]))
				retVal = false;
	}
	return retVal;
}

std::vector<std::string> Actor::getRevealActors() const
{
	return props.getList("revealActor");
}

std::vector<std::string> Actor::getRevealActorsSync() const
{
	return props.getList("revealActorSync");
}

std::vector<std::string> Actor::getAllRevealTargets() const
{
	std::vector<std::string> all = getRevealActors();
	std::vector<std::string> sync = getRevealActorsSync();
	all.insert(all.end(), sync.begin(), sync.end());
	return all;
}

bool Actor::isRevealer() const
{
	return !getAllRevealTargets().empty();
}

f32 Actor::getRotation() const
{
	return props.getFloat("glyphRotation", 0.0f);
}

bool Actor::getFlipHoriz() const
{
	return props.getBool("glyphFlipHoriz", false);
}

bool Actor::getFlipVert() const
{
	return props.getBool("glyphFlipVert", false);
}

void Actor::onPlace()
{ // 1. Reveal targets update
	std::vector<std::string> allReveals = getAllRevealTargets();
	if (!allReveals.empty() && pLocation)
	{
		float xDelta = props.getFloat("revealDeltaX", 0.0f);
		float yDelta = props.getFloat("revealDeltaY", 0.0f);
		bool revealerHide = props.getBool("revealerHide", false);

		bool anyRevealed = false;

		for (const auto& targetName : allReveals)
		{
			bool isRev = props.getBool(targetName + "Revealed", false);
			if (isRev)
				anyRevealed = true;

			// Check if all revealers in this location targeting targetName have revealed it
			bool isAllRevealed = true;
			const auto& actors = pLocation->getActors();
			for (uindex i = 0; (i < actors.size()) && isAllRevealed; ++i)
			{
				const auto& otherActor = actors[i];
				if (otherActor)
				{
					const auto& otherTargets = otherActor->getAllRevealTargets();
					for (uindex j = 0; (j < otherTargets.size()) && isAllRevealed; ++j)
					{
						const auto& otherTarget = otherTargets[j];
						if (iequals(otherTarget, targetName))
						{
							if (!otherActor->props.getBool(otherTarget + "Revealed", false))
								isAllRevealed = false;
						}
					}
				}
			}

			observer_ptr<Actor> pTarget = pLocation->findActor(targetName);
			if (pTarget)
			{
				observer_ptr<World> world = pLocation->getWorld();
				if (!world || !world->isItemInInventory(pTarget.get()))
					pTarget->setHidden(!isAllRevealed);
			}
		}

		// Apply revealer motion (revealDeltaX / revealDeltaY)
		if (xDelta != 0.0f || yDelta != 0.0f)
		{
			if (anyRevealed)
				setPos({ bounds.x + xDelta, bounds.y + yDelta });
			else
				setPos(bounds.getPosition());
		}

		// Apply revealerHide
		if (revealerHide)
			setHidden(anyRevealed);

		props.setBool("revealed", anyRevealed); // Mark revealer itself as revealed for glyphRevealed
	}

	// 2. Check position offsets from mover (moveDeltaX / moveDeltaY)
	std::vector<std::string> dx = props.getList("moveDeltaX");
	std::vector<std::string> dy = props.getList("moveDeltaY");
	if (!dx.empty() && !dy.empty())
	{
		i32 idx = props.getInt("moveIndex", -1);
		if (idx >= 0 && idx < (i32)dx.size() && idx < (i32)dy.size())
			setPos({ bounds.x + std::stof(dx[idx]), bounds.y + std::stof(dy[idx]) });
		else
			setPos(bounds.getPosition());
	}
}

bool Actor::onTap(i32 numTaps)
{
	bool retVal = false;
	if (!isHidden() && pLocation)
	{
		observer_ptr<World> pWorld = pLocation->getWorld();
		if (pWorld)
		{
			if (!retVal && props.has("relocaterDest"))
			{ // 1. Relocater check (change location within current game)
				bool canRelocate = true;
				if (props.getBool("toggle", false))
					canRelocate = props.getBool("toggled", false);

				if (canRelocate)
				{
					std::string dest = props.get("relocaterDest");
					float time = props.getFloat("relocaterDestTime", 0.75f);
					std::string snd = props.get("soundRelocate");
					if (!snd.empty())
						AudioEngine::instance().playSfx(snd);
					pWorld->changeLocation(dest, time);
					retVal = true;
				}
			}

			if (!retVal)
			{ // 2. Relocater Map check (switch to different game/map)
				if (props.has("relocaterMap"))
				{
					std::string mapName = props.get("relocaterMap");
					float time = props.getFloat("relocaterMapTime", 0.75f);
					std::string snd = props.get("soundRelocateMap");
					if (!snd.empty())
						AudioEngine::instance().playSfx(snd);
					pWorld->switchGame(mapName, time);
					retVal = true;
				}
			}

			if (!retVal)
			{ // 3. Revealer check
				std::vector<std::string> reveals = getRevealActors();
				std::vector<std::string> revealsSync = getRevealActorsSync();
				if (!reveals.empty() || !revealsSync.empty())
				{
					std::string trigger = props.get("revealTrigger");
					bool canReveal = trigger.empty() || props.getBool("revealTriggered", false);

					if (canReveal)
					{
						bool revealLoop = props.getBool("revealLoop", true);
						bool changed = false;

						if (!reveals.empty())
						{ // Non-sync: reveals all listed actors
							for (const auto& targetName : reveals)
							{
								std::string key = targetName + "Revealed";
								bool cur = props.getBool(key, false);
								if (!cur || revealLoop)
								{
									props.flipBool(key, false);
									changed = true;
								}
							}
						}

						// Sync reveals
						if (!revealsSync.empty())
						{
							if (revealLoop)
							{
								bool stateFirst = props.getBool(revealsSync[0] + "Revealed", false);
								for (uindex i = 1; i < revealsSync.size() && !changed; ++i)
								{
									const auto& targetName = revealsSync[i];
									if (props.getBool(targetName + "Revealed", false) != stateFirst)
									{
										props.flipBool(targetName + "Revealed", false);
										changed = true;
									}
								}
								if (!changed)
								{
									props.flipBool(revealsSync[0] + "Revealed", false);
									changed = true;
								}
							}
							else
							{
								for (uindex i = 0; i < revealsSync.size() && !changed; ++i)
								{
									const auto& targetName = revealsSync[i];
									if (!props.getBool(targetName + "Revealed", false))
									{
										props.setBool(targetName + "Revealed", true);
										changed = true;
									}
								}
							}
						}

						if (changed)
						{
							std::string snd = props.get("soundReveal");
							if (!snd.empty())
								AudioEngine::instance().playSfx(snd);
							onPlace();
							retVal = true;
						}
					}
				}
			}

			if (!retVal)
			{ // 3. Mover check
				std::vector<std::string> dx = props.getList("moveDeltaX");
				std::vector<std::string> dy = props.getList("moveDeltaY");
				if (!dx.empty() && !dy.empty())
				{
					i32 maxSteps = std::min((i32)dx.size(), (i32)dy.size());
					i32 curIdx = props.getInt("moveIndex", -1) + 1;
					if (curIdx >= maxSteps)
						props.setInt("moveIndex", -1);
					else
						props.setInt("moveIndex", curIdx);

					std::string snd = props.get("soundMove");
					if (!snd.empty())
						AudioEngine::instance().playSfx(snd);
					onPlace();
					retVal = true;
				}
			}

			if (!retVal)
			{ // 4. Toggle check
				if (props.getBool("toggle", false))
				{
					bool canToggle = true;
					if (props.has("toggleTrigger") && !props.getBool("toggleTriggered", false))
						canToggle = false;

					if (canToggle)
					{
						bool loop = props.getBool("toggleLoop", true);
						if (!props.getBool("toggled", false) || loop)
						{
							props.flipBool("toggled", false);
							std::string snd = props.get("soundToggle");
							if (!snd.empty())
								AudioEngine::instance().playSfx(snd);
							onPlace();
							retVal = true;
						}
					}
				}
			}

			if (!retVal)
			{ // 5. Notes / Dialogue check
				std::string noteSuffix = props.getBool("toggled", false) ? "Toggled" : "";
				std::vector<std::string> notes = props.getList("note" + noteSuffix);
				if (!notes.empty())
				{
					std::string snd = props.get("soundNote" + noteSuffix);
					if (!snd.empty())
						AudioEngine::instance().playSfx(snd);
					pWorld->showNoteBubble(this, notes);
					retVal = true;
				}
			}

			if (!retVal)
			{ // 6. Tap sound fallback
				std::string tapSnd = props.get("soundTap");
				if (!retVal && !tapSnd.empty())
					AudioEngine::instance().playSfx(tapSnd);
			}
		}
	}

	return retVal;
}

bool Actor::onDrop(observer_ptr<Actor> pDroppedActor)
{
	bool retVal = false;
	if (pDroppedActor && (pDroppedActor != this) && pLocation)
	{
		std::string trigger = props.get("revealTrigger");
		if (!trigger.empty() && pDroppedActor->getName() == trigger)
		{
			props.setBool("revealTriggered", true);
			if (!props.getBool("revealTriggerKeep", false))
				pDroppedActor->setHidden(true);
			onTap(1);
			retVal = true;
		}

		if (!retVal)
		{ // Toggle trigger check
			std::vector<std::string> toggleTriggers = props.getList("toggleTrigger");
			for (uindex i = 0; i < toggleTriggers.size(); ++i)
			{
				const auto& trig = toggleTriggers[i];
				if (pDroppedActor->getName() == trig)
				{
					props.setBool("toggleTriggered", true);
					if (!props.getBool("toggleTriggerKeep", false))
						pDroppedActor->setHidden(true);
					onTap(1);
					retVal = true;
				}
			}
		}
	}
	return retVal;
}

void Actor::render(SDL_Renderer* pRenderer, f32 alpha)
{
	if (!isHidden())
	{
		Rectangle bounds = getBounds();
		if (isDrawBackground())
			GlyphCache::instance().renderBlankBackground(bounds, alpha);
		GlyphCache::instance().renderGlyph(getActiveGlyph(), bounds, getRotation(), getFlipHoriz(), getFlipVert(), alpha);
	}
}
