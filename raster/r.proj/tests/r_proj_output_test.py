"""Pytest for r.proj output flags, with two list tests and three print tests."""

import math
import os

import pytest

import grass.script as gs
from grass.tools import Tools

SRC_PROJECT = "src4326"
INPUT_MID = "input_mid"
INPUT_POLAR = "input_polar"


@pytest.fixture(scope="module")
def session_3413(gisdbase_with_source):
    """Active session in an EPSG:3413 (north polar stereographic) project."""
    gs.create_project(gisdbase_with_source / "dst3413", epsg="3413")
    with gs.setup.init(
        gisdbase_with_source / "dst3413", env=os.environ.copy()
    ) as session:
        yield session


def test_list_plain(session_3857):
    """r.proj -l lists the source raster maps as plain text."""
    tools = Tools(session=session_3857)
    result = tools.r_proj(project=SRC_PROJECT, mapset="PERMANENT", flags="l")
    assert set(result.text.split()) == {"input_mid", "input_polar"}


def test_list_json(session_3857):
    """r.proj -l with format=json lists the source raster maps."""
    tools = Tools(session=session_3857)
    result = tools.r_proj(
        project=SRC_PROJECT, mapset="PERMANENT", flags="l", format="json"
    )
    assert set(result.json) == {"input_mid", "input_polar"}


# The bound values were taken from r.proj -p on the fixture input.
def test_print_plain(session_3857):
    """r.proj -p prints the input bounds in the current projection."""
    tools = Tools(session=session_3857)
    result = tools.r_proj(
        project=SRC_PROJECT, mapset="PERMANENT", input=INPUT_MID, flags="p"
    )
    info = {}
    for line in result.text.splitlines():
        key, _, value = line.partition(":")
        info[key.strip()] = value.strip()
    assert int(info["Source cols"]) == 50
    assert int(info["Source rows"]) == 50
    assert float(info["Local north"]) == pytest.approx(5012341.66384752)
    assert float(info["Local south"]) == pytest.approx(4865942.27950318)
    assert float(info["Local west"]) == pytest.approx(-11131949.07932736)
    assert float(info["Local east"]) == pytest.approx(-11020629.58853408)


def test_print_json(session_3857):
    """r.proj -p with format=json prints the input bounds and dimensions."""
    tools = Tools(session=session_3857)
    result = tools.r_proj(
        project=SRC_PROJECT,
        mapset="PERMANENT",
        input=INPUT_MID,
        flags="p",
        format="json",
    )
    bounds = result.json
    assert bounds["cols"] == 50
    assert bounds["rows"] == 50
    assert bounds["north"] == pytest.approx(5012341.663847517)
    assert bounds["south"] == pytest.approx(4865942.279503176)
    assert bounds["west"] == pytest.approx(-11131949.079327356)
    assert bounds["east"] == pytest.approx(-11020629.588534083)


# The expected bounds were taken from r.proj -p and match independent cs2cs
# projections of the same edge points that bordwalk_edge() samples (every 7.2
# degrees of longitude here). They are not the exact extent: the 50N circle
# reaches about 2.2 km further out, so finer edge sampling would legitimately
# change these values. The default relative tolerance (about 4.5 m here) is far
# smaller than that difference.
def test_print_json_polar(session_3413):
    """r.proj -p prints bounds found along the edges for a polar CRS."""
    tools = Tools(session=session_3413)
    result = tools.r_proj(
        project=SRC_PROJECT,
        mapset="PERMANENT",
        input=INPUT_POLAR,
        flags="p",
        format="json",
    )
    bounds = result.json
    assert bounds["cols"] == 50
    assert bounds["rows"] == 50
    assert all(math.isfinite(bounds[key]) for key in ("north", "south", "east", "west"))
    assert bounds["north"] > bounds["south"]
    assert bounds["east"] > bounds["west"]
    assert bounds["north"] == pytest.approx(4508772.99950119)
    assert bounds["south"] == pytest.approx(-4508772.99950119)
    assert bounds["east"] == pytest.approx(4508772.99950119)
    assert bounds["west"] == pytest.approx(-4508772.99950119)
