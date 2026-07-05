#pragma once
#include "physics/definitions/calculate.hpp"
#include "types.hpp"
#include <cmath>
#include <vector>

// Chenciner-Montgomery figure-8 (G=M=1), rescaled to this project's G.
inline std::vector<MassPoint> defaultFigure8Bodies() {
    constexpr double pos_x = 0.97000436;
    constexpr double pos_y = -0.24308753;
    constexpr double v1x = 0.4662036850;
    constexpr double v1y = 0.4323657300;
    constexpr double v3x = -0.9324073500;
    constexpr double v3y = -0.8647314600;

    const double v_scale = std::sqrt(Physics::G);

    return {
        {"m1", 0, 1.0, pos_x, pos_y, v1x * v_scale, v1y * v_scale},
        {"m2", 1, 1.0, -pos_x, -pos_y, v1x * v_scale, v1y * v_scale},
        {"m3", 2, 1.0, 0.0, 0.0, v3x * v_scale, v3y * v_scale},
    };
}

// Lagrange (1772): equal masses at an equilateral triangle, rotating about the center of mass.
inline std::vector<MassPoint> defaultLagrangeTriangleBodies() {
    constexpr double kPi = 3.14159265358979323846;
    constexpr double circumradius = 1.0; // AU
    const double speed = std::sqrt(Physics::G / std::sqrt(3.0));

    const double angles[] = {kPi / 2.0, kPi / 2.0 + 2.0 * kPi / 3.0, kPi / 2.0 - 2.0 * kPi / 3.0};
    std::vector<MassPoint> bodies;
    bodies.reserve(3);
    for (int i = 0; i < 3; ++i) {
        const double theta = angles[i];
        const double x = circumradius * std::cos(theta);
        const double y = circumradius * std::sin(theta);
        const double vx = -speed * std::sin(theta);
        const double vy = speed * std::cos(theta);
        bodies.push_back({"m" + std::to_string(i + 1), i, 1.0, x, y, vx, vy});
    }
    return bodies;
}

// Euler (1767): equal masses on a collinear rotating configuration.
inline std::vector<MassPoint> defaultEulerCollinearBodies() {
    constexpr double half_separation = 1.0; // AU from center to each outer body
    const double speed = std::sqrt(5.0 * Physics::G / (4.0 * half_separation));

    return {
        {"m1", 0, 1.0, -half_separation, 0.0, 0.0, -speed},
        {"m2", 1, 1.0, 0.0, 0.0, 0.0, 0.0},
        {"m3", 2, 1.0, half_separation, 0.0, 0.0, speed},
    };
}
