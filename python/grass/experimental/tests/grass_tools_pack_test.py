"""Test grass.experimental.Tools class"""

import os

from grass.experimental.tools import Tools


def test_pack_input_output_tool_name_function(xy_dataset_session, rows_raster_file3x3):
    """Check input and output pack files work with tool name call"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    assert os.path.exists(rows_raster_file3x3)
    tools.r_slope_aspect(elevation=rows_raster_file3x3, slope="file.grass_raster")
    assert os.path.exists("file.grass_raster")
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="shell").text


def test_pack_input_output_with_name_and_parameter_call(
    xy_dataset_session, rows_raster_file3x3
):
    """Check input and output pack files work with tool name as string"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    assert os.path.exists(rows_raster_file3x3)
    tools.run(
        "r.slope.aspect", elevation=rows_raster_file3x3, slope="file.grass_raster"
    )
    assert os.path.exists("file.grass_raster")
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="shell").text


def test_pack_input_output_with_subprocess_run_like_call(
    xy_dataset_session, rows_raster_file3x3
):
    """Check input and output pack files work with command as list"""
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
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="shell").text


def test_io_cleanup_after_function(xy_dataset_session, rows_raster_file3x3):
    """Check input and output rasters are deleted after function call"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    tools.r_slope_aspect(elevation=rows_raster_file3x3, slope="file.grass_raster")
    assert os.path.exists("file.grass_raster")
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="shell").text


def test_io_cleanup_after_context(xy_dataset_session, rows_raster_file3x3):
    """Check input and output rasters are deleted at the end of context"""
    with Tools(session=xy_dataset_session) as tools:
        tools.g_region(rows=3, cols=3)
        tools.r_slope_aspect(elevation=rows_raster_file3x3, slope="file.grass_raster")
        assert os.path.exists("file.grass_raster")
        assert tools.g_findfile(element="raster", file="file", format="json")["name"]
        tools.r_mapcalc_simple(
            expression="100 * A", a="file", output="file2.grass_raster"
        )
        assert os.path.exists("file2.grass_raster")
        assert tools.g_findfile(element="raster", file="file2", format="json")["name"]
    # The pack files should still exist.
    assert os.path.exists("file.grass_raster")
    assert os.path.exists("file2.grass_raster")
    # The in-project rasters should not exist.
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(element="raster", file="file2", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="shell").text


def test_io_no_cleanup(xy_dataset_session, rows_raster_file3x3):
    """Check input and output rasters are deleted only with explicit cleanup call"""
    tools = Tools(session=xy_dataset_session, keep_data=True)
    tools.g_region(rows=3, cols=3)
    tools.r_slope_aspect(elevation=rows_raster_file3x3, slope="file.grass_raster")
    assert os.path.exists("file.grass_raster")
    # Files should still be available.
    assert tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
    # But an explicit cleanup should delete the files.
    tools.cleanup()
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(element="raster", file="file2", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="shell").text


def test_io_no_cleanup_with_context(xy_dataset_session, rows_raster_file3x3):
    """Check input and output rasters are kept even with context"""
    with Tools(session=xy_dataset_session, keep_data=True) as tools:
        tools.g_region(rows=3, cols=3)
        tools.r_slope_aspect(elevation=rows_raster_file3x3, slope="file.grass_raster")
        assert os.path.exists("file.grass_raster")
        assert tools.g_findfile(element="raster", file="file", format="json")["name"]
        tools.r_mapcalc_simple(
            expression="100 * A", a="file", output="file2.grass_raster"
        )
        assert os.path.exists("file2.grass_raster")
        assert tools.g_findfile(element="raster", file="file2", format="json")["name"]
    # The pack files should still exist.
    assert os.path.exists("file.grass_raster")
    assert os.path.exists("file2.grass_raster")
    # The in-project rasters should also exist.
    assert tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert tools.g_findfile(element="raster", file="file2", format="json")["name"]
    assert tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
    # But an explicit cleanup should delete the files.
    tools.cleanup()
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(element="raster", file="file2", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="shell").text
