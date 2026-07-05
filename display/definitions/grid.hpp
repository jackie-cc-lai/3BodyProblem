#pragma once
#include "types.hpp"
#include <SFML/Graphics.hpp>
#include <vector>

namespace Display {

struct GridView {
    static constexpr float PADDING = 16.f;
    static constexpr double MIN_SCALE = 1.0;
    static constexpr double MAX_SCALE = 1.0e6;
    static constexpr double ZOOM_OUT_FACTOR = 1.5;

    float viewport_width = 0.f;
    float viewport_height = 0.f;
    double scale = 1.0;
    sf::Vector2f center{0.f, 0.f};

    void updateViewport(float width, float height);
    void setScale(double new_scale);
    static double computeFitScale(const SimulationPayload& payload, float width, float height);
    sf::Vector2f worldToScreen(double x, double y) const;
    void draw(sf::RenderTarget& target) const;
};

}
