#pragma once

#include "imgui.h"
#include "World.hpp"
#include <string>

class Editor {
public:
    Editor(World& world);
    ~Editor() = default;

    bool isEnabled() const { return m_enabled; }
    void toggle() { m_enabled = !m_enabled; }
    void setEnabled(bool enabled) { m_enabled = enabled; }

    void renderUI();
    void renderWorldOverlay();

    Actor* getSelectedActor() const { return m_selectedActor; }
    void setSelectedActor(Actor* a) { m_selectedActor = a; }

private:
    void renderTopBar();
    void renderRoomsPanel();
    void renderInspectorPanel();
    void renderGlyphPalette();

    World& m_world;
    bool m_enabled = false;
    Actor* m_selectedActor = nullptr;

    // Dragging actor in editor
    bool m_isDraggingActor = false;
    Vec2 m_editorDragOffset;

    int m_currentGameIndex = 0; // 0 = Mansion, 1 = Dino
};
