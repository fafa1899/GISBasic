import os
from numpy import double
from osgeo import osr

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

gcs = osr.SpatialReference()
pcs = osr.SpatialReference()

def create_srs():
    # WGS84  
    gcs.ImportFromEPSG(4326)

    # Web墨卡托
    pcs.ImportFromEPSG(3857) 

    # GDAL 3 changes axis order : https://github.com/OSGeo/gdal/issues/1546
    gcs.SetAxisMappingStrategy(osr.OAMS_TRADITIONAL_GIS_ORDER)
    pcs.SetAxisMappingStrategy(osr.OAMS_TRADITIONAL_GIS_ORDER)

def main():
    osr.UseExceptions() # 启用异常处理（推荐），避免静默错误
    setup_proj_lib() #设置 PROJ_LIB 

    create_srs()

    lonLat2XY = osr.CoordinateTransformation(gcs, pcs)
    xy2LonLat = osr.CoordinateTransformation(pcs, gcs)
    if lonLat2XY is None or xy2LonLat is None:
        print("创建坐标转换失败")
        return 
  
    x = 113.6
    y = 38.8   
    z = 0.0
    print(f"经纬度坐标：{x:.9f}\t{y:.9f}")
    x, y, z = lonLat2XY.TransformPoint(x, y, z)     
    print(f"平面坐标：{x:.9f}\t{y:.9f}")

    x, y, z = xy2LonLat.TransformPoint(x, y, z)   
    print(f"再次转换回的经纬度坐标：{x:.9f}\t{y:.9f}")

if __name__ == "__main__":
    main()    