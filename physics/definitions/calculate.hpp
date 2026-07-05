#pragma once
#include "types.hpp"
#include <vector>

namespace Physics {

// Natural units: AU, km/s, M_sun, internal time (1 unit = TIME_UNIT_YEARS).
extern const double G;
extern const double TIME_UNIT_YEARS;
extern const double EPSILON;
extern const double CHECKPOINT_INTERVAL_YEARS;

double yearsToInternalTime(double years);
double internalTimeToYears(double internal_time);

std::vector<MassPoint> integrateEuler(const std::vector<MassPoint>& bodies, double dt_internal);
std::vector<MassPoint> integrateRK4(const std::vector<MassPoint>& bodies, double dt_internal);
std::vector<MassPoint> integrateVerlet(const std::vector<MassPoint>& bodies, double dt_internal);

enum class IntegratorKind { Euler, RK4, Verlet };

class PhysicsController {
public:
    PhysicsController();
    ~PhysicsController();

    SimulationPayload compute(
        const SimulationToggleParams& toggle_params,
        SimulationParams& sim_params);

private:
    std::vector<MassPoint> initial_conditions_;
    std::vector<MassPoint> rk4_state_;
    std::vector<MassPoint> euler_state_;
    std::vector<MassPoint> verlet_state_;
    double current_time_internal_ = 0.0;
    double stored_dt_t_ = 0.0;
    SimulationToggleParams stored_toggles_{};

    std::vector<Checkpoint> rk4_checkpoints_;
    std::vector<Checkpoint> euler_checkpoints_;
    std::vector<Checkpoint> verlet_checkpoints_;

    void invalidateCheckpoints();
    void resetFromInitialConditions();
    void advanceAll(double dt_internal);
    void seekAll(double target_time_internal, double dt_internal);
    void maybeSaveCheckpoint(
        std::vector<Checkpoint>& checkpoints,
        const std::vector<MassPoint>& state,
        double time_internal);
    void saveAllCheckpoints(double time_internal);
    void restoreFromCheckpoint(
        std::vector<Checkpoint>& checkpoints,
        std::vector<MassPoint>& state,
        double target_time_internal,
        double dt_internal,
        IntegratorKind integrator);
    bool configurationChanged(
        const SimulationToggleParams& toggle_params,
        const SimulationParams& sim_params) const;
};

}
