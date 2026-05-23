# SPDX-License-Identifier: GPL-2.0-or-later
"""Unit tests for AbstractSpaceTimeDataset.shift() and .snap()."""

import datetime

import pytest

import grass.script as gs
import grass.temporal as tgis
from grass.tools import Tools


@pytest.fixture(scope="module")
def grass_session(tmp_path_factory):
    """Active GRASS session with tgis initialized."""
    project = tmp_path_factory.mktemp("project") / "test"
    gs.create_project(project)
    with gs.setup.init(project) as session:
        tgis.init()
        Tools(session=session).g_region(s=0, n=80, w=0, e=120, res=10)
        yield session


def _make_strds(session, name, *, interval, increment):
    """Create an STRDS with two raster maps registered starting 2001-01-01."""
    map_1, map_2 = f"{name}_a", f"{name}_b"
    tools = Tools(session=session)
    tools.r_mapcalc(expression=f"{map_1} = 1")
    tools.r_mapcalc(expression=f"{map_2} = 2")
    strds = tgis.open_new_stds(
        name=name,
        type="strds",
        temporaltype="absolute",
        title="Test",
        descr="Test",
        semantic="field",
        overwrite=True,
    )
    tgis.register_maps_in_space_time_dataset(
        type="raster",
        name=strds.get_name(),
        maps=f"{map_1},{map_2}",
        start="2001-01-01",
        increment=increment,
        interval=interval,
    )
    # register_maps_in_space_time_dataset updates the database row but not the
    # in-memory object, so the granularity stays None until we re-read it.
    strds.select()
    return strds


@pytest.fixture
def interval_strds(grass_session, request):
    """STRDS with two consecutive 1-day intervals starting 2001-01-01."""
    return _make_strds(
        grass_session, request.node.name, interval=True, increment="1 day"
    )


@pytest.fixture
def point_strds(grass_session, request):
    """STRDS with two point-in-time maps two days apart starting 2001-01-01."""
    return _make_strds(
        grass_session, request.node.name, interval=False, increment="2 days"
    )


def test_shift_invalid_granularity(interval_strds):
    """shift() returns False for invalid granularity (regression for #7228).

    The happy-path of shift() on absolute time is covered by
    temporal/t.shift/testsuite/test_shift.py.
    """
    assert interval_strds.shift(gran="invalid") is False


def test_snap_closes_gaps(point_strds):
    """snap() sets each map's end time to its successor's start time,
    and extends the last map by the dataset granularity."""
    point_strds.snap()
    maps = point_strds.get_registered_maps_as_objects(order="start_time")
    assert maps[0].get_temporal_extent_as_tuple() == (
        datetime.datetime(2001, 1, 1),
        datetime.datetime(2001, 1, 3),
    )
    assert maps[1].get_temporal_extent_as_tuple() == (
        datetime.datetime(2001, 1, 3),
        datetime.datetime(2001, 1, 5),
    )
