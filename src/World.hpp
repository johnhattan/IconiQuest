#pragma once

#include "Actor.hpp"
#include "Camera.hpp"
#include "Location.hpp"
#include "pch.hpp"
#include "SDL3_gfxPrimitives.h"

struct World
{
	std::string tmxPath;
	std::string assetsDir;
	std::string gameAssetsRoot;
	std::string currentGame = "_default";
	PropertyDictionary props;
	std::vector<std::unique_ptr<Location>> locations;
	observer_ptr<Location> currentLocation;
	observer_ptr<Location> startLocation;
	Camera camera;
	observer_ptr<Actor> actorBeingDragged;
	f32Point dragOffset;
	f32Point mouseScreenPos;
	observer_ptr<Actor> noteAnchorActor;
	std::vector<std::string> activeNotes;
	f32 noteTimer = 0.0f;
	bool inventoryOpen = false;
	f32 inventoryAnim = 0.0f;
	std::vector<observer_ptr<Actor>> inventoryItems;

	World();
	~World();

	bool load(const std::string& rTmxPath, const std::string& rAssetsDir);
	bool save();

	PropertyDictionary& getProps() { return props; }
	const PropertyDictionary& getProps() const { return props; }

	const std::vector<std::unique_ptr<Location>>& getLocations() const { return locations; }
	std::vector<std::unique_ptr<Location>>& getLocations() { return locations; }

	void addLocation(std::unique_ptr<Location> pLoc);
	observer_ptr<Location> findLocation(const std::string& rName) const;
	observer_ptr<Location> getCurrentLocation() const { return currentLocation; }

	void changeLocation(const std::string& rName, f32 transitionTime = 0.75f, const std::string& rEasing = "linear");
	void switchGame(const std::string& rGameName, f32 transitionTime = 0.75f, const std::string& rEasing = "linear");
	void revertWorld();
	void revertCurrentLocation();

	Camera& getCamera() { return camera; }
	const Camera& getCamera() const { return camera; }

	void showNoteBubble(observer_ptr<Actor> pAnchor, const std::vector<std::string>& rNotes, f32 duration = 4.0f);
	void hideNoteBubble();

	bool isInventoryOpen() const { return inventoryOpen; }
	void toggleInventory() { inventoryOpen = !inventoryOpen; }
	void openInventory(bool open) { inventoryOpen = open; }
	bool isItemInInventory(const observer_ptr<Actor> pActor) const;

	bool handleMouseDown(const f32Point& rScreenPt);
	bool handleMouseUp(const f32Point& rScreenPt, i32 numTaps = 1);
	void handleMouseMove(const f32Point& rScreenPt);

	void update(f32 dt);
	void render(SDL_Renderer* pRenderer);

	void setGameAssetsRoot(const std::string& rRoot) { gameAssetsRoot = rRoot; }
	void setCurrentGame(const std::string& rGame) { currentGame = rGame; }
	const std::string& getCurrentGame() const { return currentGame; }

private:
	void renderNoteBubble(SDL_Renderer* pRenderer);
	void renderInventoryPod(SDL_Renderer* pRenderer);
};
