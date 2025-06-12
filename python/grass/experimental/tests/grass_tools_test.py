"""Test grass.experimental.Tools class"""

import os
import io

import pytest

import grass.script as gs
from grass.experimental.mapset import TemporaryMapsetSession
from grass.experimental.tools import Tools
from grass.exceptions import CalledModuleError


def test_key_value_parser_number(xy_dataset_session):
    """Check that numbers are parsed as numbers"""
    tools = Tools(session=xy_dataset_session)
    assert tools.g_region(flags="g").keyval["nsres"] == 1


def test_key_value_parser_multiple_values(xy_dataset_session):
    """Check that strings and floats are parsed"""
    tools = Tools(session=xy_dataset_session)
    name = "surface"
    tools.r_surf_gauss(output=name)  # needs seed
    result = tools.r_info(map=name, flags="g").keyval
    assert result["datatype"] == "DCELL"
    assert result["nsres"] == 1
    result = tools.r_univar(map=name, flags="g").keyval
    # We don't worry about the specific output value, and just test the type.
    assert isinstance(result["mean"], float)


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


def test_help_call_no_parameters(xy_dataset_session):
    """Check that JSON is parsed with a name-and-parameters style call"""
    tools = Tools(session=xy_dataset_session)
    assert (
        "r.slope.aspect"
        in tools.no_nonsense_run_from_list(["r.slope.aspect", "--help"]).stderr
    )


def test_help_call_with_parameters(xy_dataset_session):
    """Check that JSON is parsed with a name-and-parameters style call"""
    tools = Tools(session=xy_dataset_session)
    assert (
        "r.slope.aspect"
        in tools.no_nonsense_run_from_list(
            ["r.slope.aspect", "elevation=dem", "slope=slope", "--help"]
        ).stderr
    )


def test_json_call_with_low_level_call(xy_dataset_session):
    """Check that --json call works including JSON data parsing"""
    tools = Tools(session=xy_dataset_session)
    data = tools.no_nonsense_run_from_list(
        ["r.slope.aspect", "elevation=dem", "slope=slope", "--json"]
    ).json
    assert "inputs" in data
    assert data["inputs"][0]["value"] == "dem"


def test_json_call_with_high_level_call(xy_dataset_session):
    """Check that --json call works including JSON data parsing"""
    tools = Tools(session=xy_dataset_session)
    data = tools.run_from_list(
        ["r.slope.aspect", "elevation=dem", "slope=slope", "--json"]
    ).json
    assert "inputs" in data
    assert data["inputs"][0]["value"] == "dem"


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
    assert tools.g_mapset(flags="l").text_split(" ") == ["PERMANENT"]


def test_stdout_without_capturing(xy_dataset_session):
    """Check that text is not present when not capturing it"""
    tools = Tools(session=xy_dataset_session, capture_output=False)
    result = tools.g_mapset(flags="p")
    assert not result.stdout
    assert result.stdout is None
    assert not result.text
    assert result.text is None


def test_capturing_stderr(xy_dataset_session):
    """Check that text is not present when not capturing it"""
    tools = Tools(session=xy_dataset_session, errors="ignore")
    result = tools.g_mapset(mapset="does_not_exist")
    assert result.stderr
    assert "does_not_exist" in result.stderr


def test_stderr_without_capturing(xy_dataset_session):
    """Check that text is not present when not capturing it"""
    tools = Tools(session=xy_dataset_session, capture_output=False, errors="ignore")
    result = tools.g_mapset(mapset="does_not_exist")
    assert not result.stderr
    assert result.stderr is None


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
