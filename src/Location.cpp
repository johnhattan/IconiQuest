#include "pch.hxx"
#include "Location.hpp"
#include "Actor.hpp"
#include "World.hpp"

Location::Location(const std::string& name, World* world)
    : m_name(name), m_world(world) {}

Location::~Location() = default;

void Location::addActor(std::unique_ptr<Actor> actor) {
    if (actor) {
        actor->setLocation(this);
        m_actors.push_back(std::move(actor));
    }
}

Actor* Location::findActor(const std::string& name) const {
    if (name.empty()) return nullptr;

    for (const auto& a : m_actors) {
        if (a && a->getName() == name) {
            return a.get();
        }
    }

    // Case-insensitive fallback
    for (const auto& a : m_actors) {
        if (a) {
            const std::string& aname = a->getName();
            if (aname.size() == name.size() &&
                std::equal(aname.begin(), aname.end(), name.begin(), [](char c1, char c2) {
                    return std::tolower((unsigned char)c1) == std::tolower((unsigned char)c2);
                })) {
                return a.get();
            }
        }
    }

    return nullptr;
}

std::unique_ptr<Actor> Location::removeActor(Actor* target) {
    auto it = std::find_if(m_actors.begin(), m_actors.end(),
        [target](const std::unique_ptr<Actor>& a) { return a.get() == target; });

    if (it != m_actors.end()) {
        std::unique_ptr<Actor> taken = std::move(*it);
        m_actors.erase(it);
        taken->setLocation(nullptr);
        return taken;
    }
    return nullptr;
}

void Location::takeActor(Actor* actor) {
    if (!actor) return;
    Location* oldLoc = actor->getLocation();
    if (oldLoc == this) return;

    if (oldLoc) {
        std::unique_ptr<Actor> ptr = oldLoc->removeActor(actor);
        if (ptr) {
            addActor(std::move(ptr));
        }
    }
}

Color Location::getBackgroundColor() const {
    std::string hex = m_props.get("colorBackground");
    if (!hex.empty()) {
        return Color::fromHex(hex, {15, 15, 20, 255});
    }
    return {15, 15, 20, 255};
}

void Location::revert() {
    m_props.revert();
    // Two-pass revert so resetting one actor's deltas does not wipe another's onPlace state
    for (auto& a : m_actors) {
        if (a) {
            a->props().revert();
        }
    }
    for (auto& a : m_actors) {
        if (a) {
            a->onPlace();
        }
    }
}

void Location::render(SDL_Renderer* renderer, float alpha) {
    for (auto& a : m_actors) {
        if (a) {
            a->render(renderer, alpha);
        }
    }
}
