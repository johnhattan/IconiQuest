# IconiQuest AI Agent Instructions

## Project overview
IconiQuest is a native C++20 game project built around SDL3, Dear ImGui, SDL_image, SDL_mixer, and pugixml. The repo is intentionally deterministic: dependencies live under `lib/` as git submodules, runtime assets live under repo-root `assets/`, and the build must not fetch code from the network at configure time.

The game architecture is centered around a minimal shared game layer in `src/`:
- `src/pch.hxx` is the canonical shared foundation for geometry, primitive aliases, and low-level types.
- `src/*.hpp` and `src/*.cpp` implement the world, editor, locations, actors, property system, and TMX/asset loading.
- `assets/mansion` and `assets/dino` are the canonical runtime data bundles; legacy `org/.../assets` paths are backup-only compatibility fallbacks and should not be treated as the primary project layout.

## Build and validation
```bash
# initialize repo-local dependencies
git submodule update --init --recursive

# configure and build
cmake -S . -B build -G Ninja
cmake --build build --parallel

# headless verification
./build/iconiquest --test

# if running in a minimal Linux environment
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/iconiquest --test
```

Rules:
- Keep dependency management repo-local. Do not add build-time fetches or remote downloads in CMake.
- Prefer `lib/` submodule checkout; if a dependency is missing, fail with a clear message rather than silently downloading code.
- Treat `assets/` as the canonical runtime folder. New map or image data should be placed there and referenced relative to that directory.

## Architecture conventions

### Shared type foundation
- `src/pch.hxx` is the shared core header for common geometry and primitive aliases.
- `Vec2`, `Rect`, and `ColorRGBA` are the central value types; keep them stable and avoid scattered duplicate definitions.
- Prefer not to duplicate type declarations across files. If a type is broadly used, move it into the shared core header instead of local copies.
- `src/Types.hpp` remains a compatibility wrapper for older includes; prefer `#include "pch.hxx"` for new code.

### Ownership and data flow
- `World` owns the active map and room graph.
- `Location` owns its actors and room-local state.
- `Actor` owns its transient runtime properties and visual state.
- Keep ownership clear and avoid deleting objects from outside their owner; prefer RAII and scoped lifetime management.

### Asset and map loading
- Resolve assets from project-root `assets/<game>` before falling back to legacy backup locations.
- Keep map path resolution resilient to source-tree and build-tree execution paths.
- Do not depend on hidden runtime assumptions such as the current working directory being the repo root.
- When adding new assets, update the asset resolution or copy step if the game requires a consistent repo-root layout.

### Rendering and UI
- SDL3 is the renderer and windowing layer; use SDL3 APIs directly for display and audio plumbing.
- Dear ImGui is used for editor/debug overlays, not for core gameplay logic.
- Keep rendering code deterministic and free from hidden state tied to the build folder.

### Coding style
- C++20, RAII, `constexpr` where practical, and `const` correctness.
- Prefer clear value semantics for small geometry types and avoid ad hoc object graphs when simple POD-like structs suffice.
- Keep public API names consistent with the codebase (`PascalCase` for types, `camelCase` for local values, `m_`/`_` naming only when already used in the file).
- Preserve repo-local dependency and asset conventions even when the code is being refactored or simplified.

## Typical workflows
- Add new engine helpers to the shared `src` layer when they are useful across the project.
- Keep game-specific logic in the relevant world/location/editor files rather than burying it into platform code.
- If changes affect map loading or editor behavior, validate with the headless test and confirm asset resolution still works from the repo root.

## Guardrails for AI-generated changes
- Never introduce automatic downloads, remote fetches, or internet access during build configuration.
- Do not move canonical assets into `build/` or a generated output folder as the primary solution.
- Keep the project portable across clones by relying on checked-out submodules and repo-local `assets/` directories.
- Prefer surgical edits with clear ownership boundaries; avoid broad rewrites unless the architecture specifically requires it.
