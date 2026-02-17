from osgeo import gdal
import os
import sys
import numpy as np


def main():
    # 注册 GDAL 驱动
    gdal.AllRegister()

    # 获取基础路径
    base_dir = os.getenv("GISBasicRepo")
    if not base_dir:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return 1

    src_file = os.path.join(base_dir, "Data", "Raster", "image1.jpg")
    dst_file = os.path.join(base_dir, "Data", "Raster", "dst.tif")

    # 确保输出目录存在
    os.makedirs(os.path.dirname(dst_file), exist_ok=True)

    # 打开源图像
    src_ds = gdal.Open(src_file, gdal.GA_ReadOnly)
    if src_ds is None:
        print("无法打开源图像！", file=sys.stderr)
        return 1

    img_width = src_ds.RasterXSize
    img_height = src_ds.RasterYSize
    band_num = src_ds.RasterCount

    # 假设数据类型为 Byte（如 JPEG），若不确定可动态获取
    band = src_ds.GetRasterBand(1)
    data_type = band.DataType  # 例如 gdal.GDT_Byte
    depth = gdal.GetDataTypeSize(data_type) // 8

    print(f"源图像：宽={img_width}, 高={img_height}, 波段数={band_num}, 深度={depth} 字节")

    # 目标图像尺寸（固定 256x256）
    buf_width = 256
    buf_height = 256

    # 创建目标 GeoTIFF
    driver = gdal.GetDriverByName("GTiff")
    if driver is None:
        print("找不到 GTiff 驱动。", file=sys.stderr)
        return 1

    options = ["BIGTIFF=IF_NEEDED"]
    dst_ds = driver.Create(dst_file, buf_width, buf_height, band_num, data_type, options)
    if dst_ds is None:
        print("无法创建目标图像！", file=sys.stderr)
        return 1

  
    # 一般读写  
    # buf = src_ds.ReadRaster(
    #     xoff=0, yoff=0,
    #     xsize=buf_width, ysize=buf_height,
    #     buf_xsize=buf_width, buf_ysize=buf_height,
    #     buf_type=data_type
    # )
    # dst_ds.WriteRaster(
    #     xoff=0, yoff=0,
    #     xsize=buf_width, ysize=buf_height,
    #     buf_string=buf,
    #     buf_xsize=buf_width, buf_ysize=buf_height,
    #     buf_type=data_type
    # )

    # 读取特定波段
    # band_map = [3, 2, 1]   #波段索引
    # buf = src_ds.ReadRaster(
    #     xoff=0, yoff=0,
    #     xsize=buf_width, ysize=buf_height,
    #     buf_xsize=buf_width, buf_ysize=buf_height,
    #     buf_type=data_type,
    #     band_list=band_map
    # )
    # dst_ds.WriteRaster(
    #     xoff=0, yoff=0,
    #     xsize=buf_width, ysize=buf_height,
    #     buf_string=buf,
    #     buf_xsize=buf_width, buf_ysize=buf_height,
    #     buf_type=data_type
    # )

    # 左下角起点读写无法直接实现

    # 重采样读写
    buf = src_ds.ReadRaster(
        xoff=0, yoff=0,
        xsize=img_width, ysize=img_height,
        buf_xsize=buf_width, buf_ysize=buf_height,
        buf_type=data_type
    )
    # 覆盖整个 dst 图像（因为 dst 就是 256x256）
    dst_ds.WriteRaster(
        xoff=0, yoff=0,
        xsize=buf_width, ysize=buf_height,
        buf_string=buf,
        buf_xsize=buf_width, buf_ysize=buf_height,
        buf_type=data_type
    )

    # 关闭数据集
    src_ds = None
    dst_ds = None

    print(f"所有操作完成！输出文件: {dst_file}")
    return 0


if __name__ == "__main__":
    sys.exit(main())