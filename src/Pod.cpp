#include "pch.hpp"
#include "Pod.hpp"
#include "Actor.hpp"
#include "GlyphCache.hpp"
#include "SDL3_gfxPrimitives.h"

Pod::Pod(PodCorner c, f32 w, f32 h, const std::string& rGlyph, const std::string& rEasing)
	: corner(c), expandedWidth(w), expandedHeight(h), glyphName(rGlyph), easingName(rEasing), easing(tweeny::easing::byName(rEasing))
{
}

f32 Pod::calculateInventoryWidth(i32 cols)
{
	f32 slotSize = 44.0f;
	f32 gap = 12.0f;
	f32 padding = 16.0f;
	return padding + (cols * slotSize) + ((cols - 1) * gap) + padding;
}

f32 Pod::calculateInventoryHeight(i32 rows)
{
	f32 slotSize = 44.0f;
	f32 gap = 12.0f;
	f32 padding = 16.0f;
	return padding + (rows * slotSize) + ((rows - 1) * gap) + padding;
}

void Pod::update(f32 dt)
{
	f32 targetProgress = isExpanded ? 1.0f : 0.0f;
	
	// Skip if already at target
	if (animationProgress == targetProgress)
		return;
	
	f32 direction = targetProgress > animationProgress ? 1.0f : -1.0f;
	f32 changeRate = dt / animationDuration;

	f32 newProgress = animationProgress + direction * changeRate;
	animationProgress = std::clamp(newProgress, 0.0f, 1.0f);
	
	// Lock to target when reached
	if (animationProgress == targetProgress)
		animationProgress = targetProgress;
}

void Pod::render(SDL_Renderer* pRenderer, f32 viewportWidth, f32 viewportHeight)
{
	bool canRender = pRenderer;
	if (!canRender)
		return;

	f32Rectangle podRect = getPodRect(viewportWidth, viewportHeight);

	// Draw pod background: light fill with dark outline
	i16 x1 = (i16)podRect.x;
	i16 y1 = (i16)podRect.y;
	i16 x2 = (i16)(podRect.x + podRect.w);
	i16 y2 = (i16)(podRect.y + podRect.h);
	i16 radius = 12;
	
	// Light background fill
	roundedBoxRGBA(pRenderer, x1, y1, x2, y2, radius, 220, 220, 225, 255);
	
	// Dark outline
	roundedRectangleRGBA(pRenderer, x1, y1, x2, y2, radius, 50, 50, 60, 255);

	// Draw icon at the pod's on-screen corner (icon position is relative to the pod)
	// For BottomLeft: icon is at top-right corner of pod (the part sticking out)
	// For TopRight: icon is at bottom-left corner of pod (the part sticking out)
	f32Point iconPosInPod = f32Point(0, 0);
	if (corner == PodCorner::TopLeft)
		iconPosInPod = f32Point(expandedWidth - iconSize, expandedHeight - iconSize); // Bottom-right
	else if (corner == PodCorner::TopRight)
		iconPosInPod = f32Point(0, expandedHeight - iconSize); // Bottom-left
	else if (corner == PodCorner::BottomLeft)
		iconPosInPod = f32Point(expandedWidth - iconSize, 0); // Top-right
	else
		iconPosInPod = f32Point(0, 0); // Top-left

	f32Point iconScreenPos(podRect.x + iconPosInPod.x, podRect.y + iconPosInPod.y);
	f32Rectangle iconRect(iconScreenPos.x, iconScreenPos.y, iconSize, iconSize);
	GlyphCache::instance().renderGlyph(glyphName, iconRect);
	
	// Render inventory items if this is the inventory pod and it's expanded
	if (pInventoryItems && isExpanded && animationProgress > 0.01f && inventoryCols > 0 && inventoryRows > 0)
	{
		f32 slotSize = 44.0f;
		f32 gap = 12.0f;
		f32 startX = podRect.x + 16.0f;
		f32 startY = podRect.y + 16.0f;
		
		for (i32 r = 0; r < inventoryRows; ++r)
		{
			for (i32 c = 0; c < inventoryCols; ++c)
			{
				f32 sx = startX + c * (slotSize + gap);
				f32 sy = startY + r * (slotSize + gap);
				f32Rectangle slotRect(sx, sy, slotSize, slotSize);
				GlyphCache::instance().renderGlyph("_podInventoryBlank", slotRect, 0.0f, false, false, animationProgress * 0.7f);
			}
		}
		
		for (uindex i = 0; (i < pInventoryItems->size()) && (i32)(i / inventoryCols) < inventoryRows; ++i)
		{
			i32 r = (i32)(i / inventoryCols);
			i32 c = (i32)(i % inventoryCols);
			f32 sx = startX + c * (slotSize + gap);
			f32 sy = startY + r * (slotSize + gap);
			f32Rectangle itemRect(sx, sy, slotSize, slotSize);
			(*pInventoryItems)[i]->setPos({ sx + slotSize * 0.5f, sy + slotSize * 0.5f });
			(*pInventoryItems)[i]->render(pRenderer, animationProgress);
		}
	}
}

bool Pod::onClick(const f32Point& rScreenPt, f32 viewportWidth, f32 viewportHeight)
{
	f32Rectangle podRect = getPodRect(viewportWidth, viewportHeight);

	f32Point iconPosInPod = f32Point(0, 0);
	if (corner == PodCorner::TopLeft)
		iconPosInPod = f32Point(expandedWidth - iconSize, expandedHeight - iconSize);
	else if (corner == PodCorner::TopRight)
		iconPosInPod = f32Point(0, expandedHeight - iconSize);
	else if (corner == PodCorner::BottomLeft)
		iconPosInPod = f32Point(expandedWidth - iconSize, 0);
	else
		iconPosInPod = f32Point(0, 0);

	f32Point iconScreenPos(podRect.x + iconPosInPod.x, podRect.y + iconPosInPod.y);
	f32Rectangle iconRect(iconScreenPos.x, iconScreenPos.y, iconSize, iconSize);

	bool clickedOnIcon = rScreenPt.x >= iconRect.x && rScreenPt.x < iconRect.x + iconRect.w && rScreenPt.y >= iconRect.y && rScreenPt.y < iconRect.y + iconRect.h;

	if (clickedOnIcon)
	{
		isExpanded = !isExpanded;
		return true;
	}

	bool wasExpanded = isExpanded;
	if (isExpanded)
	{
		bool clickedInPod = rScreenPt.x >= podRect.x && rScreenPt.x < podRect.x + podRect.w && rScreenPt.y >= podRect.y && rScreenPt.y < podRect.y + podRect.h;

		if (!clickedInPod)
		{
			isExpanded = false;
			return true;
		}
	}

	return wasExpanded;
}

f32Rectangle Pod::getPodRect(f32 viewportWidth, f32 viewportHeight) const
{
	f32 easedProgress = easing(animationProgress, 0.0f, 1.0f);
	f32 x = 0.0f;
	f32 y = 0.0f;

	// Calculate icon position within pod based on corner
	f32 iconPosX = 0.0f;
	f32 iconPosY = 0.0f;
	
	if (corner == PodCorner::TopLeft)
	{
		iconPosX = expandedWidth - iconSize;
		iconPosY = expandedHeight - iconSize;
	}
	else if (corner == PodCorner::TopRight)
	{
		iconPosX = 0.0f;
		iconPosY = expandedHeight - iconSize;
	}
	else if (corner == PodCorner::BottomLeft)
	{
		iconPosX = expandedWidth - iconSize;
		iconPosY = 0.0f;
	}
	else
	{
		iconPosX = 0.0f;
		iconPosY = 0.0f;
	}

	// Calculate collapsed and expanded pod positions based on corner
	if (corner == PodCorner::TopLeft)
	{
		// Icon peeks from top-left
		f32 collapsedX = cornerPadding - iconPosX;
		f32 collapsedY = cornerPadding - iconPosY;
		f32 expandedX = 0.0f;
		f32 expandedY = 0.0f;
		x = collapsedX + (expandedX - collapsedX) * easedProgress;
		y = collapsedY + (expandedY - collapsedY) * easedProgress;
	}
	else if (corner == PodCorner::TopRight)
	{
		// Icon peeks from top-right
		f32 collapsedX = viewportWidth - cornerPadding - iconSize - iconPosX;
		f32 collapsedY = cornerPadding - iconPosY;
		f32 expandedX = viewportWidth - expandedWidth;
		f32 expandedY = 0.0f;
		x = collapsedX + (expandedX - collapsedX) * easedProgress;
		y = collapsedY + (expandedY - collapsedY) * easedProgress;
	}
	else if (corner == PodCorner::BottomLeft)
	{
		// Icon peeks from bottom-left
		f32 collapsedX = cornerPadding - iconPosX;
		f32 collapsedY = viewportHeight - cornerPadding - iconSize - iconPosY;
		f32 expandedX = 0.0f;
		f32 expandedY = viewportHeight - expandedHeight;
		x = collapsedX + (expandedX - collapsedX) * easedProgress;
		y = collapsedY + (expandedY - collapsedY) * easedProgress;
	}
	else
	{
		// Icon peeks from bottom-right
		f32 collapsedX = viewportWidth - cornerPadding - iconSize - iconPosX;
		f32 collapsedY = viewportHeight - cornerPadding - iconSize - iconPosY;
		f32 expandedX = viewportWidth - expandedWidth;
		f32 expandedY = viewportHeight - expandedHeight;
		x = collapsedX + (expandedX - collapsedX) * easedProgress;
		y = collapsedY + (expandedY - collapsedY) * easedProgress;
	}

	return f32Rectangle(x, y, expandedWidth, expandedHeight);
}

f32Point Pod::getCornerPosition(f32 viewportWidth, f32 viewportHeight) const
{
	f32 x = 0.0f;
	f32 y = 0.0f;

	if (corner == PodCorner::TopLeft)
	{
		x = cornerPadding;
		y = cornerPadding;
	}
	else if (corner == PodCorner::TopRight)
	{
		x = viewportWidth - iconSize - cornerPadding;
		y = cornerPadding;
	}
	else if (corner == PodCorner::BottomLeft)
	{
		x = cornerPadding;
		y = viewportHeight - iconSize - cornerPadding;
	}
	else
	{
		x = viewportWidth - iconSize - cornerPadding;
		y = viewportHeight - iconSize - cornerPadding;
	}

	return f32Point(x, y);
}
