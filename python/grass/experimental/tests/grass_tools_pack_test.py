"""Test grass.experimental.Tools class"""

import os

from grass.experimental.tools import Tools


def test_pack_input_output(xy_dataset_session, rows_raster_file3x3):
    """Check that global overwrite is not used when separate env is used"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    assert os.path.exists(rows_raster_file3x3)
    tools.r_slope_aspect(elevation=rows_raster_file3x3, slope="file.grass_raster")
    assert os.path.exists("file.grass_raster")
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="json")


def test_pack_input_output_with_name_and_parameter_call(
    xy_dataset_session, rows_raster_file3x3
):
    """Check that global overwrite is not used when separate env is used"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    assert os.path.exists(rows_raster_file3x3)
    tools.run(
        "r.slope.aspect", elevation=rows_raster_file3x3, slope="file.grass_raster"
    )
    assert os.path.exists("file.grass_raster")


def test_pack_input_output_with_subprocess_run_like_call(
    xy_dataset_session, rows_raster_file3x3
):
    tools = Tools(session=xy_dataset_session)
    assert os.path.exists(rows_raster_file3x3)
    tools.run_cmd(
        [
            "r.slope.aspect",
            f"elevation={rows_raster_file3x3}",
            "aspect=file.grass_raster",
        ]
    )
    assert os.path.exists("file.grass_raster")
