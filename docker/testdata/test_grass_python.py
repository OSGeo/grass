# Import GRASS Python bindings
import os
import grass.script as gs

# hint: do not use ~ as an alias for HOME
with gs.setup.init(
    # run in PERMANENT mapset of demolocation in GRASS GIS source
    os.environ["DEMOLOCATION"]  # "/grassdata/demolocation/PERMANENT",
):
    print("grass-setup: tests for PROJ, GDAL, PDAL, GRASS GIS")
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
