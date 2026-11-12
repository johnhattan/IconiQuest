#pragma once

#include "pch.hpp"

struct Camera
{
	Camera(f32 viewportW = 800.0f, f32 viewportH = 600.0f) :
	    _viewport(viewportW, viewportH)
	{
	}

	void setViewportSize(const f32Size& rSize)
	{
		_viewport.width = rSize.width;
		_viewport.height = rSize.height;
	}

	f32 getViewportWidth() const { return _viewport.width; }
	f32 getViewportHeight() const { return _viewport.height; }

	void frameRect(const f32Rectangle& rTarget, f32 duration = 0.75f)
	{
		if (rTarget.w > 0.0f && rTarget.h > 0.0f)
		{
			_startPos = _curPos;
			_startScale = _curScale;

			// Scale to fit 90% of screen with margins
			f32 scaleX = (_viewport.width * 0.90f) / rTarget.w;
			f32 scaleY = (_viewport.height * 0.90f) / rTarget.h;
			_targetScale = std::min(scaleX, scaleY);
			_targetPos = rTarget.center();

			if (duration <= 0.0f)
				instantFrame(rTarget);
			else
			{
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
			f32 scaleX = (_viewport.width * 0.90f) / rTarget.w;
			f32 scaleY = (_viewport.height * 0.90f) / rTarget.h;
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

			// Smooth cubic ease out
			f32 ease = 1.0f - std::pow(1.0f - t, 3.0f);

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
		return { (rWorldPt.x - _curPos.x) * _curScale + _viewport.width * 0.5f, (rWorldPt.y - _curPos.y) * _curScale + _viewport.height * 0.5f };
	}

	f32Point screenToWorld(const f32Point& rScreenPt) const
	{
		return { (rScreenPt.x - _viewport.width * 0.5f) / _curScale + _curPos.x, (rScreenPt.y - _viewport.height * 0.5f) / _curScale + _curPos.y };
	}

	f32Rectangle worldToScreenRect(const f32Rectangle& rRect) const
	{
		f32Point tl = worldToScreen({ rRect.x, rRect.y });
		return { tl.x, tl.y, rRect.w * _curScale, rRect.h * _curScale };
	}

	f32 getScale() const { return _curScale; }
	f32Point getCenter() const { return _curPos; }
	bool isTransitioning() const { return _isTransitioning; }

private:
	f32Size _viewport = { 800.0f, 600.0f };

	f32Point _curPos;
	f32 _curScale = 1.0f;

	f32Point _startPos;
	f32 _startScale = 1.0f;

	f32Point _targetPos;
	f32 _targetScale = 1.0f;

	f32 _timer = 0.0f;
	f32 _duration = 0.75f;
	bool _isTransitioning = false;
};
