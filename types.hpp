#pragma once
#include <string>
#include <vector>

struct MassPoint {
    std::string name;
    int id = 0;
    double mass = 1.0; // in solar masses (1.0 = one solar mass)
    double x = 0.0;    // AU
    double y = 0.0;    // AU
    double vx = 0.0;   // km/s
    double vy = 0.0;   // km/s
};

struct SimulationToggleParams {
    bool comparison_mode = false;
    bool use_rk4 = true;
    bool use_euler = false;
    bool use_verlet = false;
};

struct SimulationParams {
    double dt = 1.0; // simulation time step in t (1 t = TIME_UNIT_YEARS yr)
    double elapsed_time = 0.0; // t since start
    std::vector<MassPoint> bodies;
    bool reset_requested = false;
    bool step_requested = false;
    bool seek_requested = false;
    double seek_target_time = 0.0;
};

struct SimulationPayload {
    std::vector<MassPoint> rk4_data;
    std::vector<MassPoint> euler_data;
    std::vector<MassPoint> verlet_data;
    double elapsed_time = 0.0;
};

struct Checkpoint {
    std::vector<MassPoint> states;
    double time = 0.0;
};
