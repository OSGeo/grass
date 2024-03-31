# Import GRASS Python bindings
# https://github.com/zarch/grass-session
# pip install grass-session

from grass_session import Session
import grass.script as gs

# hint: do not use ~ as an alias for HOME
with Session(
    # run in PERMANENT mapset after creation of location "test"
    gisdb="/grassdata/",
    location="test",
    create_opts="EPSG:25832",
):
    print("grass-session: tests for PROJ, GDAL, PDAL, GRASS GIS")
    print(gs.parse_command("g.gisenv", flags="s"))

    # simple test: just scan the LAZ file
    gs.run_command(
        "r.in.pdal",
        input="/tmp/simple.laz",
        output="count_1",
        method="n",
        flags="g",
        resolution=1,
        overwrite=True,
    )
