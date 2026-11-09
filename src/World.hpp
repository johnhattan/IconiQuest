#pragma once

#include <string>
#include <vector>
#include <memory>
#include "pch.hxx"
#include "PropertyDictionary.hpp"
#include "Camera.hpp"
#include "Location.hpp"
#include "Actor.hpp"

class World {
public:
    World();
    ~World();

    bool load(const std::string& tmxPath, const std::string& assetsDir);
    bool save();

    PropertyDictionary& props() { return m_props; }
    const PropertyDictionary& props() const { return m_props; }

    const std::vector<std::unique_ptr<Location>>& getLocations() const { return m_locations; }
    std::vector<std::unique_ptr<Location>>& getLocations() { return m_locations; }

    void addLocation(std::unique_ptr<Location> loc);
    Location* findLocation(const std::string& name) const;
    Location* getCurrentLocation() const { return m_currentLocation; }

    void changeLocation(const std::string& name, float transitionTime = 0.75f);
    void revertWorld();
    void revertCurrentLocation();

    Camera& getCamera() { return m_camera; }
    const Camera& getCamera() const { return m_camera; }

    void showNoteBubble(Actor* anchor, const std::vector<std::string>& notes, float duration = 4.0f);
    void hideNoteBubble();

    // Inventory Drawer (Pod)
    bool isInventoryOpen() const { return m_inventoryOpen; }
    void toggleInventory() { m_inventoryOpen = !m_inventoryOpen; }
    void openInventory(bool open) { m_inventoryOpen = open; }
    bool isItemInInventory(const Actor* actor) const;

    // Input
    bool handleMouseDown(const f32Point& screenPt);
    bool handleMouseUp(const f32Point& screenPt, int numTaps = 1);
    void handleMouseMove(const f32Point& screenPt);

    void update(float dt);
    void render(SDL_Renderer* renderer);

    Actor* getActorBeingDragged() const { return m_actorBeingDragged; }
    const std::string& getTmxPath() const { return m_tmxPath; }
    const std::string& getAssetsDir() const { return m_assetsDir; }

private:
    void renderNoteBubble(SDL_Renderer* renderer);
    void renderInventoryPod(SDL_Renderer* renderer);

    std::string m_tmxPath;
    std::string m_assetsDir;
    PropertyDictionary m_props;

    std::vector<std::unique_ptr<Location>> m_locations;
    Location* m_currentLocation = nullptr;
    Location* m_startLocation = nullptr;

    Camera m_camera;

    // Dragging
    Actor* m_actorBeingDragged = nullptr;
    f32Point m_dragOffset;
    f32Point m_mouseScreenPos;

    // Note Bubble
    Actor* m_noteAnchorActor = nullptr;
    std::vector<std::string> m_activeNotes;
    float m_noteTimer = 0.0f;

    // Inventory Pod
    bool m_inventoryOpen = false;
    float m_inventoryAnim = 0.0f;
    std::vector<Actor*> m_inventoryItems;

    float m_tileW = 48.0f;
    float m_tileH = 48.0f;
};
