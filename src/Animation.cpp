#include "Animation.hpp"
#include "Actor.hpp"

AnimationManager& AnimationManager::instance()
{
	static AnimationManager inst;
	return inst;
}

void ActorAnimation::start(observer_ptr<Actor> pAct, const f32Point& rTarget, f32 dur, const std::string& rEasing)
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

void ActorAnimation::update(f32 deltaTime)
{
	if (!isActive || !pActor)
	{
		isActive = false;
		return;
	}

	elapsed += deltaTime;
	bool finished = elapsed >= duration;

	f32 progress = finished ? 1.0f : elapsed / duration;
	
	// Apply easing to progress (0.0 to 1.0)
	f32 easedProgress = easing(progress, 0.0f, 1.0f);
	
	f32 x = startPos.x + (targetPos.x - startPos.x) * easedProgress;
	f32 y = startPos.y + (targetPos.y - startPos.y) * easedProgress;
	pActor->setPos({ x, y });

	if (finished)
		isActive = false;
}

void AnimationManager::update(f32 deltaTime)
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

void AnimationManager::animate(observer_ptr<Actor> pActor, const f32Point& rTarget, f32 duration, const std::string& rEasing)
{
	if (!pActor)
		return;

	ActorAnimation anim;
	anim.start(pActor, rTarget, duration, rEasing);
	_animations.push_back(anim);
}

void AnimationManager::clear()
{
	_animations.clear();
}
