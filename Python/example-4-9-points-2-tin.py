import os
import numpy as np
from scipy.spatial import Delaunay
from osgeo import ogr, gdal, osr

# 启用异常处理
gdal.UseExceptions()

def read_points_from_shapefile(src_file):
    """使用 GDAL/OGR 读取 Point/MultiPoint，返回 Nx2 或 Nx3 数组"""
    ds = ogr.Open(src_file)
    if not ds:
        raise RuntimeError(f"无法打开文件: {src_file}")

    points = []
    for i in range(ds.GetLayerCount()):
        layer = ds.GetLayer(i)
        layer.ResetReading()
        for feat in layer:
            geom = feat.GetGeometryRef()
            if not geom:
                continue
            geom_type = geom.GetGeometryType()
            if geom_type in (ogr.wkbPoint, ogr.wkbPoint25D, ogr.wkbPointM, ogr.wkbPointZM):
                x, y, z = geom.GetX(), geom.GetY(), geom.GetZ()
                points.append([x, y, z])
            elif geom_type in (ogr.wkbMultiPoint, ogr.wkbMultiPoint25D, ogr.wkbMultiPointM, ogr.wkbMultiPointZM):
                for j in range(geom.GetGeometryCount()):
                    pt = geom.GetGeometryRef(j)
                    x, y, z = pt.GetX(), pt.GetY(), pt.GetZ()
                    points.append([x, y, z])
            else:
                print(f"跳过非点类型几何: {geom.GetGeometryName()}")
            feat = None  # 显式释放（可选）
        layer = None
    ds = None
    return np.array(points)


def write_tin_to_shapefile(triangles, dst_file, srs=None):
    """使用 GDAL/OGR 将三角形写入 Polygon Shapefile"""
    driver = ogr.GetDriverByName("ESRI Shapefile")
    if os.path.exists(dst_file):
        driver.DeleteDataSource(dst_file)

    ds = driver.CreateDataSource(dst_file)
    layer = ds.CreateLayer("tin", srs=srs, geom_type=ogr.wkbPolygon)

    for tri in triangles:
        ring = ogr.Geometry(ogr.wkbLinearRing)
        for x, y in tri:
            ring.AddPoint(x, y)
        ring.CloseRings()

        poly = ogr.Geometry(ogr.wkbPolygon)
        poly.AddGeometry(ring)

        feat = ogr.Feature(layer.GetLayerDefn())
        feat.SetGeometry(poly)
        layer.CreateFeature(feat)

        # 清理
        feat = None
        poly = None
        ring = None

    ds = None  # 关闭并写入磁盘

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

def main():

    # 设置 GDAL 配置
    gdal.SetConfigOption("SHAPE_ENCODING", "")

    #设置 PROJ_LIB
    setup_proj_lib()

    gis_basic = os.getenv("GISBasicRepo")
    if not gis_basic:
        raise EnvironmentError("环境变量 GISBasicRepo 未设置！")

    src_file = os.path.join(gis_basic, "Data", "Vector", "points.shp")
    dst_file = os.path.join(gis_basic, "Data", "Out.shp")

    print(f"源文件: {src_file}")
    print(f"目标文件: {dst_file}")

    # 1. 读点
    points = read_points_from_shapefile(src_file)
    if len(points) < 3:
        raise ValueError("点数不足，无法构建三角网")

    print(f"读取到 {len(points)} 个点")

    # 2. Delaunay 三角剖分（仅用 XY）
    tri = Delaunay(points[:, :2])
    triangles = [points[simplex, :2] for simplex in tri.simplices]

    print(f"生成 {len(triangles)} 个三角形")

    # 3. 获取源文件的空间参考（可选）
    src_ds = ogr.Open(src_file)
    src_layer = src_ds.GetLayer(0)
    srs = src_layer.GetSpatialRef()
    src_ds = None

    # 4. 写出
    write_tin_to_shapefile(triangles, dst_file, srs=srs)
    print(f"三角网已保存至: {dst_file}")


if __name__ == "__main__":
    main()