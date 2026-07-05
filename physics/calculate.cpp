#include "physics/definitions/calculate.hpp"
#include <algorithm>
#include <cmath>

namespace Physics {

const double G = 0.00005930662;
const double TIME_UNIT_YEARS = 4.74;
const double EPSILON = 1.0e-5;
const double CHECKPOINT_INTERVAL_YEARS = 15.0;

double yearsToInternalTime(double years) {
    return years / TIME_UNIT_YEARS;
}

double internalTimeToYears(double internal_time) {
    return internal_time * TIME_UNIT_YEARS;
}

namespace {

struct Acceleration {
    double ax = 0.0;
    double ay = 0.0;
};

Acceleration computeAcceleration(const MassPoint& body, const std::vector<MassPoint>& bodies) {
    Acceleration acc{};
    for (const auto& other : bodies) {
        if (other.id == body.id) {
            continue;
        }

        const double dx = other.x - body.x;
        const double dy = other.y - body.y;
        const double dist_sq = dx * dx + dy * dy + EPSILON * EPSILON;
        const double dist = std::sqrt(dist_sq);
        const double accel_mag = G * other.mass / dist_sq;
        acc.ax += accel_mag * (dx / dist);
        acc.ay += accel_mag * (dy / dist);
    }
    return acc;
}

std::vector<Acceleration> computeAllAccelerations(const std::vector<MassPoint>& bodies) {
    std::vector<Acceleration> accs(bodies.size());
    for (std::size_t i = 0; i < bodies.size(); ++i) {
        accs[i] = computeAcceleration(bodies[i], bodies);
    }
    return accs;
}

} // namespace

std::vector<MassPoint> integrateEuler(const std::vector<MassPoint>& bodies, double dt_internal) {
    const auto accs = computeAllAccelerations(bodies);
    std::vector<MassPoint> next = bodies;

    for (std::size_t i = 0; i < bodies.size(); ++i) {
        next[i].vx = bodies[i].vx + accs[i].ax * dt_internal;
        next[i].vy = bodies[i].vy + accs[i].ay * dt_internal;
        next[i].x = bodies[i].x + next[i].vx * dt_internal;
        next[i].y = bodies[i].y + next[i].vy * dt_internal;
    }

    return next;
}

std::vector<MassPoint> integrateRK4(const std::vector<MassPoint>& bodies, double dt_internal) {
    const std::size_t n = bodies.size();
    const auto a1 = computeAllAccelerations(bodies);

    std::vector<MassPoint> stage2(n);
    for (std::size_t i = 0; i < n; ++i) {
        stage2[i] = bodies[i];
        stage2[i].x += 0.5 * dt_internal * bodies[i].vx;
        stage2[i].y += 0.5 * dt_internal * bodies[i].vy;
        stage2[i].vx += 0.5 * dt_internal * a1[i].ax;
        stage2[i].vy += 0.5 * dt_internal * a1[i].ay;
    }
    const auto a2 = computeAllAccelerations(stage2);

    std::vector<MassPoint> stage3(n);
    for (std::size_t i = 0; i < n; ++i) {
        stage3[i] = bodies[i];
        stage3[i].x += 0.5 * dt_internal * stage2[i].vx;
        stage3[i].y += 0.5 * dt_internal * stage2[i].vy;
        stage3[i].vx += 0.5 * dt_internal * a2[i].ax;
        stage3[i].vy += 0.5 * dt_internal * a2[i].ay;
    }
    const auto a3 = computeAllAccelerations(stage3);

    std::vector<MassPoint> stage4(n);
    for (std::size_t i = 0; i < n; ++i) {
        stage4[i] = bodies[i];
        stage4[i].x += dt_internal * stage3[i].vx;
        stage4[i].y += dt_internal * stage3[i].vy;
        stage4[i].vx += dt_internal * a3[i].ax;
        stage4[i].vy += dt_internal * a3[i].ay;
    }
    const auto a4 = computeAllAccelerations(stage4);

    std::vector<MassPoint> next = bodies;
    const double h = dt_internal / 6.0;
    for (std::size_t i = 0; i < n; ++i) {
        next[i].x += h * (bodies[i].vx + 2.0 * stage2[i].vx + 2.0 * stage3[i].vx + stage4[i].vx);
        next[i].y += h * (bodies[i].vy + 2.0 * stage2[i].vy + 2.0 * stage3[i].vy + stage4[i].vy);
        next[i].vx += h * (a1[i].ax + 2.0 * a2[i].ax + 2.0 * a3[i].ax + a4[i].ax);
        next[i].vy += h * (a1[i].ay + 2.0 * a2[i].ay + 2.0 * a3[i].ay + a4[i].ay);
    }

    return next;
}

std::vector<MassPoint> integrateVerlet(const std::vector<MassPoint>& bodies, double dt_internal) {
    const std::size_t n = bodies.size();
    const auto acc_old = computeAllAccelerations(bodies);

    std::vector<MassPoint> next = bodies;
    for (std::size_t i = 0; i < n; ++i) {
        next[i].x += bodies[i].vx * dt_internal + 0.5 * acc_old[i].ax * dt_internal * dt_internal;
        next[i].y += bodies[i].vy * dt_internal + 0.5 * acc_old[i].ay * dt_internal * dt_internal;
    }

    const auto acc_new = computeAllAccelerations(next);
    for (std::size_t i = 0; i < n; ++i) {
        next[i].vx = bodies[i].vx + 0.5 * (acc_old[i].ax + acc_new[i].ax) * dt_internal;
        next[i].vy = bodies[i].vy + 0.5 * (acc_old[i].ay + acc_new[i].ay) * dt_internal;
    }

    return next;
}

namespace {

    std::vector<MassPoint> integrate(
        IntegratorKind integrator,
        const std::vector<MassPoint>& bodies,
        double dt_internal)
    {
        switch (integrator) {
            case IntegratorKind::Euler: return integrateEuler(bodies, dt_internal);
            case IntegratorKind::RK4: return integrateRK4(bodies, dt_internal);
            case IntegratorKind::Verlet: return integrateVerlet(bodies, dt_internal);
        }
        return bodies;
    }

    } // namespace

    PhysicsController::PhysicsController() = default;
    PhysicsController::~PhysicsController() = default;

    void PhysicsController::invalidateCheckpoints() {
        rk4_checkpoints_.clear();
        euler_checkpoints_.clear();
        verlet_checkpoints_.clear();
    }

    void PhysicsController::resetFromInitialConditions() {
        rk4_state_ = initial_conditions_;
        euler_state_ = initial_conditions_;
        verlet_state_ = initial_conditions_;
        current_time_internal_ = 0.0;
        invalidateCheckpoints();
        saveAllCheckpoints(current_time_internal_);
    }

    void PhysicsController::maybeSaveCheckpoint(
        std::vector<Checkpoint>& checkpoints,
        const std::vector<MassPoint>& state,
        double time_internal)
    {
        const double interval_internal = yearsToInternalTime(CHECKPOINT_INTERVAL_YEARS);
        if (checkpoints.empty() || time_internal - checkpoints.back().time >= interval_internal - 1e-12) {
            checkpoints.push_back({state, time_internal});
        }
    }

    void PhysicsController::restoreFromCheckpoint(
        std::vector<Checkpoint>& checkpoints,
        std::vector<MassPoint>& state,
        double target_time_internal,
        double dt_internal,
        IntegratorKind integrator) {
        double time_internal = 0.0;
        if (checkpoints.empty()) {
            state = initial_conditions_;
        } else {
            const Checkpoint* start = &checkpoints.front();
            for (const auto& cp : checkpoints) {
                if (cp.time <= target_time_internal + 1e-12) {
                    start = &cp;
                } else {
                    break;
                }
            }
            state = start->states;
            time_internal = start->time;
        }

        while (time_internal + dt_internal <= target_time_internal + 1e-12) {
            state = integrate(integrator, state, dt_internal);
            time_internal += dt_internal;
            maybeSaveCheckpoint(checkpoints, state, time_internal);
        }

        const double remainder = target_time_internal - time_internal;
        if (remainder > 1e-15) {
            state = integrate(integrator, state, remainder);
        }
    }

    void PhysicsController::saveAllCheckpoints(double time_internal) {
        const double interval_internal = yearsToInternalTime(CHECKPOINT_INTERVAL_YEARS);
        if (rk4_checkpoints_.empty() ||
            time_internal - rk4_checkpoints_.back().time >= interval_internal - 1e-12) {
            rk4_checkpoints_.push_back({rk4_state_, time_internal});
        }
        if (euler_checkpoints_.empty() ||
            time_internal - euler_checkpoints_.back().time >= interval_internal - 1e-12) {
            euler_checkpoints_.push_back({euler_state_, time_internal});
        }
        if (verlet_checkpoints_.empty() ||
            time_internal - verlet_checkpoints_.back().time >= interval_internal - 1e-12) {
            verlet_checkpoints_.push_back({verlet_state_, time_internal});
        }
    }

    void PhysicsController::advanceAll(double dt_internal) {
        rk4_state_ = integrateRK4(rk4_state_, dt_internal);
        euler_state_ = integrateEuler(euler_state_, dt_internal);
        verlet_state_ = integrateVerlet(verlet_state_, dt_internal);
        current_time_internal_ += dt_internal;
        saveAllCheckpoints(current_time_internal_);
    }

    void PhysicsController::seekAll(double target_time_internal, double dt_internal) {
        restoreFromCheckpoint(rk4_checkpoints_, rk4_state_, target_time_internal, dt_internal, IntegratorKind::RK4);
        restoreFromCheckpoint(euler_checkpoints_, euler_state_, target_time_internal, dt_internal, IntegratorKind::Euler);
        restoreFromCheckpoint(verlet_checkpoints_, verlet_state_, target_time_internal, dt_internal, IntegratorKind::Verlet);
        current_time_internal_ = target_time_internal;
    }

    bool PhysicsController::configurationChanged(
        const SimulationToggleParams& toggle_params,
        const SimulationParams& sim_params) const
    {
        if (sim_params.dt != stored_dt_t_) {
            return true;
        }
        if (toggle_params.comparison_mode != stored_toggles_.comparison_mode ||
            toggle_params.use_rk4 != stored_toggles_.use_rk4 ||
            toggle_params.use_euler != stored_toggles_.use_euler ||
            toggle_params.use_verlet != stored_toggles_.use_verlet) {
            return true;
        }
        if (sim_params.bodies.size() != initial_conditions_.size()) {
            return true;
        }
        for (std::size_t i = 0; i < sim_params.bodies.size(); ++i) {
            const auto& a = sim_params.bodies[i];
            const auto& b = initial_conditions_[i];
            if (a.name != b.name || a.mass != b.mass ||
                a.x != b.x || a.y != b.y || a.vx != b.vx || a.vy != b.vy) {
                return true;
            }
        }
        return false;
    }

    SimulationPayload PhysicsController::compute(
        const SimulationToggleParams& toggle_params,
        SimulationParams& sim_params)
    {
        if (initial_conditions_.empty() || configurationChanged(toggle_params, sim_params)) {
            initial_conditions_ = sim_params.bodies;
            stored_dt_t_ = sim_params.dt;
            stored_toggles_ = toggle_params;
            resetFromInitialConditions();
        }

        const double dt_internal = sim_params.dt;

        if (sim_params.reset_requested) {
            resetFromInitialConditions();
            sim_params.elapsed_time = 0.0;
        } else if (sim_params.seek_requested) {
            seekAll(sim_params.seek_target_time, dt_internal);
            sim_params.elapsed_time = current_time_internal_;
        } else if (sim_params.step_requested) {
            advanceAll(dt_internal);
            sim_params.elapsed_time = current_time_internal_;
        }

        sim_params.reset_requested = false;
        sim_params.seek_requested = false;
        sim_params.step_requested = false;

        SimulationPayload payload{};
        payload.elapsed_time = current_time_internal_;

        const bool run_rk4 = toggle_params.comparison_mode || toggle_params.use_rk4;
        const bool run_euler = toggle_params.comparison_mode || toggle_params.use_euler;
        const bool run_verlet = toggle_params.comparison_mode || toggle_params.use_verlet;

        if (run_rk4) {
            payload.rk4_data = rk4_state_;
        }
        if (run_euler) {
            payload.euler_data = euler_state_;
        }
        if (run_verlet) {
            payload.verlet_data = verlet_state_;
        }

        return payload;
    }

}
