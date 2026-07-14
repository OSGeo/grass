# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests for the grass.temporal.gui_support listing helpers.

The temporal order of maps in the datasets is deliberately opposite to
the alphabetical order of the map names, so that a listing sorted by
name instead of by time is detected.
"""

import datetime
import os

import pytest

from grass.grassdb.create import create_mapset
import grass.script as gs
import grass.temporal as tgis
from grass.tools import Tools


@pytest.fixture(scope="module")
def session_with_stds(tmp_path_factory):
    """Session with temporal datasets in PERMANENT and a second mapset.

    PERMANENT contains:

    - strds "monthly" with maps b_map (January) and a_map (February)
    - strds "shared" registering b_map as well
    - strds "empty" with no registered maps
    - strds "relative" with maps rel_b (start 5) and rel_a (start 2)
    - strds "cross" registering foreign_map from the second mapset
    - stvds "points" with one vector map
    - raster map "loner" that is time stamped but in no dataset

    The second mapset contains strds "other" with one map. The mapset
    "plain" has no temporal database.
    """
    project = tmp_path_factory.mktemp("data") / "project"
    gs.create_project(project)
    create_mapset(project / "second")
    create_mapset(project / "plain")

    with gs.setup.init(project / "second") as session:
        tools = Tools(session=session)
        tools.g_region(n=3, s=0, e=3, w=0, res=1)
        tools.r_mapcalc(expression="other_map = 1")
        tools.r_mapcalc(expression="foreign_map = 1")
        tools.t_create(output="other", type="strds", title="other", description="other")
        tools.t_register(input="other", maps="other_map", start="2020-01-01")

    with gs.setup.init(project) as session:
        tools = Tools(session=session)
        tools.g_region(n=3, s=0, e=3, w=0, res=1)
        for expression in ("a_map = 1", "b_map = 1", "rel_a = 1", "rel_b = 1"):
            tools.r_mapcalc(expression=expression)
        tools.r_mapcalc(expression="loner = 1")
        tools.v_edit(map="point_map", tool="create")

        tools.t_create(
            output="monthly", type="strds", title="monthly", description="monthly"
        )
        tools.t_register(
            input="monthly", maps="b_map", start="2001-01-01", end="2001-02-01"
        )
        tools.t_register(
            input="monthly", maps="a_map", start="2001-02-01", end="2001-03-01"
        )

        tools.t_create(
            output="shared", type="strds", title="shared", description="shared"
        )
        tools.t_register(input="shared", maps="b_map")

        tools.t_create(output="empty", type="strds", title="empty", description="empty")

        tools.t_create(
            output="relative",
            type="strds",
            temporaltype="relative",
            title="relative",
            description="relative",
        )
        tools.t_register(input="relative", maps="rel_b", start=5, unit="years")
        tools.t_register(input="relative", maps="rel_a", start=2, unit="years")

        tools.t_create(output="cross", type="strds", title="cross", description="cross")
        tools.t_register(input="cross", maps="foreign_map@second", start="2010-01-01")

        tools.t_create(
            output="points", type="stvds", title="points", description="points"
        )
        tools.t_register(
            input="points", type="vector", maps="point_map", start="2000-01-01"
        )

        yield session


@pytest.fixture
def all_mapsets_dbif(session_with_stds):
    """Connection covering all mapsets of the project."""
    tgis.init()
    dbif = tgis.SQLDatabaseInterfaceConnection(mapsets="*")
    dbif.connect()
    yield dbif
    dbif.close()


def map_ids(result, stds_type, stds_id):
    return [map_info["id"] for map_info in result[stds_type][stds_id]]


def test_temporal_order(all_mapsets_dbif):
    """Maps are ordered by start time, not by name."""
    result = tgis.registered_maps_grouped(dbif=all_mapsets_dbif)
    assert map_ids(result, "strds", "monthly@PERMANENT") == [
        "b_map@PERMANENT",
        "a_map@PERMANENT",
    ]


def test_absolute_times(all_mapsets_dbif):
    """Absolute times are returned as datetimes without a unit."""
    result = tgis.registered_maps_grouped(dbif=all_mapsets_dbif)
    b_map, a_map = result["strds"]["monthly@PERMANENT"]
    assert b_map["start_time"] == datetime.datetime(2001, 1, 1)
    assert b_map["end_time"] == datetime.datetime(2001, 2, 1)
    assert b_map["unit"] is None
    assert a_map["start_time"] == datetime.datetime(2001, 2, 1)


def test_relative_times(all_mapsets_dbif):
    """Relative times order by integer start and carry the unit."""
    result = tgis.registered_maps_grouped(dbif=all_mapsets_dbif)
    assert map_ids(result, "strds", "relative@PERMANENT") == [
        "rel_a@PERMANENT",
        "rel_b@PERMANENT",
    ]
    rel_a = result["strds"]["relative@PERMANENT"][0]
    assert rel_a["start_time"] == 2
    assert rel_a["end_time"] is None
    assert rel_a["unit"] == "years"


def test_map_in_multiple_datasets(all_mapsets_dbif):
    """A map registered in two datasets appears in both."""
    result = tgis.registered_maps_grouped(dbif=all_mapsets_dbif)
    assert "b_map@PERMANENT" in map_ids(result, "strds", "monthly@PERMANENT")
    assert map_ids(result, "strds", "shared@PERMANENT") == ["b_map@PERMANENT"]


def test_unregistered_and_empty_are_absent(all_mapsets_dbif):
    """Time stamped maps outside datasets and empty datasets do not appear."""
    result = tgis.registered_maps_grouped(dbif=all_mapsets_dbif)
    all_ids = {
        map_info["id"]
        for datasets in result.values()
        for maps in datasets.values()
        for map_info in maps
    }
    assert "loner@PERMANENT" not in all_ids
    assert "empty@PERMANENT" not in result["strds"]


def test_vector_datasets(all_mapsets_dbif):
    """Vector datasets are reported under the stvds type."""
    result = tgis.registered_maps_grouped(dbif=all_mapsets_dbif)
    assert map_ids(result, "stvds", "points@PERMANENT") == ["point_map@PERMANENT"]
    assert result["str3ds"] == {}


def test_other_mapset_and_cross_registration(all_mapsets_dbif):
    """Datasets in other mapsets and foreign maps are reported exactly once."""
    result = tgis.registered_maps_grouped(dbif=all_mapsets_dbif)
    assert map_ids(result, "strds", "other@second") == ["other_map@second"]
    cross_maps = map_ids(result, "strds", "cross@PERMANENT")
    assert cross_maps == ["foreign_map@second"]


def test_search_path_scope(session_with_stds):
    """A default connection only covers the search path."""
    tgis.init()
    result = tgis.registered_maps_grouped()
    assert "monthly@PERMANENT" in result["strds"]
    assert "other@second" not in result["strds"]


def test_mapset_without_temporal_database_is_skipped(all_mapsets_dbif):
    """A mapset without a temporal database is not scanned."""
    assert "plain" not in all_mapsets_dbif.tgis_mapsets
    assert "second" in all_mapsets_dbif.tgis_mapsets


def test_explicit_mapset_scope(session_with_stds):
    """A connection built for named mapsets covers exactly those."""
    tgis.init()
    dbif = tgis.SQLDatabaseInterfaceConnection(mapsets="second")
    dbif.connect()
    try:
        result = tgis.registered_maps_grouped(dbif=dbif)
    finally:
        dbif.close()
    assert list(result["strds"]) == ["other@second"]


def test_project_without_temporal_database(tmp_path, monkeypatch):
    """A project with no temporal database yields empty results."""
    project = tmp_path / "project"
    gs.create_project(project)
    # tgis reads the session from os.environ. Keep the temporary session
    # out of the global environment and mirror it via monkeypatch, so the
    # environment is restored for the tests that follow.
    with gs.setup.init(project, env=os.environ.copy()) as session:
        for key, value in session.env.items():
            if os.environ.get(key) != value:
                monkeypatch.setenv(key, value)
        # Unlike tgis.init(), skipping the database initialization must
        # not create a temporal database as a side effect of listing.
        tgis.init(skip_db_init=True)
        dbif = tgis.SQLDatabaseInterfaceConnection(mapsets="*")
        assert dbif.tgis_mapsets == {}
        dbif.connect()
        try:
            result = tgis.registered_maps_grouped(dbif=dbif)
            datasets = tgis.tlist_grouped("stds", group_type=True, dbif=dbif)
        finally:
            dbif.close()
    assert result == {"strds": {}, "stvds": {}, "str3ds": {}}
    assert datasets == {}
    assert not (project / "PERMANENT" / "tgis").exists()


def test_tlist_grouped_all_mapsets(all_mapsets_dbif):
    """tlist_grouped honors the mapsets of the passed connection."""
    result = tgis.tlist_grouped("stds", group_type=True, dbif=all_mapsets_dbif)
    assert result["second"]["strds"] == ["other"]
    assert "empty" in result["PERMANENT"]["strds"]
    assert "points" in result["PERMANENT"]["stvds"]
