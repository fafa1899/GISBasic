import numpy as np

EPSILON = 1e-9


def point_in_line(point, start_point, end_point):
    """
    判断点是否在线段上（含端点）
    """
    point = np.array(point)
    start_point = np.array(start_point)
    end_point = np.array(end_point)

    # 向量 P1P2 和 P1P
    p1p2 = end_point - start_point
    p1p = point - start_point

    # 叉积判断共线（二维叉积的模）
    cross = np.cross(p1p2, p1p)
    if abs(cross) > EPSILON:
        return False

    # 点积判断是否在线段范围内
    dot_product = np.dot(p1p2, p1p)
    squared_norm = np.dot(p1p2, p1p2)

    if dot_product < -EPSILON or dot_product > squared_norm + EPSILON:
        return False

    return True


def line_intersection(start1, end1, start2, end2):
    """
    判断两条线段是否相交（不包含端点重合的情况）
    """
    start1 = np.array(start1)
    end1 = np.array(end1)
    start2 = np.array(start2)
    end2 = np.array(end2)

    d1 = end1 - start1
    d2 = end2 - start2
    diff = start2 - start1

    D = -d1[0] * d2[1] + d1[1] * d2[0]
    if abs(D) < EPSILON:
        return False  # 平行或共线

    D1 = -diff[0] * d2[1] + diff[1] * d2[0]
    D2 = d1[0] * diff[1] - d1[1] * diff[0]

    t1 = D1 / D
    t2 = D2 / D

    if 0 <= t1 <= 1 and 0 <= t2 <= 1:
        return True

    return False


def point_in_polygon_2d(point, polygon):
    """
    使用射线法判断点是否在多边形内部。
    多边形应为闭合（首尾点相同）或非闭合均可，函数内部会处理。
    """
    point = np.array(point)
    n = len(polygon)

    # 如果多边形未闭合，则自动闭合（可选）
    if not np.allclose(polygon[0], polygon[-1]):
        polygon = list(polygon) + [polygon[0]]
        n += 1

    # 检查点是否恰好在某条边上
    for i in range(n - 1):
        if point_in_line(point, polygon[i], polygon[i + 1]):
            return True

    # 构造水平向左的射线
    min_x = min(p[0] for p in polygon)
    ray_end = [min_x - 10.0, point[1]]

    count = 0
    for i in range(n - 1):
        a, b = polygon[i], polygon[i + 1]

        # 情况1：顶点在射线上（处理顶点重复计数问题）
        if point_in_line(a, point, ray_end):
            # 只有当该顶点是边的“上端点”时才计数
            if a[1] > b[1]:
                count += 1
        elif point_in_line(b, point, ray_end):
            if b[1] > a[1]:
                count += 1
        # 情况2：边与射线相交
        elif line_intersection(a, b, point, ray_end):
            count += 1

    return (count % 2) == 1


# 主程序
if __name__ == "__main__":
    # 定义六边形（最后一个点与第一个点相同，构成闭合）
    polygon = [
        [268.28, 784.75],
        [153.98, 600.60],
        [274.63, 336.02],
        [623.88, 401.64],
        [676.80, 634.47],
        [530.75, 822.85],
        [268.28, 784.75],
    ]

    a = [407.98, 579.43]
    print(f"点 {a} 是否在多边形内：{point_in_polygon_2d(a, polygon)}")

    b = [678.92, 482.07]
    print(f"点 {b} 是否在多边形内：{point_in_polygon_2d(b, polygon)}")