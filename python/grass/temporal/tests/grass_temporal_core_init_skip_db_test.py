"""Regression test for tgis.init() with and without TGIS DB initialization.

tgis.init() creates a TGIS database in the current mapset by default.
For several tools that is not needed. This test checks if tgis.init()
can be called with and without TGIS DB initialization and that the
required files are created or not created accordingly.
"""

import pytest

from pathlib import Path

import sqlite3

from grass.grassdb.create import create_mapset
import grass.script as gs
import grass.temporal as tgis


@pytest.fixture
def simple_mapset(tmp_path):
    """Create a simple mapset and yield its path."""
    project_c = tmp_path / "c"
    gs.create_project(project_c)
    create_mapset(project_c / "c")
    return project_c / "c"


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
        varfile_content = var_file.read_text(encoding="utf-8")
        assert "TGISDB_DRIVER" in varfile_content
        assert "TGISDB_DATABASE" in varfile_content
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
