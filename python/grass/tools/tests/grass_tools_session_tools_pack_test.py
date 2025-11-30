"""Test pack import-export functionality of grass.tools.Tools class"""

import os
import re
from pathlib import Path

import pytest

import grass.script as gs
from grass.tools import Tools, ToolError


def test_pack_input_output_tool_name_function(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check input and output pack files work with tool name call"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    assert rows_raster_file3x2.exists()
    output_file = tmp_path / "file.grass_raster"
    tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    assert output_file.exists()
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="json")


@pytest.mark.parametrize("parameter_type", [str, Path])
def test_pack_input_output_tool_name_function_string_value(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path, parameter_type
):
    """Check input and output pack files work string a parameter

    We make no assumption about the fixture types and explicitly test all
    supported parameter types.
    """
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    assert rows_raster_file3x2.exists()
    output_file = tmp_path / "file.grass_raster"
    tools.r_slope_aspect(
        elevation=parameter_type(rows_raster_file3x2), slope=parameter_type(output_file)
    )
    assert output_file.exists()
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="json")


def test_pack_input_output_with_name_and_parameter_call(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check input and output pack files work with tool name as string"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    assert rows_raster_file3x2.exists()
    output_file = tmp_path / "file.grass_raster"
    tools.run("r.slope.aspect", elevation=rows_raster_file3x2, slope=output_file)
    assert output_file.exists()
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="json")


def test_pack_input_output_with_subprocess_run_like_call(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check input and output pack files work with command as list"""
    tools = Tools(session=xy_dataset_session)
    assert rows_raster_file3x2.exists()
    output_file = tmp_path / "file.grass_raster"
    tools.run_cmd(
        [
            "r.slope.aspect",
            f"elevation={rows_raster_file3x2}",
            f"aspect={output_file}",
        ]
    )
    assert output_file.exists()
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="json")


def test_no_modify_command(xy_dataset_session, rows_raster_file3x2, tmp_path):
    """Check that input command is not modified by the function"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    output_file = tmp_path / "file.grass_raster"
    command = [
        "r.slope.aspect",
        f"elevation={rows_raster_file3x2}",
        f"slope={output_file}",
    ]
    original = command.copy()
    tools.run_cmd(command)
    assert original == command


def test_io_cleanup_after_function(xy_dataset_session, rows_raster_file3x2, tmp_path):
    """Check input and output rasters are deleted after function call"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    output_file = tmp_path / "file.grass_raster"
    tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    assert output_file.exists()
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="json")


def test_io_cleanup_after_context(xy_dataset_session, rows_raster_file3x2, tmp_path):
    """Check input and output rasters are deleted at the end of context"""
    output_file_1 = tmp_path / "file.grass_raster"
    output_file_2 = tmp_path / "file2.grass_raster"
    with Tools(session=xy_dataset_session) as tools:
        tools.g_region(rows=3, cols=3)
        tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file_1)
        assert output_file_1.exists()
        assert tools.g_findfile(element="raster", file="file", format="json")["name"]
        tools.r_mapcalc_simple(expression="100 * A", a="file", output=output_file_2)
        assert output_file_2.exists()
        assert tools.g_findfile(element="raster", file="file2", format="json")["name"]
    # The pack files should still exist.
    assert output_file_1.exists()
    assert output_file_2.exists()
    # The in-project rasters should not exist.
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(element="raster", file="file2", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="json")


def test_io_no_cleanup(xy_dataset_session, rows_raster_file3x2, tmp_path):
    """Check input and output rasters are deleted only with explicit cleanup call"""
    output_file = tmp_path / "file.grass_raster"
    tools = Tools(session=xy_dataset_session, use_cache=True)
    tools.g_region(rows=3, cols=3)
    tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    assert output_file.exists()
    # Files should still be available.
    assert tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    # But an explicit cleanup should delete the files.
    tools.cleanup()
    assert not tools.g_findfile(element="raster", file="file", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="json")


def test_io_no_cleanup_with_context(xy_dataset_session, rows_raster_file3x2, tmp_path):
    """Check input and output rasters are kept even with context"""
    file_1 = tmp_path / "file_1.grass_raster"
    file_2 = tmp_path / "file_2.grass_raster"
    with Tools(session=xy_dataset_session, use_cache=True) as tools:
        tools.g_region(rows=3, cols=3)
        tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=file_1)
        assert file_1.exists()
        assert tools.g_findfile(element="raster", file=file_1.stem, format="json")[
            "name"
        ]
        tools.r_mapcalc_simple(expression="100 * A", a=file_1.stem, output=file_2)
        assert file_2.exists()
        assert tools.g_findfile(element="raster", file=file_2.stem, format="json")[
            "name"
        ]
    # The pack files should still exist.
    assert file_1.exists()
    assert file_2.exists()
    # The in-project rasters should also exist.
    assert tools.g_findfile(element="raster", file=file_1.stem, format="json")["name"]
    assert tools.g_findfile(element="raster", file=file_2.stem, format="json")["name"]
    assert tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    # But an explicit cleanup should delete the files.
    tools.cleanup()
    assert not tools.g_findfile(element="raster", file=file_1.stem, format="json")[
        "name"
    ]
    assert not tools.g_findfile(element="raster", file=file_2.stem, format="json")[
        "name"
    ]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert not tools.g_list(type="raster", format="json")
    # The pack files should still exist after cleanup.
    assert file_1.exists()
    assert file_2.exists()


def test_multiple_input_usages_with_context(xy_dataset_session, rows_raster_file3x2):
    """Check multiple usages of the same input raster with context"""
    with Tools(session=xy_dataset_session) as tools:
        tools.g_region(raster=rows_raster_file3x2)
        tools.r_slope_aspect(elevation=rows_raster_file3x2, slope="slope")
        tools.r_mapcalc_simple(
            expression="100 * A", a=rows_raster_file3x2, output="a100"
        )
        assert tools.g_findfile(
            element="raster", file=rows_raster_file3x2.stem, format="json"
        )["name"]
    assert tools.g_findfile(element="raster", file="slope", format="json")["name"]
    assert tools.g_findfile(element="raster", file="a100", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]


def test_multiple_input_usages_with_use_cache(xy_dataset_session, rows_raster_file3x2):
    """Check input and output rasters are kept even with context"""
    tools = Tools(session=xy_dataset_session, use_cache=True)
    tools.g_region(raster=rows_raster_file3x2)
    tools.r_slope_aspect(elevation=rows_raster_file3x2, slope="slope")
    tools.r_mapcalc_simple(expression="100 * A", a=rows_raster_file3x2, output="a100")
    assert tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert tools.g_findfile(element="raster", file="slope", format="json")["name"]
    assert tools.g_findfile(element="raster", file="a100", format="json")["name"]
    tools.cleanup()
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]


def test_multiple_input_usages_with_defaults(xy_dataset_session, rows_raster_file3x2):
    """Check input and output rasters are kept even with context"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    tools.r_mapcalc_simple(
        expression="A + B",
        a=rows_raster_file3x2,
        b=rows_raster_file3x2,
        output="output",
    )
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert tools.g_findfile(element="raster", file="output", format="json")["name"]


def test_creation_and_use_with_context(
    xy_dataset_session, rows_raster_file3x2, tmp_path
):
    """Check that we can create an external file and then use the file later"""
    slope = tmp_path / "slope.grass_raster"
    with Tools(session=xy_dataset_session) as tools:
        tools.g_region(raster=rows_raster_file3x2)
        tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=slope)
        assert tools.r_univar(map=slope, format="json")["cells"] == 6
        assert tools.g_findfile(element="raster", file=slope.stem, format="json")[
            "name"
        ]
    assert not tools.g_findfile(element="raster", file=slope.stem, format="json")[
        "name"
    ]
    assert slope.exists()


def test_creation_and_use_with_use_cache(
    xy_dataset_session, rows_raster_file3x2, tmp_path
):
    """Check that we can create an external file and then use the file later"""
    slope = tmp_path / "slope.grass_raster"
    tools = Tools(session=xy_dataset_session, use_cache=True)
    tools.g_region(raster=rows_raster_file3x2)
    tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=slope)
    assert tools.r_univar(map=slope, format="json")["cells"] == 6
    assert tools.g_findfile(element="raster", file=slope.stem, format="json")["name"]
    assert slope.exists()


def test_creation_and_use_with_defaults(
    xy_dataset_session, rows_raster_file3x2, tmp_path
):
    """Check that we can create an external file and then use the file later"""
    slope = tmp_path / "slope.grass_raster"
    tools = Tools(session=xy_dataset_session)
    tools.g_region(raster=rows_raster_file3x2)
    tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=slope)
    assert tools.r_univar(map=slope, format="json")["cells"] == 6
    assert not tools.g_findfile(element="raster", file=slope.stem, format="json")[
        "name"
    ]
    assert slope.exists()


def test_repeated_input_usages_with_context(xy_dataset_session, rows_raster_file3x2):
    """Check multiple usages of the same input raster with context"""
    with Tools(session=xy_dataset_session) as tools:
        tools.g_region(rows=3, cols=3)
        tools.r_mapcalc_simple(
            expression="A + B",
            a=rows_raster_file3x2,
            b=rows_raster_file3x2,
            output="output",
        )
        assert tools.g_findfile(element="raster", file="output", format="json")["name"]
        assert tools.g_findfile(
            element="raster", file=rows_raster_file3x2.stem, format="json"
        )["name"]
    assert tools.g_findfile(element="raster", file="output", format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]


def test_repeated_output(xy_dataset_session, rows_raster_file3x2, tmp_path):
    """Check behavior when two outputs have the same name

    This would ideally result in error or some other clear state, but at least
    r.slope.aspect has that as undefined behavior, so we follow the same logic.
    Here, we test the current behavior which is that no error is produced
    and one of the outputs is produced (but it is not defined which one).
    """
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    output_file = tmp_path / "file.grass_raster"
    tools.r_slope_aspect(
        elevation=rows_raster_file3x2, slope=output_file, aspect=output_file
    )
    assert output_file.exists()


def test_output_without_overwrite(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check input and output pack files work with tool name call"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    assert rows_raster_file3x2.exists()
    output_file = tmp_path / "file.grass_raster"
    tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    with pytest.raises(ToolError, match=r"[Oo]verwrite"):
        tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    assert output_file.exists()


def test_output_with_object_level_overwrite(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check input and output pack files work with tool name call"""
    tools = Tools(session=xy_dataset_session, overwrite=True)
    tools.g_region(rows=3, cols=3)
    assert rows_raster_file3x2.exists()
    output_file = tmp_path / "file.grass_raster"
    tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    # Same call the second time.
    tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    assert output_file.exists()


def test_output_with_function_level_overwrite(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check input and output pack files work with tool name call"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    assert rows_raster_file3x2.exists()
    output_file = tmp_path / "file.grass_raster"
    tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    # Same call the second time.
    tools.r_slope_aspect(
        elevation=rows_raster_file3x2, slope=output_file, overwrite=True
    )
    assert output_file.exists()


def test_non_existent_pack_input(xy_dataset_session, tmp_path: Path):
    """Check input and output pack files work with tool name call"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    input_file = tmp_path / "does_not_exist.grass_raster"
    assert not input_file.exists()
    with pytest.raises(
        ToolError,
        match=rf"(?s)[^/\/a-zA-Z_]{re.escape(str(input_file))}[^/\/a-zA-Z_].*not found",
    ):
        tools.r_slope_aspect(elevation=input_file, slope="slope")
    assert not tools.g_findfile(element="raster", file=input_file.stem, format="json")[
        "name"
    ]
    assert not tools.g_findfile(element="raster", file="slope", format="json")["name"]


def test_non_existent_output_pack_directory(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check input and output pack files work with tool name call"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    output_file = tmp_path / "does_not_exist" / "file.grass_raster"
    assert not output_file.exists()
    assert not output_file.parent.exists()
    assert rows_raster_file3x2.exists()
    with pytest.raises(
        ToolError,
        match=rf"(?s)[^/\/a-zA-Z_]{re.escape(str(output_file.parent))}[^/\/a-zA-Z_].*does not exist",
    ):
        tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)


def test_wrong_parameter(xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path):
    """Check wrong parameter causes standard exception

    Since the tool is called to process its parameters with pack IO,
    the error handling takes a different path than without pack IO active.
    """
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    with pytest.raises(ToolError, match="does_not_exist"):
        tools.r_slope_aspect(
            elevation=rows_raster_file3x2,
            slope="file.grass_raster",
            does_not_exist="test",
        )


def test_direct_r_unpack_to_data(xy_dataset_session, rows_raster_file3x2: Path):
    """Check that we can r.unpack data as usual"""
    tools = Tools(session=xy_dataset_session, use_cache=True)
    tools.g_region(rows=3, cols=3)
    name = "data_1"
    tools.r_unpack(input=rows_raster_file3x2, output=name)
    assert tools.g_findfile(element="raster", file=name, format="json")["name"]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]


def test_direct_r_unpack_to_pack(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check that roundtrip from existing packed raster to new packed raster works"""
    tools = Tools(session=xy_dataset_session, use_cache=True)
    tools.g_region(rows=3, cols=3)
    name = "auto_packed_data_1.grass_raster"
    packed_file = tmp_path / name
    tools.r_unpack(input=rows_raster_file3x2, output=packed_file)
    assert packed_file.exists()
    assert tools.g_findfile(element="raster", file=packed_file.stem, format="json")[
        "name"
    ]
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]


def test_direct_r_pack_from_data(xy_dataset_session, tmp_path: Path):
    """Check that we can r.pack data as usual"""
    tools = Tools(session=xy_dataset_session, use_cache=True)
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


def test_direct_r_pack_from_pack(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check that roundtrip from existing packed raster to raster works"""
    tools = Tools(session=xy_dataset_session, use_cache=True)
    tools.g_region(rows=3, cols=3)
    name = "manually_packed_data_1.grass_raster"
    packed_file = tmp_path / name
    tools.r_pack(input=rows_raster_file3x2, output=packed_file)
    # New file was created.
    assert packed_file.exists()
    # Input still exists.
    assert rows_raster_file3x2.exists()
    # Auto-imported raster should exist.
    assert tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    # There should be no raster created automatically.
    assert not tools.g_findfile(element="raster", file=packed_file.stem, format="json")[
        "name"
    ]
    tools.cleanup()
    # Auto-imported raster should be deleted.
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]


def test_clean_after_tool_failure_with_context_and_try(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check we delete imported input when we fail after that import.

    A realistic code example with try-finally blocks, but without an explicit check
    that the exception was raised.

    We don't test multiple raster, assuming that either all are removed or all kept.
    """
    try:
        with Tools(session=xy_dataset_session) as tools:
            tools.r_mapcalc_simple(
                expression="A + does_not_exist", a=rows_raster_file3x2, output="output"
            )
    except ToolError:
        pass
    finally:
        assert not tools.g_findfile(
            element="raster", file=rows_raster_file3x2.stem, format="json"
        )["name"]


def test_clean_after_tool_failure_with_context_and_raises(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check input and output pack files work with tool name call

    Checks that the exception was actually raised, but does not show the intention
    as clearly as the test with try-finally.
    """
    with (
        pytest.raises(ToolError, match=r"r\.mapcalc\.simple"),
        Tools(session=xy_dataset_session) as tools,
    ):
        tools.r_mapcalc_simple(
            expression="A + does_not_exist", a=rows_raster_file3x2, output="output"
        )
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert rows_raster_file3x2.exists()


def test_clean_after_tool_failure_without_context(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check we delete imported input when we fail after that import.

    A single call should clean after itself unless told otherwise.
    """
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(ToolError, match=r"r\.mapcalc\.simple"):
        tools.r_mapcalc_simple(
            expression="A + does_not_exist", a=rows_raster_file3x2, output="output"
        )
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert rows_raster_file3x2.exists()


def test_clean_after_tool_failure_without_context_with_use_cache(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check we don't delete imported input even after failure when asked.

    When explicitly requested, we wait for explicit request to delete the imported
    data even after a failure.
    """
    tools = Tools(session=xy_dataset_session, use_cache=True)
    with pytest.raises(ToolError, match=r"r\.mapcalc\.simple"):
        tools.r_mapcalc_simple(
            expression="A + does_not_exist", a=rows_raster_file3x2, output="output"
        )
    assert tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    tools.cleanup()
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert rows_raster_file3x2.exists()


def test_clean_after_call_failure_with_context_and_try(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check we delete imported input when we fail after that import.

    A realistic code example with try-finally blocks, but without an explicit check
    that the exception was raised.

    We don't test multiple raster, assuming that either all are removed or all kept.
    """
    try:
        with Tools(session=xy_dataset_session) as tools:
            tools.g_region(rows=3, cols=3)
            output_file = tmp_path / "does_not_exist" / "file.grass_raster"
            assert not output_file.parent.exists()
            # Non-existence of a directory will be failure inside r.pack which is
            # what we use to get an internal failure inside the call.
            # This relies on inputs being resolved before outputs.
            tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    except ToolError:
        pass
    finally:
        assert not tools.g_findfile(
            element="raster", file=rows_raster_file3x2.stem, format="json"
        )["name"]
        assert rows_raster_file3x2.exists()


def test_clean_after_call_failure_with_context_and_raises(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check input and output pack files work with tool name call

    Checks that the exception was actually raised, but does not show the intention
    as clearly as the test with try-finally.
    """
    with Tools(session=xy_dataset_session) as tools:
        tools.g_region(rows=3, cols=3)
        output_file = tmp_path / "does_not_exist" / "file.grass_raster"
        assert not output_file.parent.exists()
        # Non-existence of a directory will be failure inside r.pack which is
        # what we use to get an internal failure inside the call.
        # This relies on inputs being resolved before outputs.
        with pytest.raises(ToolError, match=r"r\.pack"):
            tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert rows_raster_file3x2.exists()


def test_clean_after_call_failure_without_context(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check we delete imported input when we fail after that import.

    A single call should clean after itself unless told otherwise.
    """
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=3, cols=3)
    output_file = tmp_path / "does_not_exist" / "file.grass_raster"
    assert not output_file.parent.exists()
    with pytest.raises(ToolError, match=r"r\.pack"):
        # Non-existence of a directory will be failure inside r.pack which is
        # what we use to get an internal failure inside the call.
        # This relies on inputs being resolved before outputs.
        tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert rows_raster_file3x2.exists()


def test_clean_after_call_failure_without_context_with_use_cache(
    xy_dataset_session, rows_raster_file3x2: Path, tmp_path: Path
):
    """Check we don't delete imported input even after failure when asked.

    When explicitly requested, we wait for explicit request to delete the imported
    data even after a failure.
    """
    tools = Tools(session=xy_dataset_session, use_cache=True)
    tools.g_region(rows=3, cols=3)
    output_file = tmp_path / "does_not_exist" / "file.grass_raster"
    assert not output_file.parent.exists()
    with pytest.raises(ToolError, match=r"r\.pack"):
        # Non-existence of a directory will be failure inside r.pack which is
        # what we use to get an internal failure inside the call.
        # This relies on inputs being resolved before outputs.
        tools.r_slope_aspect(elevation=rows_raster_file3x2, slope=output_file)
    assert tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    tools.cleanup()
    assert not tools.g_findfile(
        element="raster", file=rows_raster_file3x2.stem, format="json"
    )["name"]
    assert rows_raster_file3x2.exists()


def test_workflow_create_project_and_run_general_crs(
    tmp_path: Path, ones_raster_file_epsg3358
):
    """Check workflow with create project"""
    project = tmp_path / "project"
    raster = tmp_path / "raster.grass_raster"
    gs.create_project(project, crs=ones_raster_file_epsg3358)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="json")["crs"]["type"] == "other"
        result_dict = tools.g_proj(flags="p", format="projjson")
        assert result_dict["id"]["authority"] == "EPSG"
        assert result_dict["id"]["code"] == 3358
        tools.g_region(raster=ones_raster_file_epsg3358)
        assert tools.g_region(flags="p", format="json")["cells"] == 4 * 5
        tools.r_mapcalc_simple(
            expression="2 * A", a=ones_raster_file_epsg3358, output=raster
        )
        stats = tools.r_univar(map=raster, format="json")
        assert stats["cells"] == 4 * 5
        assert stats["min"] == 2
        assert stats["max"] == 2
        assert stats["mean"] == 2
        assert stats["sum"] == 4 * 5 * 1 * 2
    assert raster.exists()
    assert raster.is_file()


def test_workflow_create_project_and_run_ll_crs(
    tmp_path: Path, ones_raster_file_epsg4326
):
    """Check workflow with create project"""
    project = tmp_path / "project"
    raster = tmp_path / "raster.grass_raster"
    gs.create_project(project, crs=ones_raster_file_epsg4326)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="json")["crs"]["type"] == "ll"
        result_dict = tools.g_proj(flags="p", format="projjson")
        assert result_dict["id"]["authority"] == "EPSG"
        assert result_dict["id"]["code"] == 4326
        tools.g_region(raster=ones_raster_file_epsg4326)
        assert tools.g_region(flags="p", format="json")["cells"] == 4 * 5
        tools.r_mapcalc_simple(
            expression="2 * A", a=ones_raster_file_epsg4326, output=raster
        )
        stats = tools.r_univar(map=raster, format="json")
        assert stats["cells"] == 4 * 5
        assert stats["min"] == 2
        assert stats["max"] == 2
        assert stats["mean"] == 2
        assert stats["sum"] == 4 * 5 * 1 * 2
    assert raster.exists()
    assert raster.is_file()


def test_workflow_create_project_and_run_xy_crs(
    tmp_path: Path, rows_raster_file4x5: Path
):
    """Check workflow with create project"""
    project = tmp_path / "project"
    raster = tmp_path / "raster.grass_raster"
    gs.create_project(project, crs=rows_raster_file4x5)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        assert tools.g_region(flags="p", format="json")["crs"]["type"] == "xy"
        tools.g_region(raster=rows_raster_file4x5)
        assert tools.g_region(flags="p", format="json")["cells"] == 4 * 5
        tools.r_mapcalc_simple(expression="2 * A", a=rows_raster_file4x5, output=raster)
        stats = tools.r_univar(map=raster, format="json")
        assert stats["cells"] == 4 * 5
        assert stats["min"] == 2
        assert stats["max"] == 8
    assert raster.exists()
    assert raster.is_file()
