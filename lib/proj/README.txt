Update procedure: GRASS GIS relies on GDAL/PROJ.

The EPGS DB is transformed into suitable CSV data for GDAL/PROJ
according to this procedure:
http://svn.osgeo.org/metacrs/geotiff/trunk/libgeotiff/csv/README

The CSV files are used via GDAL/OGR API from
gdal-config --datadir
