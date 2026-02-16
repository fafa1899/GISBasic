from osgeo import gdal, osr
import os
import sys

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
    # 注册所有 GDAL 格式驱动
    gdal.AllRegister()

    #设置 PROJ_LIB
    setup_proj_lib()

    # 获取环境变量 GISBasic
    gis_basic = os.getenv("GISBasicRepo")
    if not gis_basic:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return 1

    # 构造输出文件路径
    dst_file = os.path.join(gis_basic, "Data", "Raster", "dst.jpg")

    # 确保输出目录存在
    os.makedirs(os.path.dirname(dst_file), exist_ok=True)

    # 图像参数
    img_width = 256
    img_height = 256
    band_num = 3

    # 1. 创建内存数据集 (MEM)
    mem_driver = gdal.GetDriverByName("MEM")
    mem_dataset = mem_driver.Create("", img_width, img_height, band_num, gdal.GDT_Byte)
    if mem_dataset is None:
        print("错误：无法创建内存数据集。", file=sys.stderr)
        return 1

    # 2. 设置空间参考 (WGS84, EPSG:4326)
    srs = osr.SpatialReference()
    srs.ImportFromEPSG(4326)
    wkt = srs.ExportToWkt()
    mem_dataset.SetProjection(wkt)

    # 3. 设置地理变换（GeoTransform）
    geo_transform = (114.0, 0.000001, 0.0, 34.0, 0.0, -0.000001)
    mem_dataset.SetGeoTransform(geo_transform)

    # 4. 使用 JPEG 驱动导出带世界文件的图像
    jpeg_driver = gdal.GetDriverByName("JPEG")
    if jpeg_driver is None:
        print("错误：找不到 JPEG 驱动。", file=sys.stderr)
        return 1

    options = ["WORLDFILE=YES"]
    dst_dataset = jpeg_driver.CreateCopy(dst_file, mem_dataset, strict=1, options=options)

    if dst_dataset is None:
        print("错误：无法创建 JPEG 文件。", file=sys.stderr)
        return 1

    # 写入投影
    dst_dataset.SetProjection(wkt)
    # 写入地理变换
    dst_dataset.SetGeoTransform(geo_transform)

    # 关闭数据集
    dst_dataset = None
    mem_dataset = None

    print(f"成功创建文件: {dst_file}")
    print(f"对应世界文件 (.wld) 已生成。")
    return 0

if __name__ == "__main__":
    sys.exit(main())