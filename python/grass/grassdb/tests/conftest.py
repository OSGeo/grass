import os

import pytest

import grass.script as gs


@pytest.fixture(scope="module")
def pack_raster_file4x5_rows(tmp_path_factory):
    """Native raster in pack format in EPSG:3358"""
    tmp_path = tmp_path_factory.mktemp("pack_raster")
    project = tmp_path / "xy_test4x5"
    gs.create_project(project, epsg="3358")
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=4, cols=5, env=session.env)
        gs.mapcalc("rows = row()", env=session.env)
        output_file = tmp_path / "rows4x5.grass_raster"
        gs.run_command(
            "r.pack",
            input="rows",
            output=output_file,
            flags="c",
            superquiet=True,
            env=session.env,
        )
    return output_file
