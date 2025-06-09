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
    assert os.path.exists("file.grass_raster")


def test_numpy_multiple_inputs_one_output():
    """Check that a NumPy array works for multiple inputs"""
    import numpy as np

    tools = StandaloneTools()
    tools.g_region(rows=2, cols=3)
    result = tools.r_mapcalc_simple(
        expression="A + B", a=np.full((2, 3), 2), b=np.full((2, 3), 5), output=np.array
    )
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
