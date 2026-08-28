"""Tests for t.vect.import.

Test groups:
- absolute-time STVDS for every format/compression combination
  (GML bzip2/gzip/no, pack bzip2/gzip/no)
- Import flags -o (override CRS check), -e (extend extents), and -oe together

Reprojection is currently not supported for space time vector datasets.
The projection is ignored for STVDS import and import is always run with -o.

(C) 2026 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

from typing import Literal, LiteralString

import pytest

from grass.tools import Tools

ALL_FORMATS: list[tuple[Literal["GML", "pack"], Literal["bzip2", "gzip", "no"]]] = [
    ("GML", "bzip2"),
    ("GML", "gzip"),
    ("GML", "no"),
    ("pack", "no"),
    ("pack", "gzip"),
    ("pack", "bzip2"),
]


def _stvds_tinfo(tools, name):
    """Return the t.info -g key/value dict for *name*."""
    return tools.t_info(type="stvds", input=name, flags="g").keyval


@pytest.mark.parametrize(("fmt", "compression"), ALL_FORMATS)
def test_import_formats(
    stvds_session,
    fmt: Literal["GML", "pack"],
    compression: Literal["bzip2", "gzip", "no"],
):
    """Roundtrip of the absolute-time STVDS for each format and compression."""
    data = stvds_session
    tools = Tools(session=data.session)
    output: LiteralString = f"reimport_{fmt}_{compression}".lower()
    tools.t_vect_import(
        input=data.archives[fmt, compression],
        output=output,
        directory=data.archive_dir,
        title="Reimported",
        description="Roundtrip test",
        overwrite=True,
    )
    info = _stvds_tinfo(tools, output)
    assert info["temporal_type"] == "absolute"
    assert info["number_of_maps"] == 3


def test_import_override_crs_flag(stvds_session):
    """The -o flag allows import when the CRS check would otherwise fail."""
    data = stvds_session
    tools = Tools(session=data.session)
    tools.t_vect_import(
        flags="o",
        input=data.archives["GML", "bzip2"],
        output="reimport_override",
        directory=data.archive_dir,
        title="Override CRS",
        description="Override CRS flag test",
        overwrite=True,
    )
    info = _stvds_tinfo(tools, "reimport_override")
    assert info["temporal_type"] == "absolute"
    assert info["number_of_maps"] == 3


def test_import_extend_flag(stvds_session):
    """The -e flag extends the project extents based on the imported dataset."""
    data = stvds_session
    tools = Tools(session=data.session)
    tools.t_vect_import(
        flags="oe",
        input=data.archives["GML", "bzip2"],
        output="reimport_extend",
        directory=data.archive_dir,
        title="Extended",
        description="Extend flag test",
        overwrite=True,
    )
    info = _stvds_tinfo(tools, "reimport_extend")
    assert info["temporal_type"] == "absolute"
    assert info["number_of_maps"] == 3
