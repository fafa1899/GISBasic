from osgeo import ogr

# 创建线性环（Linear Ring）
linear_ring = ogr.Geometry(ogr.wkbLinearRing)
linear_ring.AddPoint(268.28, 784.75)
linear_ring.AddPoint(153.98, 600.60)
linear_ring.AddPoint(274.63, 336.02)
linear_ring.AddPoint(623.88, 401.64)
linear_ring.AddPoint(676.80, 634.47)
linear_ring.AddPoint(530.75, 822.85)
linear_ring.CloseRings()

# 创建多边形（Polygon）
polygon = ogr.Geometry(ogr.wkbPolygon)
polygon.AddGeometry(linear_ring)

# 点A
pointA = ogr.Geometry(ogr.wkbPoint)
pointA.AddPoint(407.98, 579.43)
print("点A是否在多边形内：", polygon.Contains(pointA))

# 点B
pointB = ogr.Geometry(ogr.wkbPoint)
pointB.AddPoint(678.92, 482.07)
print("点B是否在多边形内：", polygon.Contains(pointB))