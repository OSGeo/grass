"""Test MapsetSession"""

import os

import pytest

import grass.script as gs


def test_simple_create(xy_session):
    """Session creates, starts, and finishes"""
    name = "test_mapset_1"
    session_file = xy_session.env["GISRC"]
    with gs.MapsetSession(name, create=True) as session:
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
    session = gs.MapsetSession(name, create=True)
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
    with gs.MapsetSession(name, create=True) as session:
        session_mapset = gs.read_command("g.mapset", flags="p", env=session.env).strip()
        assert name == session_mapset
        gs.run_command("r.mapcalc", expression="a = 1", env=session.env)
        rasters = (
            gs.read_command("g.list", type="raster", mapset=".", env=session.env)
            .strip()
            .split()
        )
        assert len(rasters) == 1 and rasters[0] == "a"
    with gs.MapsetSession(name, create=True, overwrite=True) as session:
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
    with gs.MapsetSession(name, ensure=True) as session:
        session_mapset = gs.read_command("g.mapset", flags="p", env=session.env).strip()
        assert name == session_mapset
        gs.run_command("r.mapcalc", expression="a = 1", env=session.env)
        rasters = (
            gs.read_command("g.list", type="raster", mapset=".", env=session.env)
            .strip()
            .split()
        )
        assert len(rasters) == 1 and rasters[0] == "a"
    with gs.MapsetSession(name, ensure=True) as session:
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


def test_nested_top_env(xy_session):
    """Sessions can be nested with one top-level environment"""
    names = ["test_mapset_1", "test_mapset_2", "test_mapset_3"]
    with gs.MapsetSession(names[0], create=True, env=xy_session.env) as session1:
        with gs.MapsetSession(names[1], create=True, env=xy_session.env) as session2:
            with gs.MapsetSession(
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
    with gs.MapsetSession(names[0], create=True, env=xy_session.env) as session1:
        with gs.MapsetSession(names[1], create=True, env=session1.env) as session2:
            with gs.MapsetSession(names[2], create=True, env=session2.env) as session3:
                for name, session in zip(names, [session1, session2, session3]):
                    session_mapset = gs.read_command(
                        "g.mapset", flags="p", env=session.env
                    ).strip()
                    assert name == session_mapset
    assert os.path.exists(xy_session.env["GISRC"])


@pytest.mark.parametrize("number", [1, 2, 3.1])
def test_usage_in_fixture(xy_mapset_session, number):
    """Fixture based on location with module scope and function scope mapset"""
    gs.run_command("r.mapcalc", expression=f"a = {number}", env=xy_mapset_session.env)
