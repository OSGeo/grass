Projection string management: GRASS GIS relies on GDAL/PROJ.

EPGS DB management: see https://github.com/OSGeo/libgeotiff/tree/master/libgeotiff

The EPSG data are used via GDAL/OGR API from
gdal-config --datadir

The datum shift grids are also used from GDAL/PROJ (proj-nad package).

TODO: It is recommended to rely on PROJ4's proj-nad package. For doing so, 
      there would be some changes needed to lib/proj/get_proj.c - the call
      to pj_set_finder() should be removed so that PROJ looks in its default
      locations for the files.

      Function:
      const char* pszDatumshiftfile = CPLFindFile( "gdal", "datum_shift.csv" );

##########
# EPSG code queries:

# printing all available datum shift codes
ogrinfo /usr/share/gdal/datum_shift.csv -al
ogrinfo /usr/share/gdal/gcs.csv -al

# selective printing
ogrinfo /usr/share/gdal/datum_shift.csv -al -where "SOURCE_CRS_CODE = '4156'" -al
ogrinfo /usr/share/gdal/gcs.csv -al -where "COORD_REF_SYS_NAME LIKE '%JT%'"

# print NAD file info
gdalinfo /usr/share/proj/hawaii

# query codes
gdalsrsinfo EPSG:5514

# transform between projections
gdaltransform -s_srs EPSG:4326 -t_srs "+proj=longlat +datum=nad27" --debug on

##########
# Testing: comparing results

# Testing using GDAL
PROJ_DEBUG=ON CPL_DEBUG=ON gdaltransform -s_srs EPSG:4326 -t_srs "+proj=longlat +datum=nad27"
-100 40
-99.9995941840488 39.9999941029394 0

# Testing using GRASS GIS
PROJ_DEBUG=ON CPL_DEBUG=ON m.proj proj_in="+init=epsg:4326" proj_out="+proj=longlat +datum=NAD27" coordinates=-100,40 -d
-99.99959418|39.99999410|0.00000000

The resulting values need to be identical (note: m.proj prints with less precision)

