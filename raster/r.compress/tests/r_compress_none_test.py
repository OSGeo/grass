# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of r.compress with uncompressed maps from GRASS_COMPRESSOR=NONE"""

import numpy as np

from grass.script import array as garray
from grass.tools import Tools

NULL = -999999
EXPRESSION = "if(row() == 3, null(), row() * 1000 + col())"


def tools_with_compressor(session, compressor):
    """Tools with GRASS_COMPRESSOR set, or unset for None"""
    env = session.env.copy()
    env.pop("GRASS_COMPRESSOR", None)
    if compressor is not None:
        env["GRASS_COMPRESSOR"] = compressor
    # Gives a result with stderr for every tool.
    return Tools(env=env, consistent_return_value=True)


def read_array(session, name):
    return np.array(garray.array(name, null=NULL, env=session.env))


def test_compress_none_map(session):
    """r.compress reports a NONE map as uncompressed and can compress it"""
    tools = tools_with_compressor(session, None)
    tools_with_compressor(session, "NONE").r_mapcalc(
        expression=f"none_map = float({EXPRESSION}) / 7"
    )
    expected = read_array(session, "none_map")

    result = tools.r_compress(map="none_map", flags="p")
    assert "<none_map> is uncompressed (method 0: NONE)" in result.stderr

    tools.r_compress(map="none_map")
    assert "|NONE|" not in tools.r_compress(map="none_map", flags="g").text
    np.testing.assert_array_equal(read_array(session, "none_map"), expected)

    tools.r_compress(map="none_map", flags="u")
    assert tools.r_compress(map="none_map", flags="g").text == "none_map|FCELL|NONE|YES"
    np.testing.assert_array_equal(read_array(session, "none_map"), expected)


def test_compress_with_none(session):
    """r.compress with NONE writes the map uncompressed and says so"""
    tools = tools_with_compressor(session, None)
    tools.r_mapcalc(expression=f"default_map = {EXPRESSION}")
    expected = read_array(session, "default_map")

    result = tools_with_compressor(session, "NONE").r_compress(map="default_map")
    assert "with method NONE" in result.stderr
    assert (
        tools.r_compress(map="default_map", flags="g").text
        == "default_map|CELL|NONE|YES"
    )
    np.testing.assert_array_equal(read_array(session, "default_map"), expected)
