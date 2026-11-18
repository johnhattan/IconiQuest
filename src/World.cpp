#include "pch.hpp"
#include "World.hpp"
#include "AudioEngine.hpp"
#include "FontEngine.hpp"
#include "GlyphCache.hpp"
#include "TmxLoader.hpp"

World::World() : camera(800.0f, 600.0f), settingsPod(PodCorner::TopRight, 200.0f, 150.0f, "cog"), inventoryPod(PodCorner::BottomLeft, Pod::calculateInventoryWidth(4), Pod::calculateInventoryHeight(3), "backpack")
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

		// Clear any active drag/click state when changing locations
		actorBeingDragged = nullptr;
		clickedActor = nullptr;
		clickedInventorySlot = -1;

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
	clickedActor = nullptr;
	clickedInventorySlot = -1;
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
			if (inventoryItems[i] && inventoryItems[i] == pActor)
				retVal = true;
	}
	return retVal;
}

observer_ptr<Actor> World::findActorAtPoint(const f32Point& rScreenPt) const
{
	observer_ptr<Actor> retVal = nullptr;
	f32 minDist = 99999.0f;

	if (currentLocation)
	{
		for (const auto& a : currentLocation->getActors())
		{
			if (a && !a->isHidden() && !isItemInInventory(a.get()))
			{
				f32Rectangle screenBounds = camera.worldToScreenRect(a->getBounds());
				const f32 touchPadding = 12.0f;
				screenBounds.x -= touchPadding;
				screenBounds.y -= touchPadding;
				screenBounds.w += touchPadding * 2.0f;
				screenBounds.h += touchPadding * 2.0f;
				if (screenBounds.contains(rScreenPt))
				{
					f32Point screenCenter = screenBounds.center();
					f32 dist = screenCenter.distance(rScreenPt);
					if (dist < minDist)
					{
						minDist = dist;
						retVal = a.get();
					}
				}
			}
		}
	}

	return retVal;
}

bool World::handleMouseDown(const f32Point& rScreenPt)
{
	bool retVal = false;
	if (mousePressed)
	{
	}
	else
	{
		mousePressed = true;
		mouseScreenPos = rScreenPt;

		// First, check if clicking on a draggable actor outside the inventory
		observer_ptr<Actor> potentialActor =
		    !camera.isTransitioning() ? findActorAtPoint(rScreenPt) : nullptr;

		// 1. Check Pod clicks (but skip if an actor is underneath)
		bool settingsPodHandled = false;
		bool inventoryPodHandled = false;

		bool insideSettingsPod = settingsPod.getPodRect(camera.getViewportWidth(), camera.getViewportHeight()).contains(rScreenPt);
		f32Rectangle inventoryPodRect = inventoryPod.getPodRect(camera.getViewportWidth(), camera.getViewportHeight());
		bool insideInventoryPod = inventoryPodRect.contains(rScreenPt);
		if (!insideSettingsPod && !insideInventoryPod)
		{
			settingsPod.isExpanded = false;
			inventoryPod.isExpanded = false;
			inventoryOpen = false;
		}

		bool inventoryInteriorClicked = inventoryOpen && inventoryPodRect.contains(rScreenPt);

		f32Point inventoryIconPos(0, 0);
		if (inventoryPod.corner == PodCorner::TopLeft)
			inventoryIconPos = f32Point(inventoryPodRect.w - inventoryPod.iconSize, inventoryPodRect.h - inventoryPod.iconSize);
		else if (inventoryPod.corner == PodCorner::TopRight)
			inventoryIconPos = f32Point(0, inventoryPodRect.h - inventoryPod.iconSize);
		else if (inventoryPod.corner == PodCorner::BottomLeft)
			inventoryIconPos = f32Point(inventoryPodRect.w - inventoryPod.iconSize, 0);
		else
			inventoryIconPos = f32Point(0, 0);

		f32Rectangle inventoryIconRect(
		    inventoryPodRect.x + inventoryIconPos.x,
		    inventoryPodRect.y + inventoryIconPos.y,
		    inventoryPod.iconSize,
		    inventoryPod.iconSize);
		bool inventoryIconClicked = inventoryOpen && inventoryIconRect.contains(rScreenPt);

		if (inventoryIconClicked)
		{
			inventoryPodHandled = inventoryPod.onClick(rScreenPt, camera.getViewportWidth(), camera.getViewportHeight());
			inventoryOpen = inventoryPod.isExpanded;
			retVal = true;
		}
		else if (inventoryInteriorClicked)
		{
			i32 slotIndex = findInventorySlotAtPoint(rScreenPt);
			if (slotIndex >= 0 && slotIndex < (i32)inventoryItems.size() && inventoryItems[slotIndex])
			{
				clickedInventorySlot = slotIndex;
				clickStartPos = rScreenPt;
				retVal = true;
			}
			else
			{
				retVal = true;
			}
		}

		if (!retVal && !potentialActor)
		{
			f32Point logicalClickPt = rScreenPt;
			settingsPodHandled = settingsPod.onClick(logicalClickPt, camera.getViewportWidth(), camera.getViewportHeight());
			inventoryPodHandled = inventoryPod.onClick(logicalClickPt, camera.getViewportWidth(), camera.getViewportHeight());
		}

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
			// 2. Check if clicking inside open inventory pod
			f32Rectangle podRect = inventoryPod.getPodRect(camera.getViewportWidth(), camera.getViewportHeight());
			if (podRect.contains(rScreenPt))
			{
				// Find which slot was clicked
				i32 slotIndex = findInventorySlotAtPoint(rScreenPt);
				if (slotIndex >= 0 && slotIndex < (i32)inventoryItems.size() && inventoryItems[slotIndex])
				{
					// Clicked on an inventory item - track it for potential drag
					clickedInventorySlot = slotIndex;
					clickStartPos = rScreenPt;
					retVal = true;
				}
				else if (!retVal)
				{
					retVal = true; // Click absorbed by pod
				}
			}
			else if (potentialActor)
			{ // 3. Check Actors in Current Room (use pre-computed potentialActor)
				clickedActor = potentialActor;
				clickStartPos = rScreenPt;
				retVal = true;
			}
		}
		else if (potentialActor)
		{ // 3. Check Actors in Current Room (use pre-computed potentialActor)
			clickedActor = potentialActor;
			clickStartPos = rScreenPt;
			retVal = true;
		}
	}

	return retVal;
}

void World::handleMouseMove(const f32Point& rScreenPt)
{
	mouseScreenPos = rScreenPt;

	// Track hovering over inventory pod icon during drag for auto-expand
	if (actorBeingDragged)
	{
		f32Rectangle podRect = inventoryPod.getPodRect(camera.getViewportWidth(), camera.getViewportHeight());
		f32 dpiScale = camera.getDpiScale();
		f32Point iconPosInPod = f32Point(0, 0);

		if (inventoryPod.corner == PodCorner::TopRight)
			iconPosInPod = f32Point(0, podRect.h - 48.0f); // Bottom-left of pod
		else if (inventoryPod.corner == PodCorner::TopLeft)
			iconPosInPod = f32Point(podRect.w - 48.0f, podRect.h - 48.0f); // Bottom-right
		else if (inventoryPod.corner == PodCorner::BottomLeft)
			iconPosInPod = f32Point(podRect.w - 48.0f, 0); // Top-right
		else
			iconPosInPod = f32Point(0, 0); // Top-left

		f32Rectangle iconRectLogical(podRect.x + iconPosInPod.x, podRect.y + iconPosInPod.y, 48.0f, 48.0f);
		if (iconRectLogical.contains(rScreenPt))
		{
			podHoverTimer += 0.016f; // Approximate 60fps
			if (podHoverTimer >= podAutoExpandDelay && !inventoryPod.isExpanded)
			{
				inventoryPod.isExpanded = true;
				inventoryOpen = true;
			}
		}
		else
		{
			podHoverTimer = 0.0f;
		}
	}
	else
	{
		podHoverTimer = 0.0f;
	}

	// If we have a clicked actor and haven't started dragging yet
	if (clickedActor && !actorBeingDragged)
	{
		f32 distMoved = clickStartPos.distance(rScreenPt);
		if (distMoved > dragThreshold && clickedActor->isDraggable())
		{ // Threshold exceeded, start dragging
			actorBeingDragged = clickedActor;
			draggedFromInventory = false; // Actor is from room, not inventory
			f32Point worldPt = camera.screenToWorld(clickStartPos);
			dragOffset = actorBeingDragged->getPos() - worldPt;
			std::string dragSnd = actorBeingDragged->props.get("soundItemDrag");
			if (!dragSnd.empty())
				AudioEngine::instance().playSfx(dragSnd);
		}
	}

	// If we have a clicked inventory item and haven't started dragging yet
	if (clickedInventorySlot >= 0 && !actorBeingDragged)
	{
		f32 distMoved = clickStartPos.distance(rScreenPt);
		if (distMoved > dragThreshold)
		{ // Threshold exceeded, start dragging inventory item
			if (clickedInventorySlot < (i32)inventoryItems.size() && inventoryItems[clickedInventorySlot])
			{
				observer_ptr<Actor> item = inventoryItems[clickedInventorySlot];
				actorBeingDragged = item;
				draggedFromInventory = true;
				draggedFromInventorySlot = clickedInventorySlot;

				// For inventory drags, dragOffset is screen-based (item screen offset from mouse)
				// We'll position the dragged item at the mouse cursor
				dragOffset = f32Point(0, 0);

				// Restore original size when dragging out
				auto it_size = inventoryOriginalSizes.find(item.get());
				if (it_size != inventoryOriginalSizes.end())
				{
					item->setSize(it_size->second.width, it_size->second.height);
				}

				inventoryItems[clickedInventorySlot] = nullptr; // Remove from inventory slot

				std::string dragSnd = item->props.get("soundItemDrag");
				if (!dragSnd.empty())
					AudioEngine::instance().playSfx(dragSnd);
			}
			clickedInventorySlot = -1;
		}
	}
}

bool World::handleMouseUp(const f32Point& rScreenPt, i32 numTaps)
{
	bool retVal = false;
	if (!mousePressed)
	{
	}
	else
	{
		mousePressed = false;

		mouseScreenPos = rScreenPt;

		// If we clicked an actor but didn't drag (threshold not exceeded), treat it as a tap
		if (clickedActor && !actorBeingDragged)
		{
			retVal = clickedActor->onTap(numTaps);
		}

		// Always clear clickedActor at the end
		clickedActor = nullptr;

		// If we clicked an inventory item but didn't drag, treat it as a tap
		if (clickedInventorySlot >= 0 && !actorBeingDragged)
		{
			// Item was clicked but not dragged far enough - call onTap
			if (clickedInventorySlot < (i32)inventoryItems.size() && inventoryItems[clickedInventorySlot])
			{
				// Item is still in its slot (wasn't started to be dragged)
				observer_ptr<Actor> item = inventoryItems[clickedInventorySlot];
				retVal = item->onTap(numTaps);
			}
			clickedInventorySlot = -1;
		}

		if (actorBeingDragged)
		{ // If dragging an item, handle drop
			observer_ptr<Actor> dragged = actorBeingDragged;
			actorBeingDragged = nullptr;
			podHoverTimer = 0.0f;

			// Check if dropped over inventory pod icon (even if retracted)
			f32Rectangle podRect = inventoryPod.getPodRect(camera.getViewportWidth(), camera.getViewportHeight());
			f32 dpiScale = camera.getDpiScale();
			f32Point iconPosInPod = f32Point(0, 0);

			if (inventoryPod.corner == PodCorner::TopRight)
				iconPosInPod = f32Point(0, podRect.h - 48.0f);
			else if (inventoryPod.corner == PodCorner::TopLeft)
				iconPosInPod = f32Point(podRect.w - 48.0f, podRect.h - 48.0f);
			else if (inventoryPod.corner == PodCorner::BottomLeft)
				iconPosInPod = f32Point(podRect.w - 48.0f, 0);
			else
				iconPosInPod = f32Point(0, 0);

			f32Rectangle iconRectLogical(podRect.x + iconPosInPod.x, podRect.y + iconPosInPod.y, 48.0f, 48.0f);
			bool droppedOnIcon = iconRectLogical.contains(rScreenPt);

			// Check if dropped into inventory (or on icon)
			if (inventoryOpen || droppedOnIcon)
			{
				i32 slotIndex = findInventorySlotAtPoint(rScreenPt);
				i32 maxSlots = inventoryPod.inventoryCols * inventoryPod.inventoryRows;

				// If dropped on icon or in a slot, place it there
				if (droppedOnIcon || slotIndex >= 0)
				{
					i32 targetSlot = -1;

					if (!droppedOnIcon && slotIndex >= 0)
					{
						// Check if the slot at drop point is occupied
						bool slotOccupied = false;
						if (slotIndex < (i32)inventoryItems.size() && inventoryItems[slotIndex])
							slotOccupied = true;

						if (slotOccupied)
						{
							// Slot occupied, find first available
							for (i32 i = 0; i < maxSlots; ++i)
							{
								if (targetSlot < 0 && (i >= (i32)inventoryItems.size() || !inventoryItems[i]))
								{
									targetSlot = i;
								}
							}
						}
						else
						{
							// Slot is empty, use it
							targetSlot = slotIndex;
						}
					}
					else
					{
						// Dropped on icon, find first available slot
						for (i32 i = 0; i < maxSlots; ++i)
						{
							if (targetSlot < 0 && (i >= (i32)inventoryItems.size() || !inventoryItems[i]))
							{
								targetSlot = i;
							}
						}
					}

					if (targetSlot >= 0 && targetSlot < maxSlots)
					{
						// Pad with nullptrs if needed to reach target slot
						while ((i32)inventoryItems.size() < targetSlot)
						{
							inventoryItems.push_back(nullptr);
						}

						// Insert at the target slot
						if ((i32)inventoryItems.size() == targetSlot)
						{
							inventoryItems.push_back(dragged);
						}
						else
						{
							inventoryItems[targetSlot] = dragged;
						}

						// Store original size and resize to fit inventory cell
						inventoryOriginalSizes[dragged.get()] = f32Size(dragged->getWidth(), dragged->getHeight());
						f32 cellSize = 48.0f; // inventory cell size
						dragged->setSize(cellSize, cellSize);

						AudioEngine::instance().playSfx("sfxDrawer");
						retVal = true;
					}
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
			{
				// Drop failed: snap back to inventory
				if (draggedFromInventory)
				{
					// Put item back in inventory at the slot it came from
					if (draggedFromInventorySlot >= 0 && draggedFromInventorySlot < (i32)inventoryItems.size())
					{
						inventoryItems[draggedFromInventorySlot] = dragged;
						// Resize back to cell size
						f32 cellSize = 48.0f;
						dragged->setSize(cellSize, cellSize);
					}
					draggedFromInventory = false;
					draggedFromInventorySlot = -1;
				}
				else
				{
					// Was in world, snap back to original position
					dragged->setPos(dragged->getOrgPos());
				}
				retVal = true;
			}
		}
		else if (!retVal && currentLocation && !camera.isTransitioning())
		{ // Standard Tap / Click
			observer_ptr<Actor> closestActor = findActorAtPoint(rScreenPt);

			if (closestActor)
			{
				retVal = closestActor->onTap(numTaps);
			}
			else
				hideNoteBubble();
		}
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

	f32 dpiScale = camera.getDpiScale();

	if (currentLocation)
	{ // 2. Draw Room Actors
		for (const auto& a : currentLocation->getActors())
		{
			if (a && !a->isHidden() && !isItemInInventory(a.get()) && a.get() != actorBeingDragged)
			{
				f32Rectangle screenBounds = camera.worldToScreenRect(a->getBounds());
				// Scale to physical pixels
				screenBounds.x *= dpiScale;
				screenBounds.y *= dpiScale;
				screenBounds.w *= dpiScale;
				screenBounds.h *= dpiScale;

				if (a->isDrawBackground())
					GlyphCache::instance().renderBlankBackground(screenBounds);
				GlyphCache::instance().renderGlyph(a->getActiveGlyph(), screenBounds, a->getRotation(), a->isFlipHoriz(), a->isFlipVert());
			}
		}
	}

	renderNoteBubble(pRenderer); // 3. Draw Note / Speech Bubble if active

	settingsPod.render(pRenderer, camera.getViewportWidth(), camera.getViewportHeight(), dpiScale);
	inventoryPod.render(pRenderer, camera.getViewportWidth(), camera.getViewportHeight(), dpiScale);

	if (actorBeingDragged)
	{ // 5. Draw Actor Being Dragged (Ceiling layer under mouse)
		f32 w = actorBeingDragged->getWidth() * camera.getScale();
		f32 h = actorBeingDragged->getHeight() * camera.getScale();

		f32Point dragPosLogical;
		if (draggedFromInventory)
		{
			// For inventory drags, dragOffset is in screen space
			dragPosLogical = mouseScreenPos + dragOffset;
		}
		else
		{
			// For world drags, dragOffset is in world space
			f32Point dragPosWorld = camera.screenToWorld(mouseScreenPos) + dragOffset;
			dragPosLogical = camera.worldToScreen(dragPosWorld);
		}

		// Scale to physical pixels
		f32Point dragPosScreen = dragPosLogical * dpiScale;
		f32Rectangle dragBounds(dragPosScreen.x - w * dpiScale * 0.5f, dragPosScreen.y - h * dpiScale * 0.5f, w * dpiScale, h * dpiScale);
		if (actorBeingDragged->isDrawBackground())
			GlyphCache::instance().renderBlankBackground(dragBounds, 0.9f);

		GlyphCache::instance().renderGlyph(actorBeingDragged->getActiveGlyph(), dragBounds, actorBeingDragged->getRotation(), actorBeingDragged->isFlipHoriz(), actorBeingDragged->isFlipVert(), 0.9f);
	}
}

i32 World::findInventorySlotAtPoint(const f32Point& rScreenPt) const
{
	i32 retVal = -1;
	if (inventoryOpen && inventoryPod.inventoryCols > 0 && inventoryPod.inventoryRows > 0)
	{
		f32Rectangle podRect = inventoryPod.getPodRect(camera.getViewportWidth(), camera.getViewportHeight());
		f32 slotSize = 44.0f;
		f32 gap = 12.0f;
		f32 padding = 16.0f;
		f32 startX = podRect.x + padding;
		f32 startY = podRect.y + padding;

		if (inventoryPod.corner == PodCorner::TopRight || inventoryPod.corner == PodCorner::BottomRight)
		{
			startX = podRect.x + inventoryPod.iconSize + gap;
		}

		for (i32 r = 0; r < inventoryPod.inventoryRows && retVal < 0; ++r)
		{
			for (i32 c = 0; c < inventoryPod.inventoryCols && retVal < 0; ++c)
			{
				f32 sx = startX + c * (slotSize + gap);
				f32 sy = startY + r * (slotSize + gap);
				f32Rectangle slotRect(sx, sy, slotSize, slotSize);
				if (slotRect.contains(rScreenPt))
					retVal = r * inventoryPod.inventoryCols + c;
			}
		}
	}

	return retVal;
}

void World::renderNoteBubble(SDL_Renderer* pRenderer)
{
	if (noteAnchorActor && !activeNotes.empty())
	{
		f32 dpiScale = camera.getDpiScale();
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

			// Scale to physical pixels
			f32 bubbleXPhys = bubbleX * dpiScale;
			f32 bubbleYPhys = bubbleY * dpiScale;
			f32 bubbleWPhys = bubbleW * dpiScale;
			f32 bubbleHPhys = bubbleH * dpiScale;

			// Background bubble rect
			roundedBoxRGBA(pRenderer, (i16)bubbleXPhys, (i16)bubbleYPhys, (i16)(bubbleXPhys + bubbleWPhys), (i16)(bubbleYPhys + bubbleHPhys), (i16)(20 * dpiScale), 240, 240, 245, 230);
			roundedRectangleRGBA(pRenderer, (i16)bubbleXPhys, (i16)bubbleYPhys, (i16)(bubbleXPhys + bubbleWPhys), (i16)(bubbleYPhys + bubbleHPhys), (i16)(20 * dpiScale), 40, 40, 45, 255);

			f32Rectangle textRect(
			    bubbleXPhys + textPadding * dpiScale,
			    bubbleYPhys + textPadding * dpiScale,
			    bubbleWPhys - textPadding * dpiScale * 2.0f,
			    bubbleHPhys - textPadding * dpiScale * 2.0f);
			Color textColor(40, 40, 45, 255);
			FontEngine::instance().renderText(pRenderer, bubbleText, textRect, textColor, true, dpiScale);
		}
		else if (!glyphNotes.empty())
		{ // Render glyphs with consistent rounded rectangle styling
			f32 iconSize = 40.0f;
			f32 padding = 12.0f;
			f32 gap = 8.0f;
			f32 totalW = glyphNotes.size() * iconSize + (glyphNotes.size() - 1) * gap + padding * 2.0f;
			f32 totalH = iconSize + padding * 2.0f;

			f32 bubbleX = anchorScreen.x - totalW * 0.5f;
			f32 bubbleY = anchorScreen.y - (noteAnchorActor->getHeight() * camera.getScale() * 0.5f) - totalH - 12.0f;

			// Scale to physical pixels
			bubbleX *= dpiScale;
			bubbleY *= dpiScale;
			totalW *= dpiScale;
			totalH *= dpiScale;
			iconSize *= dpiScale;
			padding *= dpiScale;
			gap *= dpiScale;

			// Background bubble rect with rounded corners (matching text note style)
			roundedBoxRGBA(pRenderer, (i16)bubbleX, (i16)bubbleY, (i16)(bubbleX + totalW), (i16)(bubbleY + totalH), (i16)(20 * dpiScale), 240, 240, 245, 230);
			roundedRectangleRGBA(pRenderer, (i16)bubbleX, (i16)bubbleY, (i16)(bubbleX + totalW), (i16)(bubbleY + totalH), (i16)(20 * dpiScale), 40, 40, 45, 255);

			for (uindex i = 0; i < glyphNotes.size(); ++i)
			{ // Render sequence of note icons
				f32 ix = bubbleX + padding + i * (iconSize + gap);
				f32 iy = bubbleY + padding;
				f32Rectangle iconBounds(ix, iy, iconSize, iconSize);
				GlyphCache::instance().renderGlyph(glyphNotes[i], iconBounds);
			}
		}
	}
}
