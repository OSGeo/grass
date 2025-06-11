"""Test grass.experimental.Tools class"""

import os
import io

import numpy as np
import pytest

import grass.script as gs
from grass.experimental.mapset import TemporaryMapsetSession
from grass.experimental.tools import Tools
from grass.exceptions import CalledModuleError


def test_key_value_parser_number(xy_dataset_session):
    """Check that numbers are parsed as numbers"""
    tools = Tools(session=xy_dataset_session)
    assert tools.g_region(flags="g").keyval["nsres"] == 1


@pytest.mark.xfail
def test_key_value_parser_multiple_values(xy_dataset_session):
    """Check that strings and floats are parsed"""
    tools = Tools(session=xy_dataset_session)
    name = "surface"
    tools.r_surf_gauss(output=name)  # needs seed
    result = tools.r_info(map=name, flags="g").keyval
    assert result["datatype"] == "DCELL"
    assert result["nsres"] == 1
    result = tools.r_univar(map=name, flags="g").keyval
    assert result["mean"] == pytest.approx(-0.756762744552762)


def test_json_parser(xy_dataset_session):
    """Check that JSON is parsed"""
    tools = Tools(session=xy_dataset_session)
    assert (
        tools.g_search_modules(keyword="random", flags="j").json[0]["name"]
        == "r.random"
    )


def test_json_with_name_and_parameter_call(xy_dataset_session):
    """Check that JSON is parsed with a name-and-parameters style call"""
    tools = Tools(session=xy_dataset_session)
    assert (
        tools.run("g.search.modules", keyword="random", flags="j")[0]["name"]
        == "r.random"
    )


def test_json_with_subprocess_run_like_call(xy_dataset_session):
    """Check that JSON is parsed with a name-and-parameters style call"""
    tools = Tools(session=xy_dataset_session)
    assert (
        tools.run_from_list(["g.search.modules", "keyword=random", "-j"])[0]["name"]
        == "r.random"
    )


def test_json_direct_access(xy_dataset_session):
    """Check that JSON is parsed"""
    tools = Tools(session=xy_dataset_session)
    assert tools.g_search_modules(keyword="random", flags="j")[0]["name"] == "r.random"


def test_json_direct_access_bad_key_type(xy_dataset_session):
    """Check that JSON is parsed"""
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(TypeError):
        tools.g_search_modules(keyword="random", flags="j")["name"]


def test_json_direct_access_bad_key_value(xy_dataset_session):
    """Check that JSON is parsed"""
    tools = Tools(session=xy_dataset_session)
    high_number = 100_000_000
    with pytest.raises(IndexError):
        tools.g_search_modules(keyword="random", flags="j")[high_number]


def test_json_direct_access_not_json(xy_dataset_session):
    """Check that JSON parsing creates an ValueError

    Specifically, this tests the case when format="json" is not set.
    """
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(ValueError, match=r"format.*json"):
        tools.g_search_modules(keyword="random")[0]["name"]


def test_stdout_as_text(xy_dataset_session):
    """Check that simple text is parsed and has no whitespace"""
    tools = Tools(session=xy_dataset_session)
    assert tools.g_mapset(flags="p").text == "PERMANENT"


def test_stdout_as_space_items(xy_dataset_session):
    """Check that whitespace-separated items are parsed"""
    tools = Tools(session=xy_dataset_session)
    assert tools.g_mapset(flags="l").space_items == ["PERMANENT"]


def test_stdout_split_whitespace(xy_dataset_session):
    """Check that whitespace-based split function works"""
    tools = Tools(session=xy_dataset_session)
    assert tools.g_mapset(flags="l").text_split() == ["PERMANENT"]


def test_stdout_split_space(xy_dataset_session):
    """Check that the split function works with space"""
    tools = Tools(session=xy_dataset_session)
    # Not a good example usage, but it tests the functionality.
    assert tools.g_mapset(flags="l").text_split(" ") == ["PERMANENT", ""]


def test_stdout_without_capturing(xy_dataset_session):
    """Check that text is not present when not capturing it"""
    tools = Tools(session=xy_dataset_session, capture_output=False)
    assert not tools.g_mapset(flags="p").text
    assert tools.g_mapset(flags="p").text is None


def test_direct_overwrite(xy_dataset_session):
    """Check overwrite as a parameter"""
    tools = Tools(session=xy_dataset_session)
    tools.r_random_surface(output="surface", seed=42)
    tools.r_random_surface(output="surface", seed=42, overwrite=True)


def test_object_overwrite(xy_dataset_session):
    """Check overwrite as parameter of the tools object"""
    tools = Tools(session=xy_dataset_session, overwrite=True)
    tools.r_random_surface(output="surface", seed=42)
    tools.r_random_surface(output="surface", seed=42)


def test_no_overwrite(xy_dataset_session):
    """Check that it fails without overwrite"""
    tools = Tools(session=xy_dataset_session)
    tools.r_random_surface(output="surface", seed=42)
    with pytest.raises(CalledModuleError, match="overwrite"):
        tools.r_random_surface(output="surface", seed=42)


def test_env_overwrite(xy_dataset_session):
    """Check that overwrite from env parameter is used"""
    # env = xy_dataset_session.env.copy()  # ideally
    env = os.environ.copy()  # for now
    env["GRASS_OVERWRITE"] = "1"
    tools = Tools(session=xy_dataset_session, env=env)
    tools.r_random_surface(output="surface", seed=42)
    tools.r_random_surface(output="surface", seed=42)


def test_global_overwrite_vs_env(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used"""
    # env = xy_dataset_session.env.copy()  # ideally
    env = os.environ.copy()  # for now
    os.environ["GRASS_OVERWRITE"] = "1"  # change to xy_dataset_session.env
    tools = Tools(session=xy_dataset_session, env=env)
    tools.r_random_surface(output="surface", seed=42)
    with pytest.raises(CalledModuleError, match="overwrite"):
        tools.r_random_surface(output="surface", seed=42)
    del os.environ["GRASS_OVERWRITE"]  # check or ideally remove this


def test_global_overwrite_vs_init(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used"""
    tools = Tools(session=xy_dataset_session)
    os.environ["GRASS_OVERWRITE"] = "1"  # change to xy_dataset_session.env
    tools.r_random_surface(output="surface", seed=42)
    with pytest.raises(CalledModuleError, match="overwrite"):
        tools.r_random_surface(output="surface", seed=42)
    del os.environ["GRASS_OVERWRITE"]  # check or ideally remove this


def test_stdin(xy_dataset_session):
    """Test that stdin is accepted"""
    tools = Tools(session=xy_dataset_session)
    tools.feed_input_to("13.45,29.96,200").v_in_ascii(
        input="-", output="point", separator=","
    )


def test_raises(xy_dataset_session):
    """Test that exception is raised for wrong parameter value"""
    tools = Tools(session=xy_dataset_session)
    wrong_name = "wrong_standard"
    with pytest.raises(CalledModuleError, match=wrong_name):
        tools.feed_input_to("13.45,29.96,200").v_in_ascii(
            input="-",
            output="point",
            format=wrong_name,
        )


def test_run_command(xy_dataset_session):
    """Check run_command and its overwrite parameter"""
    tools = Tools(session=xy_dataset_session)
    tools.run_command("r.random.surface", output="surface", seed=42)
    tools.run_command("r.random.surface", output="surface", seed=42, overwrite=True)


def test_parse_command_key_value(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    assert tools.parse_command("g.region", flags="g")["nsres"] == "1"


def test_parse_command_json(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    assert (
        tools.parse_command("g.region", flags="g", format="json")["region"]["ns-res"]
        == 1
    )


def test_with_context_managers(tmpdir):
    project = tmpdir / "project"
    gs.create_project(project)
    with gs.setup.init(project) as session:
        tools = Tools(session=session)
        tools.r_random_surface(output="surface", seed=42)
        with TemporaryMapsetSession(env=tools.env) as mapset:
            tools.r_random_surface(output="surface", seed=42, env=mapset.env)
            with gs.MaskManager(env=mapset.env) as mask:
                # TODO: Do actual test
                tools.r_univar(map="surface", env=mask.env, format="json")["mean"]


def test_misspelling(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(AttributeError, match=r"r\.slope\.aspect"):
        tools.r_sloppy_respect()


def test_multiple_suggestions(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(AttributeError, match=r"v\.db\.univar|db\.univar"):
        tools.db_v_uni_var()


def test_tool_group_vs_model_name(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(AttributeError, match=r"r\.sim\.water"):
        tools.rSIMWEwater()


def test_wrong_attribute(xy_dataset_session):
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(AttributeError, match="execute_big_command"):
        tools.execute_big_command()


def test_numpy_one_input(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used"""
    tools = Tools(session=xy_dataset_session)
    tools.r_slope_aspect(elevation=np.ones((1, 1)), slope="slope")
    assert tools.r_info(map="slope", format="json")["datatype"] == "FCELL"


# NumPy syntax for outputs
# While inputs are straightforward, there is several possible ways how to handle
# syntax for outputs.
# Output is the type of function for creating NumPy arrays, return value is now the arrays:
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope=np.ndarray, aspect=np.array)
# Output is explicitly requested:
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope="slope", aspect="aspect", force_numpy_for_output=True)
# Output is explicitly requested at the object level:
# Tools(force_numpy_for_output=True).r_slope_aspect(elevation=np.ones((1, 1)), slope="slope", aspect="aspect")
# Output is always array or arrays when at least on input is an array:
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope="slope", aspect="aspect")
# An empty array is passed to signal the desired output:
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope=np.nulls((0, 0)))
# An array to be filled with data is passed, the return value is kept as is:
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope=np.nulls((1, 1)))
# NumPy universal function concept can be used explicitly to indicate,
# possibly more easily allowing for nameless args as opposed to keyword arguments,
# but outputs still need to be explicitly requested:
# Returns by value (tuple: (np.array, np.array)):
# tools.r_slope_aspect.ufunc(np.ones((1, 1)), slope=True, aspect=True)
# Modifies its arguments in-place:
# tools.r_slope_aspect.ufunc(np.ones((1, 1)), slope=True, aspect=True, out=(np.array((1, 1)), np.array((1, 1))))
# Custom signaling classes or objects are passed (assuming empty classes AsNumpy and AsInput):
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope=ToNumpy(), aspect=ToNumpy())
# tools.r_slope_aspect(elevation=np.ones((1, 1)), slope=AsInput, aspect=AsInput)
# NumPy functions usually return a tuple, for multiple outputs. Universal function does
# unless the output is written to out parameter which is also provided as a tuple. We
# have names, so generally, we can return a dictionary:
# {"slope": np.array(...), "aspect": np.array(...) }.


def test_numpy_one_input_one_output(xy_dataset_session):
    """Check that a NumPy array works as input and for signaling output

    It tests that the np.ndarray class is supported to signal output.
    Return type is not strictly defined, so we are not testing for it explicitly
    (only by actually using it as an NumPy array).
    """
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=2, cols=3)
    slope = tools.r_slope_aspect(elevation=np.ones((2, 3)), slope=np.ndarray)
    assert slope.shape == (2, 3)
    assert np.all(slope == np.full((2, 3), 0))


def test_numpy_with_name_and_parameter(xy_dataset_session):
    """Check that a NumPy array works as input and for signaling output

    It tests that the np.ndarray class is supported to signal output.
    Return type is not strictly defined, so we are not testing for it explicitly
    (only by actually using it as an NumPy array).
    """
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=2, cols=3)
    slope = tools.run("r.slope.aspect", elevation=np.ones((2, 3)), slope=np.ndarray)
    assert slope.shape == (2, 3)
    assert np.all(slope == np.full((2, 3), 0))


def test_numpy_one_input_multiple_outputs(xy_dataset_session):
    """Check that a NumPy array function works for signaling multiple outputs

    Besides multiple outputs it tests that np.array is supported to signal output.
    """
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=2, cols=3)
    (slope, aspect) = tools.r_slope_aspect(
        elevation=np.ones((2, 3)), slope=np.array, aspect=np.array
    )
    assert slope.shape == (2, 3)
    assert np.all(slope == np.full((2, 3), 0))
    assert aspect.shape == (2, 3)
    assert np.all(aspect == np.full((2, 3), 0))


def test_numpy_multiple_inputs_one_output(xy_dataset_session):
    """Check that a NumPy array works for multiple inputs"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(rows=2, cols=3)
    result = tools.r_mapcalc_simple(
        expression="A + B", a=np.full((2, 3), 2), b=np.full((2, 3), 5), output=np.array
    )
    assert result.shape == (2, 3)
    assert np.all(result == np.full((2, 3), 7))


def test_numpy_grass_array_input_output(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used

    When grass array output is requested, we explicitly test the return value type.
    """
    tools = Tools(session=xy_dataset_session)
    rows = 2
    cols = 3
    tools.g_region(rows=rows, cols=cols)
    tools.r_mapcalc_simple(expression="5", output="const_5")
    const_5 = gs.array.array("const_5")
    result = tools.r_mapcalc_simple(
        expression="2 * A", a=const_5, output=gs.array.array
    )
    assert result.shape == (rows, cols)
    assert np.all(result == np.full((rows, cols), 10))
    assert isinstance(result, gs.array.array)


def test_tool_groups_raster(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used"""
    raster = Tools(session=xy_dataset_session, prefix="r")
    raster.mapcalc(expression="streams = if(row() > 1, 1, null())")
    raster.buffer(input="streams", output="buffer", distance=1)
    assert raster.info(map="streams", format="json")["datatype"] == "CELL"


def test_tool_groups_vector(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used"""
    vector = Tools(prefix="v")
    vector.edit(map="points", type="point", tool="create", env=xy_dataset_session.env)
    # Here, the feed_input_to style does not make sense, but we are not using StringIO
    # here to test the feed_input_to functionality and avoid dependence on the StringIO
    # functionality.
    # The ASCII format is for one point with no categories.
    vector.feed_input_to("P 1 0\n  10 20").edit(
        map="points",
        type="point",
        tool="add",
        input="-",
        flags="n",
        env=xy_dataset_session.env,
    )
    vector.buffer(
        input="points", output="buffer", distance=1, env=xy_dataset_session.env
    )
    assert (
        vector.info(map="buffer", format="json", env=xy_dataset_session.env)["areas"]
        == 1
    )


def test_stdin_as_stringio_object(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used"""
    tools = Tools(session=xy_dataset_session)
    tools.v_edit(map="points", type="point", tool="create")
    tools.v_edit(
        map="points",
        type="point",
        tool="add",
        input=io.StringIO("P 1 0\n  10 20"),
        flags="n",
    )
    assert tools.v_info(map="points", format="json")["points"] == 1
