# 3-Body Problem Simulation

## Overview

A 2D simulation of the three-body problem with selectable numerical integration (Euler, RK4, and Verlet), an SFML-based display engine, and a physics engine with checkpointed playback for fast seeking.

## Decisions

| Topic | Decision |
|---|---|
| Force law | Newtonian gravity: \(F = G \frac{m_1 m_2}{r^2}\), direction along the line connecting the two masses |
| Gravitational constant | **G** = universal Newtonian constant (\(6.67430 \times 10^{-11}\,\mathrm{m^3\,kg^{-1}\,s^{-2}}\)). Fixed; not user-configurable |
| Close-approach handling | Add a small softening term to \(r\) (e.g. \(r \leftarrow \sqrt{r^2 + \varepsilon^2}\) or equivalent) so forces do not blow up when two bodies get very close |
| Display / GUI | **SFML** |
| Build system | **CMake** |
| Simulation control | **Continuous** time advance with **Play**, **Pause**, **Seek**, and **Reset** |
| World origin | **Fixed** at (0, 0). Positions are relative to this origin. The view does not pan; only the grid scale adjusts |
| Checkpoint payload | Per body at each checkpoint: **mass name**, **mass**, **position (x, y)** relative to center, **velocity (vx, vy)** |
| Grid padding | **16 px** margin on all edges between the viewport and the scaled simulation bounds |
| Zoom limits | Min/max zoom **must be enforced**, but specific values are **TBD** until tuned after seeing the sim run |
| Integrator colors | **Euler → White**, **RK4 → Red**, **Verlet → Green**. All bodies drawn with the same color for their active integrator (labels still identify m1 / m2 / m3) |
| Contrail colors | Match the integrator: **Euler → White**, **RK4 → Red**, **Verlet → Green** (per body, per active integrator path) |
| Diagnostics | Visual comparison only — **no** total-energy or other conservation readouts |

## Core specs

1. Integrate using **Euler**, **RK4**, and **Verlet**
2. The grid should **dynamically expand and retract** as the masses move, centered on the fixed origin (0, 0)

## Project structure

`main.cpp` wires everything together:

1. **Display engine** (`display/` folder) — draws the simulation and handles the GUI (SFML)
2. **Physics engine** (`physics/` folder) — performs force and integration calculations
3. **Checkpoint storage** — a vector of snapshots at \(t = 0, 15, 30, 45, 60, \ldots\) (15-unit intervals). Store enough checkpoints that seeking to an arbitrary time is faster than recomputing from \(t = 0\) (e.g. seek to \(t = 40\) by resuming from the \(t = 30\) checkpoint)

Build and link via **CMake** (SFML as a dependency).

### Checkpoint invalidation

When initial conditions change (masses, positions, velocities), integrator mode, or \(dt\) changes, **clear all checkpoints** and restart from \(t = 0\).

In **comparison** mode, Euler, RK4, and Verlet each maintain their own checkpoint chains (same initial conditions, diverging trajectories).

## Display engine

1. **Body setup** — configure 3 bodies (`m1`, `m2`, `m3`), each with:
   - mass name (e.g. `m1`)
   - mass value
   - position \((x, y)\) relative to (0, 0)
   - initial velocity \((v_x, v_y)\)
   - Use a struct or class as appropriate

2. **Time display** — show elapsed time \(t\) since start

3. **Timestep** — selectable integration step \(dt\): `1`, `0.1`, `0.01`, `0.001`, `0.0001`

4. **Playback controls** — **Play** (advance continuously), **Pause**, **Seek** (jump to a target \(t\), using nearest checkpoint ≤ target), **Reset** (return to \(t = 0\) with current initial conditions)

5. **Contrail** — show the trail from approximately the **last 10 steps** (per body, per active integrator path). Contrail color matches the integrator: **Euler → White**, **RK4 → Red**, **Verlet → Green**.

6. **Mode toggle — comparison vs single**
   - **Comparison**: run Euler, RK4, and Verlet from the same initial conditions to visualize deviation
   - **Single**: show one integrator; add a secondary toggle to choose **RK4**, **Euler**, or **Verlet**

7. **Comparison visuals** — render bodies from each active integrator (e.g. `m1_euler`, `m1_rk4`, `m1_verlet`). Connect each triple with **thin faded lines** so it is clear they represent the same logical body and divergence is easy to see

8. **Colors** — keyed by integrator, not by body index:
   - **Euler → White**
   - **RK4 → Red**
   - **Verlet → Green**
   - Applies in both single and comparison mode (all bodies for a given integrator share that integrator’s color)

9. **Dynamic grid** — scale the visible grid to fit all active body positions (all integrators in comparison mode), with **16 px padding** on each edge. Enforce configurable min/max zoom (values TBD). Origin (0, 0) stays fixed at the center of the view.

10. **Initial states** - Prepare 3 stable presets the user can click on to see orbital mechanics for without inputting their own parameters - figure-8, co-linear, and lagrange:
   - **Figure-8**: m1 = m = 1, x = 0.97004, y = -0.243088, vx = 0.00359, vy = 0.00333, m2 = m = 1, x = -0.97004, y = 0.243088, vx = 0.00359, vy = 0.003330, m3 = m = 1, x = 0, y = 0, vx = -0.007181, vy = -0.006659
   - **Lagrange**: m1 = m = 1, x = 0, y = 1, vx = -0.005852, vy = 0, m2 = m = 1, x = -0.866025, y = -0.5, vx = 0.002926, vy = -0.005068, m3 = m = 1, x = 0.866025, y = -0.5, vx = 0.002926, vy = 0.005068
   - **Colinear**: m1 = m = 1, x = -1, y = 0, vx = 0, vy = -0.08610, m2 = m = 1, x = 0, y = 0, vx = 0, vy = 0, m3 = m = 1, x = 1, y = 0, vx = 0, vy = 0.008610

## Physics engine

1. **Controller** — entry point that accepts toggles and parameters (including post–elapsed-time state) and decides how to proceed (single vs comparison, which integrator(s), checkpoint resume vs fresh step)

2. **`integrateRK4(...)`** — advance one body / the system one step using RK4; return updated state (Already written - do not modify)

3. **`integrateEuler(...)`** — advance one body / the system one step using Euler; return updated state (Already written - do not modify)

4. **`integrateVerlet(...)`** — advance one body / the system one step using Verlet; return updated state (Already written - do not modify)

5. **Batch compute** — given toggle params, compute and return results shaped like:
   ```cpp
   { RK4: [...], EULER: [...], VERLET: [...] }
   ```
   Each array holds the coordinate pairs and velocities for all masses at the current step.

   In **single** mode, return only the active integrator’s array (the others may be omitted or empty).

## Open / TBD (for later review if desired)

- Softening parameter **ε** tuning
- z-axis manipulation with orthonormal planar view
- Electrostatic force conversion for small particles with a switch to convert units
