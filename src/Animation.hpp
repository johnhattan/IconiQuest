#pragma once

#include "pch.hpp"
#include <tweeny/tweeny.h>
#include <tweeny/detail/easing/by-name.h>

class Actor;

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
