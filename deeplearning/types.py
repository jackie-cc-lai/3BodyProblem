from dataclasses import dataclass


@dataclass
class MassPoint:
    name: str = ""
    id: int = 0
    mass: float = 1.0  # solar masses (1.0 = one solar mass)
    x: float = 0.0     # AU
    y: float = 0.0     # AU
    vx: float = 0.0    # km/s
    vy: float = 0.0    # km/s
