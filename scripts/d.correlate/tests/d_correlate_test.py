"""Tests of d.correlate"""

import pytest

from grass.exceptions import CalledModuleError
from grass.tools import Tools


def render_tools(session, tmp_path):
    """Return a Tools object for the session's mapset with an offscreen renderer.

    A copy of the session environment is used, rather than mutating it, so the
    cairo render settings do not leak into the shared session fixture.
    d.correlate is a display tool, so the tests need a working render target;
    without one the tool would fail at its first d.text call and mask the
    behavior under test.
    """
    env = dict(session.env)
    env["GRASS_RENDER_IMMEDIATE"] = "cairo"
    env["GRASS_RENDER_FILE"] = str(tmp_path / "correlate.png")
    return Tools(env=env)


def test_fewer_than_two_maps_is_fatal(xy_raster_dataset_session_mapset, tmp_path):
    """d.correlate must fail, not silently succeed, with fewer than two maps.

    Regression test: a single map previously printed an error but still exited
    with a success code and produced no output.
    """
    tools = render_tools(xy_raster_dataset_session_mapset, tmp_path)
    tools.r_mapcalc(expression="map_a = 1")
    with pytest.raises(CalledModuleError):
        tools.d_correlate(map="map_a")


def test_more_than_two_maps_runs(xy_raster_dataset_session_mapset, tmp_path):
    """d.correlate handles more than two maps without crashing.

    Regression test: the per-pair text line counter was reused as the
    file-iteration variable, raising a TypeError on the second map pair (that
    is, whenever three or more maps were given).
    """
    tools = render_tools(xy_raster_dataset_session_mapset, tmp_path)
    tools.r_mapcalc(expression="map_a = 1")
    tools.r_mapcalc(expression="map_b = col()")
    tools.r_mapcalc(expression="map_c = row()")
    tools.d_correlate(map="map_a,map_b,map_c")
