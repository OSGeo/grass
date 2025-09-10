"""Test pack import-export functionality of grass.tools.Tools class"""

import os

import pytest

from grass.exceptions import CalledModuleError
from grass.tools import Tools


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
    assert not tools.g_list(type="raster", format="json")


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
    assert not tools.g_list(type="raster", format="json")


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
    assert not tools.g_list(type="raster", format="json")


def test_no_modify_command(xy_dataset_session, rows_raster_file3x3):
    """Check that input command is not modified by the function"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    command = [
        "r.slope.aspect",
        f"elevation={rows_raster_file3x3}",
        "slope=file.grass_raster",
    ]
    original = command.copy()
    tools.run_cmd(command)
    assert original == command


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
    assert not tools.g_list(type="raster", format="json")


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
    assert not tools.g_list(type="raster", format="json")


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
    assert not tools.g_list(type="raster", format="json")


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
    assert not tools.g_list(type="raster", format="json")


def test_wrong_parameter(xy_dataset_session, rows_raster_file3x3):
    """Check wrong parameter causes standard exception

    Since the tool is called to process its parameters with pack IO,
    the error handling takes a different path than without pack IO active.
    """
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    with pytest.raises(CalledModuleError, match="does_not_exist"):
        tools.r_slope_aspect(
            elevation=rows_raster_file3x3,
            slope="file.grass_raster",
            does_not_exist="test",
        )


def test_direct_r_unpack_to_data(xy_dataset_session, rows_raster_file3x3):
    """Check that we can r.unpack data as usual"""
    tools = Tools(session=xy_dataset_session, keep_data=True)
    tools.g_region(rows=3, cols=3)
    name = "data_1"
    tools.r_unpack(input=rows_raster_file3x3, output=name)
    assert tools.g_findfile(element="raster", file=name, format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]


def test_direct_r_unpack_to_pack(xy_dataset_session, rows_raster_file3x3, tmp_path):
    """Check that roundtrip from existing packed raster to new packed raster works"""
    tools = Tools(session=xy_dataset_session, keep_data=True)
    tools.g_region(rows=3, cols=3)
    name = "auto_packed_data_1.grass_raster"
    packed_file = tmp_path / name
    tools.r_unpack(input=rows_raster_file3x3, output=packed_file)
    assert packed_file.exists()
    assert tools.g_findfile(element="raster", file=packed_file.stem, format="json")[
        "name"
    ]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]


def test_direct_r_pack_from_data(xy_dataset_session, tmp_path):
    """Check that we can r.pack data as usual"""
    tools = Tools(session=xy_dataset_session, keep_data=True)
    tools.g_region(rows=3, cols=3)
    tools.r_mapcalc(expression="data_1 = 1")
    name = "manually_packed_data_1.grass_raster"
    packed_file = tmp_path / name
    tools.r_pack(input="data_1", output=packed_file)
    # New file was created.
    assert packed_file.exists()
    # Input still exists.
    assert tools.g_findfile(element="raster", file="data_1", format="json")["name"]
    # There should be no raster created automatically.
    assert not tools.g_findfile(element="raster", file=packed_file.stem, format="json")[
        "name"
    ]
    tools.cleanup()
    # Input still exists even after cleaning.
    assert tools.g_findfile(element="raster", file="data_1", format="json")["name"]


def test_direct_r_pack_from_pack(xy_dataset_session, rows_raster_file3x3, tmp_path):
    """Check that roundtrip from existing packed raster to raster works"""
    tools = Tools(session=xy_dataset_session, keep_data=True)
    tools.g_region(rows=3, cols=3)
    name = "manually_packed_data_1.grass_raster"
    packed_file = tmp_path / name
    tools.r_pack(input=rows_raster_file3x3, output=packed_file)
    # New file was created.
    assert packed_file.exists()
    # Input still exists.
    assert rows_raster_file3x3.exists()
    # Auto-imported raster should exist.
    assert tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
    # There should be no raster created automatically.
    assert not tools.g_findfile(element="raster", file=packed_file.stem, format="json")[
        "name"
    ]
    tools.cleanup()
    # Auto-imported raster should be deleted.
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x3.stem, format="json"
    )["name"]
