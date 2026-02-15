import numpy as np

class Triangle:
    def __init__(self, points):
        self.points = np.array(points)

def point_in_triangle_3d(triangle, P):
    v0p = P - triangle.points[0]
    v0v1 = triangle.points[1] - triangle.points[0]
    v0v2 = triangle.points[2] - triangle.points[0]

    D = v0v1[0] * v0v2[1] - v0v1[1] * v0v2[0]
    if D == 0.0:
        return False

    D1 = v0p[0] * v0v2[1] - v0p[1] * v0v2[0]
    D2 = v0v1[0] * v0p[1] - v0v1[1] * v0p[0]

    u = D1 / D
    v = D2 / D

    # 如果在三维空间判断，还需要判断第三个向量是否成立
    # eps = v0v1[2] * u + v0v2[2] * v - P[2]

    if u >= 0 and v >= 0 and (u + v) <= 1:
        return True

    return False

if __name__ == "__main__":
    triangle = Triangle([[-20, -25.6, 0], [34, -24, 0], [30, 27, 0]])

    A = np.array([1.2, 3.4, 0])
    print(f"点A在三角形内：{point_in_triangle_3d(triangle, A)}")

    B = np.array([14.4, 8.14, 0])
    print(f"点B在三角形内：{point_in_triangle_3d(triangle, B)}")