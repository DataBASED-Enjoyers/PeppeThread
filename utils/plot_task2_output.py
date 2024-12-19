import numpy as np
import matplotlib.pyplot as plt
import csv

def read_csv_to_array(filename):
    with open(filename, 'r') as file:
        reader = csv.reader(file)
        data = []
        for row in reader:
            data.append([float(value) for value in row])
    return np.array(data)

def plot_temperature_distribution(data, output_image='temperature_distribution.png'):
    plt.figure(figsize=(8, 6))
    plt.imshow(data, origin='lower', cmap='hot', extent=[0, 1, 0, 1])
    plt.colorbar(label='Temperature')
    plt.title('Temperature Distribution')
    plt.xlabel('x')
    plt.ylabel('y')
    plt.savefig(f"src/task2/benchmarks/{output_image}", dpi=300)

if __name__ == "__main__":
    input_file = 'src/task2/benchmarks/output.csv'
    temperature_data = read_csv_to_array(input_file)
    plot_temperature_distribution(temperature_data)
