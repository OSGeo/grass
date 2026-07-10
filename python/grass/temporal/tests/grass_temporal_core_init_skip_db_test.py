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

ds_types_dict = {
    "strds": ("strds"),
    "str3ds": ("str3ds"),
    "stvds": ("stvds"),
    "rast": ("raster", "rast"),
    "raster": ("raster", "rast"),
    "raster_3d": ("raster_3d", "rast3d", "raster3d"),
    "rast3d": ("raster_3d", "rast3d", "raster3d"),
    "raster3d": ("raster_3d", "rast3d", "raster3d"),
    "vect": ("vector", "vect"),
    "vector": ("vector", "vect"),
}


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
        assert tgis.get_tgis_version() == 2
        assert tgis.get_tgis_backend() == "sqlite"
        assert tgis.get_enable_mapset_check() is True
        assert tgis.get_enable_timestamp_write() is True
        assert tgis.get_sql_template_path().endswith("sql")
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
                "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE '%strds%';",
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
        assert tgis.get_tgis_version() == 2
        assert tgis.get_tgis_backend() == "sqlite"
        assert tgis.get_enable_mapset_check() is True
        assert tgis.get_enable_timestamp_write() is True
        assert tgis.get_sql_template_path().endswith("sql")
        assert not var_file.exists()
        assert not tgis_db.exists()


@pytest.mark.parametrize("ident", [None, "test@c"])
def test_stds_construction_without_tgis_db_initialization(
    simple_mapset,
    ident: str | None,
):
    """STDS objects can be constructed even if the temporal DB is not initialized.

    Empty STDS objects are constructed in several temporal modules, also where a
    temporal database is not necessarily initialized. This test verifies that
    STDS objects can be constructed even when tgis is initialized with
    tgis.init(skip_db_init=True).
    Methods that require access to the temporal database (e.g., is_in_db())
    are not tested (they would raise exceptions).
    """
    with gs.setup.init(simple_mapset):
        tgis.init(skip_db_init=True)
        # None of these must raise if the temporal database is not initialized
        strds = tgis.SpaceTimeRasterDataset(ident)
        assert "number_of_semantic_labels" in strds.metadata.D
        tgis.SpaceTimeRaster3DDataset(ident)
        tgis.SpaceTimeVectorDataset(ident)
        # Test dataset_factory for all dataset types, which should be constructed without DB access
        for ds_type, valid_ds_types in ds_types_dict.items():
            stds = tgis.dataset_factory(ds_type, ident)
            assert stds.get_id() == ident
            assert stds.get_type() in valid_ds_types


@pytest.mark.parametrize("ident", [None, "test"])
def test_stds_construction_with_tgis_db_initialization(
    simple_mapset,
    ident: str | None,
):
    """STDS objects are constructed with access to the temporal DB if initialized.

    This is the counterpart to test_stds_construction_without_tgis_db_initialization:
    When tgis.init() is called without skip_db_init=True, the temporal database is
    accessible and STDS objects can access TGIS metadata from the DB (not the dataset).
    Methods that require access to the temporal database (e.g., is_in_db())
    are tested to return successfully and valid.

    """
    with gs.setup.init(simple_mapset):
        tgis.init()
        dbif, _connection_state_changes = tgis.init_dbif(None)
        if ident is not None:
            ident = f"{ident}@{tgis.get_current_mapset()}"
        assert tgis.get_tgis_metadata() is not None
        assert tgis.get_tgis_db_version_from_metadata() == tgis.get_tgis_db_version()
        tgis.get_current_mapset()
        strds = tgis.SpaceTimeRasterDataset(ident)
        assert strds.get_id() == ident
        assert strds.is_in_db() is False
        assert "number_of_semantic_labels" in strds.metadata.D
        # Test dataset_factory for all dataset types, which should be constructed with DB access
        for ds_type, valid_ds_types in ds_types_dict.items():
            stds = tgis.dataset_factory(ds_type, ident)
            assert stds.get_id() == ident
            assert stds.get_type() in valid_ds_types
            assert stds.is_in_db(dbif) is False
