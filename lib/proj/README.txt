Update procedure: GRASS GIS relies on GDAL/PROJ.

The EPGS DB is transformed into suitable CSV data for GDAL/PROJ
according to this procedure:
http://svn.osgeo.org/metacrs/geotiff/trunk/libgeotiff/csv/README

The EPSG CSV files are used via GDAL/OGR API from
gdal-config --datadir

The datum shift grids are partially included here.
See https://github.com/OSGeo/proj.4/wiki
    --> Datum shift grids


TODO: It is recommended to rely on PROJ4's proj-nad package. For doing so, 
      there would be some changes needed to lib/proj/get_proj.c - the call
      to pj_set_finder() should be removed so that PROJ looks in its default
      locations for the files. Also changes needed to lib/proj/Makefile so 
      it no longer creates the /etc/proj and /etc/proj/nad directories within
      a GRASS installation nor installs the files there.

