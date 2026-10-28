"""Tests of the ctypes preamble shared by the grass.lib modules

POINTER(None) in the generated modules is ctypes.c_void_p itself, so the
preamble must not replace its from_param: that would change every void pointer
argument in the process, including those of other packages.
"""

import ctypes
import sys

import pytest

import grass.lib.gis  # noqa: F401  # the import is what is being tested


def test_void_pointer_converts_integers():
    """After importing grass.lib, c_void_p still converts an integer address"""
    assert not isinstance(ctypes.c_void_p.from_param(2**40), int)


@pytest.mark.skipif(sys.platform != "win32", reason="C long is 32 bits on Windows")
def test_void_pointer_argument_takes_a_64_bit_address():
    """A void pointer argument declared after the import takes any address"""
    memmove = ctypes.CDLL("msvcrt").memmove
    memmove.argtypes = (ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t)
    memmove.restype = ctypes.c_void_p
    source = ctypes.create_string_buffer(b"grass")
    target = ctypes.create_string_buffer(len(source))
    memmove(ctypes.addressof(target), ctypes.addressof(source), len(source))
    assert target.value == b"grass"
