#include "Editor.hpp"
#include "GlyphCache.hpp"
#include "AudioEngine.hpp"
#include <SDL3/SDL.h>
#include <vector>
#include <algorithm>
#include <filesystem>

namespace {
std::vector<std::filesystem::path> candidateAssetRoots() {
    std::vector<std::filesystem::path> roots;
    roots.push_back(std::filesystem::current_path());

    const char* basePathRaw = SDL_GetBasePath();
    if (basePathRaw) {
        std::filesystem::path basePath(basePathRaw);
        roots.push_back(basePath);
        roots.push_back(basePath / "..");
    }

    roots.push_back(std::filesystem::path("org"));
    roots.push_back(std::filesystem::path(".."));
    return roots;
}

std::filesystem::path resolveGameAssetDir(const std::string& gameName) {
    const std::vector<std::pair<std::string, std::vector<std::string>>> bundleCandidates = {
        {"mansion", {"assets/mansion", "org/IconiQuest0/assets"}},
        {"dino",   {"assets/dino",   "org/IconiQuest1/assets"}},
    };

    for (const auto& [name, candidates] : bundleCandidates) {
        if (name != gameName) continue;
        for (const auto& root : candidateAssetRoots()) {
            for (const auto& candidate : candidates) {
                auto candidatePath = (root / candidate).lexically_normal();
                if (std::filesystem::exists(candidatePath / "maps" / "mapMain.tmx") &&
                    std::filesystem::exists(candidatePath / "images") &&
                    std::filesystem::exists(candidatePath / "sounds")) {
                    return candidatePath;
                }
            }
        }
    }

    return std::filesystem::path("assets") / gameName;
}

std::filesystem::path resolveGameMapPath(const std::string& gameName) {
    return resolveGameAssetDir(gameName) / "maps" / "mapMain.tmx";
}
}

Editor::Editor(World& world) : m_world(world) {}

void Editor::renderUI() {
    if (!m_enabled) return;

    renderTopBar();
    renderRoomsPanel();
    renderInspectorPanel();
    renderGlyphPalette();
    renderWorldOverlay();
}

void Editor::renderTopBar() {
    if (ImGui::BeginMainMenuBar()) {
        const char* games[] = {"Mansion (IconiQuest 0)", "Dino Island (IconiQuest 1)"};
        ImGui::SetNextItemWidth(210);
        if (ImGui::Combo("##GameSelect", &m_currentGameIndex, games, 2)) {
            std::string gameName = (m_currentGameIndex == 0) ? "mansion" : "dino";
            auto assetsDir = resolveGameAssetDir(gameName).string();
            auto mapPath = resolveGameMapPath(gameName).string();
            m_world.load(mapPath, assetsDir);
            m_selectedActor = nullptr;
        }

        ImGui::Separator();

        if (ImGui::Button("Reset Room")) {
            m_world.revertCurrentLocation();
        }
        if (ImGui::Button("Reset World")) {
            m_world.revertWorld();
        }
        if (ImGui::Button("Save TMX")) {
            m_world.save();
        }

        ImGui::Separator();

        bool sound = AudioEngine::instance().isSoundEnabled();
        if (ImGui::Checkbox("SFX", &sound)) {
            AudioEngine::instance().toggleSound();
        }
        bool music = AudioEngine::instance().isMusicEnabled();
        if (ImGui::Checkbox("Music", &music)) {
            AudioEngine::instance().toggleMusic();
        }

        ImGui::Separator();
        ImGui::TextDisabled("Toggle Editor: F1");

        ImGui::EndMainMenuBar();
    }
}

void Editor::renderRoomsPanel() {
    ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(240, 400), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Rooms", nullptr)) {
        Location* cur = m_world.getCurrentLocation();
        for (const auto& loc : m_world.getLocations()) {
            if (!loc) continue;

            bool isCur = (loc.get() == cur);
            std::string label = loc->getName();
            if (isCur) label += "  (Current)";

            if (ImGui::Selectable(label.c_str(), isCur)) {
                m_world.changeLocation(loc->getName(), 0.5f);
                m_selectedActor = nullptr;
            }
        }
    }
    ImGui::End();
}

void Editor::renderInspectorPanel() {
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x - 310, 30), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 480), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Inspector", nullptr)) {
        if (!m_selectedActor) {
            ImGui::TextDisabled("No actor selected.\nClick an actor in the viewport.");
            ImGui::End();
            return;
        }

        char nameBuf[128];
        snprintf(nameBuf, sizeof(nameBuf), "%s", m_selectedActor->getName().c_str());
        if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf))) {
            m_selectedActor->setName(nameBuf);
        }

        char glyphBuf[128];
        std::string curGlyph = m_selectedActor->props().get("glyph", m_selectedActor->getName());
        snprintf(glyphBuf, sizeof(glyphBuf), "%s", curGlyph.c_str());
        if (ImGui::InputText("Glyph", glyphBuf, sizeof(glyphBuf))) {
            m_selectedActor->props().set("glyph", glyphBuf);
        }

        // Pos & Size
        Vec2 p = m_selectedActor->getPos();
        float pos[2] = {p.x, p.y};
        if (ImGui::DragFloat2("Position", pos, 1.0f)) {
            m_selectedActor->setPos({pos[0], pos[1]});
        }

        float sz[2] = {m_selectedActor->getWidth(), m_selectedActor->getHeight()};
        if (ImGui::DragFloat2("Size", sz, 1.0f, 8.0f, 256.0f)) {
            m_selectedActor->setSize(sz[0], sz[1]);
        }

        float rot = m_selectedActor->getRotation();
        if (ImGui::SliderFloat("Rotation", &rot, -180.0f, 180.0f)) {
            m_selectedActor->props().setFloat("glyphRotation", rot);
        }

        bool flipH = m_selectedActor->getFlipHoriz();
        if (ImGui::Checkbox("Flip Horiz", &flipH)) {
            m_selectedActor->props().setBool("glyphFlipHoriz", flipH);
        }
        ImGui::SameLine();
        bool flipV = m_selectedActor->getFlipVert();
        if (ImGui::Checkbox("Flip Vert", &flipV)) {
            m_selectedActor->props().setBool("glyphFlipVert", flipV);
        }

        ImGui::Separator();

        bool hidden = m_selectedActor->isHidden();
        if (ImGui::Checkbox("Hidden", &hidden)) {
            m_selectedActor->setHidden(hidden);
        }

        bool isItem = m_selectedActor->props().getBool("item", false);
        if (ImGui::Checkbox("Is Item (Draggable)", &isItem)) {
            m_selectedActor->props().setBool("item", isItem);
        }

        bool drawBg = m_selectedActor->isDrawBackground();
        if (ImGui::Checkbox("Draw Circle BG", &drawBg)) {
            m_selectedActor->props().setBool("drawBackground", drawBg);
        }

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.4f, 1.0f), "Mechanics");

        // Relocater
        char destBuf[128];
        snprintf(destBuf, sizeof(destBuf), "%s", m_selectedActor->props().get("relocaterDest").c_str());
        if (ImGui::InputText("Dest Room", destBuf, sizeof(destBuf))) {
            m_selectedActor->props().set("relocaterDest", destBuf);
        }

        // Revealer
        char revBuf[128];
        snprintf(revBuf, sizeof(revBuf), "%s", m_selectedActor->props().get("revealActor").c_str());
        if (ImGui::InputText("Reveals Actor", revBuf, sizeof(revBuf))) {
            m_selectedActor->props().set("revealActor", revBuf);
        }

        char revSyncBuf[128];
        snprintf(revSyncBuf, sizeof(revSyncBuf), "%s", m_selectedActor->props().get("revealActorSync").c_str());
        if (ImGui::InputText("Reveals Sync", revSyncBuf, sizeof(revSyncBuf))) {
            m_selectedActor->props().set("revealActorSync", revSyncBuf);
        }

        // Toggle
        bool isToggle = m_selectedActor->props().getBool("toggle", false);
        if (ImGui::Checkbox("Is Toggle", &isToggle)) {
            m_selectedActor->props().setBool("toggle", isToggle);
        }
        if (isToggle) {
            bool toggled = m_selectedActor->props().getBool("toggled", false);
            if (ImGui::Checkbox("Toggled State", &toggled)) {
                m_selectedActor->props().setBool("toggled", toggled);
                m_selectedActor->onPlace();
            }

            char trigBuf[128];
            snprintf(trigBuf, sizeof(trigBuf), "%s", m_selectedActor->props().get("toggleTrigger").c_str());
            if (ImGui::InputText("Toggle Trigger", trigBuf, sizeof(trigBuf))) {
                m_selectedActor->props().set("toggleTrigger", trigBuf);
            }
        }

        // Sounds
        char sndBuf[128];
        snprintf(sndBuf, sizeof(sndBuf), "%s", m_selectedActor->props().get("soundTap").c_str());
        if (ImGui::InputText("Tap SFX", sndBuf, sizeof(sndBuf))) {
            m_selectedActor->props().set("soundTap", sndBuf);
        }
    }
    ImGui::End();
}

void Editor::renderGlyphPalette() {
    ImGui::SetNextWindowPos(ImVec2(10, ImGui::GetIO().DisplaySize.y - 140), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(ImGui::GetIO().DisplaySize.x - 20, 130), ImGuiCond_FirstUseEver);

    if (ImGui::Begin("Glyph Palette (Click to spawn)", nullptr)) {
        std::vector<std::string> glyphs = GlyphCache::instance().getAllGlyphs();
        float btnSize = 36.0f;
        float availW = ImGui::GetContentRegionAvail().x;
        int cols = std::max(1, (int)(availW / (btnSize + 8.0f)));

        for (size_t i = 0; i < glyphs.size(); ++i) {
            const std::string& name = glyphs[i];
            if ((i % cols) != 0) ImGui::SameLine();

            if (ImGui::Button(name.substr(0, 8).c_str(), ImVec2(btnSize, btnSize))) {
                // Add actor to current room
                Location* cur = m_world.getCurrentLocation();
                if (cur) {
                    Vec2 center = cur->getBounds().center();
                    auto actor = std::make_unique<Actor>(name, cur, center.x, center.y);
                    m_selectedActor = actor.get();
                    cur->addActor(std::move(actor));
                }
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", name.c_str());
            }
        }
    }
    ImGui::End();
}

void Editor::renderWorldOverlay() {
    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    Camera& cam = m_world.getCamera();
    Location* cur = m_world.getCurrentLocation();
    if (!cur) return;

    // 1. Draw Room Bounds
    Rect roomScreen = cam.worldToScreenRect(cur->getBounds());
    drawList->AddRect(
        ImVec2(roomScreen.x, roomScreen.y),
        ImVec2(roomScreen.x + roomScreen.w, roomScreen.y + roomScreen.h),
        IM_COL32(255, 255, 255, 80),
        4.0f,
        0,
        1.5f
    );

    // 2. Draw Actor Boxes & Links
    for (const auto& a : cur->getActors()) {
        if (!a) continue;

        Rect sb = cam.worldToScreenRect(a->getBounds());
        bool isSel = (a.get() == m_selectedActor);

        // Selection rectangle
        if (isSel) {
            drawList->AddRect(
                ImVec2(sb.x - 2, sb.y - 2),
                ImVec2(sb.x + sb.w + 2, sb.y + sb.h + 2),
                IM_COL32(255, 220, 50, 255),
                4.0f,
                0,
                2.0f
            );
        } else if (a->isHidden()) {
            drawList->AddRect(
                ImVec2(sb.x, sb.y),
                ImVec2(sb.x + sb.w, sb.y + sb.h),
                IM_COL32(180, 50, 50, 150),
                2.0f,
                0,
                1.0f
            );
        }

        // Visual Links: Revealer link (Blue)
        std::vector<std::string> revTargets = a->getAllRevealTargets();
        for (const auto& rev : revTargets) {
            Actor* target = cur->findActor(rev);
            if (target) {
                Vec2 srcPt = cam.worldToScreen(a->getPos());
                Vec2 dstPt = cam.worldToScreen(target->getPos());
                drawList->AddLine(
                    ImVec2(srcPt.x, srcPt.y),
                    ImVec2(dstPt.x, dstPt.y),
                    IM_COL32(80, 160, 255, 200),
                    2.0f
                );
            }
        }

        // Relocater label (Green badge)
        std::string dest = a->props().get("relocaterDest");
        if (!dest.empty()) {
            Vec2 p = cam.worldToScreen(a->getPos());
            drawList->AddText(
                ImVec2(p.x - 20, p.y + sb.h * 0.5f + 2),
                IM_COL32(100, 240, 120, 255),
                ("-> " + dest).c_str()
            );
        }
    }
}
