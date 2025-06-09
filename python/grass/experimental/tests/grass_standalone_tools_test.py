"""Test grass.experimental.StandaloneTools class"""

import os


from grass.experimental.standalone import StandaloneTools


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
    assert os.path.exists(rows_raster_file3x3)
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


def test_numpy_multiple_inputs_one_output():
    """Check that a NumPy array works for multiple inputs"""
    import numpy as np

    tools = StandaloneTools()
    result = tools.r_mapcalc_simple(
        expression="A + B", a=np.full((2, 3), 2), b=np.full((2, 3), 5), output=np.array
    )
    assert result.shape == (2, 3)
    assert np.all(result == np.full((2, 3), 7))


def test_numpy_initial_region_set():
    """Check that a NumPy array works for multiple inputs"""
    import numpy as np

    tools = StandaloneTools()
    result = tools.r_mapcalc_simple(
        expression="A + B", a=np.full((2, 3), 2), b=np.full((2, 3), 5), output=np.array
    )
    region = tools.g_region(flags="p", format="json")
    assert region["rows"] == 2
    assert region["cols"] == 3
    assert result.shape == (2, 3)
    assert np.all(result == np.full((2, 3), 7))


# May fail depending on the order of the tests because of the session setup.
def test_existing_session_accepted(xy_mapset_non_permament):
    """Check that numbers are parsed as numbers"""
    tools = StandaloneTools(session=xy_mapset_non_permament)
    assert tools.g_mapset(flags="p").text == "test1"


def test_another_session_is_ignored(xy_session):
    """Check that numbers are parsed as numbers"""
    from grass.experimental import MapsetSession

    with MapsetSession("test1", create=True, env=xy_session.env) as session:
        tools = StandaloneTools()
        assert tools.g_mapset(flags="p").text == "PERMANENT"
        from grass.experimental.tools import Tools

        tools = Tools(session=session)
        assert tools.g_mapset(flags="p").text == "test1"
