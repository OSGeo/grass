# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests that r.compress names the method the map is written with"""

import pytest

from grass.lib.gis import (
    G_check_compressor,
    G_compressor_name,
    G_compressor_number,
    G_default_compressor,
)
from grass.tools import Tools

DEFAULT = str(G_compressor_name(G_default_compressor()))

EXPRESSIONS = {
    "CELL": "row() * col()",
    "FCELL": "float(row() * col()) / 3",
    "DCELL": "double(row() * col()) / 3",
}


def is_available(name):
    return G_check_compressor(G_compressor_number(name.encode())) == 1


def available(name):
    """Compressor name, or a skip mark when it is not compiled in"""
    if not is_available(name):
        return pytest.param(name, marks=pytest.mark.skip(f"no {name} support"))
    return name


def unavailable(name):
    """Compressor name, or a skip mark when it is compiled in"""
    if is_available(name):
        return pytest.param(name, marks=pytest.mark.skip(f"{name} is available"))
    return name


def compress(session, compressor, map_type="CELL"):
    """Compress an uncompressed map, return the message and the method of the map"""
    name = f"map_{compressor}_{map_type}"
    with Tools(session=session) as tools:
        tools.r_mapcalc(expression=f"{name} = {EXPRESSIONS[map_type]}")
        tools.r_compress(map=name, flags="u")
    env = session.env.copy()
    env["GRASS_COMPRESSOR"] = compressor
    with Tools(env=env, consistent_return_value=True) as tools:
        result = tools.r_compress(map=name)
        method = tools.r_compress(map=name, flags="g").text.split("|")[2]
    return result.stderr, method


@pytest.mark.parametrize("map_type", ["FCELL", "DCELL"])
def test_rle_floating_point(session_in_mapset, map_type):
    """For floating-point maps, RLE gives ZLIB and ZLIB is named"""
    message, method = compress(session_in_mapset, "RLE", map_type)
    assert "with method ZLIB..." in message
    assert method == "ZLIB"


@pytest.mark.parametrize(
    "compressor", [available(name) for name in ["RLE", "ZLIB", "LZ4", "BZIP2", "ZSTD"]]
)
def test_available_compressor(session_in_mapset, compressor):
    """An available compressor is named in the message and used for the map"""
    message, method = compress(session_in_mapset, compressor)
    assert f"with method {compressor}..." in message
    assert method == compressor


# Only BZIP2 and ZSTD are optional in a build.
@pytest.mark.parametrize(
    "compressor", [unavailable(name) for name in ["BZIP2", "ZSTD"]]
)
def test_unavailable_compressor(session_in_mapset, compressor):
    """For a known compressor which is not compiled in, the default is named

    The default is what the library writes the map with. Runs only in a
    build without the compressor.
    """
    message, method = compress(session_in_mapset, compressor)
    assert f"with method {DEFAULT}..." in message
    assert method == DEFAULT


def test_unknown_compressor(session_in_mapset):
    """For an unknown compressor, the default is named"""
    message, method = compress(session_in_mapset, "UNKNOWN")
    assert f"with method {DEFAULT}..." in message
    assert method == DEFAULT
