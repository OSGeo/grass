set grass=%1

call %grass% --tmp-location EPSG:4326 --exec g.region res=0.1 -p
call %grass% --tmp-location EPSG:4326 --exec C:/Windows/System32/where.exe t.create
call %grass% --tmp-location EPSG:4326 --exec t.create --help
