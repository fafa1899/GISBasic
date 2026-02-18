from osgeo import gdal
import os
import sys
import math

def main():
    # 启用异常处理
    gdal.UseExceptions()

    # 注册所有 GDAL 驱动
    gdal.AllRegister()

    # 获取基础路径
    base_dir = os.getenv("GISBasicRepo")
    if not base_dir:
        print("错误：环境变量 GISBasicRepo 未设置。", file=sys.stderr)
        return 1

    src_file = os.path.join(base_dir, "Data", "Raster", "berry_ali_2011127_crop_geo.tif")

    # 打开栅格数据集（只读）
    dataset = gdal.Open(src_file, gdal.GA_ReadOnly)
    if dataset is None:
        print("无法打开图像！", file=sys.stderr)
        return 1

    # 获取原始尺寸
    width = dataset.RasterXSize
    height = dataset.RasterYSize
    total_pixels_max = width * height
    total_pixels_min = 128 * 128

    # 计算金字塔层级（每级缩放因子为 2, 4, 8, 16, ...）
    level_factors = []
    current_pixels = total_pixels_max
    level_count = 0

    while current_pixels > total_pixels_min and level_count < 1024:
        # 缩放因子 = 2^(level+1)，对应 C++ 中 LevelArray[nLevelCount] = 2^(nLevelCount+1)
        factor = int(math.pow(2, level_count + 1))
        level_factors.append(factor)
        current_pixels //= 4  # 每次面积缩小为 1/4（长宽各 /2）
        level_count += 1

    # 如果有有效层级，构建金字塔
    if level_factors:
        print(f"正在构建金字塔，层级因子: {level_factors}")
        resampling = "nearest"
        err = dataset.BuildOverviews(resampling, level_factors, callback=None)
        if err != gdal.CE_None:
            print("警告：构建金字塔时出错。", file=sys.stderr)
        else:
            print("金字塔构建成功！")
    else:
        print("图像太小，无需构建金字塔。")

    # 关闭数据集
    dataset = None
    return 0


if __name__ == "__main__":
    sys.exit(main())