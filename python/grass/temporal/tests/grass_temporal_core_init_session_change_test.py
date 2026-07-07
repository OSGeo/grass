# SPDX-License-Identifier: GPL-2.0-or-later

"""Regression test for tgis.init() across a GRASS session change.

tgis.init() spawns message and C-library subprocesses that capture
GISRC at spawn time. When the parent process switches to a different
project (different LOCATION_NAME or GISDBASE) and calls tgis.init()
again, those subprocesses must be restarted; otherwise they remain
bound to the previous session's GISRC, which may have been deleted.
"""

import pytest

from pathlib import Path

import sqlite3

from grass.grassdb.create import create_mapset
import grass.script as gs
import grass.temporal as tgis


@pytest.fixture
def two_projects(tmp_path):
    """Create two empty projects and yield their paths."""
    project_a = tmp_path / "a"
    project_b = tmp_path / "b"
    gs.create_project(project_a)
    create_mapset(project_a / "a")
    gs.create_project(project_b)
    create_mapset(project_b / "b")
    return project_a, project_b


@pytest.fixture
def simple_mapset(tmp_path):
    """Create a simple mapset and yield its path."""
    project_c = tmp_path / "c"
    gs.create_project(project_c)
    create_mapset(project_c / "c")
    return project_c / "c"


def test_init_restarts_subprocesses_on_session_change(two_projects):
    """tgis.init() must respawn its subprocesses after a session change."""
    project_a, project_b = two_projects
    with gs.setup.init(project_a / "a"):
        tgis.init()
        first_ciface = tgis.get_tgis_c_library_interface()
        assert first_ciface.available_mapsets() == ["a", "PERMANENT"]

    with gs.setup.init(project_b / "b"):
        tgis.init()
        second_ciface = tgis.get_tgis_c_library_interface()

        # The C-library interface must have been replaced. Without this,
        # the subprocess is still bound to session A's deleted GISRC and
        # the call below fails.
        assert second_ciface is not first_ciface
        assert second_ciface.available_mapsets() == ["b", "PERMANENT"]


def test_init_succeeds_without_db_creation(simple_mapset):
    """tgis.init(skip_db_init=True) does not create a DB or connection entry in VAR."""
    project_c = simple_mapset
    var_file = Path(project_c) / "VAR"
    tgis_db = Path(project_c) / "tgis"
    with gs.setup.init(project_c):
        tgis.init(skip_db_init=True)
        first_ciface = tgis.get_tgis_c_library_interface()
        assert first_ciface.available_mapsets() == ["c", "PERMANENT"]
        assert not var_file.exists()
        assert not tgis_db.exists()


def test_init_creates_tgis_db_if_not_skipped(simple_mapset):
    """tgis.init() creates a DB and connection entry in VAR if not skipped."""
    project_c = simple_mapset
    var_file = Path(project_c) / "VAR"
    tgis_db = Path(project_c) / "tgis" / "sqlite.db"
    with gs.setup.init(project_c):
        tgis.init()
        ciface = tgis.get_tgis_c_library_interface()
        assert ciface.available_mapsets() == ["c", "PERMANENT"]
        # Test if the VAR file with the TGIS connection info is created
        assert var_file.exists()
        # Test if the TGIS DB is created with content
        assert tgis_db.parent.exists()
        assert tgis_db.exists()
        assert (
            sqlite3.connect(tgis_db)
            .execute(
                "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE '%strds%';"
            )
            .fetchall()
            != []
        )
