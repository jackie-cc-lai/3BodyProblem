#include "display/definitions/grid.hpp"
#include <algorithm>
#include <cmath>

namespace Display {

void GridView::updateViewport(float width, float height) {
    viewport_width = width;
    viewport_height = height;
    center = {width * 0.5f, height * 0.5f};
}

void GridView::setScale(double new_scale) {
    scale = std::max(MIN_SCALE, std::min(MAX_SCALE, new_scale));
}

double GridView::computeFitScale(const SimulationPayload& payload, float width, float height) {
    double min_x = 0.0;
    double max_x = 0.0;
    double min_y = 0.0;
    double max_y = 0.0;
    bool has_point = false;

    auto consider = [&](const std::vector<MassPoint>& bodies) {
        for (const auto& body : bodies) {
            if (!has_point) {
                min_x = max_x = body.x;
                min_y = max_y = body.y;
                has_point = true;
            } else {
                min_x = std::min(min_x, body.x);
                max_x = std::max(max_x, body.x);
                min_y = std::min(min_y, body.y);
                max_y = std::max(max_y, body.y);
            }
        }
    };

    consider(payload.rk4_data);
    consider(payload.euler_data);
    consider(payload.verlet_data);

    if (!has_point) {
        min_x = -1.0;
        max_x = 1.0;
        min_y = -1.0;
        max_y = 1.0;
    }

    const double span_x = std::max(max_x - min_x, 0.1);
    const double span_y = std::max(max_y - min_y, 0.1);
    const double usable_w = std::max(static_cast<double>(width) - 2.0 * PADDING, 1.0);
    const double usable_h = std::max(static_cast<double>(height) - 2.0 * PADDING, 1.0);

    double fit_scale = std::min(usable_w / span_x, usable_h / span_y);
    fit_scale /= ZOOM_OUT_FACTOR;
    return std::max(MIN_SCALE, std::min(MAX_SCALE, fit_scale));
}

sf::Vector2f GridView::worldToScreen(double x, double y) const {
    return {
        center.x + static_cast<float>(x * scale),
        center.y - static_cast<float>(y * scale)
    };
}

void GridView::draw(sf::RenderTarget& target) const {
    const sf::Color grid_color(60, 60, 70);
    const sf::Color axis_color(120, 120, 140);

    const double world_span = std::max(
        static_cast<double>(viewport_width),
        static_cast<double>(viewport_height)) / scale;

    const double log_arg = world_span / 8.0;
    const double step = (log_arg > 0.0)
        ? std::pow(10.0, std::floor(std::log10(log_arg)))
        : 0.1;
    if (step <= 0.0) {
        return;
    }
    const double start = -std::ceil(world_span / step) * step;
    const double end = std::ceil(world_span / step) * step;

    for (double wx = start; wx <= end; wx += step) {
        const auto top = worldToScreen(wx, end);
        const auto bottom = worldToScreen(wx, -end);
        sf::Vertex line[] = {
            {top, grid_color},
            {bottom, grid_color}
        };
        target.draw(line, 2, sf::Lines);
    }

    for (double wy = start; wy <= end; wy += step) {
        const auto left = worldToScreen(-end, wy);
        const auto right = worldToScreen(end, wy);
        sf::Vertex line[] = {
            {left, grid_color},
            {right, grid_color}
        };
        target.draw(line, 2, sf::Lines);
    }

    const auto x_axis_left = worldToScreen(-end, 0.0);
    const auto x_axis_right = worldToScreen(end, 0.0);
    sf::Vertex x_axis[] = {
        {x_axis_left, axis_color},
        {x_axis_right, axis_color}
    };
    target.draw(x_axis, 2, sf::Lines);

    const auto y_axis_bottom = worldToScreen(0.0, -end);
    const auto y_axis_top = worldToScreen(0.0, end);
    sf::Vertex y_axis[] = {
        {y_axis_bottom, axis_color},
        {y_axis_top, axis_color}
    };
    target.draw(y_axis, 2, sf::Lines);
}

}
