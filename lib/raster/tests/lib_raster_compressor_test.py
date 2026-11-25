# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the GRASS_COMPRESSOR variable for new raster maps

GRASS_COMPRESSOR=NONE makes Rast_open_new() and the other functions
opening new maps write uncompressed data. The tests get the compression
method of a map from r.compress -g.
"""

import numpy as np
import pytest

from grass.script import array as garray
from grass.tools import Tools

NULL = -999999
# Row 3 is all nulls, so the null file is written.
EXPRESSIONS = {
    "CELL": "if(row() == 3, null(), row() * 1000 + col())",
    "FCELL": "if(row() == 3, null(), float(row() * 1000 + col()) / 7)",
    "DCELL": "if(row() == 3, null(), double(row() * 1000 + col()) / 7)",
}


def tools_with_compressor(session, compressor, compress_nulls=None):
    """Tools with GRASS_COMPRESSOR set, or unset for None"""
    env = session.env.copy()
    # Without the variables, the tools use the defaults, whatever the
    # environment of the test run sets.
    env.pop("GRASS_COMPRESSOR", None)
    env.pop("GRASS_COMPRESS_NULLS", None)
    if compressor is not None:
        env["GRASS_COMPRESSOR"] = compressor
    if compress_nulls is not None:
        env["GRASS_COMPRESS_NULLS"] = compress_nulls
    # Gives a result with stderr for every tool.
    return Tools(env=env, consistent_return_value=True)


def compression_method(tools, name):
    """Name of the compression method of a raster map, NONE if uncompressed"""
    return tools.r_compress(map=name, flags="g").text.split("|")[2]


def read_array(session, name):
    return np.array(garray.array(name, null=NULL, env=session.env))


@pytest.fixture
def tools(session):
    """Tools with the default compressor in a mapset with a small region"""
    tools = tools_with_compressor(session, None)
    tools.g_region(n=7, s=0, e=5, w=0, res=1)
    return tools


@pytest.mark.parametrize("map_type", ["CELL", "FCELL", "DCELL"])
def test_none_writes_uncompressed(session, tools, map_type):
    """Maps written with NONE are uncompressed with the same values and nulls"""
    expression = EXPRESSIONS[map_type]
    tools.r_mapcalc(expression=f"default = {expression}")
    tools_with_compressor(session, "NONE").r_mapcalc(expression=f"none = {expression}")

    assert tools.r_info(map="default", format="json")["datatype"] == map_type
    assert tools.r_info(map="none", format="json")["datatype"] == map_type
    assert compression_method(tools, "default") != "NONE"
    assert compression_method(tools, "none") == "NONE"
    np.testing.assert_array_equal(
        read_array(session, "none"), read_array(session, "default")
    )


@pytest.mark.parametrize(("compress_nulls", "expected"), [(None, "YES"), ("0", "NO")])
def test_none_null_file_compression(session, tools, compress_nulls, expected):
    """NONE leaves the compression of the null file to GRASS_COMPRESS_NULLS"""
    none_tools = tools_with_compressor(session, "NONE", compress_nulls=compress_nulls)
    tools.r_mapcalc(expression=f"default = {EXPRESSIONS['CELL']}")
    none_tools.r_mapcalc(expression=f"none = {EXPRESSIONS['CELL']}")
    assert tools.r_compress(map="none", flags="g").text == f"none|CELL|NONE|{expected}"
    np.testing.assert_array_equal(
        read_array(session, "none"), read_array(session, "default")
    )


def test_unknown_compressor(session, tools):
    """An unknown name gives a warning and the default compressor

    This behavior is not changed by NONE support; the test pins it.
    """
    tools.r_mapcalc(expression="default = 1")
    result = tools_with_compressor(session, "NOSUCHMETHOD").r_mapcalc(
        expression="unknown = 1"
    )
    assert "Unknown compression method <NOSUCHMETHOD>" in result.stderr
    assert compression_method(tools, "unknown") == compression_method(tools, "default")
    assert compression_method(tools, "unknown") != "NONE"
