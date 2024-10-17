# Import GRASS GIS Python bindings (requires 8.4+) and test r.in.pdal

# PYTHONPATH=$(grass --config python-path) python

import grass.script as gs

# full path to new project
project = "/tmp/grasstest_epsg_25832"
gs.create_project(project, epsg="25832")

# hint: do not use ~ as an alias for HOME
with gs.setup.init(project):
    print("GRASS GIS session: tests for PROJ, GDAL, PDAL, GRASS GIS")
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
