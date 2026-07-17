"""Fixtures for t.vect.import tests.

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

VECTOR_NAMES = ["soil_1", "soil_2", "soil_3"]

ARCHIVE_SPECS = [
    ("GML", "bzip2", "gml_bz2.tar.bz2"),
    ("GML", "gzip", "gml_gz.tar.gz"),
    ("GML", "no", "gml_nocomp.tar"),
    ("pack", "no", "pack_nocomp.tar"),
    ("pack", "gzip", "pack_gz.tar.gz"),
    ("pack", "bzip2", "pack_bz2.tar.bz2"),
]


@pytest.fixture(scope="module")
def stvds_session(tmp_path_factory):
    """GRASS session with a three-map absolute-time STVDS and pre-exported archives.

    Vector maps are created with v.random (-z, seeded) and registered in an
    absolute-time STVDS. Archives for GML- and pack-format are created
    with all available compressions: bzip2, gzip, and no compression.
    """
    tmp_path = tmp_path_factory.mktemp("t_vect_import")
    project = tmp_path / "test"
    gs.create_project(project)
    archive_dir = tmp_path / "archives"
    archive_dir.mkdir()

    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session, overwrite=True)
        tools.g_gisenv(set="TGIS_USE_CURRENT_MAPSET=1")
        tools.g_region(s=0, n=80, w=0, e=120, b=0, t=50, res=10, res3=10)
        for i, name in enumerate(VECTOR_NAMES, start=1):
            tools.v_random(
                flags="z",
                output=name,
                n=100,
                zmin=0,
                zmax=100,
                column="height",
                seed=i,
            )

        register_file = tmp_path / "register.txt"
        register_file.write_text(
            "\n".join(VECTOR_NAMES) + "\n",
            encoding="utf-8",
        )
        tools.t_create(
            type="stvds",
            temporaltype="absolute",
            output="soil_abs1",
            title="A test",
            description="A test",
        )
        tools.t_register(
            flags="i",
            type="vector",
            input="soil_abs1",
            file=str(register_file),
            start="2001-01-01",
            increment="1 months",
        )

        archives = {}
        for fmt, compression, suffix in ARCHIVE_SPECS:
            archive = archive_dir / suffix
            tools.t_vect_export(
                format=fmt,
                input="soil_abs1",
                output=str(archive),
                compression=compression,
                directory=str(archive_dir),
            )
            archives[fmt, compression] = str(archive)

        yield SimpleNamespace(
            session=session,
            archives=archives,
            archive_dir=str(archive_dir),
        )
