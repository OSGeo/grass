set grass=%1
set python=C:\OSGeo4W\bin\python3

call %grass% --tmp-location XY --exec g.download.location url=https://grass.osgeo.org/sampledata/north_carolina/nc_spm_full_v2alpha2.tar.gz path=%USERPROFILE%
call %grass% --tmp-location XY --exec %python% -m grass.gunittest.main --grassdata %USERPROFILE% --location nc_spm_full_v2alpha2 --location-type nc --min-success 60
