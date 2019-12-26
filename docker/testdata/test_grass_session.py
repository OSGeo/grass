# Import GRASS Python bindings
from grass_session import Session
import grass.script as grass

with Session(gisdb="/grassdata/", location="test", mapset="PERMANENT", create_opts='EPSG:25832'):
    print("Tests for PROJ, GDAL, PDAL, GRASS")

    # simple test: just scan the LAZ file
    grass.run_command('r.in.pdal', input="/tmp/simple.laz", output='count_1', method='n', flags="s", resolution=1, overwrite=True)
