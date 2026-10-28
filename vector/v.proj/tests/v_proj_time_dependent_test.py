# SPDX-License-Identifier: GPL-2.0-or-later

"""Test v.proj with a time-dependent datum transformation.

NAD83(2011) and ITRF2014 are related by a 15-parameter Helmert
transformation whose parameters change with time (EPSG:7807, reference
epoch 2010.0). Without a coordinate epoch, PROJ evaluates the parameters at
the reference epoch. GRASS used to pass epoch 0 (year 0) instead of "no
epoch", which extrapolated the rates two millennia back and moved the
point by tens of meters.

The expected values are what ``cs2cs EPSG:6318 EPSG:9000`` and GDAL give
for the same point.
"""

import os
from io import StringIO
from types import SimpleNamespace

import pytest

import grass.script as gs
from grass.tools import Tools

# Raleigh, NC area in NAD83(2011) as "x y" (longitude latitude).
POINT_ASCII = "-78.0 35.0\n"

# Three vertices about 1 km apart; smax=0 keeps them as they are.
LINE_ASCII = """\
L 3 1
 -78.00 35.00
 -78.01 35.01
 -78.02 35.00
 1 1
"""

# The same point in ITRF2014 at the reference epoch (cs2cs, GDAL).
EXPECTED_X = -78.000004542
EXPECTED_Y = 35.000007465

# 1e-7 degrees is about 1 cm here; the bug was off by about 30 m.
TOLERANCE = 1e-7


@pytest.fixture
def nad83_to_itrf_session(tmp_path):
    """Active session in an ITRF2014 project (EPSG:9000), with a NAD83(2011)
    project (EPSG:6318) next to it containing a point and a line.
    """
    source = "nad83_2011"
    gs.create_project(tmp_path, source, epsg="6318")
    with gs.setup.init(tmp_path / source, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.v_in_ascii(input=StringIO(POINT_ASCII), output="point", separator="space")
        tools.v_in_ascii(
            input=StringIO(LINE_ASCII), output="line", format="standard", flags="n"
        )
    target = tmp_path / "itrf2014"
    gs.create_project(target, epsg="9000")
    with gs.setup.init(target, env=os.environ.copy()) as session:
        yield SimpleNamespace(session=session, source=source)


def read_coords(tools, vector_map, geometry_type):
    """Return list of (x, y) vertex tuples of the map"""
    ascii_out = tools.v_out_ascii(
        input=vector_map, format="standard", type=geometry_type, precision=9
    )
    coords = []
    for line in ascii_out.text.splitlines():
        parts = line.split()
        if len(parts) == 2 and "." in parts[0]:
            coords.append((float(parts[0]), float(parts[1])))
    return coords


def test_point_uses_reference_epoch(nad83_to_itrf_session):
    """A point goes through GPJ_transform()"""
    tools = Tools(session=nad83_to_itrf_session.session)
    tools.v_proj(project=nad83_to_itrf_session.source, input="point", output="point")
    coords = read_coords(tools, "point", "point")
    assert len(coords) == 1
    assert coords[0][0] == pytest.approx(EXPECTED_X, abs=TOLERANCE)
    assert coords[0][1] == pytest.approx(EXPECTED_Y, abs=TOLERANCE)


def test_line_uses_reference_epoch(nad83_to_itrf_session):
    """A line with smax=0 goes through GPJ_transform_array()"""
    tools = Tools(session=nad83_to_itrf_session.session)
    tools.v_proj(
        project=nad83_to_itrf_session.source, input="line", output="line", smax=0
    )
    coords = read_coords(tools, "line", "line")
    assert len(coords) == 3
    assert coords[0][0] == pytest.approx(EXPECTED_X, abs=TOLERANCE)
    assert coords[0][1] == pytest.approx(EXPECTED_Y, abs=TOLERANCE)
