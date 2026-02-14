import math

epsilon = 1e-15
pi = 3.14159265358979323846
d2r = pi / 180
r2d = 180 / pi

# WGS84椭球参数
a = 6378137.0              # 椭球长半轴
f_inverse = 298.257223563  # 扁率倒数
b = a - a / f_inverse
e = math.sqrt(a * a - b * b) / a

def Blh2Xyz(x, y, z):
    L = x * d2r
    B = y * d2r
    H = z

    N = a / math.sqrt(1 - e * e * math.sin(B) * math.sin(B))
    X = (N + H) * math.cos(B) * math.cos(L)
    Y = (N + H) * math.cos(B) * math.sin(L)
    Z = (N * (1 - e * e) + H) * math.sin(B)
    return X, Y, Z

def Xyz2Blh(x, y, z):
    tmpX = x
    tmpY = y
    tmpZ = z

    curB = 0
    calB = math.atan2(tmpZ, math.sqrt(tmpX * tmpX + tmpY * tmpY))

    counter = 0
    while abs(curB - calB) * r2d > epsilon and counter < 25:
        curB = calB
        N = a / math.sqrt(1 - e * e * math.sin(curB) * math.sin(curB))
        calB = math.atan2(tmpZ + N * e * e * math.sin(curB), math.sqrt(tmpX * tmpX + tmpY * tmpY))
        counter += 1

    Lon = math.atan2(tmpY, tmpX) * r2d
    Lat = curB * r2d
    Height = tmpZ / math.sin(curB) - N * (1 - e * e)
    return Lon, Lat, Height

if __name__ == "__main__":
    x = 113.6
    y = 38.8
    z = 100

    print("原大地经纬度坐标：{:.10f}\t{:.10f}\t{:.10f}".format(x, y, z))
    x, y, z = Blh2Xyz(x, y, z)
    print("地心地固直角坐标：{:.10f}\t{:.10f}\t{:.10f}".format(x, y, z))
    x, y, z = Xyz2Blh(x, y, z)
    print("转回大地经纬度坐标：{:.10f}\t{:.10f}\t{:.10f}".format(x, y, z))