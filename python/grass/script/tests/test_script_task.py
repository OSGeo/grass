import os

import pytest

import grass.script as gs
from grass.script.task import grassTask


@pytest.fixture
def xy_session_patched_env(tmp_path, monkeypatch):
    """Active session in an XY location (scope: function), patching env vars directly.

    This allows functions not accepting an env dictionary argument to work in tests"""
    location = "xy_test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with gs.setup.init(tmp_path / location, env=os.environ.copy()) as session:
        for key, value in session.env.items():
            monkeypatch.setenv(key, value)
        yield session


def test_mapcalc_simple_e_name(xy_session_patched_env):
    gt = grassTask("r.mapcalc.simple")
    assert gt.get_param("e")["name"] == "e"


def test_mapcalc_simple_expression_name(xy_session_patched_env):
    gt = grassTask("r.mapcalc.simple")
    assert gt.get_param("expression")["name"] == "expression"


def test_d_vect_from_bin(xy_session_patched_env):
    """Tests that a module installed in "$GISBASE/bin can be used with grassTask"""
    task = grassTask("d.vect")
    task.get_param("map")["value"] = "map_name"
    task.get_flag("i")["value"] = True
    task.get_param("layer")["value"] = 1
    task.get_param("label_bcolor")["value"] = "red"
    # the default parameter display is added automatically
    actual = " ".join(task.get_cmd())
    expected = "d.vect -i map=map_name layer=1 display=shape label_bcolor=red"
    assert actual == expected


def test_v_clip_from_scripts(xy_session_patched_env):
    """Tests that a module installed in "$GISBASE/scripts can be used with grassTask"""
    task = grassTask("v.clip")
    task.get_param("input")["value"] = "map_name"
    task.get_flag("r")["value"] = True
    task.get_param("clip")["value"] = "clip_map_name"
    task.get_param("output")["value"] = "output_map_name"
    actual = " ".join(task.get_cmd())
    expected = "v.clip -r input=map_name clip=clip_map_name output=output_map_name"
    assert actual == expected
