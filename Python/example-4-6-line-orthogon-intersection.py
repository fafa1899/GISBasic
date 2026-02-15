import numpy as np

def point_in_orthogon(orthogon, point):
    min_x, min_y, max_x, max_y = orthogon
    x, y = point
    return (min_x <= x <= max_x) and (min_y <= y <= max_y)

def line_intersection_2d(p1, p2, q1, q2, eps=1e-12):
    p1, p2, q1, q2 = map(np.asarray, (p1, p2, q1, q2))
    d1 = p2 - p1
    d2 = q2 - q1
    D = -d1[0] * d2[1] + d1[1] * d2[0]
    if abs(D) < eps:
        return False
    o = q1 - p1
    D1 = -o[0] * d2[1] + o[1] * d2[0]
    D2 = d1[0] * o[1] - d1[1] * o[0]
    t1 = D1 / D
    t2 = D2 / D
    return (0 <= t1 <= 1) and (0 <= t2 <= 1)

def is_intersects_orthogon_2d(line_seg, orthogon):
    a, b = line_seg
    if point_in_orthogon(orthogon, a) or point_in_orthogon(orthogon, b):
        return True
    min_x, min_y, max_x, max_y = orthogon
    diag1 = ([min_x, min_y], [max_x, max_y])
    diag2 = ([min_x, max_y], [max_x, min_y])
    return (line_intersection_2d(a, b, *diag1) or
            line_intersection_2d(a, b, *diag2))

# 测试
if __name__ == "__main__":
    orthogon = [-50, -20, 40, 30]
    line_segment = ([20, 20], [16, 14])
    print("线段与矩形是否相交：", is_intersects_orthogon_2d(line_segment, orthogon))