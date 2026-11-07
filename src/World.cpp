#include "World.hpp"
#include "TmxLoader.hpp"
#include "GlyphCache.hpp"
#include "AudioEngine.hpp"
#include <iostream>
#include <cmath>

World::World() : m_camera(800.0f, 600.0f) {}

World::~World() = default;

bool World::load(const std::string& tmxPath, const std::string& assetsDir) {
    m_tmxPath = tmxPath;
    m_assetsDir = assetsDir;

    m_locations.clear();
    m_currentLocation = nullptr;
    m_startLocation = nullptr;
    m_actorBeingDragged = nullptr;
    m_noteAnchorActor = nullptr;
    m_inventoryItems.clear();

    // Set paths
    GlyphCache::instance().setImagesDir(assetsDir + "/images");
    AudioEngine::instance().setSoundsDir(assetsDir + "/sounds");

    if (!TmxLoader::loadWorld(*this, tmxPath)) {
        std::cerr << "[World] Failed to load " << tmxPath << std::endl;
        return false;
    }

    // Determine start location
    for (const auto& loc : m_locations) {
        if (loc->props().getBool("locationStart", false)) {
            m_startLocation = loc.get();
            break;
        }
    }
    if (!m_startLocation) {
        m_startLocation = findLocation("part1");
    }
    if (!m_startLocation) {
        m_startLocation = findLocation("0-0");
    }
    if (!m_startLocation && !m_locations.empty()) {
        m_startLocation = m_locations.front().get();
    }

    m_currentLocation = m_startLocation;
    if (m_currentLocation) {
        m_camera.instantFrame(m_currentLocation->getBounds());

        // Play initial sounds
        std::string theme = m_currentLocation->props().get("soundThemeEnter");
        if (theme.empty()) theme = m_currentLocation->props().get("sndThemeEnter");
        if (!theme.empty()) AudioEngine::instance().playTheme(theme);

        std::string enterSnd = m_currentLocation->props().get("soundEnter");
        if (!enterSnd.empty()) AudioEngine::instance().playSfx(enterSnd);
    }

    // Trigger onPlace on all actors to set initial states
    for (const auto& loc : m_locations) {
        for (const auto& a : loc->getActors()) {
            if (a) a->onPlace();
        }
    }

    return true;
}

bool World::save() {
    if (m_tmxPath.empty()) return false;
    return TmxLoader::saveWorld(*this, m_tmxPath);
}

void World::addLocation(std::unique_ptr<Location> loc) {
    if (loc) {
        m_locations.push_back(std::move(loc));
    }
}

Location* World::findLocation(const std::string& name) const {
    for (const auto& l : m_locations) {
        if (l && l->getName() == name) {
            return l.get();
        }
    }
    return nullptr;
}

void World::changeLocation(const std::string& name, float transitionTime) {
    Location* loc = findLocation(name);
    if (!loc || loc == m_currentLocation) return;

    m_currentLocation = loc;
    m_camera.frameRect(loc->getBounds(), transitionTime);
    hideNoteBubble();

    // Play room sound and theme
    std::string theme = loc->props().get("soundThemeEnter");
    if (theme.empty()) theme = loc->props().get("sndThemeEnter");
    if (!theme.empty()) AudioEngine::instance().playTheme(theme);

    std::string enterSnd = loc->props().get("soundEnter");
    if (!enterSnd.empty()) AudioEngine::instance().playSfx(enterSnd);

    for (const auto& a : loc->getActors()) {
        if (a) a->onPlace();
    }
}

void World::revertWorld() {
    for (auto& loc : m_locations) {
        if (loc) loc->revert();
    }
    m_inventoryItems.clear();
    m_actorBeingDragged = nullptr;
    hideNoteBubble();

    if (m_startLocation) {
        changeLocation(m_startLocation->getName(), 0.0f);
    }
}

void World::revertCurrentLocation() {
    if (m_currentLocation) {
        m_currentLocation->revert();
    }
    hideNoteBubble();
}

void World::showNoteBubble(Actor* anchor, const std::vector<std::string>& notes, float duration) {
    m_noteAnchorActor = anchor;
    m_activeNotes = notes;
    m_noteTimer = duration;
}

void World::hideNoteBubble() {
    m_noteAnchorActor = nullptr;
    m_activeNotes.clear();
    m_noteTimer = 0.0f;
}

bool World::isItemInInventory(const Actor* actor) const {
    if (!actor) return false;
    for (const auto* item : m_inventoryItems) {
        if (item == actor) return true;
    }
    return false;
}

bool World::handleMouseDown(const Vec2& screenPt) {
    m_mouseScreenPos = screenPt;

    // 1. Check Inventory Button click (top right)
    float invBtnX = m_camera.getViewportWidth() - 56.0f;
    float invBtnY = 16.0f;
    Rect invBtnRect(invBtnX, invBtnY, 48.0f, 48.0f);
    if (invBtnRect.contains(screenPt)) {
        toggleInventory();
        AudioEngine::instance().playSfx("sfxDrawer");
        return true;
    }

    // 2. Check if clicking inside open inventory tray
    if (m_inventoryOpen) {
        float trayX = m_camera.getViewportWidth() - 320.0f;
        float trayY = 70.0f;
        Rect trayRect(trayX, trayY, 310.0f, 180.0f);
        if (trayRect.contains(screenPt)) {
            // Find item in inventory
            for (auto it = m_inventoryItems.begin(); it != m_inventoryItems.end(); ++it) {
                Actor* item = *it;
                Rect ib(item->getPos().x - item->getWidth() * 0.5f, item->getPos().y - item->getHeight() * 0.5f, item->getWidth(), item->getHeight());
                if (ib.contains(screenPt)) {
                    m_actorBeingDragged = item;
                    m_dragOffset = item->getPos() - screenPt;
                    m_inventoryItems.erase(it);
                    return true;
                }
            }
            return true; // Click absorbed by tray
        }
    }

    // 3. Check Actors in Current Room
    if (m_currentLocation && !m_camera.isTransitioning()) {
        Vec2 worldPt = m_camera.screenToWorld(screenPt);

        Actor* closestDraggable = nullptr;
        float minDist = 99999.0f;

        for (const auto& a : m_currentLocation->getActors()) {
            if (!a || a->isHidden() || isItemInInventory(a.get())) continue;
            if (!a->isDraggable()) continue;

            float dist = a->getPos().distance(worldPt);
            float touchRadius = std::max(a->getWidth(), a->getHeight()) * 0.55f;
            if (dist < touchRadius && dist < minDist) {
                minDist = dist;
                closestDraggable = a.get();
            }
        }

        if (closestDraggable) {
            m_actorBeingDragged = closestDraggable;
            m_dragOffset = {0.0f, 0.0f};
            std::string dragSnd = closestDraggable->props().get("soundItemDrag");
            if (!dragSnd.empty()) AudioEngine::instance().playSfx(dragSnd);
            return true;
        }
    }

    return false;
}

void World::handleMouseMove(const Vec2& screenPt) {
    m_mouseScreenPos = screenPt;
}

bool World::handleMouseUp(const Vec2& screenPt, int numTaps) {
    m_mouseScreenPos = screenPt;

    // If dragging an item, handle drop
    if (m_actorBeingDragged) {
        Actor* dragged = m_actorBeingDragged;
        m_actorBeingDragged = nullptr;

        // Check if dropped into inventory tray
        if (m_inventoryOpen) {
            float trayX = m_camera.getViewportWidth() - 320.0f;
            float trayY = 70.0f;
            Rect trayRect(trayX, trayY, 310.0f, 180.0f);
            if (trayRect.contains(screenPt)) {
                // Snap into inventory
                m_inventoryItems.push_back(dragged);
                dragged->setPos(screenPt);
                AudioEngine::instance().playSfx("sfxDrawer");
                return true;
            }
        }

        // Check if dropped onto another actor in current room
        if (m_currentLocation) {
            Vec2 worldPt = m_camera.screenToWorld(screenPt);
            for (const auto& a : m_currentLocation->getActors()) {
                if (!a || a.get() == dragged || a->isHidden() || isItemInInventory(a.get())) continue;

                float dist = a->getPos().distance(worldPt);
                float touchRadius = std::max(a->getWidth(), a->getHeight()) * 0.75f;
                if (dist < touchRadius) {
                    if (a->onDrop(dragged)) {
                        return true;
                    }
                }
            }
        }

        // Drop missed: snap back
        dragged->setPos(dragged->getOrgPos());
        return true;
    }

    // Standard Tap / Click
    if (m_currentLocation && !m_camera.isTransitioning()) {
        Vec2 worldPt = m_camera.screenToWorld(screenPt);

        Actor* closestActor = nullptr;
        float minDist = 99999.0f;

        for (const auto& a : m_currentLocation->getActors()) {
            if (!a || a->isHidden() || isItemInInventory(a.get())) continue;

            float dist = a->getPos().distance(worldPt);
            float touchRadius = std::max(a->getWidth(), a->getHeight()) * 0.60f;
            if (dist < touchRadius && dist < minDist) {
                minDist = dist;
                closestActor = a.get();
            }
        }

        if (closestActor) {
            return closestActor->onTap(numTaps);
        } else {
            hideNoteBubble();
        }
    }

    return false;
}

void World::update(float dt) {
    m_camera.update(dt);

    if (m_noteTimer > 0.0f) {
        m_noteTimer -= dt;
        if (m_noteTimer <= 0.0f) {
            hideNoteBubble();
        }
    }

    // Animate inventory slide
    float targetAnim = m_inventoryOpen ? 1.0f : 0.0f;
    m_inventoryAnim += (targetAnim - m_inventoryAnim) * std::min(1.0f, dt * 10.0f);
}

void World::render(SDL_Renderer* renderer) {
    // 1. Draw Room Background
    ColorRGBA bgColor = m_currentLocation ? m_currentLocation->getBackgroundColor() : ColorRGBA{15, 15, 20, 255};
    SDL_SetRenderDrawColor(renderer, bgColor.r, bgColor.g, bgColor.b, bgColor.a);
    SDL_RenderClear(renderer);

    // 2. Draw Room Actors
    if (m_currentLocation) {
        for (const auto& a : m_currentLocation->getActors()) {
            if (!a || a->isHidden() || isItemInInventory(a.get())) continue;
            if (a.get() == m_actorBeingDragged) continue; // Drawn on ceiling

            Rect screenBounds = m_camera.worldToScreenRect(a->getBounds());
            if (a->isDrawBackground()) {
                GlyphCache::instance().renderBlankBackground(screenBounds);
            }
            GlyphCache::instance().renderGlyph(
                a->getActiveGlyph(),
                screenBounds,
                a->getRotation(),
                a->getFlipHoriz(),
                a->getFlipVert()
            );
        }
    }

    // 3. Draw Note / Speech Bubble if active
    renderNoteBubble(renderer);

    // 4. Draw Inventory Drawer & Toggle Button
    renderInventoryPod(renderer);

    // 5. Draw Actor Being Dragged (Ceiling layer under mouse)
    if (m_actorBeingDragged) {
        float w = m_actorBeingDragged->getWidth() * m_camera.getScale();
        float h = m_actorBeingDragged->getHeight() * m_camera.getScale();
        Rect dragBounds(m_mouseScreenPos.x - w * 0.5f, m_mouseScreenPos.y - h * 0.5f, w, h);
        if (m_actorBeingDragged->isDrawBackground()) {
            GlyphCache::instance().renderBlankBackground(dragBounds, 0.9f);
        }
        GlyphCache::instance().renderGlyph(
            m_actorBeingDragged->getActiveGlyph(),
            dragBounds,
            m_actorBeingDragged->getRotation(),
            m_actorBeingDragged->getFlipHoriz(),
            m_actorBeingDragged->getFlipVert(),
            0.9f
        );
    }
}

void World::renderNoteBubble(SDL_Renderer* renderer) {
    if (!m_noteAnchorActor || m_activeNotes.empty()) return;

    Vec2 anchorScreen = m_camera.worldToScreen(m_noteAnchorActor->getPos());
    float iconSize = 40.0f;
    float padding = 10.0f;
    float totalW = m_activeNotes.size() * (iconSize + padding) + padding;
    float totalH = iconSize + padding * 2.0f;

    float bubbleX = anchorScreen.x - totalW * 0.5f;
    float bubbleY = anchorScreen.y - (m_noteAnchorActor->getHeight() * m_camera.getScale() * 0.5f) - totalH - 12.0f;

    // Background bubble rect
    SDL_FRect bgRect = {bubbleX, bubbleY, totalW, totalH};
    SDL_SetRenderDrawColor(renderer, 240, 240, 245, 230);
    SDL_RenderFillRect(renderer, &bgRect);

    SDL_SetRenderDrawColor(renderer, 40, 40, 45, 255);
    SDL_RenderRect(renderer, &bgRect);

    // Render sequence of note icons
    for (size_t i = 0; i < m_activeNotes.size(); ++i) {
        float ix = bubbleX + padding + i * (iconSize + padding);
        float iy = bubbleY + padding;
        Rect iconBounds(ix, iy, iconSize, iconSize);
        GlyphCache::instance().renderGlyph(m_activeNotes[i], iconBounds);
    }
}

void World::renderInventoryPod(SDL_Renderer* renderer) {
    float viewW = m_camera.getViewportWidth();

    // Inventory Toggle Button (top right)
    Rect invBtnRect(viewW - 56.0f, 16.0f, 48.0f, 48.0f);
    GlyphCache::instance().renderBlankBackground(invBtnRect, 0.85f);
    GlyphCache::instance().renderGlyph("_podInventory", invBtnRect);

    // Sliding Drawer
    if (m_inventoryAnim > 0.01f) {
        float trayW = 310.0f;
        float trayH = 180.0f;
        float currentX = viewW - (trayW + 10.0f) * m_inventoryAnim;
        float currentY = 70.0f;

        SDL_FRect trayRect = {currentX, currentY, trayW, trayH};
        SDL_SetRenderDrawColor(renderer, 30, 32, 40, (Uint8)(m_inventoryAnim * 230.0f));
        SDL_RenderFillRect(renderer, &trayRect);

        SDL_SetRenderDrawColor(renderer, 90, 100, 120, (Uint8)(m_inventoryAnim * 255.0f));
        SDL_RenderRect(renderer, &trayRect);

        // Slots grid (5 cols x 3 rows)
        int cols = 5;
        int rows = 3;
        float slotSize = 44.0f;
        float gap = 12.0f;
        for (int r = 0; r < rows; ++r) {
            for (int c = 0; c < cols; ++c) {
                float sx = currentX + 16.0f + c * (slotSize + gap);
                float sy = currentY + 16.0f + r * (slotSize + gap);
                Rect slotRect(sx, sy, slotSize, slotSize);
                GlyphCache::instance().renderGlyph("_podInventoryBlank", slotRect, 0.0f, false, false, m_inventoryAnim * 0.7f);
            }
        }

        // Render collected items inside slots
        for (size_t i = 0; i < m_inventoryItems.size(); ++i) {
            int r = (int)(i / cols);
            int c = (int)(i % cols);
            if (r >= rows) break;
            float sx = currentX + 16.0f + c * (slotSize + gap);
            float sy = currentY + 16.0f + r * (slotSize + gap);
            Rect itemRect(sx, sy, slotSize, slotSize);
            m_inventoryItems[i]->setPos({sx + slotSize * 0.5f, sy + slotSize * 0.5f});
            m_inventoryItems[i]->render(renderer, m_inventoryAnim);
        }
    }
}
