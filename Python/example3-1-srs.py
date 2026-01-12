import os
import sys

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

def output_gcs():
    """输出地理坐标系（WGS84）的WKT"""
    srs = osr.SpatialReference()
    srs.ImportFromEPSG(4326)  # WGS84
    # srs.ImportFromEPSG(4214)  # Beijing54
    # srs.ImportFromEPSG(4610)  # XIAN80
    # srs.ImportFromEPSG(4490)  # CGCS2000
    wkt = srs.ExportToPrettyWkt()
    print(wkt)

def output_pcs():
    """输出基于CGCS2000的高斯-克吕格投影坐标系"""
    srs = osr.SpatialReference()
    srs.ImportFromEPSG(4490)  # CGCS2000 地理坐标系

    # 设置横轴墨卡托（Transverse Mercator）
    # 参数说明：
    #   center_lat=0, center_lon=114,
    #   scale_factor=1.0,
    #   false_easting=38500000, false_northing=0
    srs.SetTM(0, 114, 1.0, 38500000, 0)
    wkt = srs.ExportToPrettyWkt()
    print(wkt)

def main():    
    osr.UseExceptions() # 启用异常处理（推荐），避免静默错误
    setup_proj_lib() #设置 PROJ_LIB 

    print("地理坐标系，WGS84坐标系：")
    output_gcs()

    print("\n投影坐标系，高斯克吕格投影坐标系：")
    output_pcs()

if __name__ == "__main__":
    main()

