echo %PATH%
dir bin.x86_64-w64-mingw32
grass79 --tmp-location EPSG:4326 --exec g.region res=0.1 -p
