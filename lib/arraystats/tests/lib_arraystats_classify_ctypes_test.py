"""Tests for the classification ctypes bindings

The AS_class_*() functions only read an array of doubles (assumed already
sorted ascending, per AS_basic_stats()) and write classbreaks into a caller
-provided array, so no GRASS session is needed here.

Several related functions are intentionally not covered:

- AS_class_discont() beyond the two cases below: its algorithm is complex
  enough that hand-verifying more cases with confidence was out of scope
  for this first pass.
- Error paths that call G_fatal_error() (an unknown algorithm name in
  AS_option_to_algorithm(), more than 10 classes in AS_class_equiprob(),
  an empty array in AS_class_apply_algorithm()): G_fatal_error() calls
  exit() by default, which would terminate the whole pytest process
  rather than raise something catchable. This matches how grass.pygrass
  itself avoids ever triggering G_fatal_error from Python, checking
  preconditions beforehand instead of relying on catching it.
"""

import math
from ctypes import byref, c_double, c_int

import pytest

from grass.lib import arraystats as libas
from grass.lib import gis as libgis

TEN_VALUES = list(range(1, 11))  # 1.0 .. 10.0, already sorted

# Three well-separated clusters, for AS_class_discont(). It looks for
# discontinuities, so clear gaps give it an unambiguous answer; on evenly
# spaced data such as TEN_VALUES the break it picks comes down to ~1e-17 of
# floating point noise and moves under -ffast-math.
CLUSTERED_VALUES = [1.0, 2.0, 3.0, 4.0, 20.0, 21.0, 22.0, 40.0, 41.0, 42.0]


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


def test_class_equiprob_reduces_classes_when_a_break_falls_outside_the_range() -> None:
    """A classbreak that lands outside [min, max] is dropped rather than
    returned out of range, and *nbreaks is written back to reflect it"""
    data = make_array([1.0] * 9 + [100.0])
    breaks = (c_double * 9)()
    nbreaks = c_int(9)
    ret = libas.AS_class_equiprob(data, 10, byref(nbreaks), breaks)
    assert ret == 1
    assert nbreaks.value == 6
    assert list(breaks)[: nbreaks.value] == pytest.approx(
        [
            3.3755050000000004,
            10.9,
            18.424495,
            26.47468,
            35.896114,
            48.96203499999999,
        ]
    )


def test_class_frequencies_counts_values_per_class() -> None:
    data = make_array(TEN_VALUES)
    breaks = make_array([3.25, 5.5, 7.75])
    frequencies = (c_int * 4)()
    ret = libas.AS_class_frequencies(data, len(TEN_VALUES), 3, breaks, frequencies)
    assert ret == 1
    assert list(frequencies) == [3, 2, 2, 3]


def test_class_discont_splits_between_clusters() -> None:
    """AS_class_discont() puts its breaks in the gaps between clusters

    The returned chi2 is deliberately not pinned to an exact value. It is a
    running minimum over floating point comparisons, and it changes between
    an ordinary build and one built with -ffast-math (measured: 0.2499...
    against 0.9349...) even though the breaks themselves stay put. The
    breaks are the useful output here, so those are what is asserted.
    """
    data = make_array(CLUSTERED_VALUES)
    breaks = (c_double * 2)()
    chi2 = libas.AS_class_discont(data, len(CLUSTERED_VALUES), 2, breaks)
    assert list(breaks) == pytest.approx([4.5, 39.5])
    # 1000 is the "found nothing" sentinel the algorithm starts from.
    assert 0 < chi2 < 1000


def test_class_discont_on_degenerate_input_returns_nan_breaks() -> None:
    """All-equal input leaves AS_class_discont() with no range to work with

    Standardizing by a zero range gives 0/0, so the breaks come back NaN
    while chi2 keeps the 1000 "found nothing" sentinel it started from.
    AS_class_apply_algorithm() only treats finfo == 0 as failure, so this
    passes straight through it and a caller receives NaN classbreaks with
    no error raised. Locked in as current behavior, not as a guarantee.

    Only the first break is asserted: the second is NaN in an ordinary
    build but 0 under -ffast-math.
    """
    data = make_array([7.0] * 5)
    breaks = (c_double * 2)()
    chi2 = libas.AS_class_discont(data, 5, 2, breaks)
    assert chi2 == 1000
    assert math.isnan(breaks[0])


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
    ],
)
def test_class_apply_algorithm_dispatches_by_constant(
    algorithm, nbreaks, expected_finfo, expected_breaks
) -> None:
    """AS_class_apply_algorithm() is a thin dispatcher to the AS_class_*()
    functions above, selected by the CLASS_* constant; each case here
    matches the corresponding AS_class_*() test above.

    nbreaks_inout is bound to a local rather than passed as an inline
    byref(c_int(nbreaks)) so its write-back can actually be asserted; none
    of these cases changes it (see
    test_class_equiprob_reduces_classes_when_a_break_falls_outside_the_range
    for a case that does). CLASS_DISCONT is dispatched in its own test
    below, since its return value cannot be pinned to an exact number.
    """
    data = make_array(TEN_VALUES)
    breaks = (c_double * nbreaks)()
    nbreaks_inout = c_int(nbreaks)
    finfo = libas.AS_class_apply_algorithm(
        algorithm, data, len(TEN_VALUES), byref(nbreaks_inout), breaks
    )
    assert finfo == pytest.approx(expected_finfo)
    assert nbreaks_inout.value == nbreaks
    assert list(breaks) == pytest.approx(expected_breaks)


def test_class_apply_algorithm_dispatches_to_discont() -> None:
    """Same dispatch check as above for CLASS_DISCONT, using clustered data
    and asserting only the breaks, for the reason given in
    test_class_discont_splits_between_clusters"""
    data = make_array(CLUSTERED_VALUES)
    breaks = (c_double * 2)()
    nbreaks_inout = c_int(2)
    finfo = libas.AS_class_apply_algorithm(
        libas.CLASS_DISCONT, data, len(CLUSTERED_VALUES), byref(nbreaks_inout), breaks
    )
    assert 0 < finfo < 1000
    assert nbreaks_inout.value == 2
    assert list(breaks) == pytest.approx([4.5, 39.5])


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
