import os
import math
import numpy as np
from osgeo import ogr, gdal
import triangle
import trimesh

# -------------------------
# 全局参数
# -------------------------

PI = math.pi
D2R = PI / 180.0

A = 6378137.0
F_INV = 298.257223563

B_AXIS = A - A / F_INV
E = math.sqrt(A * A - B_AXIS * B_AXIS) / A

L = -77.036252
B = 38.897557
H = 0

WALL_HEIGHT = 26

simple_polygon = []

vertices = []

world2local = np.eye(4)


# -------------------------
# SHP读取
# -------------------------

def read_shp():

    work_dir = os.getenv("GISBasicRepo")
    src = os.path.join(work_dir, "Data", "Model", "wh.shp")

    ds = gdal.OpenEx(src, gdal.OF_VECTOR)

    if ds is None:
        raise RuntimeError("打开SHP失败")

    layer = ds.GetLayer(0)

    for feat in layer:

        geom = feat.GetGeometryRef()

        if geom.GetGeometryType() != ogr.wkbPolygon:
            continue

        ring = geom.GetGeometryRef(0)

        n = ring.GetPointCount()

        for i in range(n):

            x, y, _ = ring.GetPoint(i)

            simple_polygon.append(
                np.array([x, y])
            )

    ds = None


# -------------------------
# BLH->ECEF
# -------------------------

def blh_to_xyz(lon, lat, h):

    lon *= D2R
    lat *= D2R

    N = A / math.sqrt(
        1 - E * E * math.sin(lat)**2
    )

    x = (N + h) * math.cos(lat) * math.cos(lon)

    y = (N + h) * math.cos(lat) * math.sin(lon)

    z = (
        N * (1 - E * E) + h
    ) * math.sin(lat)

    return np.array([x, y, z])


# -------------------------
# ENU矩阵
# -------------------------

def calc_world2local():

    global world2local

    rz = -(L * D2R + PI / 2)

    cz = math.cos(rz)
    sz = math.sin(rz)

    Rz = np.array([
        [cz, -sz, 0],
        [sz, cz, 0],
        [0, 0, 1]
    ])

    rx = -(PI/2 - B*D2R)

    cx = math.cos(rx)
    sx = math.sin(rx)

    Rx = np.array([
        [1,0,0],
        [0,cx,-sx],
        [0,sx,cx]
    ])

    R = Rx @ Rz

    center = blh_to_xyz(
        L,
        B,
        H
    )

    T = np.eye(4)

    T[:3,3] = -center

    world2local = np.eye(4)

    world2local[:3,:3] = R

    world2local = world2local @ T


# -------------------------
# 转局部
# -------------------------

def world_to_local(lon, lat, h):

    xyz = blh_to_xyz(
        lon,
        lat,
        h
    )

    p = np.ones(4)

    p[:3] = xyz

    p = world2local @ p

    return np.array([
        p[1],
        p[2],
        p[0]
    ])


# -------------------------
# 屋顶三角化
# -------------------------

def create_roof():

    polygon = np.array(simple_polygon)

    n = len(polygon)

    segments = []

    for i in range(n-1):

        segments.append(
            [i, i+1]
        )

    segments.append(
        [n-1,0]
    )

    data = dict(
        vertices=polygon,
        segments=np.array(
            segments
        )
    )

    result = triangle.triangulate(
        data,
        'p'
    )

    tris = result["triangles"]

    verts = result["vertices"]

    for tri in tris:

        for idx in tri:

            lon, lat = verts[idx]

            p = world_to_local(
                lon,
                lat,
                WALL_HEIGHT
            )

            vertices.append(p)


# -------------------------
# 墙体
# -------------------------

def create_walls():

    for i in range(
        len(simple_polygon)-1
    ):

        p0 = simple_polygon[i]

        p1 = simple_polygon[i+1]

        quad = [

            world_to_local(
                p0[0],
                p0[1],
                WALL_HEIGHT
            ),

            world_to_local(
                p1[0],
                p1[1],
                WALL_HEIGHT
            ),

            world_to_local(
                p0[0],
                p0[1],
                0
            ),

            world_to_local(
                p1[0],
                p1[1],
                0
            )
        ]

        vertices.extend([

            quad[1],
            quad[2],
            quad[0],

            quad[3],
            quad[2],
            quad[1]

        ])


# -------------------------
# gltf输出
# -------------------------

def write_gltf():

    verts = np.array(vertices)

    faces = np.arange(
        len(verts)
    ).reshape(-1,3)

    mesh = trimesh.Trimesh(
        vertices=verts,
        faces=faces,
        process=False
    )

    work_dir = os.getenv("GISBasicRepo")
    out = os.path.join(work_dir, "Data", "Model", "wh.gltf")

    mesh.export(out)

    print("输出完成:", out)


# 自动设置 PROJ_LIB for Conda (Windows/Linux/macOS) 
def setup_proj_lib():
    if 'CONDA_PREFIX' in os.environ:
        conda_prefix = os.environ['CONDA_PREFIX']
        # Windows
        proj_path = os.path.join(conda_prefix, 'Library', 'share', 'proj')
        if not os.path.exists(proj_path):
            # Unix-like
            proj_path = os.path.join(conda_prefix, 'share', 'proj')
        if os.path.exists(proj_path):       
            os.environ['PROJ_LIB'] = proj_path
            return True
    return False

# -------------------------
# 主流程
# -------------------------

def main():

    gdal.AllRegister()

    setup_proj_lib()

    read_shp()

    calc_world2local()

    create_roof()

    create_walls()

    write_gltf()


if __name__ == "__main__":
    main()