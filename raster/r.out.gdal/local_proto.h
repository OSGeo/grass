#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__

#include "gdal.h"

/* range limits */
/*
  GDAL data type               minimum          maximum

  Byte                               0              255
  UInt16                             0           65,535
  Int16, CInt16                -32,768           32,767
  UInt32                             0    4,294,967,295
  Int32, CInt32         -2,147,483,648    2,147,483,647
  Float32, CFloat32            -3.4E38           3.4E38
  Float64, CFloat64          -1.79E308         1.79E308
*/

/* copied from limits.h, checked against gdal-1.6.0/gcore/gdalrasterband.cpp */
#define TYPE_BYTE_MIN		0
#define TYPE_BYTE_MAX		255
#define TYPE_INT16_MIN		(-32768)
#define TYPE_INT16_MAX		32767
#define TYPE_UINT16_MIN		0
#define TYPE_UINT16_MAX		65535
#define TYPE_UINT32_MIN		0
#define TYPE_UINT32_MAX		4294967295U
#define TYPE_INT32_MIN		(-TYPE_INT32_MAX - 1)
#define TYPE_INT32_MAX		2147483647

/* new systems: FLT_MAX, DBL_MAX, old systems: MAXFLOAT, MAXDOUBLE, fallback: 3.4E38 and 1.79E308f */
#ifdef FLT_MAX
#define TYPE_FLOAT32_MIN	(-FLT_MAX)
#define TYPE_FLOAT32_MAX	FLT_MAX
#elif defined(MAX_FLOAT)
#define TYPE_FLOAT32_MIN	(-MAXFLOAT)
#define TYPE_FLOAT32_MAX	MAXFLOAT
#else
#define TYPE_FLOAT32_MIN	-3.4E38f
#define TYPE_FLOAT32_MAX	3.4E38f
#endif

#ifdef DBL_MAX
#define TYPE_FLOAT64_MIN	(-DBL_MAX)
#define TYPE_FLOAT64_MAX	DBL_MAX
#elif defined(MAXDOUBLE)
#define TYPE_FLOAT64_MIN	(-MAXDOUBLE)
#define TYPE_FLOAT64_MAX	MAXDOUBLE
#else
#define TYPE_FLOAT64_MIN	-1.79E308
#define TYPE_FLOAT64_MAX	1.79E308
#endif

#define GRASS_MAX_COLORS TYPE_UINT16_MAX    /* ok? */

/* export_band.c */
int export_band(GDALDatasetH, int, const char *, 
		const char *, struct Cell_head *, RASTER_MAP_TYPE, 
		double, int);
int exact_checks(GDALDataType, const char *, const char *,
                 struct Cell_head *, RASTER_MAP_TYPE, double,
		 const char *, int);

#endif /* __LOCAL_PROTO_H__ */
