"""Tests of TemporaryMapsetSession"""

import os

import grass.script as gs
import grass.experimental as experimental


def test_with_context_manager(xy_session):
    """Session creates, starts, and finishes"""
    session_file = xy_session.env["GISRC"]
    original_session_mapset_list = (
        gs.read_command("g.mapsets", flags="l", env=xy_session.env).strip().split(",")
    )
    with experimental.TemporaryMapsetSession(env=xy_session.env) as session:
        assert session.active
        gs.run_command("g.region", flags="p", env=session.env)
        gs.run_command(
            "r.surf.random", output="uniform_random", min=1, max=10, env=session.env
        )
        gs.parse_command("r.univar", map="uniform_random", flags="g", env=session.env)

        session_mapset = gs.read_command("g.mapset", flags="p", env=session.env).strip()

        top_level_mapset = gs.read_command(
            "g.mapset", flags="p", env=xy_session.env
        ).strip()
        assert top_level_mapset != session_mapset

        other_session_file = session.env["GISRC"]
        assert session_file != other_session_file
        assert os.path.exists(other_session_file)
        mapset_path = session.mapset_path
        assert os.path.exists(session.mapset_path)
    assert not session.active
    assert not os.path.exists(mapset_path)
    assert not os.path.exists(other_session_file)
    new_session_mapset_list = (
        gs.read_command("g.mapsets", flags="l", env=xy_session.env).strip().split(",")
    )
    assert sorted(original_session_mapset_list) == sorted(new_session_mapset_list)
    assert os.path.exists(session_file)
    assert xy_session.env["GISRC"]


def test_without_context_manager(xy_session):
    """Session creates, starts, and finishes but without a context manager API"""
    session_file = xy_session.env["GISRC"]
    session = experimental.TemporaryMapsetSession(env=xy_session.env)
    gs.run_command("g.region", flags="p", env=session.env)

    session_mapset = gs.read_command("g.mapset", flags="p", env=session.env).strip()

    top_level_mapset = gs.read_command(
        "g.mapset", flags="p", env=xy_session.env
    ).strip()
    assert top_level_mapset != session_mapset

    other_session_file = session.env["GISRC"]
    assert session_file != other_session_file
    assert os.path.exists(other_session_file)
    mapset_path = session.mapset_path
    assert os.path.exists(session.mapset_path)

    session.finish()

    assert not session.active
    assert not os.path.exists(mapset_path)
    assert not os.path.exists(other_session_file)
    assert os.path.exists(session_file)
    assert xy_session.env["GISRC"]


def test_multiple_sequential_with_context_manager(xy_session):
    """Session creates, starts, and finishes"""
    session_file = xy_session.env["GISRC"]
    for i in range(0, 5):
        with experimental.TemporaryMapsetSession(env=xy_session.env) as session:
            assert session.active
            gs.run_command("g.region", flags="p", env=session.env)

            session_mapset = gs.read_command(
                "g.mapset", flags="p", env=session.env
            ).strip()

            top_level_mapset = gs.read_command(
                "g.mapset", flags="p", env=xy_session.env
            ).strip()
            assert top_level_mapset != session_mapset

            other_session_file = session.env["GISRC"]
            assert session_file != other_session_file
            assert os.path.exists(other_session_file)
            mapset_path = session.mapset_path
            assert os.path.exists(session.mapset_path)
        assert not session.active
        assert not os.path.exists(mapset_path)
        assert not os.path.exists(other_session_file)
        assert os.path.exists(session_file)
        assert xy_session.env["GISRC"]


def test_multiple_parallel_without_context_manager(xy_session):
    """Session creates, starts, and finishes"""
    session_file = xy_session.env["GISRC"]
    sessions = []
    for i in range(0, 5):
        session_file = xy_session.env["GISRC"]
        session = experimental.TemporaryMapsetSession(env=xy_session.env)
        gs.run_command("g.region", flags="p", env=session.env)

        session_mapset = gs.read_command("g.mapset", flags="p", env=session.env).strip()

        top_level_mapset = gs.read_command(
            "g.mapset", flags="p", env=xy_session.env
        ).strip()
        assert top_level_mapset != session_mapset

        other_session_file = session.env["GISRC"]
        assert session_file != other_session_file
        assert os.path.exists(other_session_file)
        assert os.path.exists(session.mapset_path)
        sessions.append(session)

    for session in sessions:
        session.finish()
        assert not session.active
        assert not os.path.exists(session.mapset_path)
        assert not os.path.exists(session.env["GISRC"])

    assert os.path.exists(session_file)
    assert xy_session.env["GISRC"]


def test_nested_top_env(xy_session):
    """Sessions can be nested with one top-level environment"""
    top_level_session_file = xy_session.env["GISRC"]
    top_level_mapset = gs.read_command(
        "g.mapset", flags="p", env=xy_session.env
    ).strip()

    with experimental.TemporaryMapsetSession(env=xy_session.env) as session1:
        with experimental.TemporaryMapsetSession(env=xy_session.env) as session2:
            with experimental.TemporaryMapsetSession(env=xy_session.env) as session3:
                for session in [session1, session2, session3]:
                    session_mapset = gs.read_command(
                        "g.mapset", flags="p", env=session.env
                    ).strip()
                    session_mapset = gs.read_command(
                        "g.mapset", flags="p", env=session.env
                    ).strip()
                    assert top_level_mapset != session_mapset
                    session_file = session.env["GISRC"]
                    assert top_level_session_file != session_file
                    assert os.path.exists(session_file)
                    assert os.path.exists(session.mapset_path)
    for session in [session1, session2, session3]:
        assert not session.active
        assert not os.path.exists(session.mapset_path)
        assert not os.path.exists(session.env["GISRC"])
    assert os.path.exists(top_level_session_file)


def test_nested_inherited_env(xy_session):
    """Sessions can be nested including environment"""
    top_level_session_file = xy_session.env["GISRC"]
    top_level_mapset = gs.read_command(
        "g.mapset", flags="p", env=xy_session.env
    ).strip()

    with experimental.TemporaryMapsetSession(env=xy_session.env) as session1:
        with experimental.TemporaryMapsetSession(env=session1.env) as session2:
            with experimental.TemporaryMapsetSession(env=session2.env) as session3:
                for session in [session1, session2, session3]:
                    session_mapset = gs.read_command(
                        "g.mapset", flags="p", env=session.env
                    ).strip()
                    session_mapset = gs.read_command(
                        "g.mapset", flags="p", env=session.env
                    ).strip()
                    assert top_level_mapset != session_mapset
                    session_file = session.env["GISRC"]
                    assert top_level_session_file != session_file
                    assert os.path.exists(session_file)
                    assert os.path.exists(session.mapset_path)
    for session in [session1, session2, session3]:
        assert not session.active
        assert not os.path.exists(session.mapset_path)
        assert not os.path.exists(session.env["GISRC"])
    assert os.path.exists(top_level_session_file)
