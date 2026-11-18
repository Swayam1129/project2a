#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "MathUtil.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

namespace ECSEngine {

// Owns the SFML window and the camera transform between world coordinates and window pixels.
// GetWindow() returns a shared_ptr; the window lives as long as the WindowManager (or longer, if
// a caller keeps a copy of the pointer). All Rect/Point2D results are returned by value.
class WindowManager {
public:
    WindowManager(unsigned int width, unsigned int height, const std::string& title);
    ~WindowManager() = default;
    std::shared_ptr<sf::RenderWindow> GetWindow() const;
    Rect GetWindowBounds() const;

    void SetCamera(const Point2D& worldPt, const Point2D& screenPt);
    void SetWorldCenter(const Point2D& worldCenter);
    void SetWorldRect(const Rect& worldCameraRect);
    void SetWorldScale(float worldUnitsPerPixel);
    Point2D GetWorldCenter() const;
    float GetWorldScale() const;

    float WindowToWorld(float x) const;
    float WorldToWindow(float x) const;
    Rect WindowToWorld(const Rect& rect) const;
    Rect WorldToWindow(const Rect& rect) const;
    Point2D WorldToWindow(const Point2D& pt) const;
    Point2D WindowToWorld(const Point2D& pt) const;

    void DebugDrawRect(const Rect& r, int red, int green, int blue, int alpha = 255);
    void DebugFrameRect(const Rect& r, int red, int green, int blue, int alpha = 255);

private:
    Point2D WindowCenter() const;  // center of the window in pixels

    std::shared_ptr<sf::RenderWindow> mWindow;
    Point2D mWorldCenter;        // world point shown at the center of the window
    float mWorldUnitsPerPixel;   // how many world units one pixel covers (2 = zoomed out)
};

}  // namespace ECSEngine
