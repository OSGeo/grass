@echo off
set PATH=%PATH%;%*
echo %PATH%

call grass79 --tmp-location EPSG:4326 --exec g.region res=0.1 -p
