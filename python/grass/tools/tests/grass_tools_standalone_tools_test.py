"""Test grass.experimental.StandaloneTools class"""

import os

import numpy as np
import pytest

from grass.tools import StandaloneTools


def test_json():
    """Check that numbers are parsed as numbers"""
    tools = StandaloneTools()
    assert (
        tools.run_from_list(["g.region", "-p", "format=json"])["region"]["ns-res"] == 1
    )


def test_run_name_and_parameters():
    """Check that name-and-parameters style works"""
    tools = StandaloneTools()
    assert tools.run("g.region", flags="p", format="json")["region"]["ns-res"] == 1


# TODO: This may fail because shutils.which does not setup a session.
def test_function_call_with_json():
    """Check that numbers are parsed as numbers"""
    tools = StandaloneTools()
    assert tools.g_region(flags="p", format="json")["region"]["ns-res"] == 1


def test_key_value_parser_number():
    """Check that numbers are parsed as numbers"""
    tools = StandaloneTools()
    assert tools.run_from_list(["g.region", "-g"]).keyval["nsres"] == 1


def test_pack_input_output(rows_raster_file3x3):
    """Check that global overwrite is not used when separate env is used"""
    tools = StandaloneTools()
    assert os.path.exists(rows_raster_file3x3)
    tools.run_from_list(
        [
            "r.slope.aspect",
            f"elevation={rows_raster_file3x3}",
            "aspect=file.grass_raster",
        ]
    )
    tools.run_from_list(
        [
            "r.slope.aspect",
            f"elevation={rows_raster_file3x3}",
            "aspect=file.grass_raster",
        ]
    )
    assert os.path.exists("file.grass_raster")


def test_pack_initial_region_set(rows_raster_file3x3, rows_raster_file4x5):
    """Check that global overwrite is not used when separate env is used"""
    tools = StandaloneTools()
    assert os.path.exists(rows_raster_file3x3)
    tools.r_slope_aspect(
        elevation=rows_raster_file3x3,
        aspect="file.grass_raster",
        flags="a",
    )
    region = tools.g_region(flags="p", format="json")
    assert region["rows"] == 3
    assert region["cols"] == 3


def test_pack_initial_region_respected(rows_raster_file3x3, rows_raster_file4x5):
    """Check that global overwrite is not used when separate env is used"""
    tools = StandaloneTools()
    tools.r_slope_aspect(
        elevation=rows_raster_file3x3,
        aspect="file.grass_raster",
        flags="a",
    )
    tools.r_slope_aspect(
        elevation=rows_raster_file4x5,
        aspect="file2.grass_raster",
        flags="a",
    )
    assert os.path.exists("file.grass_raster")
    assert os.path.exists("file2.grass_raster")
    region = tools.g_region(flags="p", format="json")
    assert region["rows"] == 3
    assert region["cols"] == 3
    region = tools.r_info(map="file.grass_raster", format="json")
    assert region["rows"] == 3
    assert region["cols"] == 3
    region = tools.r_info(map="file2.grass_raster", format="json")
    assert region["rows"] == 3
    assert region["cols"] == 3


def test_pack_manual_region_from_raster_preserved(
    rows_raster_file3x3, rows_raster_file4x5
):
    """Check that global overwrite is not used when separate env is used"""
    tools = StandaloneTools()
    tools.g_region(raster=rows_raster_file4x5)
    tools.r_slope_aspect(
        elevation=rows_raster_file3x3,
        aspect="file.grass_raster",
        flags="a",
    )
    assert os.path.exists("file.grass_raster")
    region = tools.g_region(flags="p", format="json")
    assert region["rows"] == 4
    assert region["cols"] == 5


def test_pack_manual_region_from_parameters_preserved(rows_raster_file3x3):
    """Check that global overwrite is not used when separate env is used"""
    tools = StandaloneTools()
    tools.g_region(rows=4, cols=5)
    tools.r_slope_aspect(
        elevation=rows_raster_file3x3,
        aspect="file.grass_raster",
        flags="a",
    )
    assert os.path.exists("file.grass_raster")
    region = tools.g_region(flags="p", format="json")
    assert region["rows"] == 4
    assert region["cols"] == 5


def test_pack_manual_region_read_but_not_set(rows_raster_file3x3):
    """Check that global overwrite is not used when separate env is used"""
    tools = StandaloneTools()
    region = tools.g_region(flags="p", format="json")
    assert region["rows"] == 1
    assert region["cols"] == 1
    tools.r_slope_aspect(
        elevation=rows_raster_file3x3,
        aspect="file.grass_raster",
        flags="a",
    )
    assert os.path.exists("file.grass_raster")
    region = tools.g_region(flags="p", format="json")
    assert region["rows"] == 3
    assert region["cols"] == 3


def test_pack_region_manually_changed(rows_raster_file3x3, rows_raster_file4x5):
    """Check that global overwrite is not used when separate env is used"""
    tools = StandaloneTools()
    tools.r_slope_aspect(
        elevation=rows_raster_file3x3,
        aspect="file.grass_raster",
        flags="a",
    )
    region = tools.g_region(rows=2, cols=2)
    region = tools.g_region(flags="p", format="json")
    assert region["rows"] == 2
    assert region["cols"] == 2
    tools.r_slope_aspect(
        elevation=rows_raster_file4x5,
        aspect="file2.grass_raster",
        flags="a",
    )
    assert os.path.exists("file.grass_raster")
    assert os.path.exists("file2.grass_raster")
    region = tools.g_region(flags="p", format="json")
    assert region["rows"] == 2
    assert region["cols"] == 2


def test_pack_workflow(rows_raster_file4x5):
    tools = StandaloneTools()
    tools.r_slope_aspect(
        elevation=rows_raster_file4x5, slope="slope.grr", aspect="aspect"
    )
    tools.r_flow(
        elevation=rows_raster_file4x5.stem,
        aspect="aspect",
        flowaccumulation="flow_accumulation.grr",
        flags=3,
    )
    assert os.path.exists(rows_raster_file4x5)
    assert os.path.exists("slope.grr")
    assert os.path.exists("flow_accumulation.grr")
    assert not os.path.exists("aspect.grr")
    assert not os.path.exists("elevation.grr")


def test_regions_independent():
    """Check that global overwrite is not used when separate env is used"""
    tools1 = StandaloneTools()
    tools1.g_region(rows=3, cols=3)
    tools2 = StandaloneTools()
    tools2.g_region(rows=4, cols=5)
    region = tools1.g_region(flags="p", format="json")
    assert region["rows"] == 3
    assert region["cols"] == 3
    region = tools2.g_region(flags="p", format="json")
    assert region["rows"] == 4
    assert region["cols"] == 5


def test_region_changed_for_session(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used"""
    tools1 = StandaloneTools(session=xy_dataset_session)
    tools1.g_region(rows=4, cols=5)
    tools2 = StandaloneTools(session=xy_dataset_session)
    region = tools2.g_region(flags="p", format="json")
    assert region["rows"] == 4
    assert region["cols"] == 5


def test_numpy_multiple_inputs_one_output():
    """Check that a NumPy array works for multiple inputs"""
    tools = StandaloneTools()
    result = tools.r_mapcalc_simple(
        expression="A + B", a=np.full((2, 3), 2), b=np.full((2, 3), 5), output=np.array
    )
    # Size of output
    assert result.shape == (2, 3)
    assert np.all(result == np.full((2, 3), 7))
    # Computational region
    region = tools.g_region(flags="p", format="json")
    assert region["rows"] == 2
    assert region["cols"] == 3
    result = tools.r_mapcalc_simple(expression="int(10)", output=np.array)
    assert result.shape == (2, 3)
    assert np.all(result == np.full((2, 3), 10))


def test_numpy_manual_region_mismatch():
    """Check that a NumPy array works for multiple inputs"""
    tools = StandaloneTools()
    result = tools.r_mapcalc_simple(
        expression="A + B", a=np.full((2, 3), 2), b=np.full((2, 3), 5), output=np.array
    )
    assert result.shape == (2, 3)
    region = tools.g_region(rows=2, cols=2)
    region = tools.g_region(flags="p", format="json")
    assert region["rows"] == 2
    assert region["cols"] == 2
    with pytest.raises(ValueError, match=r"broadcast.*shape.*\(2.*3\).*\(2.*2\)"):
        tools.r_mapcalc_simple(
            expression="A + B",
            a=np.full((2, 3), 2),
            b=np.full((2, 3), 5),
            output=np.array,
        )


def test_numpy_workflow_region_mismatch():
    """Check that a NumPy array works for multiple inputs"""
    tools = StandaloneTools()
    result = tools.r_mapcalc_simple(
        expression="A + B", a=np.full((2, 3), 2), b=np.full((2, 3), 5), output=np.array
    )
    assert result.shape == (2, 3)
    with pytest.raises(ValueError, match=r"broadcast.*shape.*\(4.*5\).*\(2.*3\)"):
        tools.r_mapcalc_simple(
            expression="A + B",
            a=np.full((4, 5), 2),
            b=np.full((4, 5), 5),
            output=np.array,
        )


def test_numpy_inputs_region_mismatch():
    """Check that a NumPy array works for multiple inputs"""
    tools = StandaloneTools()
    with pytest.raises(ValueError, match=r"broadcast.*shape.*\(4.*5\).*\(2.*3\)"):
        tools.r_mapcalc_simple(
            expression="A + B",
            a=np.full((2, 3), 2),
            b=np.full((4, 5), 5),
            output=np.array,
        )


# May fail depending on the order of the tests because of the session setup.
def test_existing_session_accepted(xy_mapset_non_permament):
    """Check that numbers are parsed as numbers"""
    tools = StandaloneTools(session=xy_mapset_non_permament)
    assert tools.g_mapset(flags="p").text == xy_mapset_non_permament.name


def test_another_session_is_ignored(xy_dataset_session):
    """Check that numbers are parsed as numbers"""
    from grass.experimental import MapsetSession

    with MapsetSession("test1", create=True, env=xy_dataset_session.env) as session:
        tools = StandaloneTools()
        assert tools.g_mapset(flags="p").text == "PERMANENT"
        from grass.experimental.tools import Tools

        tools = Tools(session=session)
        assert tools.g_mapset(flags="p").text == "test1"
