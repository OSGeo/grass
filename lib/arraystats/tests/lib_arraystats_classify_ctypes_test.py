"""Tests for the classification ctypes bindings

The AS_class_*() functions only read an array of doubles (assumed already
sorted ascending, per AS_basic_stats()) and write classbreaks into a caller
-provided array, so no GRASS session is needed here.

Several related functions are intentionally not covered:

- AS_class_discont() beyond the one regression-style case below: its
  algorithm is complex enough that hand-verifying more cases with
  confidence was out of scope for this first pass.
- Error paths that call G_fatal_error() (an unknown algorithm name in
  AS_option_to_algorithm(), more than 10 classes in AS_class_equiprob(),
  an empty array in AS_class_apply_algorithm()): G_fatal_error() calls
  exit() by default, which would terminate the whole pytest process
  rather than raise something catchable. This matches how grass.pygrass
  itself avoids ever triggering G_fatal_error from Python, checking
  preconditions beforehand instead of relying on catching it.
"""

from ctypes import byref, c_double, c_int

import pytest

from grass.lib import arraystats as libas
from grass.lib import gis as libgis

TEN_VALUES = list(range(1, 11))  # 1.0 .. 10.0, already sorted


def make_array(values):
    """Build a ctypes double array from a Python sequence"""
    return (c_double * len(values))(*values)


def test_class_interval_splits_the_range_evenly() -> None:
    data = make_array(TEN_VALUES)
    breaks = (c_double * 3)()
    ret = libas.AS_class_interval(data, len(TEN_VALUES), 3, breaks)
    assert ret == 1
    assert list(breaks) == pytest.approx([3.25, 5.5, 7.75])


def test_class_quant_picks_evenly_spaced_data_points() -> None:
    data = make_array(TEN_VALUES)
    breaks = (c_double * 3)()
    ret = libas.AS_class_quant(data, len(TEN_VALUES), 3, breaks)
    assert ret == 1
    assert list(breaks) == [3.0, 5.0, 7.0]


@pytest.mark.parametrize(
    ("nbreaks", "expected"),
    [
        # An even number of classes (4) centers a break on the mean.
        (3, [2.6277186767309857, 5.5, 8.372281323269014]),
        # An odd number of classes (3) has no break exactly on the mean.
        (2, [4.063859338365493, 6.936140661634507]),
    ],
)
def test_class_stdev_centers_breaks_on_the_mean(nbreaks, expected) -> None:
    data = make_array(TEN_VALUES)
    breaks = (c_double * nbreaks)()
    scale = libas.AS_class_stdev(data, len(TEN_VALUES), nbreaks, breaks)
    assert scale == 1.0
    assert list(breaks) == pytest.approx(expected)


def test_class_equiprob_uses_the_normal_distribution() -> None:
    data = make_array(TEN_VALUES)
    breaks = (c_double * 3)()
    nbreaks = c_int(3)
    ret = libas.AS_class_equiprob(data, len(TEN_VALUES), byref(nbreaks), breaks)
    assert ret == 1
    assert nbreaks.value == 3
    assert list(breaks) == pytest.approx([3.56264624745505, 5.5, 7.43735375254495])


def test_class_frequencies_counts_values_per_class() -> None:
    data = make_array(TEN_VALUES)
    breaks = make_array([3.25, 5.5, 7.75])
    frequencies = (c_int * 4)()
    ret = libas.AS_class_frequencies(data, len(TEN_VALUES), 3, breaks, frequencies)
    assert ret == 1
    assert list(frequencies) == [3, 2, 2, 3]


def test_class_discont_on_a_uniform_range() -> None:
    """Locks in the current output for one deterministic input

    AS_class_discont()'s "natural breaks" algorithm is intricate enough
    that this is a regression check on verified output, not a
    hand-derivable expected value.
    """
    data = make_array(TEN_VALUES)
    breaks = (c_double * 2)()
    chi2 = libas.AS_class_discont(data, len(TEN_VALUES), 2, breaks)
    assert chi2 == pytest.approx(0.12499999999999997)
    assert list(breaks) == pytest.approx([2.5, 5.5])


@pytest.mark.parametrize(
    ("algorithm", "nbreaks", "expected_finfo", "expected_breaks"),
    [
        (libas.CLASS_INTERVAL, 3, 1.0, [3.25, 5.5, 7.75]),
        (
            libas.CLASS_STDEV,
            3,
            1.0,
            [2.6277186767309857, 5.5, 8.372281323269014],
        ),
        (libas.CLASS_QUANT, 3, 1.0, [3.0, 5.0, 7.0]),
        (
            libas.CLASS_EQUIPROB,
            3,
            1.0,
            [3.56264624745505, 5.5, 7.43735375254495],
        ),
        (libas.CLASS_DISCONT, 2, 0.12499999999999997, [2.5, 5.5]),
    ],
)
def test_class_apply_algorithm_dispatches_by_constant(
    algorithm, nbreaks, expected_finfo, expected_breaks
) -> None:
    """AS_class_apply_algorithm() is a thin dispatcher to the AS_class_*()
    functions above, selected by the CLASS_* constant; each case here
    matches the corresponding AS_class_*() test above"""
    data = make_array(TEN_VALUES)
    breaks = (c_double * nbreaks)()
    finfo = libas.AS_class_apply_algorithm(
        algorithm, data, len(TEN_VALUES), byref(c_int(nbreaks)), breaks
    )
    assert finfo == pytest.approx(expected_finfo)
    assert list(breaks) == pytest.approx(expected_breaks)


@pytest.mark.parametrize(
    ("answer", "algorithm"),
    [
        (b"int", libas.CLASS_INTERVAL),
        (b"std", libas.CLASS_STDEV),
        (b"qua", libas.CLASS_QUANT),
        (b"equ", libas.CLASS_EQUIPROB),
        (b"dis", libas.CLASS_DISCONT),
        # The comparison is case-insensitive.
        (b"INT", libas.CLASS_INTERVAL),
    ],
)
def test_option_to_algorithm_maps_the_cli_keyword(answer, algorithm) -> None:
    """AS_option_to_algorithm() only reads option->answer, so a minimal,
    otherwise zeroed Option struct is enough here; no G_parser() call or
    GRASS session is needed to construct one."""
    option = libgis.Option()
    option.answer = libgis.String(answer)
    assert libas.AS_option_to_algorithm(byref(option)) == algorithm
