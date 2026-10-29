"""Tests of compression into a destination smaller than the upper bound

The compressors accept a destination buffer smaller than the bound from
G_compress_bound(). The compressed data must then be written only when it
fits, otherwise the functions return -2 (dst too small) and leave the memory
after dst untouched.
"""

import math
import struct
import subprocess
import sys

import pytest

from grass.lib.gis import G_check_compressor, G_compressor_number


def number(name):
    """Compressor number, or a skip mark when it is not compiled in"""
    value = G_compressor_number(name.encode())
    if value < 0 or G_check_compressor(value) != 1:
        return pytest.param(name, marks=pytest.mark.skip(f"no {name} support"))
    return name


# These compress into a temporary buffer when dst is smaller than the bound.
BUFFERED = [number(name) for name in ["ZLIB", "LZ4", "BZIP2", "ZSTD"]]
ALL = [number(name) for name in ["NONE", "RLE"]] + BUFFERED

# One row of 10000 FCELL values, compressible to more than SMALL bytes
DATA = b"".join(
    struct.pack(">f", round(100 * math.sin(i / 50), 1)) for i in range(10000)
)
SMALL = 100
CANARY_BYTE = 0xA5
CANARY = bytes([CANARY_BYTE]) * len(DATA)

# The call is made in a separate process in the session because the
# compressors warn about dst smaller than the bound and warnings need the
# session environment. The destination is the start of a larger buffer whose
# tail is filled with a canary byte, and the whole buffer is returned. The
# tail is as long as the input, so that an overflow stays inside the buffer:
# the compressors write compressed data only when it is shorter than the input.
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


def compress(session, name, dst_sz):
    """Compress DATA into dst_sz bytes, return result, dst, and the tail"""
    env = session.env.copy()
    # Level 0 does not compress, so the data would never be copied to dst.
    env["GRASS_ZLIB_LEVEL"] = "1"
    output = subprocess.run(
        [
            sys.executable,
            "-c",
            SCRIPT,
            str(G_compressor_number(name.encode())),
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


@pytest.mark.parametrize("name", ALL)
def test_dst_too_small(xy_session_for_module, name):
    """Data which does not fit is reported as -2 and nothing is written after dst"""
    result, unused, tail = compress(xy_session_for_module, name, SMALL)
    assert tail == CANARY
    assert result == -2


@pytest.mark.parametrize("name", BUFFERED)
def test_dst_smaller_than_bound_fits(xy_session_for_module, name):
    """Data which fits into dst smaller than the bound is written there"""
    size, stream, unused = compress(xy_session_for_module, name, len(DATA))
    assert 0 < size < len(DATA)
    result, dst, tail = compress(xy_session_for_module, name, size)
    assert tail == CANARY
    assert result == size
    assert dst == stream[:size]
