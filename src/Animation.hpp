#pragma once
/*
 * Animation
 *
 * Lightweight actor position animation system built on the Tweeny easing
 * library.  ActorAnimation represents one in-flight movement, and
 * AnimationManager owns the collection of active animation records.
 *
 * Animation callbacks are observer-based: Actor remains owned by Location,
 * and ActorAnimation may reference an Actor only while it has valid external
 * lifetime.
 */

#include "pch.hpp"
#include "Actor.hpp"
#include <tweeny/tweeny.h>
#include <tweeny/detail/easing/by-name.h>

struct ActorAnimation
{
	observer_ptr<Actor> pActor;
	f32Point startPos;
	f32Point targetPos;
	tweeny::easing::byNameEasing easing;
	f32 duration;
	f32 elapsed;
	bool isActive;

	ActorAnimation() : easing(tweeny::easing::byName("linear")), duration(0), elapsed(0), isActive(false) {}

	void start(observer_ptr<Actor> pAct, const f32Point& rTarget, f32 dur, const std::string& rEasing = "linear");
	void update(f32 deltaTime);
	bool isRunning() const { return isActive; }
};

class AnimationManager
{
public:
	static AnimationManager& instance();

	void update(f32 deltaTime);
	void animate(observer_ptr<Actor> pActor, const f32Point& rTarget, f32 duration, const std::string& rEasing = "linear");
	void clear();

private:
	AnimationManager() = default;
	~AnimationManager() = default;

	std::vector<ActorAnimation> _animations;
};

inline AnimationManager& AnimationManager::instance()
{
	static AnimationManager inst;
	return inst;
}

inline void ActorAnimation::start(observer_ptr<Actor> pAct, const f32Point& rTarget, f32 dur, const std::string& rEasing)
{
	pActor = pAct;
	targetPos = rTarget;
	duration = dur;
	elapsed = 0;
	isActive = true;

	easing = tweeny::easing::byName(rEasing);

	if (pActor)
		startPos = pActor->getPos();
}

inline void ActorAnimation::update(f32 deltaTime)
{
	if (!isActive || !pActor)
	{
		isActive = false;
	}
	else
	{
		elapsed += deltaTime;
		bool finished = elapsed >= duration;

		f32 progress = finished ? 1.0f : elapsed / duration;
		f32 easedProgress = easing(progress, 0.0f, 1.0f);

		f32 x = startPos.x + (targetPos.x - startPos.x) * easedProgress;
		f32 y = startPos.y + (targetPos.y - startPos.y) * easedProgress;
		pActor->setPos({ x, y });

		if (finished)
			isActive = false;
	}
}

inline void AnimationManager::update(f32 deltaTime)
{
	for (auto& anim : _animations)
	{
		if (anim.isActive)
			anim.update(deltaTime);
	}

	auto retVal = std::remove_if(_animations.begin(), _animations.end(),
	    [](const ActorAnimation& anim) { return !anim.isActive; });
	_animations.erase(retVal, _animations.end());
}

inline void AnimationManager::animate(observer_ptr<Actor> pActor, const f32Point& rTarget, f32 duration, const std::string& rEasing)
{
	if (pActor)
	{
		ActorAnimation anim;
		anim.start(pActor, rTarget, duration, rEasing);
		_animations.push_back(anim);
	}
}

inline void AnimationManager::clear()
{
	_animations.clear();
}
