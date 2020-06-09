set grass=%1

call %grass% --tmp-location EPSG:4326 --exec g.region res=0.1 -p
