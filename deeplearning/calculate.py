from __future__ import annotations

import copy
import math
import sys
from pathlib import Path

from .consts import G, IC
from .types import MassPoint

TIME_UNIT_YEARS = 4.74
EPSILON = 1.0e-5

N_POINTS = 1_000_000
DT = 1.0  # one internal time unit = TIME_UNIT_YEARS years


def yearsToInternalTime(years: float) -> float:
    return years / TIME_UNIT_YEARS


def internalTimeToYears(internal_time: float) -> float:
    return internal_time * TIME_UNIT_YEARS


class Acceleration:
    def __init__(self, ax: float = 0.0, ay: float = 0.0) -> None:
        self.ax = ax
        self.ay = ay


def computeAcceleration(body: MassPoint, bodies: list[MassPoint]) -> Acceleration:
    acc = Acceleration()
    for other in bodies:
        if other.id == body.id:
            continue

        dx = other.x - body.x
        dy = other.y - body.y
        dist_sq = dx * dx + dy * dy + EPSILON * EPSILON
        dist = math.sqrt(dist_sq)
        accel_mag = G * other.mass / dist_sq
        acc.ax += accel_mag * (dx / dist)
        acc.ay += accel_mag * (dy / dist)
    return acc


def computeAllAccelerations(bodies: list[MassPoint]) -> list[Acceleration]:
    accs = [Acceleration() for _ in range(len(bodies))]
    for i in range(len(bodies)):
        accs[i] = computeAcceleration(bodies[i], bodies)
    return accs


def integrateEuler(bodies: list[MassPoint], dt_internal: float) -> list[MassPoint]:
    accs = computeAllAccelerations(bodies)
    next = [copy.copy(b) for b in bodies]

    for i in range(len(bodies)):
        next[i].vx = bodies[i].vx + accs[i].ax * dt_internal
        next[i].vy = bodies[i].vy + accs[i].ay * dt_internal
        next[i].x = bodies[i].x + next[i].vx * dt_internal
        next[i].y = bodies[i].y + next[i].vy * dt_internal

    return next


def integrateRK4(bodies: list[MassPoint], dt_internal: float) -> list[MassPoint]:
    n = len(bodies)
    a1 = computeAllAccelerations(bodies)

    stage2 = [copy.copy(bodies[i]) for i in range(n)]
    for i in range(n):
        stage2[i].x += 0.5 * dt_internal * bodies[i].vx
        stage2[i].y += 0.5 * dt_internal * bodies[i].vy
        stage2[i].vx += 0.5 * dt_internal * a1[i].ax
        stage2[i].vy += 0.5 * dt_internal * a1[i].ay
    a2 = computeAllAccelerations(stage2)

    stage3 = [copy.copy(bodies[i]) for i in range(n)]
    for i in range(n):
        stage3[i].x += 0.5 * dt_internal * stage2[i].vx
        stage3[i].y += 0.5 * dt_internal * stage2[i].vy
        stage3[i].vx += 0.5 * dt_internal * a2[i].ax
        stage3[i].vy += 0.5 * dt_internal * a2[i].ay
    a3 = computeAllAccelerations(stage3)

    stage4 = [copy.copy(bodies[i]) for i in range(n)]
    for i in range(n):
        stage4[i].x += dt_internal * stage3[i].vx
        stage4[i].y += dt_internal * stage3[i].vy
        stage4[i].vx += dt_internal * a3[i].ax
        stage4[i].vy += dt_internal * a3[i].ay
    a4 = computeAllAccelerations(stage4)

    next = [copy.copy(b) for b in bodies]
    h = dt_internal / 6.0
    for i in range(n):
        next[i].x += h * (bodies[i].vx + 2.0 * stage2[i].vx + 2.0 * stage3[i].vx + stage4[i].vx)
        next[i].y += h * (bodies[i].vy + 2.0 * stage2[i].vy + 2.0 * stage3[i].vy + stage4[i].vy)
        next[i].vx += h * (a1[i].ax + 2.0 * a2[i].ax + 2.0 * a3[i].ax + a4[i].ax)
        next[i].vy += h * (a1[i].ay + 2.0 * a2[i].ay + 2.0 * a3[i].ay + a4[i].ay)

    return next


def integrateVerlet(bodies: list[MassPoint], dt_internal: float) -> list[MassPoint]:
    n = len(bodies)
    acc_old = computeAllAccelerations(bodies)

    next = [copy.copy(b) for b in bodies]
    for i in range(n):
        next[i].x += bodies[i].vx * dt_internal + 0.5 * acc_old[i].ax * dt_internal * dt_internal
        next[i].y += bodies[i].vy * dt_internal + 0.5 * acc_old[i].ay * dt_internal * dt_internal

    acc_new = computeAllAccelerations(next)
    for i in range(n):
        next[i].vx = bodies[i].vx + 0.5 * (acc_old[i].ax + acc_new[i].ax) * dt_internal
        next[i].vy = bodies[i].vy + 0.5 * (acc_old[i].ay + acc_new[i].ay) * dt_internal

    return next


def _bodies_from_ic(ic: dict) -> list[MassPoint]:
    return [MassPoint(name=name, **fields) for name, fields in ic.items()]


def _copy_bodies(bodies: list[MassPoint]) -> list[MassPoint]:
    return [copy.copy(b) for b in bodies]


def _state_row(t: float, bodies: list[MassPoint]) -> str:
    b1, b2, b3 = bodies
    return (
        f"{t:.16g},{b1.x:.16g},{b1.y:.16g},{b1.vx:.16g},{b1.vy:.16g},"
        f"{b2.x:.16g},{b2.y:.16g},{b2.vx:.16g},{b2.vy:.16g},"
        f"{b3.x:.16g},{b3.y:.16g},{b3.vx:.16g},{b3.vy:.16g}\n"
    )


def run_streaming(
    bodies: list[MassPoint] | None = None,
    n_points: int = N_POINTS,
    dt: float = DT,
    out_dir: str | Path | None = None,
) -> tuple[Path, Path, Path]:
    """Advance Euler, RK4, and Verlet together and stream each step to disk.

    Writes n_points rows per file at times t = 0, dt, 2*dt, ... (n_points-1)*dt.
    Default dt=1 internal unit (= TIME_UNIT_YEARS years).
    Row format: t,x1,y1,vx1,vy1,x2,y2,vx2,vy2,x3,y3,vx3,vy3
    """
    if bodies is None:
        bodies = _bodies_from_ic(IC)
    if out_dir is None:
        out_dir = Path(__file__).resolve().parent
    out_dir = Path(out_dir)
    out_dir.mkdir(parents=True, exist_ok=True)

    euler_path = out_dir / "euler.dat"
    rk4_path = out_dir / "rk4.dat"
    verlet_path = out_dir / "verlet.dat"

    euler_state = _copy_bodies(bodies)
    rk4_state = _copy_bodies(bodies)
    verlet_state = _copy_bodies(bodies)

    flush_every = 10_000
    with (
        euler_path.open("w", encoding="utf-8", buffering=1024 * 1024) as euler_file,
        rk4_path.open("w", encoding="utf-8", buffering=1024 * 1024) as rk4_file,
        verlet_path.open("w", encoding="utf-8", buffering=1024 * 1024) as verlet_file,
    ):
        t = 0.0
        euler_file.write(_state_row(t, euler_state))
        rk4_file.write(_state_row(t, rk4_state))
        verlet_file.write(_state_row(t, verlet_state))

        for step in range(1, n_points):
            euler_state = integrateEuler(euler_state, dt)
            rk4_state = integrateRK4(rk4_state, dt)
            verlet_state = integrateVerlet(verlet_state, dt)
            t = step * dt

            euler_file.write(_state_row(t, euler_state))
            rk4_file.write(_state_row(t, rk4_state))
            verlet_file.write(_state_row(t, verlet_state))

            if step % flush_every == 0:
                euler_file.flush()
                rk4_file.flush()
                verlet_file.flush()
                print(f"wrote {step + 1}/{n_points} points (t={t:.16g})", file=sys.stderr)

    return euler_path, rk4_path, verlet_path
