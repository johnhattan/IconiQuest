# FontAwesome Font Integration Plan

## Current Status
✅ Generated `src/FontAwesomeIcons.hpp` - Maps 2,309 icon names to unicode codepoints
✅ FontAwesome font files are available in `assets/_default/fonts/`:
  - Font Awesome 5 Pro-Regular-400.otf
  - Font Awesome 5 Duotone-Solid-900.otf

## How to Use the Icon Map
```cpp
#include "FontAwesomeIcons.hpp"

// Get the unicode codepoint for an icon
uint32_t airFreshenerCode = FontAwesome::iconMap.at("air-freshener"); // 0xF5D0

// Or with error handling
auto it = FontAwesome::iconMap.find("air-freshener");
if (it != FontAwesome::iconMap.end()) {
    uint32_t codepoint = it->second;
}
```

## Next Steps for Implementation

1. **Load FontAwesome Fonts in FontEngine**
   - Load "Font Awesome 5 Pro-Regular-400.otf" at the default size
   - Store font pointer for glyph rendering

2. **Modify GlyphCache (or create FontGlyphCache)**
   - Add method to check if a name maps to a FontAwesome icon
   - Return unicode codepoint if found, nullptr if it's an SVG glyph instead

3. **Update Actor::getActiveGlyph() rendering logic**
   - Check if the glyph name exists in `FontAwesome::iconMap`
   - If yes: render using font glyph (unicode codepoint)
   - If no: fall back to current SVG image system

4. **Rendering in GlyphCache**
   - For font glyphs: render as text using the unicode codepoint with FontAwesome font
   - Pass color/alpha parameters to text renderer
   - Maintain rotation/flip support (may need to use SDL transforms)

## Icon Name Examples
- "air-freshener" → 0xF5D0
- "alarm-clock" → 0xF34E
- "album" → 0xF89F
- "youtube" → 0xF167
- See `src/FontAwesomeIcons.hpp` for all 2,309 available icons

## Regenerating the Icon Map
If FontAwesome is updated, regenerate with:
```bash
cd /Users/johnhattan/Projects/IconiQuest
python3 << 'EOF'
# (See script in git history or above)
EOF
```

The script parses `assets/~scratch/fontawesome-pro-5.15.4-desktop/metadata/icons.yml` and generates the header.
