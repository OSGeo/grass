"""Tests for t.rast.import.

Test groups:
- Absolute-time STRDS with every format/compression combination
- Import flags -l (r.external link), -e (extend extents), and -le together
- Relative-time STRDS with every format/compression combination
- -l flag with relative-time STRDS
- Preservation of Sentinel-2 semantic labels through export/import
- Import into a newly created project: plain, -l (link), -c (create-only),
  and pack-format input
- Import of the bundled precip_2000 test archive with t.info metadata check
- Import of bundled data into a new project, verifying the active environment
  is unchanged afterward

(C) 2026 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

import os
import pathlib
import shutil

import pytest

import grass.script as gs
from grass.tools import Tools

# ---------------------------------------------------------------------------
# Absolute-time import tests
# ---------------------------------------------------------------------------

ABSOLUTE_FORMATS = [
    ("GTiff", "bzip2"),
    ("GTiff", "gzip"),
    ("GTiff", "no"),
    ("pack", "no"),
    ("pack", "gzip"),
    ("pack", "bzip2"),
    ("AAIGrid", "bzip2"),
]

RELATIVE_FORMATS = [
    ("GTiff", "bzip2"),
    ("GTiff", "gzip"),
    ("GTiff", "no"),
    ("pack", "no"),
    ("pack", "gzip"),
    ("pack", "bzip2"),
]

# Mapping from register-file S2 shorthand to the full label printed by
# r.semantic.label.  Used in test_import_preserves_semantic_labels.
SEMANTIC_LABELS = {
    "prec_1": "S2 Visible (Coastal/Aerosol)",
    "prec_2": "S2 Visible (Blue)",
    "prec_3": "S2 Visible (Green)",
    "prec_4": "S2 Visible (Red)",
    "prec_5": "S2 Vegetation Red Edge 1",
    "prec_6": "S2 Vegetation Red Edge 2",
}


def _strds_tinfo(tools, name):
    """Return the t.info -g key/value dict for *name*."""
    return tools.t_info(type="strds", input=name, flags="g").keyval


@pytest.mark.parametrize(("fmt", "compression"), ABSOLUTE_FORMATS)
def test_import_absolute_formats(absolute_strds_session, fmt, compression):
    """Roundtrip of the absolute-time STRDS for each format and compression.

    The reimported STRDS must have the same temporal type and map count as
    the original.  The -o flag bypasses the CRS check so the test is not
    affected by the XY project having no real CRS.
    """
    data = absolute_strds_session
    tools = Tools(session=data.session)
    output = f"reimport_{fmt}_{compression}".lower()
    tools.t_rast_import(
        flags="o",
        input=data.archives[fmt, compression],
        output=output,
        directory=data.archive_dir,
        title="Reimported",
        description="Roundtrip test",
        overwrite=True,
    )
    info = _strds_tinfo(tools, output)
    assert info["temporal_type"] == "absolute"
    assert info["number_of_maps"] == 6


@pytest.mark.parametrize(("fmt", "compression"), ABSOLUTE_FORMATS)
def test_import_absolute_formats_tinfo_times(absolute_strds_session, fmt, compression):
    """Temporal extents are preserved in the roundtrip."""
    data = absolute_strds_session
    tools = Tools(session=data.session)
    output = f"reimport_time_{fmt}_{compression}".lower()
    tools.t_rast_import(
        flags="o",
        input=data.archives[fmt, compression],
        output=output,
        directory=data.archive_dir,
        title="Reimported",
        description="Time check",
        overwrite=True,
    )
    info = _strds_tinfo(tools, output)
    assert info["start_time"] == "'2001-01-01 00:00:00'"
    assert info["end_time"] == "'2001-07-01 00:00:00'"


def test_import_absolute_link_flag(absolute_strds_session):
    """The -l flag links rasters with r.external instead of copying them."""
    data = absolute_strds_session
    tools = Tools(session=data.session)
    tools.t_rast_import(
        flags="lo",
        input=data.archives["GTiff", "bzip2"],
        output="reimport_abs_link",
        directory=data.archive_dir,
        title="Linked",
        description="Link flag test",
        overwrite=True,
    )
    info = _strds_tinfo(tools, "reimport_abs_link")
    assert info["temporal_type"] == "absolute"
    assert info["number_of_maps"] == 6


def test_import_absolute_extend_flag(absolute_strds_session):
    """The -e flag extends the project extents based on the imported dataset."""
    data = absolute_strds_session
    tools = Tools(session=data.session)
    tools.t_rast_import(
        flags="oe",
        input=data.archives["GTiff", "bzip2"],
        output="reimport_abs_extend",
        directory=data.archive_dir,
        title="Extended",
        description="Extend flag test",
        overwrite=True,
    )
    info = _strds_tinfo(tools, "reimport_abs_extend")
    assert info["temporal_type"] == "absolute"
    assert info["number_of_maps"] == 6


def test_import_absolute_link_extend_flags(absolute_strds_session):
    """The -l and -e flags can be combined."""
    data = absolute_strds_session
    tools = Tools(session=data.session)
    tools.t_rast_import(
        flags="loe",
        input=data.archives["GTiff", "bzip2"],
        output="reimport_abs_link_extend",
        directory=data.archive_dir,
        title="Linked and extended",
        description="Link and extend flags test",
        overwrite=True,
    )
    info = _strds_tinfo(tools, "reimport_abs_link_extend")
    assert info["temporal_type"] == "absolute"
    assert info["number_of_maps"] == 6


# ---------------------------------------------------------------------------
# Relative-time import tests
# ---------------------------------------------------------------------------


@pytest.mark.parametrize(("fmt", "compression"), RELATIVE_FORMATS)
def test_import_relative_formats(relative_strds_session, fmt, compression):
    """Roundtrip of the relative-time STRDS for each format and compression."""
    data = relative_strds_session
    tools = Tools(session=data.session)
    output = f"reimport_rel_{fmt}_{compression}".lower()
    tools.t_rast_import(
        flags="o",
        input=data.archives[fmt, compression],
        output=output,
        directory=data.archive_dir,
        title="Reimported",
        description="Relative roundtrip test",
        overwrite=True,
    )
    info = _strds_tinfo(tools, output)
    assert info["temporal_type"] == "relative"
    assert info["number_of_maps"] == 6


@pytest.mark.parametrize(("fmt", "compression"), RELATIVE_FORMATS)
def test_import_relative_formats_tinfo_times(relative_strds_session, fmt, compression):
    """Temporal extents are preserved in the relative-time roundtrip."""
    data = relative_strds_session
    tools = Tools(session=data.session)
    output = f"reimport_rel_time_{fmt}_{compression}".lower()
    tools.t_rast_import(
        flags="o",
        input=data.archives[fmt, compression],
        output=output,
        directory=data.archive_dir,
        title="Reimported",
        description="Relative time check",
        overwrite=True,
    )
    info = _strds_tinfo(tools, output)
    assert info["start_time"] == "'1'"
    assert info["end_time"] == "'7'"


def test_import_relative_link_flag(relative_strds_session):
    """The -l flag works with relative-time STRDS."""
    data = relative_strds_session
    tools = Tools(session=data.session)
    tools.t_rast_import(
        flags="lo",
        input=data.archives["GTiff", "bzip2"],
        output="reimport_rel_link",
        directory=data.archive_dir,
        title="Relative linked",
        description="Relative link flag test",
        overwrite=True,
    )
    info = _strds_tinfo(tools, "reimport_rel_link")
    assert info["temporal_type"] == "relative"
    assert info["number_of_maps"] == 6


# ---------------------------------------------------------------------------
# Semantic-label preservation
# ---------------------------------------------------------------------------


def test_import_preserves_semantic_labels(semantic_label_session):
    """Sentinel-2 semantic labels are preserved through an export/import roundtrip.

    The register file assigns labels S2_1..S2_6 to prec_1..prec_6; after
    export with t.rast.export and reimport with t.rast.import the labels
    must still be present on each raster map.
    """
    data = semantic_label_session
    tools = Tools(session=data.session)
    tools.t_rast_import(
        flags="oe",
        input=data.archive,
        output="reimport_semantic",
        directory=data.archive_dir,
        title="Semantic",
        description="Semantic label roundtrip",
        overwrite=True,
    )
    for map_name, expected_label in SEMANTIC_LABELS.items():
        actual_label = tools.r_semantic_label(
            map=map_name,
            operation="print",
        ).text
        assert actual_label == expected_label, (
            f"Semantic label of {map_name!r}: "
            f"expected {expected_label!r}, got {actual_label!r}"
        )


# ---------------------------------------------------------------------------
# New-project import tests
# ---------------------------------------------------------------------------


def test_import_creates_new_project(absolute_strds_session):
    """Importing with the project option creates a new project under GISDBASE."""
    data = absolute_strds_session
    tools = Tools(session=data.session)
    gisenv = gs.gisenv(env=data.session.env)
    project_name = "new_test_project_plain"
    project_path = pathlib.Path(gisenv["GISDBASE"]) / project_name

    shutil.rmtree(project_path, ignore_errors=True)
    tools.t_rast_import(
        input=data.archives["GTiff", "bzip2"],
        output="precip_imported",
        directory=data.archive_dir,
        project=project_name,
        title="New project import",
        description="Test new project creation",
        overwrite=True,
    )
    assert project_path.exists()
    assert (project_path / "PERMANENT").exists()


def test_import_new_project_with_link(absolute_strds_session):
    """The -l flag can be used when importing into a new project."""
    data = absolute_strds_session
    tools = Tools(session=data.session)
    gisenv = gs.gisenv(env=data.session.env)
    project_name = "new_test_project_link"
    project_path = pathlib.Path(gisenv["GISDBASE"]) / project_name

    shutil.rmtree(project_path, ignore_errors=True)
    tools.t_rast_import(
        flags="l",
        input=data.archives["GTiff", "bzip2"],
        output="precip_imported",
        directory=data.archive_dir,
        project=project_name,
        title="New project link import",
        description="Test new project with link",
        overwrite=True,
    )
    assert project_path.exists()
    assert (project_path / "PERMANENT").exists()


def test_import_new_project_create_only(absolute_strds_session):
    """The -c flag creates the project but does not import any data."""
    data = absolute_strds_session
    tools = Tools(session=data.session)
    gisenv = gs.gisenv(env=data.session.env)
    project_name = "new_test_project_create"
    project_path = pathlib.Path(gisenv["GISDBASE"]) / project_name

    shutil.rmtree(project_path, ignore_errors=True)
    tools.t_rast_import(
        flags="c",
        input=data.archives["GTiff", "bzip2"],
        output="precip_imported",
        directory=data.archive_dir,
        project=project_name,
        title="Create only",
        description="Test project creation without import",
        overwrite=True,
    )
    assert project_path.exists()
    assert (project_path / "PERMANENT").exists()


def test_import_new_project_pack_format(absolute_strds_session):
    """A pack-format archive can be used when importing into a new project."""
    data = absolute_strds_session
    tools = Tools(session=data.session)
    gisenv = gs.gisenv(env=data.session.env)
    project_name = "new_test_project_pack"
    project_path = pathlib.Path(gisenv["GISDBASE"]) / project_name

    shutil.rmtree(project_path, ignore_errors=True)
    tools.t_rast_import(
        input=data.archives["pack", "gzip"],
        output="precip_imported",
        directory=data.archive_dir,
        project=project_name,
        title="Pack import",
        description="Test new project from pack archive",
        overwrite=True,
    )
    assert project_path.exists()
    assert (project_path / "PERMANENT").exists()


# ---------------------------------------------------------------------------
# Bundled test-data import (ported from the gunittest test)
# ---------------------------------------------------------------------------

_BUNDLED_ARCHIVE = (
    pathlib.Path(__file__).parent.parent
    / "testsuite"
    / "data"
    / "precip_2000.tar.bzip2"
)


@pytest.fixture
def bundled_archive():
    """Skip if the bundled test archive is not present."""
    if not _BUNDLED_ARCHIVE.exists():
        pytest.skip("Bundled test data archive not available")
    return _BUNDLED_ARCHIVE


def test_import_bundled_data(tmp_path, bundled_archive):
    """Import precip_2000 and verify t.info metadata matches the known values."""
    project = tmp_path / "test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_gisenv(set="TGIS_USE_CURRENT_MAPSET=1")
        tools.t_rast_import(
            flags="o",
            input=str(bundled_archive),
            output="A",
            basename="a",
            overwrite=True,
        )
        info = _strds_tinfo(tools, "A")
        assert info["start_time"] == "'2000-01-01 00:00:00'"
        assert info["end_time"] == "'2001-01-01 00:00:00'"
        assert info["granularity"] == "'1 month'"
        assert info["map_time"] == "interval"


def test_import_bundled_data_new_project(tmp_path, bundled_archive):
    """Importing into a new project must not change the active session environment."""
    project = tmp_path / "source"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_gisenv(set="TGIS_USE_CURRENT_MAPSET=1")
        gisenv_before = gs.gisenv(env=session.env)

        project_name = "test_project_import"
        project_path = pathlib.Path(gisenv_before["GISDBASE"]) / project_name
        shutil.rmtree(project_path, ignore_errors=True)
        try:
            tools.t_rast_import(
                flags="o",
                input=str(bundled_archive),
                output="B",
                basename="b",
                project=project_name,
                overwrite=True,
            )
            assert project_path.exists()
            assert gs.gisenv(env=session.env) == gisenv_before
        finally:
            shutil.rmtree(project_path, ignore_errors=True)
