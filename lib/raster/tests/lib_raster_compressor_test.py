"""Tests of the GRASS_COMPRESSOR variable for new raster maps

GRASS_COMPRESSOR=NONE makes Rast_open_new() and the other functions
opening new maps write uncompressed data. The value of the "compressed"
field in the cell header is 0 for uncompressed maps.
"""

from pathlib import Path

import numpy as np
import pytest

from grass.script import array as garray
from grass.tools import Tools

NULL = -999999
ROWS = 7
COLS = 5
# Row 3 is all nulls, so the null file is written.
EXPRESSIONS = {
    "CELL": "if(row() == 3, null(), row() * 1000 + col())",
    "FCELL": "if(row() == 3, null(), float(row() * 1000 + col()) / 7)",
    "DCELL": "if(row() == 3, null(), double(row() * 1000 + col()) / 7)",
}


def tools_with_compressor(session, compressor, compress_nulls=None):
    """Tools with GRASS_COMPRESSOR set, or unset for None"""
    env = session.env.copy()
    env.pop("GRASS_COMPRESSOR", None)
    env.pop("GRASS_COMPRESS_NULLS", None)
    if compressor is not None:
        env["GRASS_COMPRESSOR"] = compressor
    if compress_nulls is not None:
        env["GRASS_COMPRESS_NULLS"] = compress_nulls
    env["GRASS_MESSAGE_FORMAT"] = "plain"
    # The tests check stderr also for tools without stdout.
    return Tools(env=env, consistent_return_value=True)


def cellhd_compressed(tools, name):
    """Value of the compressed field in the cell header of a raster map"""
    path = Path(tools.g_findfile(element="cellhd", file=name, format="json")["file"])
    for line in path.read_text(encoding="utf-8").splitlines():
        key, value = line.split(":", maxsplit=1)
        if key.strip() == "compressed":
            return int(value)
    msg = f"No compressed field in {path}"
    raise ValueError(msg)


def read_array(session, name):
    return np.array(garray.array(name, null=NULL, env=session.env))


@pytest.fixture
def tools(session):
    """Tools with the default compressor in a mapset with a small region"""
    tools = tools_with_compressor(session, None)
    tools.g_region(n=ROWS, s=0, e=COLS, w=0, res=1)
    return tools


@pytest.mark.parametrize("map_type", ["CELL", "FCELL", "DCELL"])
def test_none_writes_uncompressed(session, tools, map_type):
    """Maps written with NONE are uncompressed with the same values and nulls"""
    none_tools = tools_with_compressor(session, "NONE")
    expression = EXPRESSIONS[map_type]
    tools.r_mapcalc(expression=f"default = {expression}")
    none_tools.r_mapcalc(expression=f"none = {expression}")

    assert tools.r_info(map="none", format="json")["datatype"] == map_type
    assert cellhd_compressed(tools, "default") > 0
    assert cellhd_compressed(tools, "none") == 0

    default_array = read_array(session, "default")
    none_array = read_array(session, "none")
    np.testing.assert_array_equal(none_array, default_array)
    assert np.all(none_array[2] == NULL)
    assert np.count_nonzero(none_array == NULL) == COLS


def test_lowercase_none(session, tools):
    """The compressor name is not case sensitive"""
    tools_with_compressor(session, "none").r_mapcalc(expression="none = 1")
    assert cellhd_compressed(tools, "none") == 0


@pytest.mark.parametrize(("compress_nulls", "expected"), [(None, "YES"), ("0", "NO")])
def test_none_null_file_compression(session, tools, compress_nulls, expected):
    """NONE leaves the compression of the null file to GRASS_COMPRESS_NULLS"""
    none_tools = tools_with_compressor(session, "NONE", compress_nulls=compress_nulls)
    none_tools.r_mapcalc(expression=f"none = {EXPRESSIONS['CELL']}")
    assert tools.r_compress(map="none", flags="g").text == f"none|CELL|NONE|{expected}"
    assert np.count_nonzero(read_array(session, "none") == NULL) == COLS


def test_r_compress_none_map(session, tools):
    """r.compress reports a NONE map as uncompressed and can compress it"""
    none_tools = tools_with_compressor(session, "NONE")
    none_tools.r_mapcalc(expression=f"none = {EXPRESSIONS['FCELL']}")
    expected = read_array(session, "none")

    result = tools.r_compress(map="none", flags="p")
    assert "<none> is uncompressed (method 0: NONE)" in result.stderr
    assert tools.r_compress(map="none", flags="g").text == "none|FCELL|NONE|YES"

    tools.r_compress(map="none")
    assert cellhd_compressed(tools, "none") > 0
    assert "|NONE|" not in tools.r_compress(map="none", flags="g").text
    np.testing.assert_array_equal(read_array(session, "none"), expected)

    tools.r_compress(map="none", flags="u")
    assert cellhd_compressed(tools, "none") == 0
    np.testing.assert_array_equal(read_array(session, "none"), expected)


def test_r_compress_with_none(session, tools):
    """r.compress with NONE writes the map uncompressed and says so"""
    tools.r_mapcalc(expression=f"default = {EXPRESSIONS['CELL']}")
    expected = read_array(session, "default")
    result = tools_with_compressor(session, "NONE").r_compress(map="default")
    assert "with method NONE" in result.stderr
    assert cellhd_compressed(tools, "default") == 0
    np.testing.assert_array_equal(read_array(session, "default"), expected)


def test_unknown_compressor(session, tools):
    """An unknown name gives a warning and the default compressor"""
    tools.r_mapcalc(expression="default = 1")
    result = tools_with_compressor(session, "NOSUCHMETHOD").r_mapcalc(
        expression="unknown = 1"
    )
    assert "Unknown compression method <NOSUCHMETHOD>" in result.stderr
    assert cellhd_compressed(tools, "unknown") == cellhd_compressed(tools, "default")
    assert cellhd_compressed(tools, "unknown") > 0
