# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the upper bound of the compressed size from G_compress_bound()

A dst of the bound size must hold the compressed data of any input, so
compression into it must not fail or warn even when the data do not compress.
"""

import subprocess
import sys

import pytest

from grass.lib.gis import G_check_compressor, G_compress_bound, G_compressor_number

INT_MAX = 2**31 - 1


def number(name):
    """Compressor number, or a skip mark when it is not compiled in"""
    value = G_compressor_number(name.encode())
    if value < 0 or G_check_compressor(value) != 1:
        return pytest.param(name, marks=pytest.mark.skip(f"no {name} support"))
    return name


COMPRESSING = [number(name) for name in ["ZLIB", "LZ4", "BZIP2", "ZSTD"]]

# Tiny inputs, the size where bzip2 output came closest to the documented
# bound, and sizes around 64 KiB and 1 MiB
SIZES = [1, 2, 16, 4765, 65535, 65537, 1048577]

# Compressors warn and fail through G_warning(), which needs the session, so
# the calls run in a separate process in the session. The input is random,
# so it does not compress.
SCRIPT = """
import ctypes
import random
import sys

from grass.lib.gis import G_compress, G_compress_bound, G_gisinit

G_gisinit("lib_gis_compress_bound_test")
number = int(sys.argv[1])
for size in (int(arg) for arg in sys.argv[2:]):
    data = random.Random(size).randbytes(size)
    src = (ctypes.c_ubyte * size).from_buffer_copy(data)
    bound = G_compress_bound(size, number)
    dst = (ctypes.c_ubyte * bound)()
    print(G_compress(src, size, dst, bound, number))
"""


@pytest.mark.parametrize("name", COMPRESSING)
def test_incompressible_data_fit_bound(xy_session_for_module, name):
    """Incompressible data are reported as not compressible, without warnings"""
    output = subprocess.run(
        [
            sys.executable,
            "-c",
            SCRIPT,
            str(G_compressor_number(name.encode())),
            *(str(size) for size in SIZES),
        ],
        capture_output=True,
        text=True,
        env=xy_session_for_module.env,
        check=True,
    )
    assert not output.stderr
    # -2 means that the compressed data are not smaller than the input.
    assert [int(result) for result in output.stdout.split()] == [-2] * len(SIZES)


@pytest.mark.parametrize(
    "size", [0, 1, 600, 4765, 65536, 1048577, 2_000_000_000, INT_MAX]
)
def test_bzip2_bound_documented(size):
    """BZIP2 bound is at least 1% + 600 bytes over the input, up to INT_MAX"""
    number = G_compressor_number(b"BZIP2")
    expected = min(size + size // 100 + 600, INT_MAX)
    assert G_compress_bound(size, number) >= expected
