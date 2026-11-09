# IconiQuest AI Agent Instructions

## Project overview
IconiQuest is a native C++20 game project built around SDL3, Dear ImGui, SDL_image, SDL_mixer, and pugixml. The repo is intentionally deterministic: dependencies live under `lib/` as git submodules, runtime assets live under repo-root `assets/`, and the build must not fetch code from the network at configure time.

The game architecture is centered around a minimal shared game layer in `src/`:
- `src/pch.hpp` is the canonical shared foundation for geometry, primitive aliases, and low-level types.
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
- `src/pch.hpp` is the shared core header. **All .cpp and .hpp files must `#include "pch.hpp"` as the first include.**
- **STL headers**: All standard library includes (algorithm, array, chrono, cmath, cstdint, filesystem, fstream, iostream, limits.h, memory, set, sstream, string, string_view, unordered_map, vector) are consolidated in pch.hpp. Do not repeat them locally.
- **Numeric types**: `u8/u16/u32/u64`, `i8/i16/i32/i64`, `f32/f64`, `usize` with min/max constants.
- **Geometry types**:
  - `Point<T>` — templated 2D point with arithmetic operators and conversions. Aliases: `f32Point` (primary), `i32Point`, `Vec2` (backward compat).
  - `Rect<T>` — templated rectangle with x/y/w/h. Methods: `Left()`, `Top()`, `Right()`, `Bottom()`, `Center()`, `Contains()`, `Normalized()`, `Union()`, `Intersection()`, `FromMinMax()`.
  - `Color` — 8-bit RGBA with hex parsing via `fromHex()`.
- **Utility macros**:
  - `endcase` — used in switch statements instead of break for clarity.
  - `nullfn` — alias for `delete` to mark unavailable class functions.
- **observer_ptr<T>** — non-owning smart pointer for references into externally-owned objects (World, Location, etc.); never delete through this type.
- Prefer not to duplicate type declarations across files. If a type is broadly used, move it into the shared core header instead of local copies.
- `src/Types.hpp` remains a compatibility wrapper for older includes; prefer `#include "pch.hpp"` for new code.

**Include order** (enforced):
1. `#include "pch.hpp"` — always first, provides all STL, type aliases, geometry
2. Third-party headers (`<SDL3/SDL.h>`, `<pugixml.hpp>`, etc.) — only if not in pch.hpp
3. Project headers (`#include "Actor.hpp"`, etc.) — last

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

### Coding style (adapted from BOM)
- **C++20**: Use `constexpr`, `enum class`, RAII, and `const` correctness.
- **Header/impl consistency**: Keep headers `.hpp` and implementation `.cpp` consistent with existing codebase.
- **Naming conventions**:
  - `PascalCase` for types, structs, and classes.
  - `camelCase` for local variables, function parameters, and member access functions.
  - `m_` prefix for private member variables when consistent with existing file style (prefer modern approach).
  - Use `= nullfn` (defined in `pch.hpp`) instead of `= delete` for explicitly unavailable class functions.
- **Geometry types**: Prefer value semantics for `Point<T>`, `Rect<T>`, and `Color`; avoid ad hoc object graphs.
- Prefer `f32Point` for screen/world coordinates; use `i32Point` when integer semantics are needed (e.g., tile indices).
- **Pointer ownership**: Use `observer_ptr<T>` for non-owning references into objects owned elsewhere (World, Location, etc.); document ownership when introducing new pointers.
- **Code formatting**: Use clang-format with `.clang-format` config (tabs, Microsoft style, `endcase` support).
  - Brace wrapping: After function, class, struct, namespace, union, enum; before catch/else.
  - Indentation: 4-space tabs; no column limit.
- **Function/method style**:
  - Return type on same line as function name.
  - Opening brace on same line for constructors, on next line for function bodies.
  - Prefer explicit copy/move semantics; default when appropriate via `= default`.
- **Comments**: Only comment code that needs clarification; avoid obvious comments.
- **Preserve repo-local conventions**: Never introduce automatic downloads, remote fetches, or network access during build.

**Example (C++20 struct with value semantics)**:
```cpp
struct Actor
{
	Point<f32> position;
	observer_ptr<Location> room;

	Actor() = default;
	Actor(const Point<f32>& pos, Location* r) : position(pos), room(r) {}

	auto operator<=>(const Actor&) const = default;
};
```

## Typical workflows
- Add new engine helpers to the shared `src` layer when they are useful across the project.
- Keep game-specific logic in the relevant world/location/editor files rather than burying it into platform code.
- If changes affect map loading or editor behavior, validate with the headless test and confirm asset resolution still works from the repo root.

## Guardrails for AI-generated changes
- Never introduce automatic downloads, remote fetches, or internet access during build configuration.
- Do not move canonical assets into `build/` or a generated output folder as the primary solution.
- Keep the project portable across clones by relying on checked-out submodules and repo-local `assets/` directories.
- Prefer surgical edits with clear ownership boundaries; avoid broad rewrites unless the architecture specifically requires it.
