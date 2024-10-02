import matplotlib.pyplot as plt
import pandas as pd
import numpy as np


def main() -> None:
    df = pd.read_csv('mandelbrot.csv', header=None, names=['x', 'y'])

    plt.figure(figsize=(10, 8))

    sc = plt.scatter(df['x'], df['y'],
                     c=np.sqrt(df['x']**2 + df['y']**2),
                     cmap='plasma',
                     s=10,
                     edgecolor='none')

    cbar = plt.colorbar(sc)
    cbar.set_label('Distance from Origin', fontsize=12)
    plt.xlabel('Real part (X)', fontsize=14)
    plt.ylabel('Imaginary part (Y)', fontsize=14)
    plt.title('Mandelbrot Set Visualization', fontsize=16)
    plt.grid(True, which='both', linestyle='--', linewidth=0.7)
    plt.gca().set_aspect('equal', adjustable='box')
    plt.show()


if __name__ == '__main__':
    main()
