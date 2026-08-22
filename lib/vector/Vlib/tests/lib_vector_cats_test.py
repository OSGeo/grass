"""Regression tests for converting a cat_list to an array

Vect_new_cat_list() returns a list with no ranges, and adding ranges is a
separate step, so an empty list is a state a caller can reach and pass on.
Vect_cat_list_to_array() left its output array unallocated in that case and
then read the first element of it, which is a null pointer dereference.
Vect_copy_table_by_cat_list() guards only against a null cat_list, not an
empty one, so the crash was reachable through the public API.

The library is called in a subprocess because a null pointer dereference
there would end the test run rather than fail a single test.
"""

import subprocess
import sys

CONVERT_CAT_LISTS = """
import ctypes

from grass.lib.vector import (
    Vect_cat_list_to_array,
    Vect_destroy_cat_list,
    Vect_new_cat_list,
    Vect_str_to_cat_list,
)


def convert(cat_list):
    values = ctypes.POINTER(ctypes.c_int)()
    count = ctypes.c_int(-1)
    ret = Vect_cat_list_to_array(cat_list, ctypes.byref(values), ctypes.byref(count))
    return ret, [values[i] for i in range(count.value)]


cat_list = Vect_new_cat_list()
try:
    ret, values = convert(cat_list)
finally:
    Vect_destroy_cat_list(cat_list)
# An empty selection is not a failure, so the success code is expected.
assert ret == 0, f"empty list returned {ret}"
assert values == [], f"empty list gave {values}"

cat_list = Vect_new_cat_list()
try:
    # Returns the number of errors in the ranges, so zero means parsed.
    assert Vect_str_to_cat_list(b"4,1-3,2", cat_list) == 0
    ret, values = convert(cat_list)
finally:
    Vect_destroy_cat_list(cat_list)
assert ret == 0, f"ranges returned {ret}"
assert values == [1, 2, 3, 4], f"ranges gave {values}"

print("conversions ok")
"""


def test_cat_list_conversion(xy_session_env):
    """An empty list converts to an empty array, and ranges still sort and dedup

    The empty list is the case that used to crash. The ranges cover what must
    not change, both duplicates and ranges given out of order.
    """
    process = subprocess.run(
        [sys.executable, "-c", CONVERT_CAT_LISTS],
        env=xy_session_env,
        capture_output=True,
        text=True,
        check=False,
    )
    assert process.returncode == 0, (
        f"return code {process.returncode}\n{process.stdout}\n{process.stderr}"
    )
    assert "conversions ok" in process.stdout
