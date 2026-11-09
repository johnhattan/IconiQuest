# Copilot instructions for IconiQuest

This repository is a deterministic, repo-local C++ game project. Treat it as a submodule-based project, not a networked dependency build.

## Defaults
- Keep dependencies in `lib/` as git submodules and fail clearly if they are missing.
- Use repo-root `assets/` as the canonical runtime data path.
- Prefer `src/pch.hxx` for shared core types and geometry; `src/Types.hpp` is compatibility-only.
- Preserve SDL3 + ImGui + pugixml architecture and avoid broad framework churn.

## Build/test commands
```bash
git submodule update --init --recursive
cmake -S . -B build -G Ninja
cmake --build build --parallel
./build/iconiquest --test
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./build/iconiquest --test
```

## Code expectations
- Prefer RAII, value types, and explicit ownership.
- Keep map/asset resolution robust across repo root and legacy backup directories.
- Do not add build-time downloads, package install steps, or network access in CMake.
- Keep editor and gameplay logic separate and avoid coupling UI behavior to asset folder internals.

## When editing types or shared headers
- Centralize shared primitives in `src/pch.hxx`.
- Keep `Vec2`, `Rect`, and `ColorRGBA` consistent with the existing engine semantics.
- If a type is used broadly, move it into the shared core header instead of continuing to copy it into multiple files.

## Notes for generated patches
- Make surgical changes.
- Preserve runtime asset resolution and repo-local submodule assumptions.
- Validate the effect on map loading and the `--test` path before concluding the work is complete.
