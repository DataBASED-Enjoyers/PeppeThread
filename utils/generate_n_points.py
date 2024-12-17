from random import uniform
from dataclasses import dataclass
N = 1_000

@dataclass
class Body:
    x: float
    y: float
    vx: float
    vy: float
    mass: float

    def __str__(self):
        return f"{self.x} {self.y} {self.vx} {self.vy} {self.mass}"

with open('input.txt', 'w') as file:
    file.write(f"{N}\n")
    for _ in range(N):
        point = Body(
            x=uniform(-2**16, 2**16),
            y=uniform(-2**16, 2**16),
            vx=uniform(-1e6, 1e6),
            vy=uniform(-1e6, 1e6),
            mass=uniform(-1e6, 1e6),
        )
        file.write(f"{str(point)}\n")

print('File was generated!')