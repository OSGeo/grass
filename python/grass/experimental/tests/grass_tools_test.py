"""Test grass.experimental.Tools class"""

import os
import pytest

import grass.script as gs
from grass.experimental.tools import Tools


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
    with pytest.raises(gs.CalledModuleError, match="overwrite"):
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
    with pytest.raises(gs.CalledModuleError, match="overwrite"):
        tools.r_random_surface(output="surface", seed=42)
    del os.environ["GRASS_OVERWRITE"]  # check or ideally remove this


def test_global_overwrite_vs_init(xy_dataset_session):
    """Check that global overwrite is not used when separate env is used"""
    tools = Tools(session=xy_dataset_session)
    os.environ["GRASS_OVERWRITE"] = "1"  # change to xy_dataset_session.env
    tools.r_random_surface(output="surface", seed=42)
    with pytest.raises(gs.CalledModuleError, match="overwrite"):
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
    with pytest.raises(gs.CalledModuleError, match=wrong_name):
        tools.feed_input_to("13.45,29.96,200").v_in_ascii(
            input="-",
            output="point",
            format=wrong_name,
        )
