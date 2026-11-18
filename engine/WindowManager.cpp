#include "WindowManager.h"

namespace ECSEngine {

WindowManager::WindowManager(unsigned int width, unsigned int height, const std::string& title)
    : mWindow(std::make_shared<sf::RenderWindow>(sf::VideoMode({width, height}), title)),
      mWorldCenter(width / 2.0f, height / 2.0f),
      mWorldUnitsPerPixel(1.0f) {
    mWindow->setFramerateLimit(60);
}

std::shared_ptr<sf::RenderWindow> WindowManager::GetWindow() const { return mWindow; }

Rect WindowManager::GetWindowBounds() const {
    sf::Vector2u size = mWindow->getSize();
    return Rect{Point2D(0.0f, 0.0f), static_cast<float>(size.x), static_cast<float>(size.y)};
}

Point2D WindowManager::WindowCenter() const {
    sf::Vector2u size = mWindow->getSize();
    return Point2D(size.x / 2.0f, size.y / 2.0f);
}

// ---------- conversions ----------

// A length (not a position): 10 world units -> how many pixels?
float WindowManager::WorldToWindow(float x) const {
    return x / mWorldUnitsPerPixel;
}

// A length: 10 pixels -> how many world units?
float WindowManager::WindowToWorld(float x) const {
    return x * mWorldUnitsPerPixel;
}

Point2D WindowManager::WorldToWindow(const Point2D& pt) const {
    Point2D c = WindowCenter();
    float x = (pt.x - mWorldCenter.x) / mWorldUnitsPerPixel + c.x;
    float y = (pt.y - mWorldCenter.y) / mWorldUnitsPerPixel + c.y;
    return Point2D(x, y);
}

Point2D WindowManager::WindowToWorld(const Point2D& pt) const {
    Point2D c = WindowCenter();
    float x = (pt.x - c.x) * mWorldUnitsPerPixel + mWorldCenter.x;
    float y = (pt.y - c.y) * mWorldUnitsPerPixel + mWorldCenter.y;
    return Point2D(x, y);
}

Rect WindowManager::WorldToWindow(const Rect& rect) const {
    return Rect{WorldToWindow(rect.topLeft), WorldToWindow(rect.width), WorldToWindow(rect.height)};
}

Rect WindowManager::WindowToWorld(const Rect& rect) const {
    return Rect{WindowToWorld(rect.topLeft), WindowToWorld(rect.width), WindowToWorld(rect.height)};
}

// ---------- camera setters / getters ----------

void WindowManager::SetCamera(const Point2D& worldPt, const Point2D& screenPt) {
    Point2D c = WindowCenter();
    mWorldCenter = Point2D(worldPt.x - (screenPt.x - c.x) * mWorldUnitsPerPixel,
                           worldPt.y - (screenPt.y - c.y) * mWorldUnitsPerPixel);
}

void WindowManager::SetWorldCenter(const Point2D& worldCenter) { mWorldCenter = worldCenter; }

void WindowManager::SetWorldScale(float worldUnitsPerPixel) { mWorldUnitsPerPixel = worldUnitsPerPixel; }

void WindowManager::SetWorldRect(const Rect& worldCameraRect) {
    sf::Vector2u size = mWindow->getSize();
    float sx = worldCameraRect.width / size.x;
    float sy = worldCameraRect.height / size.y;
    mWorldUnitsPerPixel = (sx > sy) ? sx : sy;  // fit the whole rect on screen
    mWorldCenter = Point2D(worldCameraRect.topLeft.x + worldCameraRect.width / 2.0f,
                           worldCameraRect.topLeft.y + worldCameraRect.height / 2.0f);
}

Point2D WindowManager::GetWorldCenter() const { return mWorldCenter; }

float WindowManager::GetWorldScale() const { return mWorldUnitsPerPixel; }

// ---------- debug drawing (r is in world coordinates) ----------

void WindowManager::DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha) {
    Rect w = WorldToWindow(r);
    sf::RectangleShape shape({w.width, w.height});
    shape.setPosition({w.topLeft.x, w.topLeft.y});
    shape.setFillColor(sf::Color(red, green, blue, alpha));
    mWindow->draw(shape);
}

void WindowManager::DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha) {
    Rect w = WorldToWindow(r);
    sf::RectangleShape shape({w.width, w.height});
    shape.setPosition({w.topLeft.x, w.topLeft.y});
    shape.setFillColor(sf::Color::Transparent);
    shape.setOutlineColor(sf::Color(red, green, blue, alpha));
    shape.setOutlineThickness(1.0f);
    mWindow->draw(shape);
}

}  // namespace ECSEngine
