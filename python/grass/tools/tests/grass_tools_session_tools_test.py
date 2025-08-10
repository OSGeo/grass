"""Test grass.tools.Tools class"""

import io
import os

import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError
from grass.experimental.mapset import TemporaryMapsetSession
from grass.tools import Tools


def test_key_value_parser_number(xy_dataset_session):
    """Check that numbers are parsed as numbers from key-value (shell) output"""
    tools = Tools(session=xy_dataset_session)
    assert tools.g_region(flags="p", format="shell").keyval["nsres"] == 1


def test_key_value_parser_multiple_values(xy_dataset_session):
    """Check that strings and floats are parsed"""
    tools = Tools(session=xy_dataset_session)
    name = "surface"
    tools.r_surf_gauss(output=name)  # needs seed
    result = tools.r_info(map=name, format="shell").keyval
    assert result["datatype"] == "DCELL"
    assert result["nsres"] == 1
    result = tools.r_univar(map=name, format="shell").keyval
    # We don't worry about the specific output value, and just test the type.
    assert isinstance(result["mean"], float)


def test_json_parser(xy_dataset_session):
    """Check that JSON is parsed"""
    tools = Tools(session=xy_dataset_session)
    assert tools.g_region(flags="p", format="json").json["cols"] == 1


def test_json_with_name_and_parameter_call(xy_dataset_session):
    """Check that JSON is parsed with a name-and-parameters style call"""
    tools = Tools(session=xy_dataset_session)
    assert tools.run("g.region", flags="p", format="json").json["cols"] == 1


def test_json_with_direct_name_and_parameter_call(xy_dataset_session):
    """Check that JSON is parsed with a name-and-parameters style call"""
    tools = Tools(session=xy_dataset_session)
    assert tools.call("g.region", flags="p", format="json").json["cols"] == 1


def test_json_with_subprocess_run_like_call(xy_dataset_session):
    """Check that JSON is parsed with a name-and-parameters style call"""
    tools = Tools(session=xy_dataset_session)
    assert tools.run_cmd(["g.region", "format=json", "-p"]).json["cols"] == 1


def test_json_with_direct_subprocess_run_like_call(xy_dataset_session):
    """Check that JSON is parsed with a name-and-parameters style call"""
    tools = Tools(session=xy_dataset_session)
    assert tools.call_cmd(["g.region", "format=json", "-p"]).json["cols"] == 1


def test_json_as_list(xy_dataset_session):
    """Check that a JSON result behaves as a list"""
    tools = Tools(session=xy_dataset_session)
    # This also tests JSON parsing with a format option.
    result = tools.g_search_modules(keyword="random", flags="j")
    for item in result:
        assert "name" in item
    assert len(result)


def test_help_call_no_parameters(xy_dataset_session):
    """Check that help text is generated without any one parameters provided"""
    tools = Tools(session=xy_dataset_session)
    assert "r.slope.aspect" in tools.call_cmd(["r.slope.aspect", "--help"]).stderr


def test_help_call_with_parameters(xy_dataset_session):
    """Check that help text is generated with parameters provided"""
    tools = Tools(session=xy_dataset_session)
    assert (
        "r.slope.aspect"
        in tools.call_cmd(
            ["r.slope.aspect", "elevation=dem", "slope=slope", "--help"]
        ).stderr
    )


def test_json_call_with_low_level_call(xy_dataset_session):
    """Check that --json call works including JSON data parsing"""
    tools = Tools(session=xy_dataset_session)
    # This also tests JSON parsing with a format option.
    data = tools.call_cmd(
        ["r.slope.aspect", "elevation=dem", "slope=slope", "--json"]
    ).json
    assert "inputs" in data
    assert data["inputs"][0]["value"] == "dem"


def test_json_call_with_high_level_call(xy_dataset_session):
    """Check that --json call works including JSON data parsing"""
    tools = Tools(session=xy_dataset_session)
    data = tools.run_cmd(
        ["r.slope.aspect", "elevation=dem", "slope=slope", "--json"]
    ).json
    assert "inputs" in data
    assert data["inputs"][0]["value"] == "dem"


def test_json_direct_access(xy_dataset_session):
    """Check that JSON data can be directly accessed"""
    tools = Tools(session=xy_dataset_session)
    assert tools.g_region(flags="p", format="json")["cols"] == 1


def test_json_direct_access_bad_key_type(xy_dataset_session):
    """Check that JSON indexing fails when using a wrong type"""
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(TypeError):
        tools.g_search_modules(keyword="random", flags="j")["name"]


def test_json_direct_access_bad_key_value(xy_dataset_session):
    """Check that JSON indexing fails when using a wrong value"""
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


def test_stdout_comma_items(xy_dataset_session):
    """Check that the comma-separated split works"""
    tools = Tools(session=xy_dataset_session)
    result = tools.g_gisenv(
        get="GISDBASE,LOCATION_NAME,MAPSET", sep="comma"
    ).comma_items
    assert len(result) == 3
    for item in result:
        assert item


def test_stdout_without_capturing(xy_dataset_session):
    """Check that stdout and stderr are not present when not capturing it"""
    tools = Tools(session=xy_dataset_session, capture_output=False)
    result = tools.g_mapset(flags="p")
    assert not result.stdout
    assert result.stdout is None
    assert not result.stderr
    assert result.stderr is None


def test_attributes_without_stdout(xy_dataset_session):
    """Check that attributes behave as expected when not capturing stdout"""
    tools = Tools(session=xy_dataset_session, capture_output=False)
    result = tools.run_cmd(["g.region", "format=json", "-p"])
    assert not result.text
    assert result.text is None
    # We pretend like we want the value. That makes for more natural syntax
    # for the test and also makes code analysis happy.
    value = None
    with pytest.raises(ValueError, match="No text output"):
        value = result.json
    with pytest.raises(ValueError, match="No text output"):
        value = result["cols"]
    assert not result.keyval
    with pytest.raises(KeyError):
        value = result.keyval["cols"]
    assert value is None


def test_capturing_stdout_and_stderr(xy_dataset_session):
    """Check that both stdout and stdin are present when capturing them"""
    tools = Tools(session=xy_dataset_session, capture_output=True, errors="ignore")
    result = tools.g_mapset(flags="l")
    assert result.stdout
    assert result.stdout is not None
    assert "PERMANENT" in result.stdout
    result = tools.g_mapset(mapset="does_not_exist")
    assert result.stderr
    assert result.stderr is not None
    assert "does_not_exist" in result.stderr


def test_capturing_stdout_without_stderr(xy_dataset_session):
    """Check that only stdout is present when stderr is not captured"""
    tools = Tools(
        session=xy_dataset_session,
        capture_output=True,
        capture_stderr=False,
        errors="ignore",
    )
    result = tools.g_mapset(flags="l")
    assert result.stdout
    assert result.stdout is not None
    assert "PERMANENT" in result.stdout
    result = tools.g_mapset(mapset="does_not_exist")
    assert not result.stderr
    assert result.stderr is None


def test_capturing_stderr_without_stdout(xy_dataset_session):
    """Check that only stderr is present when stdout is not captured"""
    tools = Tools(
        session=xy_dataset_session,
        capture_output=False,
        capture_stderr=True,
        errors="ignore",
    )
    result = tools.g_mapset(flags="p")
    assert not result.stdout
    assert result.stdout is None
    result = tools.g_mapset(mapset="does_not_exist")
    assert result.stderr
    assert result.stderr is not None
    assert "does_not_exist" in result.stderr


def test_capturing_stderr_for_exception_without_stdout(xy_dataset_session):
    """Check that stderr is part of the exception message when not capturing stdout"""
    tools = Tools(session=xy_dataset_session, capture_output=False, capture_stderr=True)
    # Testing the actual English message here because that is really generated by the
    # tool itself rather than the message containing the tool parameters.
    with pytest.raises(CalledModuleError, match="does not exist"):
        tools.g_mapset(mapset="does_not_exist")


def test_raises(xy_dataset_session):
    """Test that exception is raised for wrong parameter value"""
    tools = Tools(session=xy_dataset_session)
    wrong_name = "does_not_exist"
    with pytest.raises(CalledModuleError, match=wrong_name):
        tools.r_slope_aspect(
            elevation=wrong_name,
            slope="slope",
        )


def test_error_handler_default(xy_dataset_session):
    """Check that default error handler works as expected"""
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(CalledModuleError, match="does_not_exist"):
        tools.g_mapset(mapset="does_not_exist")
    # Works after errors as usual.
    assert tools.g_mapset(flags="p").text == "PERMANENT"


def test_error_handler_none(xy_dataset_session):
    """Check that explicit None error handler works as expected"""
    tools = Tools(session=xy_dataset_session, errors=None)
    with pytest.raises(CalledModuleError, match="does_not_exist"):
        tools.g_mapset(mapset="does_not_exist")
    # Works after errors as usual.
    assert tools.g_mapset(flags="p").text == "PERMANENT"


def test_error_handler_raise(xy_dataset_session):
    """Check that raise error handler raises an exception"""
    tools = Tools(session=xy_dataset_session, errors="raise")
    with pytest.raises(CalledModuleError, match="does_not_exist"):
        tools.g_mapset(mapset="does_not_exist")
    # Works after errors as usual.
    assert tools.g_mapset(flags="p").text == "PERMANENT"


def test_error_handler_ignore(xy_dataset_session):
    """Check that ignore error handler does not raise and terminate"""
    tools = Tools(session=xy_dataset_session, errors="ignore")
    tools.g_mapset(mapset="does_not_exist")
    tools.g_region(raster="does_not_exist")
    # Works after errors as usual.
    assert tools.g_mapset(flags="p").text == "PERMANENT"


def test_error_handler_check_returncode(xy_dataset_session):
    """Check that ignore error handler allows to check return code"""
    tools = Tools(session=xy_dataset_session, errors="ignore")
    assert tools.g_mapset(mapset="does_not_exist").returncode == 1
    assert tools.g_region(raster="does_not_exist").returncode == 1
    # Works after errors as usual with return code.
    assert tools.g_mapset(flags="p").returncode == 0


def test_error_handler_fatal(xy_dataset_session):
    """Check that fatal error handler exits"""
    tools = Tools(session=xy_dataset_session, errors="fatal")
    with pytest.raises(SystemExit):
        assert tools.g_mapset(mapset="does_not_exist")


def test_error_handler_exit(xy_dataset_session):
    """Check that exit error handler exits"""
    tools = Tools(session=xy_dataset_session, errors="exit")
    with pytest.raises(SystemExit):
        assert tools.g_mapset(mapset="does_not_exist")


def test_verbose(xy_dataset_session):
    """Check that verbose message is generated"""
    tools = Tools(session=xy_dataset_session, verbose=True)
    tools.g_mapset(mapset="test1", flags="c")
    tools.g_mapset(mapset="test2", flags="c")
    result = tools.g_mapsets(mapset="test1")
    # This depends on the specific user messages being produced.
    # (Assumes name of the mapset is in a verbose message.)
    assert "test1" in result.stderr


def test_quiet(xy_dataset_session):
    """Check that quiet configuration works"""
    tools = Tools(session=xy_dataset_session, quiet=True)
    result = tools.g_mapsets(mapset="PERMANENT")
    # Expecting an important message about no modification.
    assert result.stderr


def test_superquiet(xy_dataset_session):
    """Check that superquiet configuration does not generate any messages"""
    tools = Tools(session=xy_dataset_session, superquiet=True)
    result = tools.g_mapsets(mapset="PERMANENT")
    assert not result.stderr


def test_superquiet_verbose(xy_dataset_session):
    """Check that text function-level verbose overrides object-level superquiet"""
    tools = Tools(session=xy_dataset_session, superquiet=True)
    result = tools.g_mapsets(mapset="PERMANENT", verbose=True)
    assert result.stderr


def test_verbose_superquiet(xy_dataset_session):
    """Check that text function-level superquiet overrides object-level verbose"""
    tools = Tools(session=xy_dataset_session, verbose=True)
    result = tools.g_mapsets(mapset="PERMANENT", superquiet=True)
    assert not result.stderr


def test_superquiet_false(xy_dataset_session):
    """Check that text object-level superquiet False overrides global superquiet"""
    xy_dataset_session.env["GRASS_VERBOSE"] = "0"
    tools = Tools(session=xy_dataset_session, superquiet=False)
    result = tools.g_mapsets(mapset="PERMANENT")
    assert result.stderr
    assert "GRASS_VERBOSE" in xy_dataset_session.env
    assert xy_dataset_session.env["GRASS_VERBOSE"] == "0"
    assert "GRASS_VERBOSE" not in tools._modified_env_if_needed(), (
        f"GRASS_VERBOSE has value {tools._modified_env_if_needed()['GRASS_VERBOSE']}"
    )  # pylint: disable=protected-access


def test_verbose_false(xy_dataset_session):
    """Check that text object-level verbose False overrides global verbose"""
    xy_dataset_session.env["GRASS_VERBOSE"] = "3"
    tools = Tools(session=xy_dataset_session, verbose=False)
    tools.g_mapset(mapset="test1", flags="c")
    tools.g_mapset(mapset="test2", flags="c")
    result = tools.g_mapsets(mapset="test1")
    assert not result.stderr
    assert "GRASS_VERBOSE" in xy_dataset_session.env
    assert xy_dataset_session.env["GRASS_VERBOSE"] == "3"
    assert "GRASS_VERBOSE" not in tools._modified_env_if_needed(), (
        f"GRASS_VERBOSE has value {tools._modified_env_if_needed()['GRASS_VERBOSE']}"
    )  # pylint: disable=protected-access


def test_direct_overwrite(xy_dataset_session):
    """Check overwrite as a function parameter"""
    assert "GRASS_OVERWRITE" not in xy_dataset_session.env
    tools = Tools(session=xy_dataset_session)
    tools.r_random_surface(output="surface", seed=42)
    tools.r_random_surface(output="surface", seed=42, overwrite=True)


def test_object_overwrite(xy_dataset_session):
    """Check overwrite as parameter of the tools object"""
    assert "GRASS_OVERWRITE" not in xy_dataset_session.env
    tools = Tools(session=xy_dataset_session, overwrite=True)
    tools.r_random_surface(output="surface", seed=42)
    tools.r_random_surface(output="surface", seed=42)


def test_no_overwrite(xy_dataset_session):
    """Check that it fails without overwrite"""
    assert "GRASS_OVERWRITE" not in xy_dataset_session.env
    tools = Tools(session=xy_dataset_session)
    tools.r_random_surface(output="surface", seed=42)
    with pytest.raises(CalledModuleError, match="overwrite"):
        tools.r_random_surface(output="surface", seed=42)


def test_overwrite_true_in_call(xy_dataset_session):
    """Check explicitly disabled overwrite, but working tool-level overwrite"""
    assert "GRASS_OVERWRITE" not in xy_dataset_session.env
    tools = Tools(session=xy_dataset_session, overwrite=False)
    tools.r_random_surface(output="surface", seed=42)
    tools.r_random_surface(output="surface", seed=42, overwrite=True)
    with pytest.raises(CalledModuleError, match="overwrite"):
        tools.r_random_surface(output="surface", seed=42)


def test_overwrite_false_in_call(xy_dataset_session):
    """Individual overwrite is not used when global is True and individual is False

    Not ideal behavior, but we simply test for the current behavior.
    """
    assert "GRASS_OVERWRITE" not in xy_dataset_session.env
    tools = Tools(session=xy_dataset_session, overwrite=True)
    tools.r_random_surface(output="surface", seed=42)
    tools.r_random_surface(output="surface", seed=42, overwrite=False)


def test_env_overwrite(xy_dataset_session):
    """Check that overwrite from env parameter is used"""
    assert "GRASS_OVERWRITE" not in xy_dataset_session.env
    env = xy_dataset_session.env.copy()
    env["GRASS_OVERWRITE"] = "1"
    tools = Tools(env=env)
    tools.r_random_surface(output="surface", seed=42)
    tools.r_random_surface(output="surface", seed=42)


def test_session_overwrite(xy_dataset_session):
    """Check that overwrite from session env attribute is used"""
    assert "GRASS_OVERWRITE" not in xy_dataset_session.env
    xy_dataset_session.env["GRASS_OVERWRITE"] = "1"
    tools = Tools(session=xy_dataset_session)
    tools.r_random_surface(output="surface", seed=42)
    tools.r_random_surface(output="surface", seed=42)


def test_parent_overwrite_vs_env(xy_dataset_session):
    """Check that parent overwrite is not used when separate env is used"""
    assert "GRASS_OVERWRITE" not in xy_dataset_session.env
    # Set up everything head of time.
    env = xy_dataset_session.env.copy()
    xy_dataset_session.env["GRASS_OVERWRITE"] = "1"
    # Only after the setup, create tools.
    tools = Tools(session=xy_dataset_session, env=env)
    tools.r_random_surface(output="surface", seed=42)
    with pytest.raises(CalledModuleError, match="overwrite"):
        tools.r_random_surface(output="surface", seed=42)


def test_parent_overwrite_vs_object_init(xy_dataset_session):
    """Check that parent overwrite is not used when parameter is used"""
    assert "GRASS_OVERWRITE" not in xy_dataset_session.env
    tools = Tools(session=xy_dataset_session, overwrite=False)
    xy_dataset_session.env["GRASS_OVERWRITE"] = "1"
    tools.r_random_surface(output="surface", seed=42)
    with pytest.raises(CalledModuleError, match="overwrite"):
        tools.r_random_surface(output="surface", seed=42)


def test_parent_overwrite_vs_call(xy_dataset_session):
    """Check that parent overwrite is not used when function parameter is used"""
    assert "GRASS_OVERWRITE" not in xy_dataset_session.env
    tools = Tools(session=xy_dataset_session)
    xy_dataset_session.env["GRASS_OVERWRITE"] = "0"
    tools.r_random_surface(output="surface", seed=42)
    tools.r_random_surface(output="surface", seed=42, overwrite=True)


def test_parent_overwrite_after_call(xy_dataset_session):
    """Check that parent overwrite is used when using the same env"""
    assert "GRASS_OVERWRITE" not in xy_dataset_session.env
    tools = Tools(session=xy_dataset_session)
    tools.r_random_surface(output="surface", seed=42)
    xy_dataset_session.env["GRASS_OVERWRITE"] = "1"
    tools.r_random_surface(output="surface", seed=42)


def test_migration_from_run_command_family(xy_dataset_session):
    """Check example usages corresponding to run_command family calls

    We also check the run_command family calls just to be sure they are
    actually correct.
    """
    # run_command
    # original:
    gs.run_command(
        "r.random.surface", output="surface", seed=42, env=xy_dataset_session.env
    )
    # replacement:
    tools = Tools(env=xy_dataset_session.env)  # same for one or multiple calls
    tools.run("r.random.surface", output="surface2", seed=42)  # name as a string
    tools.r_random_surface(output="surface3", seed=42)  # name as a function

    # write_command
    # original:
    gs.write_command(
        "v.in.ascii",
        input="-",
        output="point1",
        separator=",",
        env=xy_dataset_session.env,
        stdin="13.45,29.96,200\n",
    )
    # replacement:
    tools.run(
        "v.in.ascii",
        input=io.StringIO("13.45,29.96,200\n"),
        output="point2",
        separator=",",
    )
    tools.v_in_ascii(
        input=io.StringIO("13.45,29.96,200\n"),
        output="point3",
        separator=",",
    )

    # read_command
    # original:
    assert (
        gs.read_command("g.region", flags="c", env=xy_dataset_session.env)
        == "center easting:  0.500000\ncenter northing: 0.500000\n"
    )
    # replacement:
    assert (
        tools.run("g.region", flags="c").stdout
        == "center easting:  0.500000\ncenter northing: 0.500000\n"
    )
    assert (
        tools.g_region(flags="c").text
        == "center easting:  0.500000\ncenter northing: 0.500000"
    )

    # parse_command
    # original (numbers are strings):
    assert gs.parse_command(
        "g.region", flags="c", format="shell", env=xy_dataset_session.env
    ) == {
        "center_easting": "0.500000",
        "center_northing": "0.500000",
    }
    assert gs.parse_command(
        "g.region", flags="c", format="json", env=xy_dataset_session.env
    ) == {
        "center_easting": 0.5,
        "center_northing": 0.5,
    }
    # replacement with format=shell (numbers are not strings, but actual numbers as in JSON
    # if they convert to Python int or float):
    assert tools.run("g.region", flags="c", format="shell").keyval == {
        "center_easting": 0.5,
        "center_northing": 0.5,
    }
    # parse_command with JSON and the function call syntax:
    assert tools.g_region(flags="c", format="json").json == {
        "center_easting": 0.5,
        "center_northing": 0.5,
    }

    # parse_command storing JSON output in a variable and accessing individual values
    # original:
    data = gs.parse_command(
        "g.region", flags="c", format="json", env=xy_dataset_session.env
    )
    assert data["center_easting"] == 0.5
    assert data["center_northing"] == 0.5
    # replacement:
    data = tools.g_region(flags="c", format="json")
    assert data["center_easting"] == 0.5
    assert data["center_northing"] == 0.5

    # mapcalc wrapper of r.mapcalc
    # original:
    gs.mapcalc("a = 1", env=xy_dataset_session.env)
    # replacement for short expressions:
    tools.r_mapcalc(expression="b = 1")
    # replacement for long expressions:
    tools.r_mapcalc(file=io.StringIO("c = 1"))

    # g.list wrappers
    # test data preparation (for comparison of the results):
    names = ["a", "b", "c", "surface", "surface2", "surface3"]
    # original:
    assert gs.list_grouped("raster", env=xy_dataset_session.env)["PERMANENT"] == names
    # replacement (using the JSON output of g.list):
    assert [
        item["name"]
        for item in tools.g_list(type="raster", format="json")
        if item["mapset"] == "PERMANENT"
    ] == names
    # original and replacement (directly comparing the results):
    assert gs.list_strings("raster", env=xy_dataset_session.env) == [
        item["fullname"] for item in tools.g_list(type="raster", format="json")
    ]
    # original and replacement (directly comparing the results):
    assert gs.list_pairs("raster", env=xy_dataset_session.env) == [
        (item["name"], item["mapset"])
        for item in tools.g_list(type="raster", format="json")
    ]

    # all other wrappers
    # Wrappers in grass.script usually parse shell-script style key-value pairs,
    # and convert values from strings to numbers, e.g. g.region:
    assert gs.region(env=xy_dataset_session.env)["rows"] == 1
    # Conversion is done automatically in Tools and/or with JSON, and the basic tool
    # call syntax is more lightweight, so the direct tool call is not that different
    # from a wrapper. Direct tool calling also benefits from better defaults (e.g.,
    # printing more in JSON) and more consistent tool behavior (e.g., tools accepting
    # format="json"). So, direct call of g.region to obtain the number of rows:
    assert tools.g_region(flags="p", format="json")["rows"] == 1

    # run_command with returncode
    # original:
    assert (
        gs.run_command(
            "r.mask.status", flags="t", env=xy_dataset_session.env, errors="status"
        )
        == 1
    )
    # replacement:
    tools_with_returncode = Tools(errors="ignore", env=xy_dataset_session.env)
    assert tools_with_returncode.run("r.mask.status", flags="t").returncode == 1
    assert tools_with_returncode.r_mask_status(flags="t").returncode == 1

    # run_command with overwrite
    # original:
    gs.run_command(
        "r.random.surface",
        output="surface",
        seed=42,
        overwrite=True,
        env=xy_dataset_session.env,
    )
    # replacement:
    tools = Tools(env=xy_dataset_session.env)
    tools.r_random_surface(output="surface", seed=42, overwrite=True)
    # or with global overwrite:
    tools = Tools(overwrite=True, env=xy_dataset_session.env)
    tools.r_random_surface(output="surface", seed=42)


def test_as_context_manager(xy_dataset_session):
    """Test usage of as a context manager"""
    with Tools(session=xy_dataset_session) as tools:
        tools.r_random_surface(output="surface1", seed=42)
    # There is no limit on entering the context manager multiple times.
    with tools:
        tools.r_random_surface(output="surface2", seed=42)
    # Neither there is limit on using it after exiting the context.
    tools.r_random_surface(output="surface3", seed=42)
    assert [item["name"] for item in tools.g_list(type="raster", format="json")] == [
        "surface1",
        "surface2",
        "surface3",
    ]


def test_with_context_managers_explicit_env(tmpdir):
    """Test with other context managers while passing the environment to functions"""
    project = tmpdir / "project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools()
        tools.r_random_surface(output="surface", seed=42, env=session.env)
        with TemporaryMapsetSession(env=session.env) as mapset:
            tools.r_random_surface(output="surface", seed=42, env=mapset.env)
            with gs.MaskManager(env=mapset.env.copy()) as mask:
                tools.r_mask(raster="surface", env=mask.env)
                assert tools.r_mask_status(format="json", env=mask.env)["present"]
                with gs.RegionManager(
                    rows=100, cols=100, env=mask.env.copy()
                ) as region:
                    assert (
                        tools.g_region(flags="p", format="json", env=region.env)["rows"]
                        == 100
                    )


def test_with_context_managers_session_env(tmpdir):
    """Test with other context managers while using environment from a session"""
    project = tmpdir / "project"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as main_session,
        TemporaryMapsetSession(env=main_session.env) as mapset_session,
    ):
        # TemporaryMapsetSession does not modify like the others, but creates its own env.
        tools = Tools(session=mapset_session)
        tools.r_random_surface(output="surface", seed=42)
        with gs.RegionManager(rows=100, cols=100, env=mapset_session.env):
            assert tools.g_region(flags="p", format="json")["rows"] == 100
            with gs.MaskManager(env=mapset_session.env):
                tools.r_mask(raster="surface")
                assert tools.r_mask_status(format="json")["present"]


def test_with_context_managers_session_env_one_block(tmpdir):
    """Check behavior in a single with statement with other context managers"""
    project = tmpdir / "project"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as main_session,
        TemporaryMapsetSession(env=main_session.env) as mapset_session,
        gs.RegionManager(rows=100, cols=100, env=mapset_session.env),
        gs.MaskManager(env=mapset_session.env),
        Tools(session=mapset_session) as tools,
    ):
        tools.r_random_surface(output="surface", seed=42)
        tools.r_mask(raster="surface")
        assert tools.r_mask_status(format="json")["present"]
        assert tools.g_region(flags="p", format="json")["rows"] == 100


def test_misspelling(xy_dataset_session):
    """Check a misspelled tool name as a function name results in a right suggestion"""
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(AttributeError, match=r"r_slope_aspect"):
        tools.r_sloppy_respect()


def test_multiple_suggestions(xy_dataset_session):
    """Check a confused tool name results in multiple suggestions"""
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(
        AttributeError,
        match=r".*".join(sorted(["v_univar", "v_db_univar", "db_univar"])),
    ):
        tools.db_v_univar()


def test_tool_group_vs_model_name(xy_dataset_session):
    """Check a confused tool group name as a function name results in a right suggestion"""
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(AttributeError, match=r"r_sim_water"):
        tools.rSIMWEwater()


def test_wrong_attribute(xy_dataset_session):
    """Check a wrong attribute results in an exception with its name"""
    tools = Tools(session=xy_dataset_session)
    with pytest.raises(AttributeError, match="execute_big_command"):
        tools.execute_big_command()


def test_stdin_as_stringio_object(xy_dataset_session):
    """Check that StringIO interface for stdin works"""
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


def test_tool_attribute_access_c_tools():
    """Check C tool names can be listed without a session"""
    tools = Tools()
    assert "g_region" in dir(tools)
    assert "r_slope_aspect" in dir(tools)


def test_tool_attribute_access_python_tools():
    """Check Python tool names can be listed without a session"""
    tools = Tools()
    assert "g_search_modules" in dir(tools)
    assert "r_mask" in dir(tools)


def test_tool_doc_access_c_tools():
    """Check C tools have doc strings without a session"""
    tools = Tools()
    assert tools.g_region.__doc__
    assert tools.r_slope_aspect.__doc__


def test_tool_doc_access_python_tools():
    """Check Python tools have doc strings without a session"""
    tools = Tools()
    assert tools.g_search_modules.__doc__
    assert tools.r_mask.__doc__
