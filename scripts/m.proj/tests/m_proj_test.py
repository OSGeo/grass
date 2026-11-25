# SPDX-License-Identifier: GPL-2.0-or-later

"""Test m.proj with different kinds of CRS definitions.

The point (-78, 35) near Raleigh, NC is used throughout. Its UTM 17N
coordinates and its ITRF2014 coordinates (from NAD83(2011) at the
reference epoch of the transformation) come from cs2cs and GDAL.
"""

import os
from io import StringIO

import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError
from grass.tools import Tools

LON = -78.0
LAT = 35.0
UTM_X = 773798.096
UTM_Y = 3877156.692
# NAD83(2011) -> ITRF2014 (EPSG:7807), evaluated at the reference epoch.
ITRF_LON = -78.000004542
ITRF_LAT = 35.000007465

UTM_TOLERANCE = 0.01
DEGREE_TOLERANCE = 1e-7

WGS84_LL_PROJ = "+proj=longlat +datum=WGS84"
UTM17_PROJ = "+proj=utm +zone=17 +datum=WGS84"


@pytest.fixture(scope="module")
def nad83_session(tmp_path_factory):
    """Active session in a NAD83(2011) project (EPSG:6318)"""
    project = tmp_path_factory.mktemp("m_proj") / "nad83_2011"
    gs.create_project(project, epsg="6318")
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session


def reproject(session, text, **kwargs):
    """Run m.proj on the text and return list of rows of floats"""
    output = Tools(session=session).m_proj(input=StringIO(text), **kwargs).text
    return [[float(value) for value in line.split("|")] for line in output.splitlines()]


def test_epsg_codes_use_x_y_order(nad83_session):
    """EPSG:4326 has latitude first, but m.proj reads longitude latitude"""
    rows = reproject(
        nad83_session,
        f"{LON} {LAT}\n",
        proj_in="EPSG:4326",
        proj_out="EPSG:32617",
        flags="d",
    )
    assert len(rows) == 1
    assert rows[0][0] == pytest.approx(UTM_X, abs=UTM_TOLERANCE)
    assert rows[0][1] == pytest.approx(UTM_Y, abs=UTM_TOLERANCE)


def test_epsg_output_uses_x_y_order(nad83_session):
    """Longitude is written first even when the output CRS has latitude first"""
    rows = reproject(
        nad83_session,
        f"{UTM_X} {UTM_Y}\n",
        proj_in="EPSG:32617",
        proj_out="EPSG:4326",
        flags="d",
    )
    assert rows[0][0] == pytest.approx(LON, abs=DEGREE_TOLERANCE)
    assert rows[0][1] == pytest.approx(LAT, abs=DEGREE_TOLERANCE)


def test_proj_strings(nad83_session):
    """Two PROJ strings keep working"""
    rows = reproject(
        nad83_session,
        f"{LON} {LAT}\n",
        proj_in=WGS84_LL_PROJ,
        proj_out=UTM17_PROJ,
        flags="d",
    )
    assert rows[0][0] == pytest.approx(UTM_X, abs=UTM_TOLERANCE)
    assert rows[0][1] == pytest.approx(UTM_Y, abs=UTM_TOLERANCE)


def test_proj_string_and_epsg_code(nad83_session):
    """A PROJ string on one side and an authority code on the other"""
    rows = reproject(
        nad83_session,
        f"{LON} {LAT}\n",
        proj_in=WGS84_LL_PROJ,
        proj_out="EPSG:32617",
        flags="d",
    )
    assert rows[0][0] == pytest.approx(UTM_X, abs=UTM_TOLERANCE)
    assert rows[0][1] == pytest.approx(UTM_Y, abs=UTM_TOLERANCE)


def test_wkt(nad83_session):
    """WKT of the project (NAD83(2011)) to ITRF2014 given as EPSG code"""
    tools = Tools(session=nad83_session)
    wkt = tools.g_proj(flags="pf", format="wkt").text.strip()
    assert wkt.startswith("GEOGCRS")
    rows = reproject(
        nad83_session, f"{LON} {LAT}\n", proj_in=wkt, proj_out="EPSG:9000", flags="d"
    )
    assert rows[0][0] == pytest.approx(ITRF_LON, abs=DEGREE_TOLERANCE)
    assert rows[0][1] == pytest.approx(ITRF_LAT, abs=DEGREE_TOLERANCE)


def test_current_project_crs_is_exact(nad83_session):
    """The datum of the current project is used, not a lossy PROJ string

    NAD83(2011) and ITRF2014 differ by about a meter here. A PROJ string
    of the project would carry towgs84=0,0,0 and give no shift at all.
    """
    rows = reproject(nad83_session, f"{LON} {LAT}\n", proj_out="EPSG:9000", flags="d")
    assert rows[0][0] == pytest.approx(ITRF_LON, abs=DEGREE_TOLERANCE)
    assert rows[0][1] == pytest.approx(ITRF_LAT, abs=DEGREE_TOLERANCE)


def test_ll_flags_round_trip(nad83_session):
    """-i and -o convert between LL WGS84 and the current project"""
    to_project = reproject(nad83_session, f"{LON} {LAT}\n", flags="id")
    back = reproject(
        nad83_session, f"{to_project[0][0]} {to_project[0][1]}\n", flags="od"
    )
    assert back[0][0] == pytest.approx(LON, abs=DEGREE_TOLERANCE)
    assert back[0][1] == pytest.approx(LAT, abs=DEGREE_TOLERANCE)


def test_copy_input_and_header(nad83_session):
    """-e echoes the input coordinates and -c adds column names"""
    output = (
        Tools(session=nad83_session)
        .m_proj(
            input=StringIO(f"{LON} {LAT}\n"),
            proj_in="EPSG:4326",
            proj_out="EPSG:32617",
            flags="dec",
        )
        .text
    )
    lines = output.splitlines()
    assert lines[0] == "input_x|input_y|x|y|z"
    values = [float(value) for value in lines[1].split("|")]
    assert values[0] == pytest.approx(LON)
    assert values[1] == pytest.approx(LAT)
    assert values[2] == pytest.approx(UTM_X, abs=UTM_TOLERANCE)
    assert values[3] == pytest.approx(UTM_Y, abs=UTM_TOLERANCE)


def test_unknown_crs_fails(nad83_session):
    """An unknown authority code is an error"""
    with pytest.raises(CalledModuleError):
        reproject(
            nad83_session, f"{LON} {LAT}\n", proj_in="EPSG:999999", proj_out="EPSG:4326"
        )
