import numpy as np

G = 6.67430e-11

def read_input(filename):
    with open(filename, 'r') as f:
        n = int(f.readline().strip())
        data = []
        for _ in range(n):
            x, y, vx, vy, mass = f.readline().strip().split()
            data.append([float(x), float(y), float(vx), float(vy), float(mass)])
    return np.array(data)  # shape: (n, 5) = [x,y,vx,vy,m]

def total_momentum(positions_velocities):
    # positions_velocities: (n,5) = [x,y,vx,vy,m]
    px = np.sum(positions_velocities[:,4] * positions_velocities[:,2])
    py = np.sum(positions_velocities[:,4] * positions_velocities[:,3])
    return px, py

def total_energy(positions_velocities):
    # Кинетическая энергия
    m = positions_velocities[:,4]
    vx = positions_velocities[:,2]
    vy = positions_velocities[:,3]
    kinetic = 0.5 * np.sum(m * (vx**2 + vy**2))

    # Потенциальная энергия
    x = positions_velocities[:,0]
    y = positions_velocities[:,1]
    n = len(m)

    potential = 0.0
    for i in range(n):
        for j in range(i+1, n):
            dx = x[j] - x[i]
            dy = y[j] - y[i]
            r = np.sqrt(dx*dx + dy*dy)
            potential += (-G * m[i] * m[j]) / r

    return kinetic + potential

def center_of_mass(positions_velocities):
    m = positions_velocities[:,4]
    x = positions_velocities[:,0]
    y = positions_velocities[:,1]
    M = np.sum(m)
    cm_x = np.sum(m * x) / M
    cm_y = np.sum(m * y) / M
    return cm_x, cm_y

if __name__ == "__main__":
    input_file = "src/task1/input.txt"
    trajectory_file = "src/task1/trajectory.csv"

    # Считываем исходные данные, чтобы знать массы тел.
    initial_data = read_input(input_file)
    n = initial_data.shape[0]
    masses = initial_data[:,4]

    # Считываем файл траекторий
    # Формат: первая строка - заголовки, затем строки вида:
    # t, x_0, y_0, vx_0, vy_0, x_1, y_1, vx_1, vy_1, ... x_n, y_n, vx_n, vy_n
    data = np.genfromtxt(trajectory_file, delimiter=',', skip_header=1)
    # data.shape: (num_records, 1+4*n)
    # Колонки: 0: t, затем для каждого тела 4 колонки: x,y,vx,vy.
    
    # Извлечём нужную информацию
    time = data[:,0]
    # Для удобства работы создадим для каждого шага массива вида (n,5)
    # [x, y, vx, vy, m]
    # Для i-го тела:
    # x_i = data[:, 1 + 4*i]
    # y_i = data[:, 2 + 4*i]
    # vx_i = data[:, 3 + 4*i]
    # vy_i = data[:, 4 + 4*i]

    # Проверка корректности индексов:
    # Индекс тела i:
    # start = 1 + 4*i
    # x_i: start
    # y_i: start+1
    # vx_i: start+2
    # vy_i: start+3

    # Для анализа на каждом шаге посчитаем центр масс, импульс и энергию
    cm_positions = []  # Список (cm_x, cm_y) для каждого шага
    momenta = []       # Список (px, py) для каждого шага
    energies = []      # Список энергии для каждого шага

    for step_idx in range(time.size):
        # Формируем массив для текущего шага
        step_positions_vel = np.zeros((n,5))
        for i_body in range(n):
            start_col = 1 + 4*i_body
            x_i = data[step_idx, start_col]
            y_i = data[step_idx, start_col+1]
            vx_i = data[step_idx, start_col+2]
            vy_i = data[step_idx, start_col+3]
            m_i = masses[i_body]
            step_positions_vel[i_body] = [x_i, y_i, vx_i, vy_i, m_i]

        # Вычисляем характеристики
        cm_x, cm_y = center_of_mass(step_positions_vel)
        px, py = total_momentum(step_positions_vel)
        E = total_energy(step_positions_vel)

        cm_positions.append((cm_x, cm_y))
        momenta.append((px, py))
        energies.append(E)

    # Пример вывода результатов:
    # Выводим начальный и конечный импульс, энергию и сдвиг центра масс
    cm_shift = np.sqrt((cm_positions[-1][0] - cm_positions[0][0])**2 + (cm_positions[-1][1] - cm_positions[0][1])**2)

    print("Initial center of mass:", cm_positions[0])
    print("Final center of mass:  ", cm_positions[-1])
    print("Shift in center of mass:", cm_shift)

    print("Initial total momentum:", momenta[0])
    print("Final total momentum:  ", momenta[-1])
    print("Δ momentum: (%.6e, %.6e)" % (momenta[-1][0]-momenta[0][0], momenta[-1][1]-momenta[0][1]))

    print("Initial total energy:  %.6e" % energies[0])
    print("Final total energy:    %.6e" % energies[-1])
    print("Δ energy:              %.6e" % (energies[-1] - energies[0]))

    # Далее, при желании, можно сохранить результаты в отдельный файл
    # или построить графики с помощью matplotlib.
    #
    # Пример:
    # import matplotlib.pyplot as plt
    # cm_xs = [p[0] for p in cm_positions]
    # cm_ys = [p[1] for p in cm_positions]
    # plt.plot(cm_xs, cm_ys, label="Center of mass")
    # plt.legend()
    # plt.show()
