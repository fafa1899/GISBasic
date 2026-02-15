import numpy as np
import math

# 设置 NumPy 打印选项：不使用科学计数法，保留6位小数
np.set_printoptions(suppress=True, precision=6, floatmode='fixed')

# 常量定义
epsilon = 1e-15
pi = math.pi
d2r = pi / 180.0
r2d = 180.0 / pi

# WGS84 椭球参数
a = 6378137.0                # 长半轴
f_inverse = 298.257223563    # 扁率倒数
b = a - a / f_inverse
e = math.sqrt(a * a - b * b) / a


def Blh2Xyz(lon, lat, height):
    """大地坐标 (L, B, H) 转地心地固坐标 (X, Y, Z)"""
    L = lon * d2r
    B = lat * d2r
    H = height

    N = a / math.sqrt(1 - e**2 * math.sin(B)**2)
    X = (N + H) * math.cos(B) * math.cos(L)
    Y = (N + H) * math.cos(B) * math.sin(L)
    Z = (N * (1 - e**2) + H) * math.sin(B)
    return X, Y, Z


def Xyz2Blh(x, y, z):
    """地心地固坐标 (X, Y, Z) 转大地坐标 (L, B, H)"""
    tmpX, tmpY, tmpZ = x, y, z

    curB = 0.0
    calB = math.atan2(tmpZ, math.sqrt(tmpX**2 + tmpY**2))

    counter = 0
    while abs(curB - calB) * r2d > epsilon and counter < 25:
        curB = calB
        N = a / math.sqrt(1 - e**2 * math.sin(curB)**2)
        calB = math.atan2(tmpZ + N * e**2 * math.sin(curB),
                          math.sqrt(tmpX**2 + tmpY**2))
        counter += 1

    lon = math.atan2(tmpY, tmpX) * r2d
    lat = curB * r2d
    height = tmpZ / math.sin(curB) - N * (1 - e**2)
    return lon, lat, height


def rotation_matrix_z(angle):
    """绕 Z 轴旋转 angle 弧度的旋转矩阵"""
    c, s = math.cos(angle), math.sin(angle)
    return np.array([[c, -s, 0],
                     [s,  c, 0],
                     [0,  0, 1]], dtype=float)


def rotation_matrix_x(angle):
    """绕 X 轴旋转 angle 弧度的旋转矩阵"""
    c, s = math.cos(angle), math.sin(angle)
    return np.array([[1, 0,  0],
                     [0, c, -s],
                     [0, s,  c]], dtype=float)


def CalEcef2Enu(origin_blh):
    """
    构建从 ECEF 到 ENU 的 4x4 齐次变换矩阵（世界 → 局部）
    origin_blh: [lon, lat, height] in degrees & meters
    返回: 4x4 numpy array
    """
    lon, lat, height = origin_blh
    tx, ty, tz = Blh2Xyz(lon, lat, height)

    rz_angle = -(lon * d2r + pi / 2)
    rx_angle = -(pi / 2 - lat * d2r)

    Rz = rotation_matrix_z(rz_angle)
    Rx = rotation_matrix_x(rx_angle)
    R = Rx @ Rz

    T = np.eye(4)
    T[:3, :3] = R
    T[:3, 3] = -R @ np.array([tx, ty, tz])
    return T


def CalEnu2Ecef(origin_blh):
    """
    构建从 ENU 到 ECEF 的 4x4 齐次变换矩阵（局部 → 世界）
    origin_blh: [lon, lat, height]
    返回: 4x4 numpy array
    """
    lon, lat, height = origin_blh
    tx, ty, tz = Blh2Xyz(lon, lat, height)

    rz_angle = lon * d2r + pi / 2
    rx_angle = pi / 2 - lat * d2r

    Rz = rotation_matrix_z(rz_angle)
    Rx = rotation_matrix_x(rx_angle)
    R = Rz @ Rx

    T = np.eye(4)
    T[:3, :3] = R
    T[:3, 3] = np.array([tx, ty, tz])
    return T


def TestXYZ2ENU():
    # 站心点（参考点）
    L, B, H = 116.9395751953, 36.7399177551, 0.0
    origin = [L, B, H]

    print("使用 NumPy 进行转换实现：\n")

    wolrd2localMatrix = CalEcef2Enu(origin)
    print("地心转站心矩阵（ECEF → ENU）：")
    print(wolrd2localMatrix)
    print()

    local2WorldMatrix = CalEnu2Ecef(origin)
    print("站心转地心矩阵（ENU → ECEF）：")
    print(local2WorldMatrix)
    print()

    # 测试点 BLH → ECEF
    test_blh = [117.0, 37.0, 10.3]
    x, y, z = Blh2Xyz(*test_blh)
    xyz_homogeneous = np.array([x, y, z, 1.0])

    print("ECEF坐标（世界坐标）：")
    print(xyz_homogeneous)
    print()

    enu = wolrd2localMatrix @ xyz_homogeneous
    print("ENU坐标（局部坐标）：")
    print(enu)


if __name__ == "__main__":
    TestXYZ2ENU()