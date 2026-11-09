#pragma once

#include "pch.hxx"

class Camera {
public:
    Camera(float viewportW = 800.0f, float viewportH = 600.0f)
        : m_viewW(viewportW), m_viewH(viewportH) {}

    void setViewportSize(float w, float h) {
        m_viewW = w;
        m_viewH = h;
    }

    float getViewportWidth() const { return m_viewW; }
    float getViewportHeight() const { return m_viewH; }

    void frameRect(const f32Rect& target, float duration = 0.75f) {
        if (target.w <= 0.0f || target.h <= 0.0f) return;

        m_startPos = {m_curX, m_curY};
        m_startScale = m_curScale;

        // Scale to fit 90% of screen with margins
        float scaleX = (m_viewW * 0.90f) / target.w;
        float scaleY = (m_viewH * 0.90f) / target.h;
        m_targetScale = std::min(scaleX, scaleY);
        m_targetPos = target.center();

        if (duration <= 0.0f) {
            instantFrame(target);
        } else {
            m_duration = duration;
            m_timer = 0.0f;
            m_isTransitioning = true;
        }
    }

    void instantFrame(const f32Rect& target) {
        if (target.w <= 0.0f || target.h <= 0.0f) return;
        float scaleX = (m_viewW * 0.90f) / target.w;
        float scaleY = (m_viewH * 0.90f) / target.h;
        m_curScale = m_targetScale = std::min(scaleX, scaleY);
        f32Point c = target.center();
        m_curX = m_targetPos.x = c.x;
        m_curY = m_targetPos.y = c.y;
        m_isTransitioning = false;
    }

    void update(float dt) {
        if (!m_isTransitioning) return;

        m_timer += dt;
        float t = std::min(1.0f, m_timer / m_duration);

        // Smooth cubic ease out
        float ease = 1.0f - std::pow(1.0f - t, 3.0f);

        m_curX = m_startPos.x + (m_targetPos.x - m_startPos.x) * ease;
        m_curY = m_startPos.y + (m_targetPos.y - m_startPos.y) * ease;
        m_curScale = m_startScale + (m_targetScale - m_startScale) * ease;

        if (t >= 1.0f) {
            m_curX = m_targetPos.x;
            m_curY = m_targetPos.y;
            m_curScale = m_targetScale;
            m_isTransitioning = false;
        }
    }

    f32Point worldToScreen(const f32Point& worldPt) const {
        return {
            (worldPt.x - m_curX) * m_curScale + m_viewW * 0.5f,
            (worldPt.y - m_curY) * m_curScale + m_viewH * 0.5f
        };
    }

    f32Point screenToWorld(const f32Point& screenPt) const {
        return {
            (screenPt.x - m_viewW * 0.5f) / m_curScale + m_curX,
            (screenPt.y - m_viewH * 0.5f) / m_curScale + m_curY
        };
    }

    f32Rect worldToScreenRect(const f32Rect& r) const {
        f32Point tl = worldToScreen({r.x, r.y});
        return {tl.x, tl.y, r.w * m_curScale, r.h * m_curScale};
    }

    float getScale() const { return m_curScale; }
    f32Point getCenter() const { return {m_curX, m_curY}; }
    bool isTransitioning() const { return m_isTransitioning; }

private:
    float m_viewW = 800.0f;
    float m_viewH = 600.0f;

    float m_curX = 0.0f;
    float m_curY = 0.0f;
    float m_curScale = 1.0f;

    f32Point m_startPos;
    float m_startScale = 1.0f;

    f32Point m_targetPos;
    float m_targetScale = 1.0f;

    float m_timer = 0.0f;
    float m_duration = 0.75f;
    bool m_isTransitioning = false;
};
