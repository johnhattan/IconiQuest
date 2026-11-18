#pragma once

#include "pch.hpp"
#include <tweeny/tweeny.h>
#include <tweeny/detail/easing/by-name.h>

struct Camera
{
	Camera(f32 viewportW = 800.0f, f32 viewportH = 600.0f) :
	    _logicalViewport(viewportW, viewportH),
	    _physicalViewport(viewportW, viewportH),
	    _easing(tweeny::easing::byName("cubicOut"))
	{
	}

	void setViewportSize(const f32Size& rSize)
	{
		_logicalViewport.width = rSize.width;
		_logicalViewport.height = rSize.height;
		_physicalViewport = rSize;
	}

	void setPhysicalViewportSize(const f32Size& rSize)
	{
		_physicalViewport = rSize;
	}

	f32 getViewportWidth() const { return _logicalViewport.width; }
	f32 getViewportHeight() const { return _logicalViewport.height; }
	
	f32 getPhysicalViewportWidth() const { return _physicalViewport.width; }
	f32 getPhysicalViewportHeight() const { return _physicalViewport.height; }

	void frameRect(const f32Rectangle& rTarget, f32 duration = 0.75f, const std::string& rEasing = "cubicOut")
	{
		if (rTarget.w > 0.0f && rTarget.h > 0.0f)
		{
			_startPos = _curPos;
			_startScale = _curScale;

			// Scale to fit 90% of screen with margins (use logical size for consistency)
			f32 scaleX = (_logicalViewport.width * 0.90f) / rTarget.w;
			f32 scaleY = (_logicalViewport.height * 0.90f) / rTarget.h;
			_targetScale = std::min(scaleX, scaleY);
			_targetPos = rTarget.center();

			if (duration <= 0.0f)
				instantFrame(rTarget);
			else
			{
				_easing = tweeny::easing::byName(rEasing);
				_duration = duration;
				_timer = 0.0f;
				_isTransitioning = true;
			}
		}
	}

	void instantFrame(const f32Rectangle& rTarget)
	{
		if (rTarget.w > 0.0f && rTarget.h > 0.0f)
		{
			f32 scaleX = (_logicalViewport.width * 0.90f) / rTarget.w;
			f32 scaleY = (_logicalViewport.height * 0.90f) / rTarget.h;
			_curScale = _targetScale = std::min(scaleX, scaleY);
			f32Point c = rTarget.center();
			_curPos = _targetPos = c;
			_isTransitioning = false;
		}
	}

	void update(f32 dt)
	{
		if (_isTransitioning)
		{
			_timer += dt;
			f32 t = std::min(1.0f, _timer / _duration);

			// Apply easing function
			f32 ease = _easing(t, 0.0f, 1.0f);

			_curPos.x = _startPos.x + (_targetPos.x - _startPos.x) * ease;
			_curPos.y = _startPos.y + (_targetPos.y - _startPos.y) * ease;
			_curScale = _startScale + (_targetScale - _startScale) * ease;

			if (t >= 1.0f)
			{
				_curPos = _targetPos;
				_curScale = _targetScale;
				_isTransitioning = false;
			}
		}
	}

	f32Point worldToScreen(const f32Point& rWorldPt) const
	{
		return { (rWorldPt.x - _curPos.x) * _curScale + _logicalViewport.width * 0.5f, (rWorldPt.y - _curPos.y) * _curScale + _logicalViewport.height * 0.5f };
	}

	f32Point screenToWorld(const f32Point& rScreenPt) const
	{
		return { (rScreenPt.x - _logicalViewport.width * 0.5f) / _curScale + _curPos.x, (rScreenPt.y - _logicalViewport.height * 0.5f) / _curScale + _curPos.y };
	}

	f32Rectangle worldToScreenRect(const f32Rectangle& rRect) const
	{
		f32Point tl = worldToScreen({ rRect.x, rRect.y });
		return { tl.x, tl.y, rRect.w * _curScale, rRect.h * _curScale };
	}

	f32 getScale() const { return _curScale; }
	f32Point getCenter() const { return _curPos; }
	bool isTransitioning() const { return _isTransitioning; }
	
	f32 getDpiScale() const 
	{
		if (_logicalViewport.width > 0.0f)
			return _physicalViewport.width / _logicalViewport.width;
		return 1.0f;
	}

private:
	f32Size _logicalViewport = { 800.0f, 600.0f };
	f32Size _physicalViewport = { 800.0f, 600.0f };

	f32Point _curPos;
	f32 _curScale = 1.0f;

	f32Point _startPos;
	f32 _startScale = 1.0f;

	f32Point _targetPos;
	f32 _targetScale = 1.0f;

	f32 _timer = 0.0f;
	f32 _duration = 0.75f;
	bool _isTransitioning = false;
	tweeny::easing::byNameEasing _easing;
};
