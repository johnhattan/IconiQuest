# IconiQuest (C++ / SDL3 / Dear ImGui)

A native C++ implementation of the **IconiQuest** "Textless Text Adventure" engine featuring:
- **SDL3** Hardware-Accelerated Renderer & Sub-pixel coordinate system
- **Direct SVG Vector Glyph Rendering** via `SDL3_image`
- **Full Tiled XML (.tmx) Compatibility** via `pugixml` (loads original maps from `IconiQuest0` and `IconiQuest1`)
- **Integrated In-Engine Level Editor** powered by **Dear ImGui** (toggled with `F1`)
- **Audio Engine** powered by SDL_mixer (plays SFX and looping music themes)
- **Sliding UI Drawer ("Pod") System** for inventory management and clues
- **3-Tier Property System** with runtime deltas, instant room/world reverts, and two-way TMX saving

---

## Quick Start

### Build
```bash
cd iconiquest-cpp
# If you want repo-local dependencies under lib/, initialize submodules first.
git submodule update --init --recursive
cmake -B build -G Ninja
cmake --build build
```

Dependencies are intended to live under the `lib/` folder as git submodules:
- `lib/imgui`
- `lib/SDL3`
- `lib/SDL_image`
- `lib/SDL_mixer`
- `lib/pugixml`

If the submodules are not present yet, the project falls back to the local install prefix created by the build itself.

### Run
To play the **Haunted Mansion** (IconiQuest 0):
```bash
./build/iconiquest
```

To play the **Dinosaur Island** (IconiQuest 1):
```bash
./build/iconiquest --dino
```

To run the verification test:
```bash
./build/iconiquest --test
```

---

## Controls

### Gameplay Mode
- **Left Click**: Tap / interact with hotspots, doors, drawers, and props.
- **Double Click**: Special interactions (e.g. whistle).
- **Click & Drag**: Pick up items (e.g. keys, matches, corkscrew) and drop them onto puzzle targets or into the inventory.
- **Top-Right Bag Icon**: Open / close the sliding Inventory drawer.

### In-Engine Level Editor (`F1`)
- Press **`F1`** at any time to open/close the Dear ImGui editor overlay.
- **Game Selector**: Switch dynamically between the Mansion and Dinosaur Island.
- **Rooms Panel**: Lists all locations in the world. Click any room to fly the camera to it immediately.
- **Inspector Panel**: Click any actor to edit its name, glyph, position, size, rotation, flip, toggle triggers, revealer links, destination rooms, and sounds live.
- **Visual Links Overlay**: Shows real-time connection lines between doors, targets, and triggers.
- **Glyph Palette**: Bottom tray displaying all available SVGs. Click any glyph to add an actor to the current room.
- **Save TMX**: Saves your modifications directly back into the `.tmx` XML file, keeping Tiled 100% in sync.
