# SPDX-License-Identifier: GPL-2.0-or-later
"""Test that registering a map from another mapset does not
write file-based metadata (timestamps, semantic labels) to
the source mapset or create ghost metadata files in the current mapset.

A map in a foreign mapset is read-only from the perspective of the current
mapset, so timestamps and semantic labels must only be stored in the temporal
database, not written back to the file-system metadata of that foreign map.
The C-level write functions ignore the mapset argument and always write into
the current mapset, so without the read-only guard a registration would create
ghost cell_misc entries in PERMANENT for a map that does not exist there.
"""

from datetime import datetime
from pathlib import Path

import pytest

from grass.grassdb.create import create_mapset
import grass.script as gs
import grass.temporal as tgis
from grass.tools import Tools


@pytest.fixture(scope="module")
def cross_mapset_session(tmp_path_factory):
    """Project with a *source* mapset and PERMANENT.

    source: raster map *foreign_map* (no timestamp, no semantic label)
    PERMANENT: raster map *local_map* registered in *test_strds*; also
    registers *foreign_map@source* with an absolute timestamp and a
    semantic label via a file.
    """
    project = tmp_path_factory.mktemp("data") / "project"
    file_dir = tmp_path_factory.mktemp("files")
    gs.create_project(project)
    create_mapset(project / "source")

    with gs.setup.init(project / "source"):
        tools = Tools()
        tools.g_region(s=0, n=1, w=0, e=1, res=1)
        tools.r_mapcalc(expression="foreign_map = 1")

    with gs.setup.init(project) as session:
        tools = Tools(session=session)
        tools.g_region(s=0, n=1, w=0, e=1, res=1)
        tools.r_mapcalc(expression="local_map = 1")

        tools.t_create(output="test_strds", type="strds", title="T", description="T")
        tools.t_register(input="test_strds", maps="local_map", start="2001-01-01")

        # Pipe in a timestamp and semantic label for the foreign map.
        map_file = file_dir / "maps.txt"
        map_file.write_text("foreign_map@source|2010-06-01|S2_1\n")
        tools.t_register(input="test_strds", file=map_file)

        yield session


def test_local_map_gets_file_timestamp(cross_mapset_session):
    """A map in the current mapset receives a file-based timestamp."""
    tgis.init()
    rmap = tgis.RasterDataset("local_map@PERMANENT")
    assert not rmap.get_readonly()
    assert rmap.has_grass_timestamp(), (
        "local_map should have a file-based timestamp after t.register"
    )


def test_foreign_map_no_file_timestamp(cross_mapset_session):
    """A map in another mapset must not receive a file-based timestamp."""
    tgis.init()
    rmap = tgis.RasterDataset("foreign_map@source")
    assert rmap.get_readonly()
    assert not rmap.has_grass_timestamp(), (
        "foreign_map in source mapset must not have its file-based timestamp "
        "written when registered from PERMANENT"
    )


def test_foreign_map_no_file_semantic_label(cross_mapset_session):
    """A map in another mapset must not have a semantic label written to file."""
    tgis.init()
    rmap = tgis.RasterDataset("foreign_map@source")
    # read_semantic_label_from_grass returns False when no label is on disk
    assert rmap.get_readonly()
    assert not rmap.read_semantic_label_from_grass(), (
        "foreign_map in source mapset must not have its semantic label "
        "written to the file system when registered from PERMANENT"
    )


def test_foreign_map_direct_metadata_writes_are_skipped(
    cross_mapset_session, monkeypatch
):
    """Direct file-metadata helpers must also respect a foreign mapset."""
    tgis.init()
    rmap = tgis.RasterDataset("foreign_map@source")
    rmap.set_absolute_time(datetime(2010, 6, 1))
    rmap.metadata.set_semantic_label("S2_1")

    def unexpected_write(*args):
        pytest.fail("GRASS metadata write was called for a foreign map")

    monkeypatch.setattr(rmap.ciface, "write_raster_timestamp", unexpected_write)
    monkeypatch.setattr(rmap.ciface, "write_raster_semantic_label", unexpected_write)

    assert not rmap.write_timestamp_to_grass()
    assert not rmap.write_semantic_label_to_grass()


def test_foreign_map_direct_timestamp_removal_is_skipped(cross_mapset_session):
    """Removing a foreign timestamp must not remove a current-mapset file."""
    tgis.init()
    rmap = tgis.RasterDataset("foreign_map@source")
    env = gs.gisenv()
    project = Path(env["GISDBASE"]) / env["LOCATION_NAME"]
    timestamp = project / "PERMANENT" / "cell_misc" / "foreign_map" / "timestamp"
    timestamp.parent.mkdir(parents=True, exist_ok=True)
    timestamp.write_text("local timestamp")

    try:
        assert not rmap.remove_timestamp_from_grass()
        assert timestamp.read_text() == "local timestamp"
    finally:
        timestamp.unlink(missing_ok=True)


def test_mapset_change_updates_readonly_state(cross_mapset_session):
    """Changing a dataset mapset must update its file-write protection."""
    tgis.init()
    rmap = tgis.RasterDataset("local_map@PERMANENT")
    assert not rmap.get_readonly()

    rmap.base.set_mapset("source")
    assert rmap.get_readonly()

    rmap.base.set_mapset("PERMANENT")
    assert not rmap.get_readonly()


def test_foreign_map_has_db_timestamp(cross_mapset_session):
    """The temporal DB entry for the foreign map has the registered time."""
    tgis.init()
    dbif = tgis.SQLDatabaseInterfaceConnection()
    dbif.connect()
    try:
        rmap = tgis.RasterDataset("foreign_map@source")
        rmap.select(dbif=dbif, mapset=tgis.get_current_mapset())
        start, _ = rmap.get_absolute_time()
    finally:
        dbif.close()
    assert rmap.get_readonly()
    assert start is not None
    assert start.year == 2010


def test_no_ghost_timestamp_in_current_mapset(cross_mapset_session):
    """Registering a foreign map must not create a ghost timestamp in PERMANENT.

    G_write_*_timestamp ignores the mapset argument and always writes into the
    current mapset.  Without the read-only guard this would create
    cell_misc/foreign_map/timestamp in PERMANENT for a map that lives in source.
    """
    env = gs.gisenv()
    project = Path(env["GISDBASE"]) / env["LOCATION_NAME"]
    ghost = project / "PERMANENT" / "cell_misc" / "foreign_map" / "timestamp"
    assert not ghost.exists(), (
        "Registering foreign_map@source must not create a ghost timestamp "
        "file in PERMANENT/cell_misc/foreign_map/"
    )


def test_no_ghost_semantic_label_in_current_mapset(cross_mapset_session):
    """Registering a foreign map must not create a ghost semantic label in PERMANENT."""
    env = gs.gisenv()
    project = Path(env["GISDBASE"]) / env["LOCATION_NAME"]
    ghost = project / "PERMANENT" / "cell_misc" / "foreign_map" / "semantic_label"
    assert not ghost.exists(), (
        "Registering foreign_map@source must not create a ghost semantic_label "
        "file in PERMANENT/cell_misc/foreign_map/"
    )
