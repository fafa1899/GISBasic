import numpy as np

def cal_distance_point_and_line(point, lineBegin, lineEnd):
    # 直线方向向量
    n = lineEnd - lineBegin

    # 直线上某一点的向量到点的向量
    m = point - lineBegin

    return np.linalg.norm(np.cross(n, m)) / np.linalg.norm(n)


def cal_distance_point_and_line_1(point, lineBegin, lineEnd):
    A = 1 / (lineEnd[0] - lineBegin[0])
    B = -1 / (lineEnd[1] - lineBegin[1])
    C = lineBegin[1] / (lineEnd[1] - lineBegin[1]) - lineBegin[0] / (lineEnd[0] - lineBegin[0])

    return abs(A * point[0] + B * point[1] + C) / np.sqrt(A * A + B * B)


if __name__ == "__main__":
    point = np.array([0.5, 0.6, 0.0])
    O = np.array([1.0, 2.4, 0.0])
    E = np.array([10.2, 11.5, 0.0])

    print("点到直线的距离为:", cal_distance_point_and_line(point, O, E))
    print("进行验算:", cal_distance_point_and_line_1(point, O, E))