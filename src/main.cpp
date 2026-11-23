#include "pch.hpp"
#include "Animation.hpp"
#include "AudioEngine.hpp"
#include "FontEngine.hpp"
#include "GlyphCache.hpp"
#include "World.hpp"
#include "imgui.h"
#include "imgui_impl_sdl3.h"
#include "imgui_impl_sdlrenderer3.h"
#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>

namespace
{
	f32 resolveDpiScale(SDL_Window* pWindow, SDL_Renderer* pRenderer)
	{
		f32 retVal = 1.0f;
		if (pWindow && pRenderer)
		{
			i32 windowW = 0;
			i32 windowH = 0;
			i32 outputW = 0;
			i32 outputH = 0;
			SDL_GetWindowSize(pWindow, &windowW, &windowH);
			SDL_GetCurrentRenderOutputSize(pRenderer, &outputW, &outputH);
			if (windowW > 0 && windowH > 0)
			{
				f32 scaleX = (f32)outputW / (f32)windowW;
				f32 scaleY = (f32)outputH / (f32)windowH;
				retVal = std::max(1.0f, std::min(scaleX, scaleY));
			}
		}
		return retVal;
	}

	void applyImGuiDpiScale(f32 dpiScale)
	{
		static f32 appliedScale = 1.0f;
		f32 clampedScale = std::clamp(dpiScale, 1.0f, 4.0f);
		if (std::abs(clampedScale - appliedScale) > 0.01f)
		{
			f32 ratio = clampedScale / appliedScale;
			ImGui::GetStyle().ScaleAllSizes(ratio);
			ImGui::GetIO().FontGlobalScale = clampedScale;
			appliedScale = clampedScale;
		}
	}

	std::vector<std::filesystem::path> candidateAssetRoots()
	{
		std::vector<std::filesystem::path> roots;

		roots.push_back(std::filesystem::current_path());

		const char* basePathRaw = SDL_GetBasePath();
		if (basePathRaw)
		{
			std::filesystem::path basePath(basePathRaw);
			roots.push_back(basePath);
			roots.push_back(basePath / "..");
		}

		roots.push_back(std::filesystem::path("org").lexically_normal());
		roots.push_back(std::filesystem::path("..").lexically_normal());

		return roots;
	}

	std::filesystem::path resolveAssetsRoot()
	{
		std::filesystem::path retVal("assets");
		const auto& roots = candidateAssetRoots();
		for (usize i = 0; (i < roots.size()) && retVal == std::filesystem::path("assets"); ++i)
		{
			auto& root = roots[i];
			auto assetsPath = (root / "assets").lexically_normal();
			if (std::filesystem::exists(assetsPath / "_default"))
				retVal = assetsPath;
		}
		return retVal;
	}

	std::filesystem::path resolveMapPath(const std::string& gameName)
	{
		auto assetsRoot = resolveAssetsRoot();
		std::string mapFileName = "_map.xml";
		std::filesystem::path retVal;

		if (gameName.empty() || gameName == "_default")
			retVal = assetsRoot / "_default" / mapFileName;
		else
			retVal = assetsRoot / gameName / mapFileName;

		return retVal;
	}

	std::string getDefaultFontPath()
	{
		auto assetsRoot = resolveAssetsRoot();
		return (assetsRoot / "_default" / "fonts" / "SourceSansPro-Black.ttf").string();
	}

	void renderPropertyTable(const char* rLabel, const PropertyDictionary& rProps)
	{
		if (ImGui::CollapsingHeader(rLabel, ImGuiTreeNodeFlags_DefaultOpen))
		{
			if (ImGui::BeginTable(rLabel, 3, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY))
			{
				ImGui::TableSetupColumn("Property");
				ImGui::TableSetupColumn("Value");
				ImGui::TableSetupColumn("State");
				ImGui::TableHeadersRow();

				for (const std::string& rKey : rProps.allKeys())
				{
					ImGui::TableNextRow();
					ImGui::TableSetColumnIndex(0);
					ImGui::TextUnformatted(rKey.c_str());
					ImGui::TableSetColumnIndex(1);
					std::string value = rProps.get(rKey);
					ImGui::TextUnformatted(value.c_str());
					ImGui::TableSetColumnIndex(2);
					bool hasDelta = rProps.getDeltas().find(rKey) != rProps.getDeltas().end();
					ImGui::TextUnformatted(hasDelta ? "modified" : "original");
				}

				ImGui::EndTable();
			}
		}
	}

	void renderActorNode(observer_ptr<Actor> pActor, observer_ptr<Actor>& rSelectedActor)
	{
		if (pActor)
		{
			std::string label = pActor->getName();
			if (pActor->isHidden())
				label += " [hidden]";

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
			if (rSelectedActor == pActor)
				flags |= ImGuiTreeNodeFlags_Selected;

			if (ImGui::TreeNodeEx(pActor.get(), flags, "%s", label.c_str()))
			{
				if (ImGui::SmallButton("Select"))
					rSelectedActor = pActor;

				ImGui::Text("Position: %.1f, %.1f", pActor->getPos().x, pActor->getPos().y);
				ImGui::Text("Size: %.1f x %.1f", pActor->getWidth(), pActor->getHeight());
				ImGui::Text("Draggable: %s", pActor->isDraggable() ? "yes" : "no");
				ImGui::Text("Glyph: %s", pActor->getActiveGlyph().c_str());

				renderPropertyTable("Properties", pActor->props);
				ImGui::TreePop();
			}
		}
	}

	void renderDebugWindow(World& rWorld, bool& rVisible)
	{
		static observer_ptr<Actor> selectedActor = nullptr;

		ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2(520.0f, 560.0f), ImGuiCond_Always);
		ImGui::SetNextWindowCollapsed(false, ImGuiCond_Always);
		if (ImGui::Begin("Debug", &rVisible, ImGuiWindowFlags_None))
		{
			observer_ptr<Location> pLocation = rWorld.getCurrentLocation();
			if (pLocation)
			{
				ImGui::Text("Current Room: %s", pLocation->getName().c_str());
				renderPropertyTable("World Properties", rWorld.props);

				if (ImGui::CollapsingHeader("Actors in Current Room", ImGuiTreeNodeFlags_DefaultOpen))
				{
					for (const auto& pActor : pLocation->getActors())
						renderActorNode(observer_ptr<Actor>(pActor.get()), selectedActor);
				}
			}
			else
			{
				ImGui::TextUnformatted("Current Room: none");
			}

			if (ImGui::CollapsingHeader("Inventory", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Text("Inventory Open: %s", rWorld.isInventoryOpen() ? "yes" : "no");
				for (usize i = 0; i < rWorld.inventoryItems.size(); ++i)
				{
					observer_ptr<Actor> pActor = rWorld.inventoryItems[i];
					if (pActor)
					{
						std::string slotLabel = "Slot " + std::to_string(i);
						if (ImGui::TreeNode(slotLabel.c_str()))
						{
							renderActorNode(pActor, selectedActor);
							ImGui::TreePop();
						}
					}
					else
					{
						ImGui::TextDisabled("Slot %zu: empty", i);
					}
				}
			}

			if (selectedActor)
			{
				ImGui::Separator();
				ImGui::Text("Selected Actor: %s", selectedActor->getName().c_str());
				renderPropertyTable("Selected Properties", selectedActor->props);
			}
		}
		ImGui::End();
	}
} // namespace

i32 main(i32 argc, char* argv[])
{
	i32 retVal = 0;

	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_EVENTS))
	{
		std::cerr << "Failed to init SDL3: " << SDL_GetError() << std::endl;
		retVal = 1;
	}
	else
	{
		i32 windowWidth = 800;
		i32 windowHeight = 600;

		SDL_Window* window = SDL_CreateWindow(
		    "IconiQuest Engine (SDL3 + ImGui)",
		    windowWidth,
		    windowHeight,
		    SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);

		if (!window)
		{
			std::cerr << "Failed to create SDL3 window: " << SDL_GetError() << std::endl;
			retVal = 1;
		}
		else
		{
			// Try to set mouse focus
			SDL_RaiseWindow(window);
			SDL_SetWindowMouseGrab(window, false);  // Don't confine mouse, just track it
			
			SDL_Renderer* renderer = SDL_CreateRenderer(window, NULL);
			if (!renderer)
			{
				std::cerr << "Failed to create SDL3 renderer: " << SDL_GetError() << std::endl;
				retVal = 1;
			}
			else
			{
				SDL_SetRenderVSync(renderer, 1);

				IMGUI_CHECKVERSION();
				ImGui::CreateContext();
				ImGuiIO& io = ImGui::GetIO();
				io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
				ImGui::StyleColorsDark();

				ImGuiStyle& style = ImGui::GetStyle();
				style.WindowRounding = 6.0f;
				style.FrameRounding = 4.0f;
				style.PopupRounding = 4.0f;

				ImGui_ImplSDL3_InitForSDLRenderer(window, renderer);
				ImGui_ImplSDLRenderer3_Init(renderer);
				applyImGuiDpiScale(resolveDpiScale(window, renderer));

				std::string defaultGame = "_default";
				std::string currentGame = defaultGame;
				bool headlessTest = false;
				bool debugVisible = false;
				for (i32 i = 1; i < argc; ++i)
				{
					std::string arg = argv[i];
					if (arg == "--mansion")
					{
						currentGame = "mansion";
					}
					else if (arg == "--dino")
					{
						currentGame = "dino";
					}
					else if (arg == "--debug")
					{
						debugVisible = true;
					}
					else if (arg == "--test")
					{
						headlessTest = true;
						currentGame = "mansion";
					}
				}

				auto assetsRoot = resolveAssetsRoot();
				std::string defaultFontPath = getDefaultFontPath();

				GlyphCache::instance().init(renderer, (assetsRoot / "_default" / "images").string());
				GlyphCache::instance().setGameAssetsRoot(assetsRoot.string());
				GlyphCache::instance().setCurrentGame(currentGame);
				AudioEngine::instance().init((assetsRoot / "_default" / "sounds").string());
				AudioEngine::instance().setGameAssetsRoot(assetsRoot.string());
				AudioEngine::instance().setCurrentGame(currentGame);
				FontEngine::instance().init(defaultFontPath, 16);

				World world;
				
				// Keep viewport at logical size for game logic and input
				world.getCamera().setViewportSize(f32Size((f32)windowWidth, (f32)windowHeight));
				
				// But track the physical size for rendering operations
				i32 renderWidth = windowWidth, renderHeight = windowHeight;
				SDL_GetCurrentRenderOutputSize(renderer, &renderWidth, &renderHeight);
				world.getCamera().setPhysicalViewportSize(f32Size((f32)renderWidth, (f32)renderHeight));
				
				world.setGameAssetsRoot(assetsRoot.string());
				world.setCurrentGame(currentGame);

				std::string mapPath = resolveMapPath(currentGame).string();
				std::cout << "[IconiQuest] Loading " << mapPath << " ..." << std::endl;
				if (!world.load(mapPath, assetsRoot.string()))
				{
					std::cerr << "[IconiQuest] Warning: Could not load initial map. Check assets directory." << std::endl;
					retVal = 1;
				}
				else
				{
					std::cout << "[IconiQuest] Successfully loaded map with " << world.getLocations().size() << " locations!" << std::endl;
					for (const auto& loc : world.getLocations())
					{
						std::cout << "  - Location '" << loc->getName() << "' with " << loc->getActors().size() << " actors. Bounds: ["
						          << loc->getBounds().x << ", " << loc->getBounds().y << ", " << loc->getBounds().w << ", " << loc->getBounds().h << "]" << std::endl;
					}

					if (headlessTest)
					{
						observer_ptr<Location> foyer = world.findLocation("foyer");
						observer_ptr<Location> startupLocation = world.getCurrentLocation();
						if (!foyer)
						{
							std::cerr << "[Test Error] foyer location not found in mansion XML." << std::endl;
							retVal = 1;
						}
						else if (!startupLocation || startupLocation != foyer)
						{
							std::cerr << "[Test Error] Startup location should be foyer." << std::endl;
							retVal = 1;
						}
						else
						{
							observer_ptr<Actor> foyerTable = foyer->findActor("foyer_table");
							observer_ptr<Actor> foyerBook = foyer->findActor("foyer_book");
							observer_ptr<Actor> foyerWindup = foyer->findActor("foyer_windup");
							if (!foyerTable || !foyerBook || !foyerWindup)
							{
								std::cerr << "[Test Error] Missing expected mansion entities (foyer_table/book/windup)." << std::endl;
								retVal = 1;
							}
							else if (!foyerBook->isHidden() || !foyerWindup->isHidden())
							{
								std::cerr << "[Test Error] foyer_book and foyer_windup should be hidden on startup." << std::endl;
								retVal = 1;
							}
							else
							{
								std::cout << "[IconiQuest] Headless test passed: XML startup location and nested actors are valid." << std::endl;
								retVal = 0;
							}
						}
					}
					else
					{
						bool running = true;
						auto lastTime = std::chrono::high_resolution_clock::now();

						u64 lastClickTime = 0;
						const u64 doubleClickThresholdMs = 350;
						// Track mouse button state from previous frame for polling-based detection
						SDL_MouseButtonFlags prevMouseState = 0;
						f32 prevMouseX = 0.0f;
						f32 prevMouseY = 0.0f;

						while (running)
						{
							auto now = std::chrono::high_resolution_clock::now();
							f32 dt = std::chrono::duration<f32>(now - lastTime).count();
							lastTime = now;
							dt = std::min(dt, 0.1f);

							SDL_Event event;
							while (SDL_PollEvent(&event))
							{
								SDL_Event imguiEvent = event;
								SDL_ConvertEventToRenderCoordinates(renderer, &imguiEvent);
								ImGui_ImplSDL3_ProcessEvent(&imguiEvent);

								if (event.type == SDL_EVENT_QUIT)
									running = false;
								else if (event.type == SDL_EVENT_KEY_DOWN)
								{
									if (event.key.key == SDLK_GRAVE && !event.key.repeat)
										debugVisible = !debugVisible;
								}
								else if (event.type == SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED || event.type == SDL_EVENT_WINDOW_RESIZED)
								{
									i32 w, h;
									SDL_GetWindowSize(window, &w, &h);
									world.getCamera().setViewportSize(f32Size((f32)w, (f32)h));
									
									i32 renderWidth = w, renderHeight = h;
									SDL_GetCurrentRenderOutputSize(renderer, &renderWidth, &renderHeight);
									world.getCamera().setPhysicalViewportSize(f32Size((f32)renderWidth, (f32)renderHeight));
									applyImGuiDpiScale(resolveDpiScale(window, renderer));
								}

								if (!io.WantCaptureMouse && event.type == SDL_EVENT_FINGER_DOWN)
								{
									i32 viewWidth, viewHeight;
									SDL_GetWindowSize(window, &viewWidth, &viewHeight);
									world.handleMouseDown({ event.tfinger.x * viewWidth, event.tfinger.y * viewHeight });
								}
								else if (!io.WantCaptureMouse && event.type == SDL_EVENT_FINGER_UP)
								{
									i32 viewWidth, viewHeight;
									SDL_GetWindowSize(window, &viewWidth, &viewHeight);
									u64 currentMs = SDL_GetTicks();
									i32 taps = 1;
									if (currentMs - lastClickTime <= doubleClickThresholdMs)
									{
										taps = 2;
										lastClickTime = 0;
									}
									else
										lastClickTime = currentMs;
									world.handleMouseUp({ event.tfinger.x * viewWidth, event.tfinger.y * viewHeight }, taps);
								}
							}

							// Fallback: Poll mouse button state directly in case events aren't being generated
							float currMouseX, currMouseY;
							SDL_MouseButtonFlags currMouseState = SDL_GetMouseState(&currMouseX, &currMouseY);

							SDL_MouseButtonFlags leftButtonPressed = currMouseState & SDL_BUTTON_LMASK;
							SDL_MouseButtonFlags leftButtonWasPressed = prevMouseState & SDL_BUTTON_LMASK;
							
							if (!io.WantCaptureMouse)
							{
								if (currMouseX != prevMouseX || currMouseY != prevMouseY)
									world.handleMouseMove({ currMouseX, currMouseY });

								if (leftButtonPressed && !leftButtonWasPressed)
									world.handleMouseDown({ currMouseX, currMouseY });
								else if (!leftButtonPressed && leftButtonWasPressed)
								{
									u64 currentMs = SDL_GetTicks();
									i32 taps = 1;
									if (currentMs - lastClickTime <= doubleClickThresholdMs)
									{
										taps = 2;
										lastClickTime = 0;
									}
									else
										lastClickTime = currentMs;
									world.handleMouseUp({ currMouseX, currMouseY }, taps);
								}
							}

							prevMouseState = currMouseState;
							prevMouseX = currMouseX;
							prevMouseY = currMouseY;

							AnimationManager::instance().update(dt);
							world.update(dt);

							world.render(renderer);

							ImGui_ImplSDLRenderer3_NewFrame();
							ImGui_ImplSDL3_NewFrame();
							i32 imguiRenderW = 0;
							i32 imguiRenderH = 0;
							SDL_GetCurrentRenderOutputSize(renderer, &imguiRenderW, &imguiRenderH);
							io.DisplaySize = ImVec2((f32)imguiRenderW, (f32)imguiRenderH);
							io.DisplayFramebufferScale = ImVec2(1.0f, 1.0f);
							ImGui::NewFrame();
							if (debugVisible)
								renderDebugWindow(world, debugVisible);

							ImGui::Render();
							ImGui_ImplSDLRenderer3_RenderDrawData(ImGui::GetDrawData(), renderer);
							SDL_RenderPresent(renderer);
						}

						retVal = 0;
					}
				}

				AudioEngine::instance().shutdown();
				GlyphCache::instance().shutdown();
				FontEngine::instance().shutdown();

				ImGui_ImplSDLRenderer3_Shutdown();
				ImGui_ImplSDL3_Shutdown();
				ImGui::DestroyContext();

				SDL_DestroyRenderer(renderer);
			}

			SDL_DestroyWindow(window);
		}

		SDL_Quit();
	}

	return retVal;
}
