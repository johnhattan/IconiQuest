# FontAwesome 5.15.4 Pro - Quick Reference

## Generated Files
- `src/FontAwesomeIcons.hpp` - Auto-generated icon mapping (2,309 icons)

## Font Files Available
- `assets/_default/fonts/Font Awesome 5 Pro-Regular-400.otf` (1.2 MB)
- `assets/_default/fonts/Font Awesome 5 Duotone-Solid-900.otf` (1.9 MB)

## Usage Example

```cpp
#include "FontAwesomeIcons.hpp"

// Method 1: Direct lookup
uint32_t codepoint = FontAwesome::iconMap.at("air-freshener"); // 0xF5D0

// Method 2: Safe lookup
auto it = FontAwesome::iconMap.find("youtube");
if (it != FontAwesome::iconMap.end()) {
    uint32_t youtubeCode = it->second; // 0xF167
}
```

## Common Icons Reference

| Icon Name | Unicode | Hex |
|-----------|---------|-----|
| air-freshener | &#xF5D0; | 0xF5D0 |
| alarm-clock | &#xF34E; | 0xF34E |
| album | &#xF89F; | 0xF89F |
| youtube | &#xF167; | 0xF167 |
| yin-yang | &#xF6AD; | 0xF6AD |

## Next: Integration Plan

See `FONTAWESOME_INTEGRATION_PLAN.md` for implementation roadmap.

The plan involves:
1. Loading FontAwesome fonts in `FontEngine`
2. Adding icon lookup in `GlyphCache`
3. Modifying render paths to support font glyphs vs SVG images
4. Maintaining backward compatibility with existing SVG system
