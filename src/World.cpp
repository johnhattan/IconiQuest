#include "World.hpp"
#include "AudioEngine.hpp"
#include "FontEngine.hpp"
#include "GlyphCache.hpp"
#include "TmxLoader.hpp"
#include "pch.hpp"

World::World() : camera(800.0f, 600.0f), settingsPod(PodCorner::BottomLeft, 200.0f, 150.0f, "cog"), inventoryPod(PodCorner::TopRight, Pod::calculateInventoryWidth(4), Pod::calculateInventoryHeight(3), "backpack") 
{
	inventoryPod.pInventoryItems = observer_ptr<std::vector<observer_ptr<Actor>>>(&inventoryItems);
	inventoryPod.inventoryCols = 4;
	inventoryPod.inventoryRows = 3;
}

World::~World() = default;

bool World::load(const std::string& rTmxPath, const std::string& rAssetsDir)
{
	this->tmxPath = rTmxPath;
	this->assetsDir = rAssetsDir;

	locations.clear();
	currentLocation = nullptr;
	startLocation = nullptr;
	actorBeingDragged = nullptr;
	noteAnchorActor = nullptr;
	inventoryItems.clear();

	// Set audio path only (glyph cache is managed by GlyphCache::setCurrentGame())
	AudioEngine::instance().setSoundsDir(assetsDir + "/_default/sounds");

	bool retVal = true;
	if (!TmxLoader::loadWorld(*this, rTmxPath))
	{
		std::cerr << "[World] Failed to load " << rTmxPath << std::endl;
		retVal = false;
	}

	if (retVal)
	{
		// Determine start location
		for (uindex i = 0; (i < locations.size()) && !startLocation; ++i)
		{
			const auto& loc = locations[i];
			if (loc->props.getBool("locationStart", false))
				startLocation = loc.get();
		}

		if (!startLocation)
			startLocation = findLocation("part1");

		if (!startLocation)
			startLocation = findLocation("0-0");

		if (!startLocation && !locations.empty())
			startLocation = locations.front().get();

		currentLocation = startLocation;
		if (currentLocation)
		{
			camera.instantFrame(currentLocation->getBounds());

			// Play initial sounds
			std::string theme = currentLocation->props.get("soundThemeEnter");
			if (theme.empty())
				theme = currentLocation->props.get("sndThemeEnter");
			if (!theme.empty())
				AudioEngine::instance().playTheme(theme);

			std::string enterSnd = currentLocation->props.get("soundEnter");
			if (!enterSnd.empty())
				AudioEngine::instance().playSfx(enterSnd);
		}

		// Trigger onPlace on all actors to set initial states
		for (const auto& loc : locations)
		{
			for (const auto& a : loc->getActors())
				if (a)
					a->onPlace();
		}
	}

	return retVal;
}

bool World::save()
{
	bool retVal = false;
	if (!tmxPath.empty())
		retVal = TmxLoader::saveWorld(*this, tmxPath);
	return retVal;
}

void World::addLocation(std::unique_ptr<Location> pLoc)
{
	if (pLoc)
	{
		locations.push_back(std::move(pLoc));
	}
}

observer_ptr<Location> World::findLocation(const std::string& rName) const
{
	observer_ptr<Location> retVal = nullptr;
	for (uindex i = 0; (i < locations.size()) && !retVal; ++i)
	{
		const auto& l = locations[i];
		if (l && l->getName() == rName)
			retVal = l.get();
	}
	return retVal;
}

void World::changeLocation(const std::string& rName, f32 transitionTime, const std::string& rEasing)
{
	observer_ptr<Location> loc = findLocation(rName);
	if (loc && loc != currentLocation)
	{
		currentLocation = loc;
		camera.frameRect(loc->getBounds(), transitionTime, rEasing);
		hideNoteBubble();

		// Play room sound and theme
		std::string theme = loc->props.get("soundThemeEnter");
		if (theme.empty())
			theme = loc->props.get("sndThemeEnter");
		if (!theme.empty())
			AudioEngine::instance().playTheme(theme);

		std::string enterSnd = loc->props.get("soundEnter");
		if (!enterSnd.empty())
			AudioEngine::instance().playSfx(enterSnd);

		for (const auto& a : loc->getActors())
			if (a)
				a->onPlace();
	}
}

void World::switchGame(const std::string& rGameName, f32 transitionTime, const std::string& rEasing)
{ // Resolve the map path for the new game
	std::string mapFileName = "_map.tmx";
	std::filesystem::path newMapPath;

	if (rGameName.empty() || rGameName == "_default")
		newMapPath = std::filesystem::path(gameAssetsRoot) / "_default" / mapFileName;
	else
		newMapPath = std::filesystem::path(gameAssetsRoot) / rGameName / mapFileName;

	// Clear current game state
	locations.clear();
	currentLocation = nullptr;
	startLocation = nullptr;
	inventoryItems.clear();
	actorBeingDragged = nullptr;
	hideNoteBubble();

	// Update current game and notify subsystems
	currentGame = rGameName;
	GlyphCache::instance().setCurrentGame(rGameName);
	AudioEngine::instance().setCurrentGame(rGameName);

	// Load the new map
	bool loadSuccess = load(newMapPath.string(), gameAssetsRoot);
	if (!loadSuccess)
		std::cerr << "[World] Failed to load game '" << rGameName << "' from " << newMapPath.string() << std::endl;

	if (loadSuccess)
	{ // Frame the first location with transition
		if (currentLocation)
			camera.frameRect(currentLocation->getBounds(), transitionTime, rEasing);
	}
}

void World::revertWorld()
{
	for (auto& loc : locations)
		if (loc)
			loc->revert();

	inventoryItems.clear();
	actorBeingDragged = nullptr;
	hideNoteBubble();

	if (startLocation)
		changeLocation(startLocation->getName(), 0.0f);
}

void World::revertCurrentLocation()
{
	if (currentLocation)
		currentLocation->revert();

	hideNoteBubble();
}

void World::showNoteBubble(observer_ptr<Actor> pAnchor, const std::vector<std::string>& rNotes, f32 duration)
{
	noteAnchorActor = pAnchor;
	activeNotes = rNotes;
	noteTimer = duration;
}

void World::hideNoteBubble()
{
	noteAnchorActor = nullptr;
	activeNotes.clear();
	noteTimer = 0.0f;
}

bool World::isItemInInventory(const observer_ptr<Actor> pActor) const
{
	bool retVal = false;
	if (pActor)
	{
		for (uindex i = 0; (i < inventoryItems.size()) && !retVal; ++i)
			if (inventoryItems[i] == pActor)
				retVal = true;
	}
	return retVal;
}

bool World::handleMouseDown(const f32Point& rScreenPt)
{
	mouseScreenPos = rScreenPt;
	bool retVal = false;

	// 1. Check Pod clicks
	bool settingsPodHandled = settingsPod.onClick(rScreenPt, camera.getViewportWidth(), camera.getViewportHeight());
	bool inventoryPodHandled = inventoryPod.onClick(rScreenPt, camera.getViewportWidth(), camera.getViewportHeight());

	if (inventoryPodHandled)
	{
		inventoryOpen = inventoryPod.isExpanded;
		AudioEngine::instance().playSfx("sfxDrawer");
		retVal = true;
	}

	if (settingsPodHandled && !inventoryPodHandled)
	{
		AudioEngine::instance().playSfx("sfxDrawer");
		retVal = true;
	}

	if (!retVal && inventoryOpen)
	{
		// 2. Check if clicking inside open inventory tray
		f32 trayX = camera.getViewportWidth() - 320.0f;
		f32 trayY = 70.0f;
		f32Rectangle trayRect(trayX, trayY, 310.0f, 180.0f);
		if (trayRect.contains(rScreenPt))
		{ // Find item in inventory
			for (auto it = inventoryItems.begin(); (it != inventoryItems.end()) && !retVal; ++it)
			{
				observer_ptr<Actor> item = *it;
				f32Rectangle ib(item->getPos().x - item->getWidth() * 0.5f, item->getPos().y - item->getHeight() * 0.5f, item->getWidth(), item->getHeight());
				if (ib.contains(rScreenPt))
				{
					actorBeingDragged = item;
					dragOffset = item->getPos() - rScreenPt;
					inventoryItems.erase(it);
					retVal = true;
				}
			}
			if (!retVal)
				retVal = true; // Click absorbed by tray
		}
		else if (currentLocation && !camera.isTransitioning())
		{ // 3. Check Actors in Current Room
			f32Point worldPt = camera.screenToWorld(rScreenPt);

			observer_ptr<Actor> closestDraggable = nullptr;
			f32 minDist = 99999.0f;

			for (const auto& a : currentLocation->getActors())
			{
				if (a && !a->isHidden() && !isItemInInventory(a.get()) && a->isDraggable())
				{
					f32 dist = a->getPos().distance(worldPt);
					f32 touchRadius = std::max(a->getWidth(), a->getHeight()) * 0.55f;
					if (dist < touchRadius && dist < minDist)
					{
						minDist = dist;
						closestDraggable = a.get();
					}
				}
			}

			if (closestDraggable)
			{
				actorBeingDragged = closestDraggable;
				dragOffset = { 0.0f, 0.0f };
				std::string dragSnd = closestDraggable->props.get("soundItemDrag");
				if (!dragSnd.empty())
					AudioEngine::instance().playSfx(dragSnd);
				retVal = true;
			}
		}
	}
	else if (currentLocation && !camera.isTransitioning())
	{ // 3. Check Actors in Current Room
		f32Point worldPt = camera.screenToWorld(rScreenPt);

		observer_ptr<Actor> closestDraggable = nullptr;
		f32 minDist = 99999.0f;

		for (const auto& a : currentLocation->getActors())
		{
			if (a && !a->isHidden() && !isItemInInventory(a.get()) && a->isDraggable())
			{
				f32 dist = a->getPos().distance(worldPt);
				f32 touchRadius = std::max(a->getWidth(), a->getHeight()) * 0.55f;
				if (dist < touchRadius && dist < minDist)
				{
					minDist = dist;
					closestDraggable = a.get();
				}
			}
		}

		if (closestDraggable)
		{
			actorBeingDragged = closestDraggable;
			dragOffset = { 0.0f, 0.0f };
			std::string dragSnd = closestDraggable->props.get("soundItemDrag");
			if (!dragSnd.empty())
				AudioEngine::instance().playSfx(dragSnd);
			retVal = true;
		}
	}

	return retVal;
}

void World::handleMouseMove(const f32Point& rScreenPt)
{
	mouseScreenPos = rScreenPt;
}

bool World::handleMouseUp(const f32Point& rScreenPt, i32 numTaps)
{
	mouseScreenPos = rScreenPt;
	bool retVal = false;

	if (actorBeingDragged)
	{ // If dragging an item, handle drop
		observer_ptr<Actor> dragged = actorBeingDragged;
		actorBeingDragged = nullptr;

		// Check if dropped into inventory tray
		if (inventoryOpen)
		{
			f32 trayX = camera.getViewportWidth() - 320.0f;
			f32 trayY = 70.0f;
			f32Rectangle trayRect(trayX, trayY, 310.0f, 180.0f);
			if (trayRect.contains(rScreenPt))
			{ // Snap into inventory
				inventoryItems.push_back(dragged);
				dragged->setPos(rScreenPt);
				AudioEngine::instance().playSfx("sfxDrawer");
				retVal = true;
			}
		}

		if (!retVal && currentLocation)
		{
			// Check if dropped onto another actor in current room
			f32Point worldPt = camera.screenToWorld(rScreenPt);
			for (uindex i = 0; (i < currentLocation->getActors().size()) && !retVal; ++i)
			{
				const auto& a = currentLocation->getActors()[i];
				if (a && a.get() != dragged && !a->isHidden() && !isItemInInventory(a.get()))
				{
					f32 dist = a->getPos().distance(worldPt);
					f32 touchRadius = std::max(a->getWidth(), a->getHeight()) * 0.75f;
					if (dist < touchRadius)
					{
						if (a->onDrop(dragged))
							retVal = true;
					}
				}
			}
		}

		if (!retVal)
		{ // Drop missed: snap back
			dragged->setPos(dragged->getOrgPos());
			retVal = true;
		}
	}
	else if (currentLocation && !camera.isTransitioning())
	{ // Standard Tap / Click
		f32Point worldPt = camera.screenToWorld(rScreenPt);

		observer_ptr<Actor> closestActor = nullptr;
		f32 minDist = 99999.0f;

		for (const auto& a : currentLocation->getActors())
		{
			if (a && !a->isHidden() && !isItemInInventory(a.get()))
			{
				f32 dist = a->getPos().distance(worldPt);
				f32 touchRadius = std::max(a->getWidth(), a->getHeight()) * 0.60f;
				if (dist < touchRadius && dist < minDist)
				{
					minDist = dist;
					closestActor = a.get();
				}
			}
		}

		if (closestActor)
			retVal = closestActor->onTap(numTaps);
		else
			hideNoteBubble();
	}

	return retVal;
}

void World::update(f32 dt)
{
	camera.update(dt);

	if (noteTimer > 0.0f)
	{
		noteTimer -= dt;
		if (noteTimer <= 0.0f)
			hideNoteBubble();
	}

	// Update pods
	settingsPod.update(dt);
	inventoryPod.update(dt);

	// Sync inventory pod state with inventoryOpen flag
	inventoryOpen = inventoryPod.isExpanded;
}

void World::render(SDL_Renderer* pRenderer)
{ // 1. Draw Room Background
	Color bgColor = currentLocation ? currentLocation->getBackgroundColor() : Color{ 15, 15, 20, 255 };
	SDL_SetRenderDrawColor(pRenderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
	SDL_RenderClear(pRenderer);

	if (currentLocation)
	{ // 2. Draw Room Actors
		for (const auto& a : currentLocation->getActors())
		{
			if (a && !a->isHidden() && !isItemInInventory(a.get()) && a.get() != actorBeingDragged)
			{
				f32Rectangle screenBounds = camera.worldToScreenRect(a->getBounds());
				if (a->isDrawBackground())
					GlyphCache::instance().renderBlankBackground(screenBounds);
				GlyphCache::instance().renderGlyph(a->getActiveGlyph(), screenBounds, a->getRotation(), a->isFlipHoriz(), a->isFlipVert());
			}
		}
	}

	renderNoteBubble(pRenderer); // 3. Draw Note / Speech Bubble if active

	settingsPod.render(pRenderer, camera.getViewportWidth(), camera.getViewportHeight());
	inventoryPod.render(pRenderer, camera.getViewportWidth(), camera.getViewportHeight());

	if (actorBeingDragged)
	{ // 5. Draw Actor Being Dragged (Ceiling layer under mouse)
		f32 w = actorBeingDragged->getWidth() * camera.getScale();
		f32 h = actorBeingDragged->getHeight() * camera.getScale();
		f32Rectangle dragBounds(mouseScreenPos.x - w * 0.5f, mouseScreenPos.y - h * 0.5f, w, h);
		if (actorBeingDragged->isDrawBackground())
			GlyphCache::instance().renderBlankBackground(dragBounds, 0.9f);

		GlyphCache::instance().renderGlyph(actorBeingDragged->getActiveGlyph(), dragBounds, actorBeingDragged->getRotation(), actorBeingDragged->isFlipHoriz(), actorBeingDragged->isFlipVert(), 0.9f);
	}
}

void World::renderNoteBubble(SDL_Renderer* pRenderer)
{
	if (noteAnchorActor && !activeNotes.empty())
	{
		f32Point anchorScreen = camera.worldToScreen(noteAnchorActor->getPos());

		// activeNotes contains entries that could be either:
		// - Text notes: bracketed with # (e.g., "#Copyright (c) 2015#")
		// - Glyph names: not bracketed (e.g., "glyphBottle")

		std::string bubbleText;
		std::vector<std::string> glyphNotes;
		bool hasTextContent = false;

		for (uindex i = 0; i < activeNotes.size(); ++i)
		{
			const std::string& noteEntry = activeNotes[i];

			if (!(noteEntry == "blank" || noteEntry.empty()))
			{ // Check if this is text (bracketed with #)
				if (!noteEntry.empty() && noteEntry.front() == '#' && noteEntry.back() == '#')
				{ // Strip the # brackets and treat as text
					std::string textContent = noteEntry.substr(1, noteEntry.length() - 2);
					if (!textContent.empty())
					{
						if (!bubbleText.empty())
							bubbleText += "\n";
						bubbleText += textContent;
						hasTextContent = true;
					}
				}
				else
				{ // It's a glyph name
					glyphNotes.push_back(noteEntry);
				}
			}
		}

		if (hasTextContent)
		{ // If we have text content, render it
			f32 textPadding = 12.0f;
			f32 maxWidth = 300.0f;

			i32 textW = FontEngine::instance().getTextWidth(bubbleText);
			i32 textH = FontEngine::instance().getTextHeight(bubbleText);

			f32 bubbleW = std::min((f32)(textW + textPadding * 2.0f), maxWidth);
			f32 bubbleH = (f32)(textH + textPadding * 2.0f);

			f32 bubbleX = anchorScreen.x - bubbleW * 0.5f;
			f32 bubbleY = anchorScreen.y - (noteAnchorActor->getHeight() * camera.getScale() * 0.5f) - bubbleH - 12.0f;

			// Background bubble rect
			f32Rectangle bgRectF(bubbleX, bubbleY, bubbleW, bubbleH);
			roundedBoxRGBA(pRenderer, (i16)bubbleX, (i16)bubbleY, (i16)(bubbleX + bubbleW), (i16)(bubbleY + bubbleH), 20, 240, 240, 245, 230);
			roundedRectangleRGBA(pRenderer, (i16)bubbleX, (i16)bubbleY, (i16)(bubbleX + bubbleW), (i16)(bubbleY + bubbleH), 20, 40, 40, 45, 255);

			// Render text (centered both horizontally and vertically)
			f32Rectangle textRect(bubbleX + textPadding, bubbleY + textPadding, bubbleW - textPadding * 2.0f, bubbleH - textPadding * 2.0f);
			Color textColor(40, 40, 45, 255);
			FontEngine::instance().renderText(pRenderer, bubbleText, textRect, textColor, true);
		}
		else if (!glyphNotes.empty())
		{ // Render glyphs (original behavior)
			f32 iconSize = 40.0f;
			f32 padding = 10.0f;
			f32 totalW = glyphNotes.size() * (iconSize + padding) + padding;
			f32 totalH = iconSize + padding * 2.0f;

			f32 bubbleX = anchorScreen.x - totalW * 0.5f;
			f32 bubbleY = anchorScreen.y - (noteAnchorActor->getHeight() * camera.getScale() * 0.5f) - totalH - 12.0f;

			// Background bubble rect
			SDL_FRect bgRect = { bubbleX, bubbleY, totalW, totalH };
			SDL_SetRenderDrawColor(pRenderer, 240, 240, 245, 230);
			SDL_RenderFillRect(pRenderer, &bgRect);

			SDL_SetRenderDrawColor(pRenderer, 40, 40, 45, 255);
			SDL_RenderRect(pRenderer, &bgRect);

			for (uindex i = 0; i < glyphNotes.size(); ++i)
			{ // Render sequence of note icons
				f32 ix = bubbleX + padding + i * (iconSize + padding);
				f32 iy = bubbleY + padding;
				f32Rectangle iconBounds(ix, iy, iconSize, iconSize);
				GlyphCache::instance().renderGlyph(glyphNotes[i], iconBounds);
			}
		}
	}
}


