from osgeo import gdal
import os
import sys
import numpy as np

def compute_histogram_lut(band_array):
    """
    对单波段 uint8 数组计算直方图均衡化的 LUT（0~255 映射表）
    """
    # 确保输入是 uint8
    if band_array.dtype != np.uint8:
        band_array = band_array.astype(np.uint8)

    # 计算直方图（256 bins，0~255）
    hist, _ = np.histogram(band_array, bins=256, range=(0, 256))
    hist = hist.astype(np.float64)

    # 总像素数
    total = hist.sum()
    if total == 0:
        return np.arange(256, dtype=np.uint8)  # 无数据时返回恒等映射

    # 计算累积分布函数 (CDF)
    cdf = np.cumsum(hist)
    cdf_normalized = cdf / total  # 归一化到 [0, 1]

    # 映射到 [0, 255]
    lut = np.clip(np.round(cdf_normalized * 255), 0, 255).astype(np.uint8)
    return lut


def main():
    # 启用异常处理
    gdal.UseExceptions()

    # 注册 GDAL 驱动
    gdal.AllRegister()

    # 获取环境变量
    gis_basic = os.getenv("GISBasicRepo")
    if not gis_basic:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return 1

    src_path = os.path.join(gis_basic, "Data", "Raster", "image1.jpg")
    dst_path = os.path.join(gis_basic, "Data", "Raster", "dst.bmp")

    # 确保输出目录存在
    os.makedirs(os.path.dirname(dst_path), exist_ok=True)

    # 打开源图像
    src_ds = gdal.Open(src_path, gdal.GA_ReadOnly)
    if src_ds is None:
        print("无法打开源图像！", file=sys.stderr)
        return 1

    width = src_ds.RasterXSize
    height = src_ds.RasterYSize
    band_count = src_ds.RasterCount

    print(f"图像尺寸: {width} x {height}, 波段数: {band_count}")

    # 读取所有波段为 NumPy 数组 (shape: [bands, height, width])
    try:
        img_array = src_ds.ReadAsArray().astype(np.uint8)
    except Exception as e:
        print(f"读取图像失败: {e}", file=sys.stderr)
        return 1

    # 如果是单波段，reshape 为 (1, H, W)
    if img_array.ndim == 2:
        img_array = img_array[np.newaxis, :, :]

    # 为每个波段计算 LUT 并应用均衡化
    for b in range(band_count):
        print(f"正在处理波段 {b + 1}...")
        lut = compute_histogram_lut(img_array[b])
        # 应用 LUT：使用高级索引
        img_array[b] = lut[img_array[b]]

    # 创建输出 BMP 文件
    driver = gdal.GetDriverByName("BMP")
    if driver is None:
        print("找不到 BMP 驱动。", file=sys.stderr)
        return 1

    dst_ds = driver.Create(dst_path, width, height, band_count, gdal.GDT_Byte)
    if dst_ds is None:
        print("无法创建输出文件！", file=sys.stderr)
        return 1

    # 写入各波段
    for b in range(band_count):
        dst_band = dst_ds.GetRasterBand(b + 1)
        dst_band.WriteArray(img_array[b])

    # 清理资源
    src_ds = None
    dst_ds = None

    print(f"直方图均衡化完成！输出文件: {dst_path}")
    return 0


if __name__ == "__main__":
    sys.exit(main())