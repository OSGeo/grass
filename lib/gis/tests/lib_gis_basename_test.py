# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of joining strings with the basename separator"""

import subprocess
import sys

import pytest

# Builds with fortified string functions abort the process on a write past the
# allocation, so the call runs in a separate process.
SCRIPT = """
import sys
from ctypes import POINTER, c_char_p, cast

from grass.lib.gis import G_join_basename_strings

strings = [s.encode() for s in sys.argv[1:]]
# A spare element, so a bad length cannot read outside the array.
array = (c_char_p * (len(strings) + 1))(*strings, b"spare")
result = G_join_basename_strings(cast(array, POINTER(c_char_p)), len(strings))
sys.stdout.buffer.write(result.data)
"""


def join(*strings):
    return subprocess.run(
        [sys.executable, "-c", SCRIPT, *strings],
        capture_output=True,
        check=True,
    ).stdout


def test_join_two():
    assert join("a", "b") == b"a_b"


def test_join_one():
    assert join("a") == b"a"


@pytest.mark.xfail(
    strict=True,
    reason="G_join_basename_strings() reads strings[0] when len is 0",
)
def test_join_none_is_empty():
    """No strings give an empty string, not the first element of the array"""
    assert join() == b""
