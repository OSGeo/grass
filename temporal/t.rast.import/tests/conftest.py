"""Fixtures for t.rast.import tests.

(C) 2026 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

import os
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools

RASTER_NAMES = ["prec_1", "prec_2", "prec_3", "prec_4", "prec_5", "prec_6"]
RASTER_MAX_VALUES = [550, 450, 320, 510, 300, 650]

ABSOLUTE_ARCHIVE_SPECS = [
    ("GTiff", "bzip2", "gtiff_bz2.tar.bz2"),
    ("GTiff", "gzip", "gtiff_gz.tar.gz"),
    ("GTiff", "no", "gtiff_nocomp.tar"),
    ("pack", "no", "pack_nocomp.tar"),
    ("pack", "gzip", "pack_gz.tar.gz"),
    ("pack", "bzip2", "pack_bz2.tar.bz2"),
    ("AAIGrid", "bzip2", "aaigrid_bz2.tar.bz2"),
]

RELATIVE_ARCHIVE_SPECS = [
    ("GTiff", "bzip2", "gtiff_bz2.tar.bz2"),
    ("GTiff", "gzip", "gtiff_gz.tar.gz"),
    ("GTiff", "no", "gtiff_nocomp.tar"),
    ("pack", "no", "pack_nocomp.tar"),
    ("pack", "gzip", "pack_gz.tar.gz"),
    ("pack", "bzip2", "pack_bz2.tar.bz2"),
]


@pytest.fixture(scope="module")
def absolute_strds_session(tmp_path_factory):
    """GRASS session with a six-map absolute-time STRDS and pre-exported archives.

    Archives are created in GTiff (bzip2/gzip/no compression), pack
    (bzip2/gzip/no compression), and AAIGrid (bzip2) formats, matching the
    combinations exercised by the shell script test.t.rast.import.sh.
    """
    tmp_path = tmp_path_factory.mktemp("t_rast_import_abs")
    project = tmp_path / "test"
    gs.create_project(project)
    archive_dir = tmp_path / "archives"
    archive_dir.mkdir()

    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session, overwrite=True)
        tools.g_gisenv(set="TGIS_USE_CURRENT_MAPSET=1")
        # Set a real CRS so that the exported proj.txt contains a valid PROJ4
        # string.  The new-project import tests read that string and pass it to
        # gs.create_location(); a plain XY project produces a placeholder that
        # modern PROJ cannot parse, causing proj_create errors.
        tools.g_proj(flags="c", epsg=3358)
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        for name, maxval in zip(RASTER_NAMES, RASTER_MAX_VALUES, strict=True):
            tools.r_mapcalc(expression=f"{name} = rand(0, {maxval})", seed=1)

        register_file = tmp_path / "register_abs.txt"
        register_file.write_text(
            "prec_1|2001-01-01|2001-07-01\n"
            "prec_2|2001-02-01|2001-04-01\n"
            "prec_3|2001-03-01|2001-04-01\n"
            "prec_4|2001-04-01|2001-06-01\n"
            "prec_5|2001-05-01|2001-06-01\n"
            "prec_6|2001-06-01|2001-07-01\n",
            encoding="utf-8",
        )
        tools.t_create(
            type="strds",
            temporaltype="absolute",
            output="precip_abs1",
            title="A test with input files",
            description="A test with input files",
        )
        tools.t_register(
            type="raster",
            input="precip_abs1",
            file=str(register_file),
        )

        archives = {}
        for fmt, compression, suffix in ABSOLUTE_ARCHIVE_SPECS:
            archive = archive_dir / suffix
            tools.t_rast_export(
                input="precip_abs1",
                output=str(archive),
                compression=compression,
                format=fmt,
                directory=str(archive_dir),
            )
            archives[fmt, compression] = str(archive)

        yield SimpleNamespace(
            session=session,
            archives=archives,
            archive_dir=str(archive_dir),
        )


@pytest.fixture(scope="module")
def relative_strds_session(tmp_path_factory):
    """GRASS session with a six-map relative-time STRDS and pre-exported archives.

    Archives cover GTiff (bzip2/gzip/no) and pack (bzip2/gzip/no), matching
    the combinations in test.t.rast.import.relative.sh.
    """
    tmp_path = tmp_path_factory.mktemp("t_rast_import_rel")
    project = tmp_path / "test"
    gs.create_project(project)
    archive_dir = tmp_path / "archives"
    archive_dir.mkdir()

    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session, overwrite=True)
        tools.g_gisenv(set="TGIS_USE_CURRENT_MAPSET=1")
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        for name, maxval in zip(RASTER_NAMES, RASTER_MAX_VALUES, strict=True):
            tools.r_mapcalc(expression=f"{name} = rand(0, {maxval})", seed=1)

        register_file = tmp_path / "register_rel.txt"
        register_file.write_text(
            "prec_1|1|2\nprec_2|2|3\nprec_3|3|4\nprec_4|4|5\nprec_5|5|6\nprec_6|6|7\n",
            encoding="utf-8",
        )
        tools.t_create(
            type="strds",
            temporaltype="relative",
            output="precip_rel",
            title="A test with input files",
            description="A test with input files",
        )
        tools.t_register(
            type="raster",
            input="precip_rel",
            file=str(register_file),
            unit="years",
        )

        archives = {}
        for fmt, compression, suffix in RELATIVE_ARCHIVE_SPECS:
            archive = archive_dir / suffix
            tools.t_rast_export(
                input="precip_rel",
                output=str(archive),
                compression=compression,
                format=fmt,
                directory=str(archive_dir),
            )
            archives[fmt, compression] = str(archive)

        yield SimpleNamespace(
            session=session,
            archives=archives,
            archive_dir=str(archive_dir),
        )


@pytest.fixture(scope="module")
def semantic_label_session(tmp_path_factory):
    """GRASS session with a relative-time STRDS carrying Sentinel-2 semantic labels.

    Maps prec_1..prec_6 are registered with labels S2_1..S2_6 so that the
    export/import roundtrip can be verified to preserve those labels.  Matches
    test.t.rast.import.relative.with.semantic.label.sh.
    """
    tmp_path = tmp_path_factory.mktemp("t_rast_import_sem")
    project = tmp_path / "test"
    gs.create_project(project)
    archive_dir = tmp_path / "archives"
    archive_dir.mkdir()

    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session, overwrite=True)
        tools.g_gisenv(set="TGIS_USE_CURRENT_MAPSET=1")
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        for name, maxval in zip(RASTER_NAMES, RASTER_MAX_VALUES, strict=True):
            tools.r_mapcalc(expression=f"{name} = rand(0, {maxval})", seed=1)

        register_file = tmp_path / "register_sem.txt"
        register_file.write_text(
            "prec_1|1|2|S2_1\n"
            "prec_2|2|3|S2_2\n"
            "prec_3|3|4|S2_3\n"
            "prec_4|4|5|S2_4\n"
            "prec_5|5|6|S2_5\n"
            "prec_6|6|7|S2_6\n",
            encoding="utf-8",
        )
        tools.t_create(
            type="strds",
            temporaltype="relative",
            output="precip_rel",
            title="A test with input files",
            description="A test with input files",
        )
        tools.t_register(
            type="raster",
            input="precip_rel",
            file=str(register_file),
            unit="years",
        )

        archive = archive_dir / "semantic_bz2.tar.bz2"
        tools.t_rast_export(
            input="precip_rel",
            output=str(archive),
            compression="bzip2",
            format="GTiff",
            directory=str(archive_dir),
        )

        yield SimpleNamespace(
            session=session,
            archive=str(archive),
            archive_dir=str(archive_dir),
        )
