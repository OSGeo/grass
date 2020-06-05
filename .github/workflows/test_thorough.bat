echo %PATH%
dir c:\osgeo4w64\opt\grass

grass79 --tmp-location XY --exec g.extension g.download.location
grass79 --tmp-location XY --exec g.download.location url=http://fatra.cnr.ncsu.edu/data/nc_spm_full_v2alpha2.tar.gz dbase=%USERPROFILE%

grass79 --tmp-location XY --exec python3 -m grass.gunittest.main --grassdata %USERPROFILE% --location nc_spm_full_v2alpha2 --location-type nc --min-success 80
