@echo off
set PATH=%PATH%;%1

cmd /c grass79 --tmp-location XY --exec g.extension g.download.location
cmd /c grass79 --tmp-location XY --exec g.download.location url=http://fatra.cnr.ncsu.edu/data/nc_spm_full_v2alpha2.tar.gz dbase=%USERPROFILE%

cmd /c grass79 --tmp-location XY --exec python3 -m grass.gunittest.main --grassdata %USERPROFILE% --location nc_spm_full_v2alpha2 --location-type nc --min-success 80
