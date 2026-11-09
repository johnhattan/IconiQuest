#include "pch.hxx"
#include "Actor.hpp"
#include "Location.hpp"
#include "World.hpp"
#include "GlyphCache.hpp"
#include "AudioEngine.hpp"

Actor::Actor(
    const std::string& name,
    Location* location,
    float x,
    float y,
    float width,
    float height
) : m_name(name), m_location(location), m_orgPos(x, y), m_width(width), m_height(height) {
    m_props.setOrg("name", name);
    m_props.setOrg("x", std::to_string(x));
    m_props.setOrg("y", std::to_string(y));
    m_props.setOrg("width", std::to_string(width));
    m_props.setOrg("height", std::to_string(height));
}

f32Point Actor::getPos() const {
    return {
        m_props.getFloat("x", m_orgPos.x),
        m_props.getFloat("y", m_orgPos.y)
    };
}

void Actor::setPos(const f32Point& pos) {
    m_props.setFloat("x", pos.x);
    m_props.setFloat("y", pos.y);
}

f32Rect Actor::getBounds() const {
    f32Point p = getPos();
    return {
        p.x - m_width * 0.5f,
        p.y - m_height * 0.5f,
        m_width,
        m_height
    };
}

bool Actor::isHidden() const {
    if (m_props.has("hiddenUnlessActor")) {
        std::string required = m_props.get("hiddenUnlessActor");
        if (m_location && !m_location->findActor(required)) {
            return true;
        }
    }
    return m_props.getBool("hidden", false);
}

void Actor::setHidden(bool hidden) {
    m_props.setBool("hidden", hidden);
}

bool Actor::isDraggable() const {
    return m_props.getBool("item", false) || m_props.getBool("draggable", false);
}

bool Actor::isDrawBackground() const {
    return m_props.getBool("drawBackground", true);
}

std::string Actor::getActiveGlyph() const {
    bool toggled = m_props.getBool("toggled", false);
    if (toggled) {
        if (m_props.has("glyphToggled")) return m_props.get("glyphToggled");
        return m_name + "Toggled";
    }

    if (m_props.has("glyphRevealed") && m_props.getBool("revealed", false)) {
        return m_props.get("glyphRevealed");
    }

    if (m_props.has("glyph")) {
        return m_props.get("glyph");
    }

    return m_name;
}

static bool iequals(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (std::tolower((unsigned char)a[i]) != std::tolower((unsigned char)b[i])) return false;
    }
    return true;
}

std::vector<std::string> Actor::getRevealActors() const {
    return m_props.getList("revealActor");
}

std::vector<std::string> Actor::getRevealActorsSync() const {
    return m_props.getList("revealActorSync");
}

std::vector<std::string> Actor::getAllRevealTargets() const {
    std::vector<std::string> all = getRevealActors();
    std::vector<std::string> sync = getRevealActorsSync();
    all.insert(all.end(), sync.begin(), sync.end());
    return all;
}

bool Actor::isRevealer() const {
    return !getAllRevealTargets().empty();
}

float Actor::getRotation() const {
    return m_props.getFloat("glyphRotation", 0.0f);
}

bool Actor::getFlipHoriz() const {
    return m_props.getBool("glyphFlipHoriz", false);
}

bool Actor::getFlipVert() const {
    return m_props.getBool("glyphFlipVert", false);
}

void Actor::onPlace() {
    // 1. Reveal targets update
    std::vector<std::string> allReveals = getAllRevealTargets();
    if (!allReveals.empty() && m_location) {
        float xDelta = m_props.getFloat("revealDeltaX", 0.0f);
        float yDelta = m_props.getFloat("revealDeltaY", 0.0f);
        bool revealerHide = m_props.getBool("revealerHide", false);

        bool anyRevealed = false;

        for (const auto& targetName : allReveals) {
            bool isRev = m_props.getBool(targetName + "Revealed", false);
            if (isRev) anyRevealed = true;

            // Check if all revealers in this location targeting targetName have revealed it
            bool isAllRevealed = true;
            for (const auto& otherActor : m_location->getActors()) {
                if (!otherActor) continue;
                for (const auto& otherTarget : otherActor->getAllRevealTargets()) {
                    if (iequals(otherTarget, targetName)) {
                        if (!otherActor->props().getBool(otherTarget + "Revealed", false)) {
                            isAllRevealed = false;
                            break;
                        }
                    }
                }
                if (!isAllRevealed) break;
            }

            Actor* target = m_location->findActor(targetName);
            if (target) {
                World* world = m_location->getWorld();
                if (!world || !world->isItemInInventory(target)) {
                    target->setHidden(!isAllRevealed);
                }
            }
        }

        // Apply revealer motion (revealDeltaX / revealDeltaY)
        if (xDelta != 0.0f || yDelta != 0.0f) {
            if (anyRevealed) {
                setPos({m_orgPos.x + xDelta, m_orgPos.y + yDelta});
            } else {
                setPos(m_orgPos);
            }
        }

        // Apply revealerHide
        if (revealerHide) {
            setHidden(anyRevealed);
        }

        // Mark revealer itself as revealed for glyphRevealed
        m_props.setBool("revealed", anyRevealed);
    }

    // 2. Check position offsets from mover (moveDeltaX / moveDeltaY)
    std::vector<std::string> dx = m_props.getList("moveDeltaX");
    std::vector<std::string> dy = m_props.getList("moveDeltaY");
    if (!dx.empty() && !dy.empty()) {
        int idx = m_props.getInt("moveIndex", -1);
        if (idx >= 0 && idx < (int)dx.size() && idx < (int)dy.size()) {
            setPos({m_orgPos.x + std::stof(dx[idx]), m_orgPos.y + std::stof(dy[idx])});
        } else {
            setPos(m_orgPos);
        }
    }
}

bool Actor::onTap(int numTaps) {
    if (isHidden() || !m_location) return false;
    World* world = m_location->getWorld();
    if (!world) return false;

    // 1. Relocater check
    if (m_props.has("relocaterDest")) {
        bool canRelocate = true;
        if (m_props.getBool("toggle", false)) {
            canRelocate = m_props.getBool("toggled", false);
        }

        if (canRelocate) {
            std::string dest = m_props.get("relocaterDest");
            float time = m_props.getFloat("relocaterDestTime", 0.75f);
            std::string snd = m_props.get("soundRelocate");
            if (!snd.empty()) {
                AudioEngine::instance().playSfx(snd);
            }
            world->changeLocation(dest, time);
            return true;
        }
    }

    // 2. Revealer check
    std::vector<std::string> reveals = getRevealActors();
    std::vector<std::string> revealsSync = getRevealActorsSync();
    if (!reveals.empty() || !revealsSync.empty()) {
        std::string trigger = m_props.get("revealTrigger");
        bool canReveal = trigger.empty() || m_props.getBool("revealTriggered", false);

        if (canReveal) {
            bool revealLoop = m_props.getBool("revealLoop", true);
            bool changed = false;

            // Non-sync: reveals all listed actors
            if (!reveals.empty()) {
                for (const auto& targetName : reveals) {
                    std::string key = targetName + "Revealed";
                    bool cur = m_props.getBool(key, false);
                    if (!cur || revealLoop) {
                        m_props.flipBool(key, false);
                        changed = true;
                    }
                }
            }

            // Sync reveals
            if (!revealsSync.empty()) {
                if (revealLoop) {
                    bool stateFirst = m_props.getBool(revealsSync[0] + "Revealed", false);
                    for (size_t i = 1; i < revealsSync.size(); ++i) {
                        const auto& targetName = revealsSync[i];
                        if (m_props.getBool(targetName + "Revealed", false) != stateFirst) {
                            m_props.flipBool(targetName + "Revealed", false);
                            changed = true;
                            break;
                        }
                    }
                    if (!changed) {
                        m_props.flipBool(revealsSync[0] + "Revealed", false);
                        changed = true;
                    }
                } else {
                    for (const auto& targetName : revealsSync) {
                        if (!m_props.getBool(targetName + "Revealed", false)) {
                            m_props.setBool(targetName + "Revealed", true);
                            changed = true;
                            break;
                        }
                    }
                }
            }

            if (changed) {
                std::string snd = m_props.get("soundReveal");
                if (!snd.empty()) AudioEngine::instance().playSfx(snd);
                onPlace();
                return true;
            }
        }
    }

    // 3. Mover check
    std::vector<std::string> dx = m_props.getList("moveDeltaX");
    std::vector<std::string> dy = m_props.getList("moveDeltaY");
    if (!dx.empty() && !dy.empty()) {
        int maxSteps = std::min((int)dx.size(), (int)dy.size());
        int curIdx = m_props.getInt("moveIndex", -1) + 1;
        if (curIdx >= maxSteps) {
            m_props.setInt("moveIndex", -1);
        } else {
            m_props.setInt("moveIndex", curIdx);
        }

        std::string snd = m_props.get("soundMove");
        if (!snd.empty()) AudioEngine::instance().playSfx(snd);
        onPlace();
        return true;
    }

    // 4. Toggle check
    if (m_props.getBool("toggle", false)) {
        bool canToggle = true;
        if (m_props.has("toggleTrigger") && !m_props.getBool("toggleTriggered", false)) {
            canToggle = false;
        }

        if (canToggle) {
            bool loop = m_props.getBool("toggleLoop", true);
            if (!m_props.getBool("toggled", false) || loop) {
                m_props.flipBool("toggled", false);
                std::string snd = m_props.get("soundToggle");
                if (!snd.empty()) AudioEngine::instance().playSfx(snd);
                onPlace();
                return true;
            }
        }
    }

    // 5. Notes / Dialogue check
    std::string noteSuffix = m_props.getBool("toggled", false) ? "Toggled" : "";
    std::vector<std::string> notes = m_props.getList("note" + noteSuffix);
    if (!notes.empty()) {
        std::string snd = m_props.get("soundNote" + noteSuffix);
        if (!snd.empty()) AudioEngine::instance().playSfx(snd);
        world->showNoteBubble(this, notes);
        return true;
    }

    // Tap sound fallback
    std::string tapSnd = m_props.get("soundTap");
    if (!tapSnd.empty()) {
        AudioEngine::instance().playSfx(tapSnd);
    }

    return false;
}

bool Actor::onDrop(Actor* droppedActor) {
    if (!droppedActor || droppedActor == this || !m_location) return false;

    std::string trigger = m_props.get("revealTrigger");
    if (!trigger.empty() && droppedActor->getName() == trigger) {
        m_props.setBool("revealTriggered", true);
        if (!m_props.getBool("revealTriggerKeep", false)) {
            droppedActor->setHidden(true);
        }
        onTap(1);
        return true;
    }

    std::vector<std::string> toggleTriggers = m_props.getList("toggleTrigger");
    for (const auto& trig : toggleTriggers) {
        if (droppedActor->getName() == trig) {
            m_props.setBool("toggleTriggered", true);
            if (!m_props.getBool("toggleTriggerKeep", false)) {
                droppedActor->setHidden(true);
            }
            onTap(1);
            return true;
        }
    }

    return false;
}

void Actor::render(SDL_Renderer* renderer, float alpha) {
    if (isHidden()) return;

    Rect bounds = getBounds();
    if (isDrawBackground()) {
        GlyphCache::instance().renderBlankBackground(bounds, alpha);
    }

    GlyphCache::instance().renderGlyph(
        getActiveGlyph(),
        bounds,
        getRotation(),
        getFlipHoriz(),
        getFlipVert(),
        alpha
    );
}
