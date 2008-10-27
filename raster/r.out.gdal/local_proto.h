#ifndef __LOCAL_PROTO_H__
#define __LOCAL_PROTO_H__


#define GRASS_MAX_COLORS 100000	/* what is the right value? UInt16 -> 65535 ? */
/* TODO: better- set from lesser of above and TYPE limits listed below */

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

/* TODO: for data export range checks and nodata value check
#define TYPE_BYTE_MIN		0
#define TYPE_BYTE_MAX		255
#define TYPE_INT16_MIN		-32768
#define TYPE_INT16_MAX		32767
#define TYPE_UINT16_MIN		0
#define TYPE_UINT16_MAX		65535
#define TYPE_UINT32_MIN		0
#define TYPE_UINT32_MAX		4294967295     // better to use (double)(unsigned int)0xFFFFFFFFu  ?
#define TYPE_INT32_MIN		-2147483648
#define TYPE_INT32_MAX		2147483647
#define TYPE_FLOAT32_MIN	-3.4E38f
#define TYPE_FLOAT32_MAX	3.4E38f
#define TYPE_FLOAT64_MIN	-1.79E308f
#define TYPE_FLOAT64_MAX	1.79E308f

 better to not define Ctypes here, rather in the code use
  if( type == F16 || type== CF16 ) 
#define TYPE_CINT16_MIN		TYPE_INT16_MIN
#define TYPE_CINT16_MAX		TYPE_INT16_MAX
#define TYPE_CINT32_MIN		TYPE_INT32_MIN
#define TYPE_CINT32_MAX		TYPE_INT32_MAX
#define TYPE_CFLOAT32_MIN	TYPE_FLOAT32_MIN
#define TYPE_CFLOAT32_MAX	TYPE_FLOAT32_MAX
#define TYPE_CFLOAT64_MIN	TYPE_FLOAT64_MIN
#define TYPE_CFLOAT64_MAX	TYPE_FLOAT64_MAX
*/


/* export_band.c */
int export_band(GDALDatasetH, int, const char *, const char *,
		struct Cell_head *, RASTER_MAP_TYPE, double,
		const char *, int);

#endif /* __LOCAL_PROTO_H__ */
