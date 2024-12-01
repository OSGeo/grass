set grass=%1

rem Test execution of binary command
call %grass% --tmp-project EPSG:4326 --exec g.region res=0.1 -p
rem Test if batch-wrapper-scripts without extension are found
call %grass% --tmp-project EPSG:4326 --exec C:/Windows/System32/where.exe t.create
rem Test if python-scripts can be called
call %grass% --tmp-project EPSG:4326 --exec t.create --help
