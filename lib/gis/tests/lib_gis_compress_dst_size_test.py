# SPDX-FileCopyrightText: 2026 Vaclav Petras
# SPDX-FileCopyrightText: GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of compression into destinations of the bound size and smaller

The compressors accept a destination buffer smaller than the bound from
G_compress_bound(). The compressed data must then be written only when it
fits, otherwise the functions return -2 (dst too small) and leave the memory
after dst untouched. A dst of the bound size holds the compressed data of any
input.
"""

import math
import struct
import subprocess
import sys

import pytest

from grass.lib.gis import G_check_compressor, G_compress_bound, G_compressor_number


def number(name):
    """Compressor number, or a skip mark when it is not compiled in"""
    value = G_compressor_number(name.encode())
    if value < 0 or G_check_compressor(value) != 1:
        return pytest.param(name, marks=pytest.mark.skip(f"no {name} support"))
    return name


# These compress DATA, so a dst of the compressed size is smaller than the bound.
COMPRESSING = [number(name) for name in ["ZLIB", "LZ4", "BZIP2", "ZSTD"]]
ALL = [number(name) for name in ["NONE", "RLE"]] + COMPRESSING

# One row of 10000 FCELL values, compressible to more than SMALL bytes
DATA = b"".join(
    struct.pack(">f", round(100 * math.sin(i / 50), 1)) for i in range(10000)
)
SMALL = 100
CANARY_BYTE = 0xA5
CANARY = bytes([CANARY_BYTE]) * len(DATA)

# Compressors warn about dst smaller than the bound, and warnings need the
# session, so the call runs in a separate process in the session. The tail
# after dst holds canary bytes. It is as long as the input because the
# compressors write only data shorter than the input.
SCRIPT = """
import ctypes
import sys

from grass.lib.gis import G_compress, G_gisinit

G_gisinit("lib_gis_compress_dst_size_test")
number, dst_sz, canary = (int(arg) for arg in sys.argv[1:])
data = sys.stdin.buffer.read()
src = (ctypes.c_ubyte * len(data)).from_buffer_copy(data)
buffer = (ctypes.c_ubyte * (dst_sz + len(data))).from_buffer_copy(
    bytes(dst_sz) + bytes([canary]) * len(data)
)
result = G_compress(src, len(data), buffer, dst_sz, number)
sys.stdout.buffer.write(b"%d\\n" % result + bytes(buffer))
"""


def compress(session, compressor_name, dst_sz):
    """Compress DATA into dst_sz bytes, return result, dst, and the tail"""
    env = session.env.copy()
    # Level 0 does not compress, so the data would never be copied to dst.
    env["GRASS_ZLIB_LEVEL"] = "1"
    output = subprocess.run(
        [
            sys.executable,
            "-c",
            SCRIPT,
            str(G_compressor_number(compressor_name.encode())),
            str(dst_sz),
            str(CANARY_BYTE),
        ],
        input=DATA,
        capture_output=True,
        env=env,
        check=True,
    ).stdout
    result, buffer = output.split(b"\n", 1)
    return int(result), buffer[:dst_sz], buffer[dst_sz:]


@pytest.mark.parametrize("compressor_name", ALL)
def test_dst_too_small(xy_session_for_module, compressor_name):
    """Data which does not fit is reported as -2 and nothing is written after dst"""
    result, _, tail = compress(xy_session_for_module, compressor_name, SMALL)
    assert tail == CANARY
    assert result == -2


@pytest.mark.parametrize("compressor_name", COMPRESSING)
def test_dst_smaller_than_bound_fits(xy_session_for_module, compressor_name):
    """Data which fits into dst smaller than the bound is written there"""
    size, stream, _ = compress(xy_session_for_module, compressor_name, len(DATA))
    assert 0 < size < len(DATA)
    result, dst, tail = compress(xy_session_for_module, compressor_name, size)
    assert tail == CANARY
    assert result == size
    assert dst == stream[:size]


# One byte, typical row sizes, and more than one bzip2 block (900 kB)
SIZES = [1, 100, 10_000, 1_000_000]

# The compressors report problems with G_warning(), which needs a session, so
# the calls run in a separate process in the session. The input is random
# bytes, which practically never compress (the output is not smaller).
BOUND_SCRIPT = """
import ctypes
import random
import sys

from grass.lib.gis import G_compress, G_compress_bound, G_gisinit

G_gisinit("lib_gis_compress_dst_size_test")
number = int(sys.argv[1])
for size in (int(arg) for arg in sys.argv[2:]):
    data = random.Random(size).randbytes(size)
    src = (ctypes.c_ubyte * size).from_buffer_copy(data)
    bound = G_compress_bound(size, number)
    dst = (ctypes.c_ubyte * bound)()
    print(G_compress(src, size, dst, bound, number))
"""


@pytest.mark.parametrize("compressor_name", COMPRESSING)
def test_incompressible_data_fit_bound(xy_session_for_module, compressor_name):
    """Random data are reported as not compressible (-2), without warnings"""
    output = subprocess.run(
        [
            sys.executable,
            "-c",
            BOUND_SCRIPT,
            str(G_compressor_number(compressor_name.encode())),
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


@pytest.mark.parametrize("size", [0, 1000, 2_000_000_000])
def test_bzip2_bound_documented(size):
    """BZIP2 bound is at least the size the bzip2 documentation asks for

    The documentation asks for 1% + 600 bytes over the input.
    """
    number = G_compressor_number(b"BZIP2")
    assert G_compress_bound(size, number) >= size + size // 100 + 600


def test_bzip2_bound_large_input():
    """BZIP2 bound does not overflow when 1% + 600 bytes exceed an int

    2.14e9 fits into a 32-bit int, but with 1% + 600 bytes added it does not,
    so the bound is limited and must still not be smaller than the input.
    """
    size = 2_140_000_000
    assert G_compress_bound(size, G_compressor_number(b"BZIP2")) >= size
