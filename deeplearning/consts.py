from __future__ import annotations

import math

# Must match the engine G used in force calculations.
G = 0.00005930662

_v_scale = math.sqrt(G)

# Presets — not exported. Point IC at whichever you want to run.
_FIGURE8 = {
    "m1": {
        "id": 0,
        "mass": 1.0,
        "x": 0.97000436,
        "y": -0.24308753,
        "vx": 0.4662036850 * _v_scale,
        "vy": 0.4323657300 * _v_scale,
    },
    "m2": {
        "id": 1,
        "mass": 1.0,
        "x": -0.97000436,
        "y": 0.24308753,
        "vx": 0.4662036850 * _v_scale,
        "vy": 0.4323657300 * _v_scale,
    },
    "m3": {
        "id": 2,
        "mass": 1.0,
        "x": 0.0,
        "y": 0.0,
        "vx": -0.9324073500 * _v_scale,
        "vy": -0.8647314600 * _v_scale,
    },
}

_lagrange_speed = math.sqrt(G / math.sqrt(3.0))
_lagrange_t1 = math.pi / 2.0
_lagrange_t2 = math.pi / 2.0 + 2.0 * math.pi / 3.0
_lagrange_t3 = math.pi / 2.0 - 2.0 * math.pi / 3.0

_LAGRANGE = {
    "m1": {
        "id": 0,
        "mass": 1.0,
        "x": math.cos(_lagrange_t1),
        "y": math.sin(_lagrange_t1),
        "vx": -_lagrange_speed * math.sin(_lagrange_t1),
        "vy": _lagrange_speed * math.cos(_lagrange_t1),
    },
    "m2": {
        "id": 1,
        "mass": 1.0,
        "x": math.cos(_lagrange_t2),
        "y": math.sin(_lagrange_t2),
        "vx": -_lagrange_speed * math.sin(_lagrange_t2),
        "vy": _lagrange_speed * math.cos(_lagrange_t2),
    },
    "m3": {
        "id": 2,
        "mass": 1.0,
        "x": math.cos(_lagrange_t3),
        "y": math.sin(_lagrange_t3),
        "vx": -_lagrange_speed * math.sin(_lagrange_t3),
        "vy": _lagrange_speed * math.cos(_lagrange_t3),
    },
}

IC = _FIGURE8

__all__ = ["IC"]
