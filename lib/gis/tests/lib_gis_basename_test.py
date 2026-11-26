# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of joining strings with the basename separator"""

from ctypes import POINTER, c_char_p, cast

import pytest

from grass.lib.gis import G_join_basename_strings


def join(strings):
    """Join strings, passing a spare element so a bad length cannot read outside"""
    array = (c_char_p * (len(strings) + 1))(*[s.encode() for s in strings], b"spare")
    return G_join_basename_strings(cast(array, POINTER(c_char_p)), len(strings))


def test_join_two():
    assert join(["a", "b"]) == b"a_b"


def test_join_one():
    assert join(["a"]) == b"a"


@pytest.mark.xfail(
    strict=True,
    reason="G_join_basename_strings() reads strings[0] when len is 0",
)
def test_join_none_is_empty():
    """No strings give an empty string, not the first element of the array"""
    assert join([]) == b""
