#include "World.hpp"
#include "AudioEngine.hpp"
#include "FontEngine.hpp"
#include "GlyphCache.hpp"
#include "TmxLoader.hpp"
#include "pch.hpp"

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

bool World::handleMouseDown(const f32Point& rScreenPt)
{
	mouseScreenPos = rScreenPt;
	bool retVal = false;

	// First, check if clicking on a draggable actor outside the inventory
	// If so, don't let the pod close
	observer_ptr<Actor> potentialDraggable = nullptr;
	if (currentLocation && !camera.isTransitioning())
	{
		f32Point logicalPt = rScreenPt / camera.getDpiScale();
		f32Point worldPt = camera.screenToWorld(logicalPt);
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
					potentialDraggable = a.get();
				}
			}
		}
	}

	// 1. Check Pod clicks (but skip if we're about to drag a room actor)
	bool settingsPodHandled = false;
	bool inventoryPodHandled = false;
	
	if (!potentialDraggable)
	{
		f32Point logicalClickPt = rScreenPt / camera.getDpiScale();
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
		f32 dpiScale = camera.getDpiScale();
		f32Rectangle podRectPhysical(podRect.x * dpiScale, podRect.y * dpiScale, podRect.w * dpiScale, podRect.h * dpiScale);
		if (podRectPhysical.contains(rScreenPt))
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
		else if (potentialDraggable)
		{ // 3. Check Actors in Current Room (use pre-computed potentialDraggable)
			clickedActor = potentialDraggable;
			clickStartPos = rScreenPt;
			retVal = true;
		}
	}
	else if (potentialDraggable)
	{ // 3. Check Actors in Current Room (use pre-computed potentialDraggable)
		clickedActor = potentialDraggable;
		clickStartPos = rScreenPt;
		retVal = true;
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
		f32Rectangle iconRectPhysical(iconRectLogical.x * dpiScale, iconRectLogical.y * dpiScale, iconRectLogical.w * dpiScale, iconRectLogical.h * dpiScale);
		
		if (iconRectPhysical.contains(rScreenPt))
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
		if (distMoved > dragThreshold)
		{ // Threshold exceeded, start dragging
			actorBeingDragged = clickedActor;
			draggedFromInventory = false; // Actor is from room, not inventory
			f32Point logicalClickPt = clickStartPos / camera.getDpiScale();
			f32Point worldPt = camera.screenToWorld(logicalClickPt);
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
					inventoryOriginalSizes.erase(it_size);
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
	mouseScreenPos = rScreenPt;
	bool retVal = false;

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
		f32Rectangle iconRectPhysical(iconRectLogical.x * dpiScale, iconRectLogical.y * dpiScale, iconRectLogical.w * dpiScale, iconRectLogical.h * dpiScale);
		bool droppedOnIcon = iconRectPhysical.contains(rScreenPt);

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
							if (i >= (i32)inventoryItems.size() || !inventoryItems[i])
							{
								targetSlot = i;
								break;
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
						if (i >= (i32)inventoryItems.size() || !inventoryItems[i])
						{
							targetSlot = i;
							break;
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
			// Only check for drops on actors if NOT dragging from inventory
			// Items from inventory should only go back to inventory, not to world locations
			if (!draggedFromInventory)
			{
				// Check if dropped onto another actor in current room
				f32Point logicalDropPt = rScreenPt / camera.getDpiScale();
				f32Point worldPt = camera.screenToWorld(logicalDropPt);
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
	else if (currentLocation && !camera.isTransitioning())
	{ // Standard Tap / Click
		f32Point logicalClickPt2 = rScreenPt / camera.getDpiScale();
		f32Point worldPt = camera.screenToWorld(logicalClickPt2);

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
			f32Point logicalMousePos = mouseScreenPos / camera.getDpiScale();
			f32Point dragPosWorld = camera.screenToWorld(logicalMousePos) + dragOffset;
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
	if (!inventoryOpen || inventoryPod.inventoryCols <= 0 || inventoryPod.inventoryRows <= 0)
		return -1;

	f32 dpiScale = camera.getDpiScale();
	f32Rectangle podRect = inventoryPod.getPodRect(camera.getViewportWidth(), camera.getViewportHeight());
	f32 slotSize = 44.0f;
	f32 gap = 12.0f;
	f32 padding = 16.0f;
	f32 startX = podRect.x + padding;
	f32 startY = podRect.y + padding;

	// Adjust start position based on corner to match Pod::render layout
	if (inventoryPod.corner == PodCorner::TopRight || inventoryPod.corner == PodCorner::BottomRight)
	{
		// Icon is on the left, inventory grid starts after it
		startX = podRect.x + inventoryPod.iconSize + gap;
	}
	else if (inventoryPod.corner == PodCorner::BottomLeft)
	{
		// Icon is on the left for BottomLeft too
		startX = podRect.x + inventoryPod.iconSize + gap;
	}
	else if (inventoryPod.corner == PodCorner::TopLeft)
	{
		// Icon is at bottom-right for TopLeft, inventory starts at top-left
		// No offset needed
	}

	// Check which slot the point is in (must match Pod::render calculations exactly)
	for (i32 r = 0; r < inventoryPod.inventoryRows; ++r)
	{
		for (i32 c = 0; c < inventoryPod.inventoryCols; ++c)
		{
			// Match the exact calculation used in Pod::render
			f32 sx = (startX + c * (slotSize + gap)) * dpiScale;
			f32 sy = (startY + r * (slotSize + gap)) * dpiScale;
			f32Rectangle slotRect(sx, sy, slotSize * dpiScale, slotSize * dpiScale);
			if (slotRect.contains(rScreenPt))
			{
				return r * inventoryPod.inventoryCols + c;
			}
		}
	}

	return -1;
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

			// Render text (use logical coordinates for text, it will be rendered at screen position)
			f32Rectangle textRect(bubbleX + textPadding, bubbleY + textPadding, bubbleW - textPadding * 2.0f, bubbleH - textPadding * 2.0f);
			Color textColor(40, 40, 45, 255);
			FontEngine::instance().renderText(pRenderer, bubbleText, textRect, textColor, true);
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


