# Copilot instructions for IconiQuest

This repository is a deterministic, repo-local C++20 game project. Treat it as a submodule-based project, not a networked dependency build.

## Key architecture
- Dependencies: git submodules in `lib/`, fail clearly if missing.
- Runtime assets: repo-root `assets/` is canonical.
- Core foundation: `src/pch.hxx` (geometry types, numeric aliases, STL headers).
- SDK: SDL3 + Dear ImGui + SDL_mixer + pugixml.

## Build and test
```bash
git submodule update --init --recursive
cmake -S . -B build -G Ninja
cmake --build build --parallel
./build/iconiquest --test
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/iconiquest --test
```

## Code rules
- **Types**: Use `Point<T>`, `Rect<T>`, `Color` from pch.hxx; prefer f32Point/i32Point for explicit intent.
- **Includes**: All .cpp and .hpp files must include `pch.hxx` first (provides all STL headers, type aliases, geometry).
- **Ownership**: RAII, explicit, non-owning refs via `observer_ptr<T>`.
- **Asset paths**: Resolve from repo-root `assets/` first; fallback to legacy locations only as compatibility.
- **No network access**: No build-time fetches, downloads, or remote code in CMake.
- **Formatting**: Use clang-format with `.clang-format` (tabs, Microsoft style, endcase support).

## When editing
- Centralize shared types in `src/pch.hxx` if used broadly; do not duplicate type defs.
- Make surgical, focused changes; preserve submodule and asset-folder assumptions.
- Validate with `./build/iconiquest --test` before concluding.
