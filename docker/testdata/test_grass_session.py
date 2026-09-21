# Import GRASS Python bindings (requires 8.4+) and test r.in.pdal

# PYTHONPATH=$(grass --config python-path) python
#  or
# export PYTHONPATH=$(grass --config python-path)

import grass.script as gs
from grass.tools import Tools

# Create a new project (formerly "location")
# hint: do not use ~ as an alias for HOME
project = "/tmp/my_project_3358"
gs.create_project(project, epsg="3358")

# Initialize the GRASS session
with gs.setup.init(project) as session:
    print("GRASS session: tests for PROJ, GDAL, PDAL, GRASS")
    print(gs.parse_command("g.gisenv", flags="s"))
    tools = Tools(session=session)

    # simple test: just scan the LAZ file
    tools.g_region(n=853535.43, w=635619.85, e=638982.55, s=848899.7)
    tools.r_in_pdal(
        input="/tmp/simple.laz",
        output="count_1",
        method="n",
        flags="g",
        resolution=1,
        overwrite=True,
    )
