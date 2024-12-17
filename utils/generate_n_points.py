import argparse
from random import uniform
from dataclasses import dataclass
import os

@dataclass
class Body:
    x: float
    y: float
    vx: float
    vy: float
    mass: float

    def __str__(self):
        return f"{self.x} {self.y} {self.vx} {self.vy} {self.mass}"

def generate_bodies(N):
    root_dir = os.getcwd()
    with open(f'{root_dir}/src/task1/input.txt', 'w') as file:
        file.write(f"{N}\n")
        for _ in range(N):
            point = Body(
                x=uniform(-1e6, 1e6),
                y=uniform(-1e6, 1e6),
                vx=uniform(-1e3, 1e3),
                vy=uniform(-1e3, 1e3),
                mass=uniform(-1e6, 1e6),
            )
            file.write(f"{str(point)}\n")

    print('File was generated!')

def main():
    parser = argparse.ArgumentParser(description="Generate N random bodies and save them to 'input.txt'.")
    parser.add_argument('N', type=int, help='Number of bodies to generate')
    args = parser.parse_args()
    generate_bodies(args.N)

if __name__ == '__main__':
    main()
