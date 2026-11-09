#include "pch.hpp"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include "GlyphCache.hpp"
#include "AudioEngine.hpp"
#include "World.hpp"

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

    // Repo-local fallback for source checkouts where the original project assets
    // live under org/IconiQuest0/assets and org/IconiQuest1/assets.
    roots.push_back(std::filesystem::path("org").lexically_normal());
    roots.push_back(std::filesystem::path("..").lexically_normal());

    return roots;
}

std::filesystem::path resolveGameAssetDir(const std::string& gameName) {
    const std::vector<std::pair<std::string, std::vector<std::string>>> bundleCandidates = {
        {"mansion", {"assets/mansion", "org/IconiQuest0/assets"}},
        {"dino",   {"assets/dino",   "org/IconiQuest1/assets"}},
    };

    std::filesystem::path found;
    for (const auto& [name, candidates] : bundleCandidates) {
        if (name != gameName) continue;
        for (auto& root : candidateAssetRoots()) {
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
    auto assetsDir = resolveGameAssetDir(gameName);
    return assetsDir / "maps" / "mapMain.tmx";
}
}

int main(int argc, char* argv[]) {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS)) {
        std::cerr << "Failed to init SDL3: " << SDL_GetError() << std::endl;
        return 1;
    }

    int windowWidth = 960;
    int windowHeight = 640;

    SDL_Window* window = SDL_CreateWindow(
        "IconiQuest Engine (SDL3 + ImGui)",
        windowWidth,
        windowHeight,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (!window) {
        std::cerr << "Failed to create SDL3 window: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer) {
        std::cerr << "Failed to create SDL3 renderer: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }
    SDL_SetRenderVSync(renderer, 1);

    // Setup Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    // Style tweaks for dark sleek look
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 6.0f;
    style.FrameRounding = 4.0f;
    style.PopupRounding = 4.0f;

    ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
    ImGui_ImplSDLRenderer3_Init(renderer);

    std::string defaultGame = "mansion";
    std::string selectedGame = defaultGame;
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--dino") {
            selectedGame = "dino";
        } else if (arg == "--test") {
            selectedGame = defaultGame;
        }
    }

    std::string assetsDir = resolveGameAssetDir(selectedGame).string();
    std::string mapPath = resolveGameMapPath(selectedGame).string();

    // Initialize subsystems
    GlyphCache::instance().init(renderer, (std::filesystem::path(assetsDir) / "images").string());
    AudioEngine::instance().init((std::filesystem::path(assetsDir) / "sounds").string());

    World world;
    world.getCamera().setViewportSize((float)windowWidth, (float)windowHeight);

    bool headlessTest = false;
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--test") {
            headlessTest = true;
        }
    }

    std::cout << "[IconiQuest] Loading " << mapPath << " ..." << std::endl;
    if (!world.load(mapPath, assetsDir)) {
        std::cerr << "[IconiQuest] Warning: Could not load initial map. Check assets directory." << std::endl;
        return 1;
    }

    std::cout << "[IconiQuest] Successfully loaded map with " << world.getLocations().size() << " locations!" << std::endl;
    for (const auto& loc : world.getLocations()) {
        std::cout << "  - Location '" << loc->getName() << "' with " << loc->getActors().size() << " actors. Bounds: [" 
                  << loc->getBounds().x << ", " << loc->getBounds().y << ", " << loc->getBounds().w << ", " << loc->getBounds().h << "]" << std::endl;
    }

    if (headlessTest) {
        Location* part1 = world.findLocation("part1");
        if (!part1) {
            std::cerr << "[Test Error] part1 not found in Mansion!" << std::endl;
            return 1;
        }

        Actor* table = part1->findActor("glyphTable");
        Actor* note = part1->findActor("glyphNoteInstructions");
        Actor* copy = part1->findActor("glyphCopyright");
        Actor* rug = part1->findActor("glyphRug");
        Actor* key = part1->findActor("glyphKey");

        if (!table || !note || !copy || !rug || !key) {
            std::cerr << "[Test Error] Missing actors in part1!" << std::endl;
            return 1;
        }

        // Initially: items targeted by revealers MUST be hidden!
        if (!note->isHidden()) {
            std::cerr << "[Test Error] glyphNoteInstructions should be hidden before table is clicked!" << std::endl;
            return 1;
        }
        if (!copy->isHidden()) {
            std::cerr << "[Test Error] glyphCopyright should be hidden before table is clicked!" << std::endl;
            return 1;
        }
        if (!key->isHidden()) {
            std::cerr << "[Test Error] glyphKey should be hidden before rug is clicked!" << std::endl;
            return 1;
        }

        std::cout << "[Test] Confirmed: glyphNoteInstructions, glyphCopyright, and glyphKey are initially hidden." << std::endl;

        // Click 1 on table: reveals note
        table->onTap(1);
        if (note->isHidden()) {
            std::cerr << "[Test Error] glyphNoteInstructions should be revealed after first click on table!" << std::endl;
            return 1;
        }
        std::cout << "[Test] Confirmed: glyphNoteInstructions revealed after clicking table." << std::endl;

        // Click 2 on table: reveals copyright
        table->onTap(1);
        if (copy->isHidden()) {
            std::cerr << "[Test Error] glyphCopyright should be revealed after second click on table!" << std::endl;
            return 1;
        }
        std::cout << "[Test] Confirmed: glyphCopyright revealed after second click on table." << std::endl;

        // Click rug: reveals key and shifts rug
        f32Point rugOrgPos = rug->getOrgPos();
        float deltaX = rug->props().getFloat("revealDeltaX", 0.0f);
        rug->onTap(1);
        if (key->isHidden()) {
            std::cerr << "[Test Error] glyphKey should be revealed after clicking rug!" << std::endl;
            return 1;
        }
        if (std::abs(rug->getPos().x - (rugOrgPos.x + deltaX)) > 0.01f) {
            std::cerr << "[Test Error] glyphRug should shift by revealDeltaX=" << deltaX << "!" << std::endl;
            return 1;
        }
        std::cout << "[Test] Confirmed: glyphRug shifts by " << deltaX << " and reveals glyphKey." << std::endl;

        // Click rug again: loop closes it
        rug->onTap(1);
        if (!key->isHidden()) {
            std::cerr << "[Test Error] glyphKey should be hidden after clicking rug again (revealLoop=true)!" << std::endl;
            return 1;
        }
        if (std::abs(rug->getPos().x - rugOrgPos.x) > 0.01f) {
            std::cerr << "[Test Error] glyphRug should return to org pos!" << std::endl;
            return 1;
        }
        std::cout << "[Test] Confirmed: glyphRug loops back and re-hides glyphKey." << std::endl;

        std::cout << "[IconiQuest] Testing Dino map as well..." << std::endl;
        World dinoWorld;
        std::string dinoAssetsDir = resolveGameAssetDir("dino").string();
        std::string dinoMapPath = resolveGameMapPath("dino").string();
        if (!dinoWorld.load(dinoMapPath, dinoAssetsDir)) {
            std::cerr << "[IconiQuest] Failed to load dino map!" << std::endl;
            return 1;
        }
        std::cout << "[IconiQuest] Successfully loaded Dino map with " << dinoWorld.getLocations().size() << " locations!" << std::endl;
        std::cout << "[IconiQuest] Headless test passed successfully!" << std::endl;
        return 0;
    }

    bool running = true;
    auto lastTime = std::chrono::high_resolution_clock::now();

    uint64_t lastClickTime = 0;
    const uint64_t doubleClickThresholdMs = 350;

    while (running) {
        auto now = std::chrono::high_resolution_clock::now();
        float dt = std::chrono::duration<float>(now - lastTime).count();
        lastTime = now;
        dt = std::min(dt, 0.1f); // clamp delta time for smooth stepping

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL3_ProcessEvent(&event);

            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            } else if (event.type == SDL_EVENT_KEY_DOWN) {
                // Key handling removed (editor toggle was here)
            } else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event.type == SDL_EVENT_WINDOW_RESIZED) {
                int w, h;
                SDL_GetWindowSize(window, &w, &h);
                world.getCamera().setViewportSize((float)w, (float)h);
            }

            // Game interaction if mouse not captured by ImGui
            if (!io.WantCaptureMouse) {
                if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        world.handleMouseDown({(float)event.button.x, (float)event.button.y});
                    }
                } else if (event.type == SDL_EVENT_MOUSE_BUTTON_UP) {
                    if (event.button.button == SDL_BUTTON_LEFT) {
                        uint64_t currentMs = SDL_GetTicks();
                        int taps = 1;
                        if (currentMs - lastClickTime <= doubleClickThresholdMs) {
                            taps = 2;
                            lastClickTime = 0;
                        } else {
                            lastClickTime = currentMs;
                        }

                        world.handleMouseUp({(float)event.button.x, (float)event.button.y}, taps);
                    }
                } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
                    world.handleMouseMove({(float)event.motion.x, (float)event.motion.y});
                }
            }
        }

        // Update
        world.update(dt);

        // Render World
        world.render(renderer);

        // Render ImGui UI
        ImGui_ImplSDLRenderer3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();

        ImGui::Render();
        ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);

        SDL_RenderPresent(renderer);
    }

    // Cleanup
    AudioEngine::instance().shutdown();
    GlyphCache::instance().shutdown();

    ImGui_ImplSDLRenderer3_Shutdown();
    ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
