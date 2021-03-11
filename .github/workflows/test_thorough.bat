set grass=%1
set python=C:\OSGeo4W64\bin\python3

call %grass% --tmp-location XY --exec g.extension g.download.location
call %grass% --tmp-location XY --exec g.download.location url=http://fatra.cnr.ncsu.edu/data/nc_spm_full_v2alpha2.tar.gz dbase=%USERPROFILE%
call %grass% --tmp-location XY --exec %python% -m grass.gunittest.main --grassdata %USERPROFILE% --location nc_spm_full_v2alpha2 --location-type nc --min-success 60
