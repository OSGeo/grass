import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture(scope="module")
def pack_raster_file4x5_rows(tmp_path_factory):
    """Native raster in pack format in EPSG:3358"""
    tmp_path = tmp_path_factory.mktemp("pack_raster")
    project = tmp_path / "xy_test4x5"
    gs.create_project(project, epsg="3358")
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session, superquiet=True) as tools,
    ):
        tools.g_region(rows=4, cols=5)
        tools.r_mapcalc(expression="rows = row()")
        output_file = tmp_path / "rows4x5.grass_raster"
        tools.r_pack(input="rows", output=output_file, flags="c")
    return output_file
