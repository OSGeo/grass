"""Test MapsetSession"""

import os

import pytest

import grass.script as gs
import grass.experimental as experimental


def test_simple_create(xy_session):
    """Session creates, starts, and finishes"""
    name = "test_mapset_1"
    session_file = xy_session.env["GISRC"]
    with experimental.MapsetSession(name, create=True, env=xy_session.env) as session:
        gs.run_command("g.region", flags="p", env=session.env)

        session_mapset = gs.read_command("g.mapset", flags="p", env=session.env).strip()
        assert name == session_mapset

        top_level_mapset = gs.read_command(
            "g.mapset", flags="p", env=xy_session.env
        ).strip()
        assert top_level_mapset != session_mapset

        other_session_file = session.env["GISRC"]
        assert session_file != other_session_file
        assert os.path.exists(other_session_file)
    assert not os.path.exists(other_session_file)
    assert os.path.exists(session_file)
    assert xy_session.env["GISRC"]


def test_without_context_manager(xy_session):
    """Session creates, starts, and finishes but without a context manager API"""
    name = "test_mapset_1"
    session_file = xy_session.env["GISRC"]
    session = experimental.MapsetSession(name, create=True, env=xy_session.env)
    gs.run_command("g.region", flags="p", env=session.env)

    session_mapset = gs.read_command("g.mapset", flags="p", env=session.env).strip()
    assert name == session_mapset

    top_level_mapset = gs.read_command(
        "g.mapset", flags="p", env=xy_session.env
    ).strip()
    assert top_level_mapset != session_mapset

    other_session_file = session.env["GISRC"]
    assert session_file != other_session_file
    assert os.path.exists(other_session_file)
    session.finish()
    assert not os.path.exists(other_session_file)
    assert os.path.exists(session_file)
    assert xy_session.env["GISRC"]


def test_create_overwrite(xy_session):
    """Session creates and creates again with overwrite"""
    name = "test_mapset_1"
    session_file = xy_session.env["GISRC"]
    with experimental.MapsetSession(name, create=True, env=xy_session.env) as session:
        session_mapset = gs.read_command("g.mapset", flags="p", env=session.env).strip()
        assert name == session_mapset
        gs.run_command("r.mapcalc", expression="a = 1", env=session.env)
        rasters = (
            gs.read_command("g.list", type="raster", mapset=".", env=session.env)
            .strip()
            .split()
        )
        assert len(rasters) == 1 and rasters[0] == "a"
    with experimental.MapsetSession(
        name, create=True, overwrite=True, env=xy_session.env
    ) as session:
        session_mapset = gs.read_command("g.mapset", flags="p", env=session.env).strip()
        assert name == session_mapset
        rasters = (
            gs.read_command("g.list", type="raster", mapset=".", env=session.env)
            .strip()
            .split()
        )
        assert not rasters
        gs.run_command("r.mapcalc", expression="a = 1", env=session.env)
        rasters = (
            gs.read_command("g.list", type="raster", mapset=".", env=session.env)
            .strip()
            .split()
        )
        assert len(rasters) == 1 and rasters[0] == "a"
    assert os.path.exists(session_file)


def test_ensure(xy_session):
    """Session ensures and does not delete"""
    name = "test_mapset_1"
    session_file = xy_session.env["GISRC"]
    with experimental.MapsetSession(name, ensure=True, env=xy_session.env) as session:
        session_mapset = gs.read_command("g.mapset", flags="p", env=session.env).strip()
        assert name == session_mapset
        gs.run_command("r.mapcalc", expression="a = 1", env=session.env)
        rasters = (
            gs.read_command("g.list", type="raster", mapset=".", env=session.env)
            .strip()
            .split()
        )
        assert len(rasters) == 1 and rasters[0] == "a"
    with experimental.MapsetSession(name, ensure=True, env=xy_session.env) as session:
        session_mapset = gs.read_command("g.mapset", flags="p", env=session.env).strip()
        assert name == session_mapset
        rasters = (
            gs.read_command("g.list", type="raster", mapset=".", env=session.env)
            .strip()
            .split()
        )
        assert len(rasters) == 1 and rasters[0] == "a"
        gs.run_command("r.mapcalc", expression="b = 1", env=session.env)
        rasters = (
            gs.read_command("g.list", type="raster", mapset=".", env=session.env)
            .strip()
            .split()
        )
        assert sorted(rasters) == ["a", "b"]
    assert os.path.exists(session_file)


def get_mapset_names(env):
    """Get list of mapsets in the current location based on the environment"""
    return (
        gs.read_command("g.mapsets", flags="l", separator=",", env=env)
        .strip()
        .split(",")
    )


def test_create_multiple(xy_session):
    """Multiple mapsets are created and preserved while the top session is intact"""
    create_names = ["test_mapset_1", "test_mapset_2", "test_mapset_3"]
    collected = []
    top_level_collected = []
    original_mapsets = get_mapset_names(env=xy_session.env)
    for name in create_names:
        with experimental.MapsetSession(
            name, create=True, env=xy_session.env
        ) as session:
            collected.append(
                gs.read_command("g.mapset", flags="p", env=session.env).strip()
            )
            top_level_collected.append(
                gs.read_command("g.mapset", flags="p", env=xy_session.env).strip()
            )
    assert sorted(collected) == sorted(create_names)
    existing_mapsets = get_mapset_names(env=xy_session.env)
    assert sorted(existing_mapsets) == sorted(create_names + original_mapsets)
    assert (
        len(set(top_level_collected)) == 1
    ), f"Top level mapset changed: {top_level_collected}"


def test_nested_top_env(xy_session):
    """Sessions can be nested with one top-level environment"""
    names = ["test_mapset_1", "test_mapset_2", "test_mapset_3"]
    with experimental.MapsetSession(
        names[0], create=True, env=xy_session.env
    ) as session1:
        with experimental.MapsetSession(
            names[1], create=True, env=xy_session.env
        ) as session2:
            with experimental.MapsetSession(
                names[2], create=True, env=xy_session.env
            ) as session3:
                for name, session in zip(names, [session1, session2, session3]):
                    session_mapset = gs.read_command(
                        "g.mapset", flags="p", env=session.env
                    ).strip()
                    assert name == session_mapset
    assert os.path.exists(xy_session.env["GISRC"])


def test_nested_inherited_env(xy_session):
    """Sessions can be nested including environment"""
    names = ["test_mapset_1", "test_mapset_2", "test_mapset_3"]
    with experimental.MapsetSession(
        names[0], create=True, env=xy_session.env
    ) as session1:
        with experimental.MapsetSession(
            names[1], create=True, env=session1.env
        ) as session2:
            with experimental.MapsetSession(
                names[2], create=True, env=session2.env
            ) as session3:
                for name, session in zip(names, [session1, session2, session3]):
                    session_mapset = gs.read_command(
                        "g.mapset", flags="p", env=session.env
                    ).strip()
                    assert name == session_mapset
    assert os.path.exists(xy_session.env["GISRC"])


@pytest.mark.parametrize("number", [1, 2, 3.1])
def test_usage_in_fixture_function_location(xy_mapset_non_permament, number):
    """Test fixture based on location with function scope and function scope mapset

    Each test gets a unique location and the mapset is not PERMANENT.
    """
    gs.run_command(
        "r.mapcalc", expression=f"a = {number}", env=xy_mapset_non_permament.env
    )
    session_mapset = gs.read_command(
        "g.mapset", flags="p", env=xy_mapset_non_permament.env
    ).strip()
    assert session_mapset != "PERMANENT"
    assert xy_mapset_non_permament.name == session_mapset
