import os
from osgeo import gdal, ogr, osr

# 启用异常处理（可选）
gdal.UseExceptions()
ogr.UseExceptions()

def read_shapefile(src_file):
    """读取 Shapefile，返回几何对象列表（每个为 [exterior_ring, hole1, hole2, ...]）"""
    ds = gdal.OpenEx(src_file, gdal.OF_VECTOR)
    if not ds:
        raise RuntimeError("无法打开文件，请检查路径或数据有效性！")

    layer = ds.GetLayer(0)
    if layer.GetFeatureCount() == 0:
        raise RuntimeError("图层中无要素！")

    # 获取源坐标系
    src_srs = layer.GetSpatialRef()

    polygons = []  # 每个元素是 [[(x,y,z), ...], [(x,y,z), ...], ...]

    layer_defn = layer.GetLayerDefn()
    field_count = layer_defn.GetFieldCount()
    for i in range(field_count):
        field_defn = layer_defn.GetFieldDefn(i)
        print(field_defn.GetName())

    for feature in layer:
        geom = feature.GetGeometryRef()
        if not geom:
            continue

        geom_type = geom.GetGeometryType()

        def process_polygon(ogr_poly):
            rings = []
            # 外环
            exterior = ogr_poly.GetGeometryRef(0)
            ring_pts = [(exterior.GetX(i), exterior.GetY(i), exterior.GetZ(i)) 
                        for i in range(exterior.GetPointCount())]
            rings.append(ring_pts)

            # 内环（洞）
            for j in range(1, ogr_poly.GetGeometryCount()):
                interior = ogr_poly.GetGeometryRef(j)
                hole_pts = [(interior.GetX(i), interior.GetY(i), interior.GetZ(i))
                            for i in range(interior.GetPointCount())]
                rings.append(hole_pts)
            return rings

        if geom_type in (ogr.wkbPolygon, ogr.wkbPolygon25D, ogr.wkbPolygonM, ogr.wkbPolygonZM):
            rings = process_polygon(geom)
            polygons.append(rings)

        elif geom_type in (ogr.wkbMultiPolygon, ogr.wkbMultiPolygon25D, ogr.wkbMultiPolygonM, ogr.wkbMultiPolygonZM):
            for i in range(geom.GetGeometryCount()):
                poly = geom.GetGeometryRef(i)
                rings = process_polygon(poly)
                polygons.append(rings)

        else:
            print("跳过未处理的几何类型:", geom.GetGeometryName())

        # 打印属性值（可选）
        attrs = [feature.GetFieldAsString(i) for i in range(field_count)]
        print("  ".join(attrs))

    ds = None  # 关闭数据集
    return polygons, src_srs


def transform_coordinates(polygons, src_srs):
    """将所有点从 src_srs 转换到 EPSG:3857"""
    dst_srs = osr.SpatialReference()
    dst_srs.ImportFromEPSG(3857)
    # 设置传统 GIS 轴顺序（Lon, Lat）——GDAL 默认可能为 Lat,Lon，需显式指定
    dst_srs.SetAxisMappingStrategy(osr.OAMS_TRADITIONAL_GIS_ORDER)
    src_srs.SetAxisMappingStrategy(osr.OAMS_TRADITIONAL_GIS_ORDER)

    transform = osr.CoordinateTransformation(src_srs, dst_srs)

    transformed = []
    for polygon in polygons:
        new_rings = []
        for ring in polygon:
            new_ring = []
            for x, y, z in ring:
                xt, yt, zt = transform.TransformPoint(x, y, z)
                new_ring.append((xt, yt, zt))
            new_rings.append(new_ring)
        transformed.append(new_rings)
    return transformed


def write_geojson(dst_file, polygons, dst_srs):
    """将多边形写入 GeoJSON"""
    driver = ogr.GetDriverByName("GeoJSON")
    if os.path.exists(dst_file):
        driver.DeleteDataSource(dst_file)

    ds = driver.CreateDataSource(dst_file)
    layer = ds.CreateLayer("FirstLayer", srs=dst_srs, geom_type=ogr.wkbPolygon)

    # 添加字段
    layer.CreateField(ogr.FieldDefn("Type", ogr.OFTString))
    layer.CreateField(ogr.FieldDefn("Area", ogr.OFTReal))
    layer.CreateField(ogr.FieldDefn("VertexCount", ogr.OFTInteger))

    for polygon_rings in polygons:
        # 创建 OGRPolygon
        ogr_poly = ogr.Geometry(ogr.wkbPolygon)

        for ring_pts in polygon_rings:
            ring = ogr.Geometry(ogr.wkbLinearRing)
            for x, y, z in ring_pts:
                ring.AddPoint(x, y, z)
            ogr_poly.AddGeometry(ring)

        # 创建 Feature
        feat = ogr.Feature(layer.GetLayerDefn())
        feat.SetGeometry(ogr_poly)
        feat.SetField("Type", "Polygon")
        feat.SetField("Area", ogr_poly.GetArea())
        vertex_count = sum(len(ring) for ring in polygon_rings)
        feat.SetField("VertexCount", vertex_count)

        layer.CreateFeature(feat)
        feat = None  # 释放

    ds = None  # 关闭

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

    # 获取环境变量
    gis_basic = os.getenv("GISBasicRepo")
    if not gis_basic:
        raise EnvironmentError("环境变量 GISBasicRepo 未设置！")
    
    src_shp = os.path.join(gis_basic, "Data", "Vector", "multipolygons.shp")
    dst_geojson = os.path.join(gis_basic, "Data", "Out.geojson")
    
    print("正在读取 Shapefile...")
    polygons, src_srs = read_shapefile(src_shp)

    print("正在执行坐标转换...")
    polygons_3857 = transform_coordinates(polygons, src_srs)

    print("正在写入 GeoJSON...")
    dst_srs = osr.SpatialReference()
    dst_srs.ImportFromEPSG(3857)
    dst_srs.SetAxisMappingStrategy(osr.OAMS_TRADITIONAL_GIS_ORDER)
    write_geojson(dst_geojson, polygons_3857, dst_srs)

    print("完成！输出文件:", dst_geojson)


if __name__ == "__main__":
    main()