#pragma once

#include "pch.hpp"
#include <tweeny/tweeny.h>
#include <tweeny/detail/easing/by-name.h>

enum class PodCorner
{
	TopLeft,
	TopRight,
	BottomLeft,
	BottomRight
};

class Actor;

struct Pod
{
	PodCorner corner;
	f32 expandedWidth;
	f32 expandedHeight;
	std::string glyphName;
	std::string easingName = "linear";

	bool isExpanded = false;
	f32 animationProgress = 0.0f;
	f32 iconSize = 48.0f;
	f32 animationDuration = 0.5f;
	f32 cornerPadding = 8.0f;
	tweeny::easing::byNameEasing easing;
	
	// For inventory pod
	observer_ptr<std::vector<observer_ptr<Actor>>> pInventoryItems = nullptr;
	i32 inventoryCols = 0;
	i32 inventoryRows = 0;

	Pod(PodCorner c, f32 w, f32 h, const std::string& rGlyph, const std::string& rEasing = "linear");

	void update(f32 dt);
	void render(SDL_Renderer* pRenderer, f32 viewportWidth, f32 viewportHeight);
	bool onClick(const f32Point& rScreenPt, f32 viewportWidth, f32 viewportHeight);
	
	static f32 calculateInventoryWidth(i32 cols);
	static f32 calculateInventoryHeight(i32 rows);

private:
	f32Rectangle getPodRect(f32 viewportWidth, f32 viewportHeight) const;
	f32Point getCornerPosition(f32 viewportWidth, f32 viewportHeight) const;
};
