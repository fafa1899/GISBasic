import numpy as np

def line_intersection_2d(start_point1, end_point1, start_point2, end_point2):
    """
    判断两条 2D 线段是否相交，并返回交点（如果存在）。
    
    参数:
        start_point1, end_point1: 第一条线段的起点和终点 (array-like, shape=(2,) 或 (3,))
        start_point2, end_point2: 第二条线段的起点和终点
    
    返回:
        (bool, np.ndarray): 是否相交，以及交点坐标（3D 形式，z=0）
    """
    # 转换为 NumPy 数组，并只取前两个维度（x, y）
    p1 = np.array(start_point1[:2], dtype=float)
    p2 = np.array(end_point1[:2], dtype=float)
    p3 = np.array(start_point2[:2], dtype=float)
    p4 = np.array(end_point2[:2], dtype=float)

    d1 = p2 - p1  # direction1
    d2 = p4 - p3  # direction2

    # 计算分母 D = -d1.x * d2.y + d1.y * d2.x
    D = -d1[0] * d2[1] + d1[1] * d2[0]
    if np.isclose(D, 0.0):
        return False, None  # 平行或共线

    o12 = p3 - p1
    D1 = -o12[0] * d2[1] + o12[1] * d2[0]
    D2 = d1[0] * o12[1] - d1[1] * o12[0]

    t1 = D1 / D
    t2 = D2 / D

    if not (0.0 <= t1 <= 1.0 and 0.0 <= t2 <= 1.0):
        return False, None

    intersection_2d = p1 + t1 * d1
    # 返回 3D 点（z=0）
    intersection_3d = np.array([intersection_2d[0], intersection_2d[1], 0.0])
    return True, intersection_3d


if __name__ == "__main__":
    O1 = [1.0, 2.4, 0]
    E1 = [10.2, 11.5, 0]
    O2 = [10.8, 3.2, 0]
    E2 = [2.6, 10.4, 0]

    intersect, ins_point = line_intersection_2d(O1, E1, O2, E2)

    if intersect:
        print("空间两线段相交的交点为：", ins_point)
    else:
        print("两线段不相交")