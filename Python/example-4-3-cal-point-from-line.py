import numpy as np

def cal_point_from_line_with_distance(O, E, d):
    """
    从起点 O 沿着 OE 方向，计算距离为 d 的点 P。

    参数:
        O (array-like): 起点坐标，形如 [x, y]
        E (array-like): 终点坐标，形如 [x, y]
        d (float): 从 O 出发沿 OE 方向的距离

    返回:
        P (np.ndarray): 计算得到的点 P 坐标
    """
    O = np.array(O, dtype=float)
    E = np.array(E, dtype=float)
    D = E - O
    norm_D = np.linalg.norm(D)
    if norm_D == 0:
        raise ValueError("O 和 E 不能是同一个点")
    t = d / norm_D
    P = O + t * D
    return P

if __name__ == "__main__":
    O = [1.0, 2.4]
    E = [10.2, 11.5]
    d = 5.0

    P = cal_point_from_line_with_distance(O, E, d)
    print(f"计算的点为：{P[0]:.6f}\t{P[1]:.6f}")
    distance = np.linalg.norm(P - np.array(O))
    print(f"验算距离是否为 {d} ：{distance:.6f}")