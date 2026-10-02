# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Regression tests for the library drand48-family PRNG

These tests pin the exact output sequence of G_lrand48, G_mrand48 and
G_drand48 for a set of fixed seeds. Any change to the generator that alters
the produced numbers (e.g. a future reimplementation of the internal state)
is caught here, which makes this a guard for output compatibility.

The reference values below were captured from the current implementation.

The tests after those cover the generators a program owns, the
G_random_*() functions and their layouts. Their expectations are computed
with a closed-form jump written here, independent of the library's jump,
and the single sequence is cross-checked against the shared generator and
the reference values.
"""

import subprocess
import sys
import threading
from ctypes import byref

import pytest

from grass.lib.gis import (
    G_drand48,
    G_lrand48,
    G_mrand48,
    G_random_advance,
    G_random_double,
    G_random_init_layout,
    G_random_init_layout_bounded,
    G_random_init_layout_exact,
    G_random_layout_batches,
    G_random_layout_length,
    G_random_state_for_batch,
    G_random_state_for_unit,
    G_random_state_from_seed,
    G_srand48,
    struct_G_random_layout,
    struct_G_random_state,
)

# First ten outputs of each generator after G_srand48(seed). The seeds cover
# 0, 1, two ordinary values, and the largest 32-bit seed value. Matching the
# first output verifies the seeding and the second one the state advancement;
# the remaining outputs and seeds guard against errors that occur only for
# some state values (e.g., a mishandled overflow), so a few of each suffice.
# The drand48 values are compared exactly on purpose: the generator state is
# a 48-bit integer, the returned value is state / 2**48, and a double
# represents both without rounding. The literals below were written by
# Python's repr, which produces the shortest decimal that parses back to
# exactly the double the implementation returned.
REFERENCE = {
    0: {
        "lrand48": [
            366850414,
            1610402240,
            206956554,
            1869309841,
            1239749840,
            1687491058,
            1486475625,
            791919534,
            1876694714,
            1600079540,
        ],
        "mrand48": [
            733700828,
            -1074162815,
            413913109,
            -556347614,
            -1815467615,
            -919985179,
            -1322016045,
            1583839069,
            -541577867,
            -1094808216,
        ],
        "drand48": [
            0.17082803610628972,
            0.7499019804849638,
            0.09637165562356742,
            0.8704652270270756,
            0.5773035067951078,
            0.785799258839674,
            0.6921941534586402,
            0.36876626992042105,
            0.8739040768618089,
            0.745095098450065,
        ],
    },
    1: {
        "lrand48": [
            89400484,
            976015093,
            1792756325,
            721524505,
            1214379247,
            3794415,
            402845420,
            2126940991,
            1611680321,
            786566648,
        ],
        "mrand48": [
            178800969,
            1952030186,
            -709454646,
            1443049011,
            -1866208802,
            7588830,
            805690840,
            -41085314,
            -1071606654,
            1573133297,
        ],
        "drand48": [
            0.041630344771878214,
            0.45449244472862915,
            0.8348172181669149,
            0.33598603014520023,
            0.5654894035661364,
            0.001766912391744313,
            0.18758951699996018,
            0.9904340799376641,
            0.7504971332295192,
            0.36627363815273384,
        ],
    },
    42: {
        "lrand48": [
            1598855263,
            735945821,
            238553827,
            906966006,
            174184913,
            1839192415,
            1071163602,
            1028245859,
            1483508427,
            1792276465,
        ],
        "mrand48": [
            -1097256770,
            1471891643,
            477107655,
            1813932012,
            348369827,
            -616582465,
            2142327205,
            2056491719,
            -1327950441,
            -710414366,
        ],
        "drand48": [
            0.7445250000610066,
            0.342701478718908,
            0.11108528244416149,
            0.422338957988309,
            0.08111117117831057,
            0.856440708026625,
            0.4987994221940788,
            0.4788142906446282,
            0.6908124443056387,
            0.8345937659621541,
        ],
    },
    1337: {
        "lrand48": [
            930965776,
            1690826993,
            854889137,
            583640949,
            1679004699,
            1147941803,
            76869624,
            1156695387,
            1887252525,
            560069492,
        ],
        "mrand48": [
            1861931553,
            -913313310,
            1709778274,
            1167281899,
            -936957898,
            -1999083690,
            153739248,
            -1981576522,
            -520462246,
            1120138985,
        ],
        "drand48": [
            0.4335147219981117,
            0.7873526742655201,
            0.3980887760791454,
            0.2717789959596715,
            0.7818474896603966,
            0.5345520579576117,
            0.03579520820343518,
            0.5386282629743491,
            0.8788204404903901,
            0.260802680918232,
        ],
    },
    2147483647: {
        "lrand48": [
            1718042167,
            1171047564,
            1842382256,
            1943353352,
            191378610,
            149962230,
            1496364007,
            530639902,
            1067967284,
            1339850607,
        ],
        "mrand48": [
            -858882961,
            -1952872168,
            -610202784,
            -408260591,
            382757220,
            299924460,
            -1302239282,
            1061279804,
            2135934568,
            -1615266081,
        ],
        "drand48": [
            0.8000257274407012,
            0.5453115162412985,
            0.8579260930802199,
            0.904944423908951,
            0.08911761002407914,
            0.06983160528760379,
            0.6967987899173202,
            0.24709845990317802,
            0.4973110204940987,
            0.6239165587473963,
        ],
    },
}


@pytest.mark.parametrize("seed", sorted(REFERENCE))
def test_lrand48_sequence_matches_reference(seed):
    """G_lrand48 reproduces the reference sequence for a fixed seed."""
    expected = REFERENCE[seed]["lrand48"]
    G_srand48(seed)
    assert [G_lrand48() for _ in range(len(expected))] == expected


@pytest.mark.parametrize("seed", sorted(REFERENCE))
def test_mrand48_sequence_matches_reference(seed):
    """G_mrand48 reproduces the reference sequence for a fixed seed."""
    expected = REFERENCE[seed]["mrand48"]
    G_srand48(seed)
    assert [G_mrand48() for _ in range(len(expected))] == expected


@pytest.mark.parametrize("seed", sorted(REFERENCE))
def test_drand48_sequence_matches_reference(seed):
    """G_drand48 reproduces the reference sequence for a fixed seed."""
    expected = REFERENCE[seed]["drand48"]
    G_srand48(seed)
    assert [G_drand48() for _ in range(len(expected))] == expected


def test_srand48_is_reproducible():
    """Re-seeding restarts the same sequence."""
    G_srand48(1337)
    first = [G_lrand48() for _ in range(20)]
    G_srand48(1337)
    second = [G_lrand48() for _ in range(20)]
    assert first == second


# The generator constants, as in lrand48.c. Changing the generator changes
# every expectation below, so these tests are meant to fail when that
# happens.
LCG_A = 0x5DEECE66D
LCG_B = 0xB
LCG_MODULUS = 2**48

# The part of the cycle the layouts place their streams in: the multiplier
# has order 2^46, so states 2^46 apart give values differing by a constant.
SPAN = 2**46

# The most units a layout of the whole span takes.
WHOLE_SPAN_MAX_UNITS = 2**20

# Unit counts for the layout of the whole span: small counts, powers of two, which
# give an even stride before rounding, a hundred thousand, and the two
# largest counts allowed.
WHOLE_SPAN_UNITS = [
    2,
    3,
    4,
    6,
    64,
    4096,
    100000,
    WHOLE_SPAN_MAX_UNITS - 1,
    WHOLE_SPAN_MAX_UNITS,
]


def lcg_jump_reference(state, steps):
    """Generator state after the given number of steps, in closed form

    Steps compose to a^n * x + b * (a^n - 1) / (a - 1) modulo 2^48. The
    division is exact over the integers but a - 1 has no inverse modulo
    2^48, so a^n is computed modulo 2^48 * (a - 1), which keeps the
    quotient correct modulo 2^48. Unlike the library, which composes the
    affine map by repeated squaring, this takes the additive term in
    closed form and reuses none of the library's code.
    """
    power = pow(LCG_A, steps, LCG_MODULUS * (LCG_A - 1))
    return (power * state + LCG_B * ((power - 1) // (LCG_A - 1))) % LCG_MODULUS


def seed_state(seed):
    """Generator state which seeding with the given value produces"""
    return ((seed & 0xFFFFFFFF) << 16) | 0x330E


def reference_states(seed, offset, n):
    """States of draws offset + 1 to offset + n after the seed, in closed form"""
    start = lcg_jump_reference(seed_state(seed), offset)
    return [lcg_jump_reference(start, i) for i in range(1, n + 1)]


def draw_states(state, n):
    """Draw n values from a state and return the generator states behind them

    A value is its state divided by 2^48, which a double holds exactly, so
    the multiplication recovers the state without rounding.
    """
    return [int(G_random_double(byref(state)) * LCG_MODULUS) for _ in range(n)]


def seeded_states(seed, n):
    """States of the first n draws of the single sequence of a seed"""
    state = struct_G_random_state()
    G_random_state_from_seed(byref(state), seed)
    return draw_states(state, n)


def exact_layout(seed, units, draws):
    layout = struct_G_random_layout()
    G_random_init_layout_exact(byref(layout), seed, units, draws)
    return layout


def bounded_layout(seed, units, max_draws):
    layout = struct_G_random_layout()
    G_random_init_layout_bounded(byref(layout), seed, units, max_draws)
    return layout


def whole_span_layout(seed, units):
    layout = struct_G_random_layout()
    G_random_init_layout(byref(layout), seed, units)
    return layout


def batch_distance(units, stride):
    """Draws from the start of one batch to the next: units * stride, made odd"""
    return (units * stride) | 1


def batches_that_fit(units, stride):
    """Batches whose last stream ends within the span"""
    draws = units * stride
    if draws > SPAN:
        return 0
    return (SPAN - draws) // batch_distance(units, stride) + 1


def unit_states(layout, unit, n, batch=0):
    """States of the first n draws of a unit in a batch of a layout"""
    state = struct_G_random_state()
    G_random_state_for_batch(byref(state), byref(layout), batch, unit)
    return draw_states(state, n)


def whole_span_stride(units):
    """The whole-span stride: the span divided by the parts, rounded down to odd.

    The parts are the units, or one more when the units are even, so that
    the unit halfway along does not start 2^45 draws after unit 0.
    """
    parts = units | 1
    stride = SPAN // parts
    return stride - 1 if parts > 1 and stride % 2 == 0 else stride


def constant_shift(first, second, max_lag=4):
    """Lag at which second is first plus a constant, or None if there is none.

    A constant difference at some lag means the two sequences of states are
    one sequence shifted in time and in value, which is what states a
    multiple of 2^46 apart produce on this generator.
    """
    n = len(first)
    for lag in range(-max_lag, max_lag + 1):
        pairs = [(first[i], second[i + lag]) for i in range(n) if 0 <= i + lag < n]
        if len({(b - a) % LCG_MODULUS for a, b in pairs}) == 1:
            return lag
    return None


# Runs calls given as "function number..." in order, on one state and one
# layout, so that a test can check which call ends in a fatal error.
CALLS_SCRIPT = """
import sys
from ctypes import byref

from grass.lib import gis

state = gis.struct_G_random_state()
layout = gis.struct_G_random_layout()
gis.G_random_state_from_seed(byref(state), 1337)
for call in sys.argv[1:]:
    name, *numbers = call.split()
    function = getattr(gis, name)
    numbers = [int(number) for number in numbers]
    if name.startswith("G_random_init_layout"):
        function(byref(layout), *numbers)
    elif name.startswith("G_random_state_for"):
        function(byref(state), byref(layout), *numbers)
    else:
        function(byref(state), *numbers)
print("all calls returned")
"""


def run_calls(session, tmp_path, *calls):
    """Run the calls in a subprocess and return the completed process.

    A fatal error exits the calling process, so the calls run in a
    subprocess, and with a session environment because without GISBASE the
    error message is not printed.
    """
    script = tmp_path / "calls.py"
    script.write_text(CALLS_SCRIPT, encoding="utf-8")
    return subprocess.run(
        [sys.executable, str(script), *calls],
        env=session.env,
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )


def assert_fatal(result, message):
    """Check that the calls ended in a fatal error containing the message.

    The library wraps long messages across lines, so whitespace is
    normalized before the comparison.
    """
    assert result.returncode != 0, result.stdout
    assert "all calls returned" not in result.stdout
    assert message in " ".join(result.stderr.split()), result.stderr


def test_lcg_jump_reference_matches_stepping():
    """The closed form agrees with the generator drawn step by step.

    This ties the reference used below to the shared generator, whose
    sequences are pinned in REFERENCE.
    """
    G_srand48(1337)
    stepped = [int(G_drand48() * LCG_MODULUS) for _ in range(1000)]
    assert reference_states(1337, 0, 1000) == stepped
    assert [
        state / LCG_MODULUS for state in reference_states(1337, 0, 10)
    ] == REFERENCE[1337]["drand48"]


@pytest.mark.parametrize("seed", [0, 1, 42, 1337, 2147483647, -1, 4294967295])
def test_random_seed_matches_shared_generator(seed):
    """G_random_state_from_seed() gives the sequence G_srand48() and G_drand48() give.

    This is what lets code switch from the shared generator to one of its
    own without changing its single-threaded results.
    """
    G_srand48(seed)
    shared = [G_drand48() for _ in range(100)]
    state = struct_G_random_state()
    G_random_state_from_seed(byref(state), seed)
    assert [G_random_double(byref(state)) for _ in range(100)] == shared
    if seed in REFERENCE:
        assert shared[:10] == REFERENCE[seed]["drand48"]


@pytest.mark.parametrize(
    ("negative", "equivalent"), [(-1, 4294967295), (-2147483648, 2147483648)]
)
def test_random_negative_seed_is_its_32_bit_value(negative, equivalent):
    """A negative seed gives the sequence of its two's complement 32-bit value."""
    assert seeded_states(negative, 20) == seeded_states(equivalent, 20)


@pytest.mark.parametrize("seed", [-(2**31), 2**32 - 1])
def test_random_seed_accepts_the_range_boundaries(seed):
    """The seed range is inclusive at both ends."""
    assert seeded_states(seed, 5) == reference_states(seed, 0, 5)


@pytest.mark.parametrize("seed", [2**32, -(2**31) - 1])
@pytest.mark.parametrize(
    "call",
    [
        "G_random_state_from_seed {}",
        "G_random_init_layout_exact {} 5 7",
        "G_random_init_layout_bounded {} 5 7",
        "G_random_init_layout {} 5",
    ],
    ids=["seed", "exact", "bounded", "whole_span"],
)
def test_random_seed_outside_range_is_fatal(
    xy_session_for_module, tmp_path, seed, call
):
    """A seed outside -2^31 to 2^32 - 1 is a fatal error, not used modulo 2^32."""
    result = run_calls(xy_session_for_module, tmp_path, call.format(seed))
    assert_fatal(result, f"seed {seed} is outside the range")


@pytest.mark.parametrize("seed", [0, 1337, -1])
def test_exact_layout_units_follow_the_single_sequence(seed):
    """Units of an exact layout draw one sequence in consecutive streams.

    Unit u's first value is draw u * 7 + 1 of the single sequence, and the
    last draw of unit u is followed by the first draw of unit u + 1, so
    the units together reproduce a serial run.
    """
    units, draws = 5, 7
    layout = exact_layout(seed, units, draws)
    single = seeded_states(seed, units * draws + 1)
    for unit in range(units):
        states = unit_states(layout, unit, draws + 1)
        assert states == single[unit * draws : (unit + 1) * draws + 1], unit


@pytest.mark.parametrize(
    ("units", "draws"),
    [(5, 7), (1, 1), (1000, 100), (4096, 2**34), (2**23, 2**23), (2**23, 2**23 + 1)],
)
def test_exact_layout_batches_and_length(units, draws):
    """The batches that fit are those whose last stream ends within the span.

    A batch of exactly 2^46 draws fits once. The last pair's product exceeds
    2^46, so no batch fits and batches() is 0. The length is the number of
    draws exactly.
    """
    layout = exact_layout(1337, units, draws)
    assert G_random_layout_batches(byref(layout)) == batches_that_fit(units, draws)
    assert G_random_layout_length(byref(layout)) == draws


@pytest.mark.parametrize(("units", "draws", "distance"), [(5, 7, 35), (4, 6, 25)])
def test_exact_layout_batches_start_an_odd_distance_apart(units, draws, distance):
    """Batches start units * draws apart, or one more when that is even.

    Unit u of batch b starts b * distance + u * draws into the single
    sequence.
    """
    assert batch_distance(units, draws) == distance
    layout = exact_layout(1337, units, draws)
    for batch, unit in [(0, 2), (1, 0), (4, 2)]:
        assert unit_states(layout, unit, 3, batch=batch) == reference_states(
            1337, batch * distance + unit * draws, 3
        ), (batch, unit)


def test_exact_layout_which_does_not_fit_allows_batch_zero(
    xy_session_for_module, tmp_path
):
    """With no batch fitting, batch 0 is still allowed but batch 1 is fatal.

    A tool which warned that its layout does not fit into the span may
    still use it, as earlier versions did.
    """
    units, draws = 2**23, 2**23 + 1
    layout = exact_layout(1337, units, draws)
    assert G_random_layout_batches(byref(layout)) == 0
    unit = units - 1
    assert unit_states(layout, unit, 3) == reference_states(1337, unit * draws, 3)
    result = run_calls(
        xy_session_for_module,
        tmp_path,
        f"G_random_init_layout_exact 1337 {units} {draws}",
        "G_random_state_for_batch 1 0",
    )
    assert_fatal(result, "batch 1 is out of range")


def test_exact_layout_batch_past_the_batches_that_fit_is_fatal(
    xy_session_for_module, tmp_path
):
    """A batch beyond those that fit is a fatal error naming both numbers."""
    units, draws = 4096, 2**34
    batches = batches_that_fit(units, draws)
    assert batches == 1
    ok = run_calls(
        xy_session_for_module,
        tmp_path,
        f"G_random_init_layout_exact 1337 {units} {draws}",
        f"G_random_state_for_batch {batches - 1} {units - 1}",
    )
    assert ok.returncode == 0, ok.stderr
    result = run_calls(
        xy_session_for_module,
        tmp_path,
        f"G_random_init_layout_exact 1337 {units} {draws}",
        f"G_random_state_for_batch {batches} 0",
    )
    assert_fatal(result, f"batch {batches} is out of range ({batches} batch fits")


@pytest.mark.parametrize(
    ("call", "message"),
    [
        ("G_random_init_layout_exact 1337 0 7", "must be positive, not 0"),
        ("G_random_init_layout_exact 1337 -5 7", "must be positive, not -5"),
        ("G_random_init_layout_exact 1337 5 0", "must be positive, not 0"),
        ("G_random_init_layout_exact 1337 5 -7", "must be positive, not -7"),
        ("G_random_init_layout_exact 1337 4294967296 4294967296", "too large"),
        ("G_random_init_layout_bounded 1337 5 0", "must be positive, not 0"),
        ("G_random_init_layout_bounded 1337 0 7", "must be positive, not 0"),
        ("G_random_init_layout_bounded 1337 4294967296 4294967296", "too large"),
        ("G_random_init_layout 1337 0", "must be positive, not 0"),
    ],
)
def test_layout_with_impossible_counts_is_fatal(
    xy_session_for_module, tmp_path, call, message
):
    """Counts which are not positive or whose product overflows are fatal."""
    assert_fatal(run_calls(xy_session_for_module, tmp_path, call), message)


@pytest.mark.parametrize(("bound", "stride"), [(100, 101), (101, 101), (1, 1), (2, 3)])
def test_bounded_layout_rounds_an_even_bound_up_to_odd(bound, stride):
    """An even bound becomes the next odd number, an odd bound stays."""
    layout = bounded_layout(1337, 1000, bound)
    assert G_random_layout_length(byref(layout)) == stride
    assert G_random_layout_batches(byref(layout)) == batches_that_fit(1000, stride)


@pytest.mark.parametrize("seed", [0, 1337, -1])
def test_bounded_layout_unit_starts(seed):
    """Unit u starts u * stride draws after the seed or after the start of batch 1."""
    units, stride = 1000, 101
    layout = bounded_layout(seed, units, 100)
    for unit in (0, 1, 2, 999):
        assert unit_states(layout, unit, 3) == reference_states(
            seed, unit * stride, 3
        ), unit
        assert unit_states(layout, unit, 3, batch=1) == reference_states(
            seed, batch_distance(units, stride) + unit * stride, 3
        ), unit


@pytest.mark.parametrize("units", WHOLE_SPAN_UNITS)
def test_whole_span_layout_stride(units):
    """The stride is the span divided by the parts, rounded down to odd."""
    layout = whole_span_layout(1337, units)
    stride = G_random_layout_length(byref(layout))
    assert stride == whole_span_stride(units)
    assert stride % 2 == 1
    assert G_random_layout_batches(byref(layout)) == 1


def test_whole_span_layout_of_one_unit_keeps_the_span():
    """A single unit keeps the whole span of 2^46 draws.

    Its stream is the single sequence of the seed.
    """
    layout = whole_span_layout(1337, 1)
    assert G_random_layout_length(byref(layout)) == SPAN
    assert G_random_layout_batches(byref(layout)) == 1
    assert unit_states(layout, 0, 20) == seeded_states(1337, 20)


@pytest.mark.parametrize("seed", [0, 1337, -1])
@pytest.mark.parametrize("units", [4, 4096, 100000, WHOLE_SPAN_MAX_UNITS])
def test_whole_span_layout_unit_starts(seed, units):
    """Unit u starts u * stride draws after the seed.

    The first, a middle and the last unit are checked; the last one also
    shows that the highest unit is accepted.
    """
    layout = whole_span_layout(seed, units)
    stride = whole_span_stride(units)
    for unit in sorted({0, 1, units // 2, units - 1}):
        assert unit_states(layout, unit, 2) == reference_states(
            seed, unit * stride, 2
        ), unit


def test_whole_span_layout_batch_one_is_fatal(xy_session_for_module, tmp_path):
    """A layout of the whole span holds a single batch."""
    result = run_calls(
        xy_session_for_module,
        tmp_path,
        "G_random_init_layout 1337 4",
        "G_random_state_for_batch 1 0",
    )
    assert_fatal(result, "a layout of the whole span holds a single batch")


@pytest.mark.parametrize("units", [WHOLE_SPAN_MAX_UNITS + 1, 2**46 + 1, 2**62])
def test_whole_span_layout_with_too_many_units_is_fatal(
    xy_session_for_module, tmp_path, units
):
    """A layout of the whole span takes at most 2^20 units."""
    result = run_calls(
        xy_session_for_module, tmp_path, f"G_random_init_layout 1337 {units}"
    )
    assert_fatal(result, f"at most {WHOLE_SPAN_MAX_UNITS} units, not {units}")


def test_whole_span_layout_of_the_most_units():
    """2^20 units, the most allowed, get a stride of 67,108,799 draws.

    The span divided by 2^20 + 1 parts is just over 67,108,800, which is
    even, so the stride is rounded down to the odd 67,108,799. The last
    unit starts where the formula says.
    """
    layout = whole_span_layout(1337, WHOLE_SPAN_MAX_UNITS)
    assert G_random_layout_length(byref(layout)) == 67108799
    assert whole_span_stride(WHOLE_SPAN_MAX_UNITS) == 67108799
    last = WHOLE_SPAN_MAX_UNITS - 1
    assert unit_states(layout, last, 3) == reference_states(1337, last * 67108799, 3)


def nearest_unit_distance(units, distance):
    """Distance of the unit start nearest to a distance, in strides.

    Units start a multiple of the stride apart, so this is also the
    nearest any two units come to being that distance apart.
    """
    stride = whole_span_stride(units)
    nearest = distance // stride
    return min(
        abs(k * stride - distance) / stride
        for k in (nearest, nearest + 1)
        if 1 <= k < units
    )


def test_whole_span_layout_keeps_units_off_the_shifted_distances():
    """No unit starts near 2^45 or 2^44 draws after another unit.

    At those distances the values of two positions differ by two or four
    constants in turn. Dividing the span by an even number of parts would
    put the unit halfway along 2^45 draws after unit 0 and the unit a
    quarter of the way along 2^44 draws after it. With an odd number of
    parts, the nearest units are half and a quarter of a stride away from
    those distances, less the draws lost by rounding the stride down,
    which accumulate along the span. For every allowed number of units,
    the nearest unit is at least 0.48 of a stride from 2^45 and 0.24 of a
    stride from 2^44. This checks the rule; test_whole_span_layout_stride ties
    the rule to the library.
    """
    closest_to_half = min(
        nearest_unit_distance(units, 2**45)
        for units in range(2, WHOLE_SPAN_MAX_UNITS + 1)
    )
    closest_to_quarter = min(
        nearest_unit_distance(units, 2**44)
        for units in range(4, WHOLE_SPAN_MAX_UNITS + 1)
    )
    assert closest_to_half >= 0.48
    assert closest_to_quarter >= 0.24


def layouts_for_span_check():
    """Layouts with at least one batch fitting, as (layout, units, stride)

    The stride is computed here from the documented rules, not read from the
    layout.
    """
    cases = [
        (whole_span_layout(1337, units), units, whole_span_stride(units))
        for units in [1, *WHOLE_SPAN_UNITS]
    ]
    cases += [
        (exact_layout(1337, units, draws), units, draws)
        for units, draws in [(5, 7), (4096, 2**34), (2**23, 2**23)]
    ]
    cases += [
        (bounded_layout(1337, units, bound), units, bound | 1)
        for units, bound in [(1000, 100), (4096, 2**34 - 2), (2**23, 2**23 - 2)]
    ]
    return cases


@pytest.mark.parametrize(("layout", "units", "stride"), layouts_for_span_check())
def test_layout_lies_within_the_span(layout, units, stride):
    """The last stream of the last batch that fits ends within the span.

    Every position in a stream is then less than 2^46 draws from every
    other, so no two streams can be 2^46, 2^47 or 3 * 2^46 draws apart,
    the distances at which this generator repeats its values plus a
    constant.
    """
    batches = G_random_layout_batches(byref(layout))
    length = G_random_layout_length(byref(layout))
    assert length == stride
    assert batches >= 1
    last_start = (batches - 1) * batch_distance(units, stride) + (units - 1) * stride
    assert last_start + length <= SPAN


def test_whole_span_layout_units_have_no_constant_shift():
    """Two units of a layout of the whole span are not one sequence shifted in value.

    The first and the last unit of the largest layout of the whole span start less
    than 2^46 draws apart, so no lag relates their values by a constant.
    """
    layout = whole_span_layout(1337, WHOLE_SPAN_MAX_UNITS)
    first = unit_states(layout, 0, 100)
    second = unit_states(layout, WHOLE_SPAN_MAX_UNITS - 1, 100)
    assert constant_shift(first, second) is None


def test_states_half_a_span_apart_alternate_between_two_shifts():
    """States 2^45 apart differ by one of two constants, alternately.

    This level of relation lies within the span, and the library makes no
    promise about it. At draw n, the difference of two states d draws
    apart is the multiplier to the power n times the difference at the
    start, which for d = 2^45 is 2^45 times an odd number. The multiplier
    is 5 modulo 8, so its powers alternate between 1 and 5 modulo 8, and
    the difference alternates between two values 2^47 apart. For seed 42
    the values differ by 0.625 and 0.125 in turn. This is documented here,
    not claimed away.
    """
    plain = reference_states(42, 0, 50)
    advanced = reference_states(42, 2**45, 50)
    shifts = [
        ((a - p) % LCG_MODULUS) / LCG_MODULUS
        for p, a in zip(plain, advanced, strict=True)
    ]
    assert set(shifts[0::2]) == {0.625}
    assert set(shifts[1::2]) == {0.125}
    state = struct_G_random_state()
    G_random_state_from_seed(byref(state), 42)
    G_random_advance(byref(state), 2**45)
    assert draw_states(state, 50) == advanced


def low_bits_relation(first, second):
    """Low bits in which the differences of two sequences of states are constant"""
    differences = [(b - a) % LCG_MODULUS for a, b in zip(first, second, strict=True)]
    bits = 0
    while bits < 48 and len({d % 2 ** (bits + 1) for d in differences}) == 1:
        bits += 1
    return bits


def test_whole_span_layout_units_two_strides_apart_relate_three_low_bits():
    """Units 2 * stride apart, stride odd, share only the low 3 bits of a shift.

    A distance of 2^k times an odd number makes the differences of the
    two units' states constant in their low k + 2 bits; above those bits
    the difference varies from draw to draw.
    """
    layout = whole_span_layout(1337, 4)
    assert whole_span_stride(4) % 2 == 1
    first = unit_states(layout, 0, 200)
    second = unit_states(layout, 2, 200)
    assert low_bits_relation(first, second) == 3


def test_bounded_layout_even_bound_relates_three_low_bits_too():
    """The stride of an even bound is rounded to odd, so units two apart relate 3 bits.

    Without the rounding, a stride of 100 = 4 * 25 would make units two
    apart 8 * 25 draws apart and relate 5 low bits.
    """
    layout = bounded_layout(1337, 1000, 100)
    first = unit_states(layout, 0, 200)
    second = unit_states(layout, 2, 200)
    assert low_bits_relation(first, second) == 3
    unrounded = reference_states(1337, 200, 200)
    assert low_bits_relation(first, unrounded) == 5


def advanced_states(seed, draws, n):
    state = struct_G_random_state()
    G_random_state_from_seed(byref(state), seed)
    G_random_advance(byref(state), draws)
    return draw_states(state, n)


def test_random_advance_equals_drawing():
    """Advancing gives what drawing and discarding gives; 0 is a no-op."""
    assert advanced_states(1337, 60, 40) == seeded_states(1337, 100)[60:]
    assert advanced_states(1337, 0, 10) == seeded_states(1337, 10)


def test_random_advance_composes():
    """Two advances are one advance by their sum."""
    state = struct_G_random_state()
    G_random_state_from_seed(byref(state), 1337)
    G_random_advance(byref(state), 25)
    G_random_advance(byref(state), 35)
    assert draw_states(state, 10) == advanced_states(1337, 60, 10)


def test_random_advance_beyond_the_span():
    """Advancing past the span gives the states the closed form gives."""
    draws = SPAN + 12345
    assert advanced_states(1337, draws, 10) == reference_states(1337, draws, 10)


def test_random_advance_by_the_cycle_returns_to_the_state():
    """After 2^48 draws, the generator's cycle, the state is the same again."""
    assert advanced_states(1337, LCG_MODULUS, 10) == seeded_states(1337, 10)


def test_random_advance_by_the_span_shifts_by_a_quarter():
    """After 2^46 draws the generator repeats its values plus one quarter."""
    plain = seeded_states(1337, 20)
    shifted = advanced_states(1337, SPAN, 20)
    assert shifted == reference_states(1337, SPAN, 20)
    assert {(s - p) % LCG_MODULUS for p, s in zip(plain, shifted, strict=True)} == {
        LCG_MODULUS // 4
    }


def test_random_advance_negative_is_fatal(xy_session_for_module, tmp_path):
    """Advancing by a negative number of draws is a fatal error."""
    result = run_calls(xy_session_for_module, tmp_path, "G_random_advance -1")
    assert_fatal(result, "by -1 draws")


@pytest.mark.parametrize(
    ("layout_call", "unit", "message"),
    [
        ("G_random_init_layout 1337 4", 4, "unit 4 is out of range"),
        ("G_random_init_layout 1337 4", -1, "unit -1 is out of range"),
        ("G_random_init_layout_exact 1337 5 7", 5, "unit 5 is out of range"),
        ("G_random_init_layout_bounded 1337 5 7", -1, "unit -1 is out of range"),
    ],
)
def test_random_state_for_unit_out_of_range_is_fatal(
    xy_session_for_module, tmp_path, layout_call, unit, message
):
    """A unit outside 0 to units - 1 is a fatal error."""
    result = run_calls(
        xy_session_for_module,
        tmp_path,
        layout_call,
        f"G_random_state_for_unit {unit}",
    )
    assert_fatal(result, message)


def test_random_state_for_batch_negative_batch_is_fatal(
    xy_session_for_module, tmp_path
):
    """A negative batch is a fatal error."""
    result = run_calls(
        xy_session_for_module,
        tmp_path,
        "G_random_init_layout_exact 1337 5 7",
        "G_random_state_for_batch -1 0",
    )
    assert_fatal(result, "batch -1 is out of range")


def test_random_state_for_unit_is_batch_zero():
    """G_random_state_for_unit() gives batch 0 of G_random_state_for_batch()."""
    layout = bounded_layout(1337, 1000, 100)
    state = struct_G_random_state()
    G_random_state_for_unit(byref(state), byref(layout), 7)
    assert draw_states(state, 10) == unit_states(layout, 7, 10)


GENERATE_SEED_SCRIPT = """
from grass.lib.gis import G_drand48, G_random_generate_seed, G_srand48_auto

print(G_random_generate_seed())
print(G_srand48_auto())
print(" ".join(str(int(G_drand48() * 2**48)) for _ in range(10)))
"""


def generate_seed(session, tmp_path, **variables):
    """Run G_random_generate_seed() and G_srand48_auto() in a subprocess.

    The variables replace GRASS_RANDOM_SEED and SOURCE_DATE_EPOCH, which
    are unset unless given. A warning or fatal error is written by the
    library to the process's standard error and needs a session to be
    printed, and a fatal error exits the process.
    """
    env = dict(session.env)
    env.pop("GRASS_RANDOM_SEED", None)
    env.pop("SOURCE_DATE_EPOCH", None)
    env.update(variables)
    script = tmp_path / "generate_seed.py"
    script.write_text(GENERATE_SEED_SCRIPT, encoding="utf-8")
    return subprocess.run(
        [sys.executable, str(script)],
        env=env,
        capture_output=True,
        text=True,
        timeout=60,
        check=False,
    )


def test_random_generate_seed_is_in_range_without_environment(
    xy_session_for_module, tmp_path
):
    """A seed from the time and process ID is one the generators accept."""
    result = generate_seed(xy_session_for_module, tmp_path)
    assert result.returncode == 0, result.stderr
    generated, automatic = (int(value) for value in result.stdout.split()[:2])
    assert 0 <= generated < 2**32
    assert 0 <= automatic < 2**32


def test_random_generate_seed_reads_environment(xy_session_for_module, tmp_path):
    """GRASS_RANDOM_SEED takes precedence over SOURCE_DATE_EPOCH."""
    result = generate_seed(
        xy_session_for_module,
        tmp_path,
        GRASS_RANDOM_SEED="1337",
        SOURCE_DATE_EPOCH="42",
    )
    assert result.returncode == 0, result.stderr
    assert result.stdout.split()[0] == "1337"
    result = generate_seed(xy_session_for_module, tmp_path, SOURCE_DATE_EPOCH="42")
    assert result.returncode == 0, result.stderr
    assert result.stdout.split()[0] == "42"


def test_srand48_auto_uses_the_generated_seed(xy_session_for_module, tmp_path):
    """The shared generator's automatic seed is the generated seed.

    The returned value seeds a generator of the program's own to the
    sequence the shared generator then produces.
    """
    result = generate_seed(xy_session_for_module, tmp_path, GRASS_RANDOM_SEED="1337")
    assert result.returncode == 0, result.stderr
    lines = result.stdout.splitlines()
    assert lines[:2] == ["1337", "1337"]
    assert [int(state) for state in lines[2].split()] == seeded_states(1337, 10)


@pytest.mark.parametrize("name", ["GRASS_RANDOM_SEED", "SOURCE_DATE_EPOCH"])
def test_random_generate_seed_reduces_large_environment_value_with_a_warning(
    xy_session_for_module, tmp_path, name
):
    """A value past 32 bits is reduced to its low 32 bits with a warning.

    Both the generated seed and the shared generator's automatic seed are
    the reduced value.
    """
    result = generate_seed(xy_session_for_module, tmp_path, **{name: "5000000000"})
    assert result.returncode == 0, result.stderr
    reduced = str(5000000000 % 2**32)
    assert result.stdout.split()[:2] == [reduced, reduced]
    message = " ".join(result.stderr.split())
    assert f"5000000000 from {name}" in message
    assert "low 32 bits" in message


@pytest.mark.parametrize("value", ["-5", "-2147483648", "4294967295"])
def test_random_generate_seed_keeps_value_in_range_without_warning(
    xy_session_for_module, tmp_path, value
):
    """A value from -2^31 to 2^32 - 1 is the seed as it is, also when negative."""
    result = generate_seed(xy_session_for_module, tmp_path, GRASS_RANDOM_SEED=value)
    assert result.returncode == 0, result.stderr
    assert result.stdout.split()[0] == value
    assert "WARNING" not in result.stderr


def test_random_generate_seed_skips_empty_variable(xy_session_for_module, tmp_path):
    """An empty GRASS_RANDOM_SEED counts as unset, so SOURCE_DATE_EPOCH is used."""
    result = generate_seed(
        xy_session_for_module, tmp_path, GRASS_RANDOM_SEED="", SOURCE_DATE_EPOCH="42"
    )
    assert result.returncode == 0, result.stderr
    assert result.stdout.split()[:2] == ["42", "42"]


@pytest.mark.parametrize(
    ("value", "message"),
    [
        ("abc", "is not an integer"),
        ("1e9", "is not an integer"),
        ("12abc", "is not an integer"),
        ("99999999999999999999", "is too large in magnitude"),
        ("-99999999999999999999", "is too large in magnitude"),
    ],
)
def test_random_generate_seed_rejects_invalid_value(
    xy_session_for_module, tmp_path, value, message
):
    """A value which is not an integer or overflows is a fatal error naming it."""
    result = generate_seed(xy_session_for_module, tmp_path, GRASS_RANDOM_SEED=value)
    assert result.returncode != 0
    assert result.stdout == ""
    stderr = " ".join(result.stderr.split())
    assert f"{value} from GRASS_RANDOM_SEED {message}" in stderr


def test_random_state_is_independent_of_shared_generator():
    """Drawing from the shared generator between draws changes nothing."""
    layout = whole_span_layout(1337, 4096)
    expected = unit_states(layout, 5, 10)
    state = struct_G_random_state()
    G_random_state_for_unit(byref(state), byref(layout), 5)
    G_srand48(99)
    interleaved = []
    for _ in range(10):
        G_lrand48()
        interleaved.extend(draw_states(state, 1))
    assert interleaved == expected


def test_random_states_are_unaffected_by_threading():
    """Each thread drawing from its own state gets the serial values.

    ctypes releases the GIL around each call, so the threads do run the C
    code concurrently.
    """
    units = 4
    n = 5000
    layout = whole_span_layout(1337, units)
    serial = [unit_states(layout, unit, n) for unit in range(units)]
    threaded = [None] * units

    def worker(unit):
        threaded[unit] = unit_states(layout, unit, n)

    threads = [threading.Thread(target=worker, args=(unit,)) for unit in range(units)]
    for thread in threads:
        thread.start()
    for thread in threads:
        thread.join()
    assert threaded == serial


# First ten values of unit 3 with seed 1337, computed by the closed form at
# 3 * 17175675903 draws for a layout of the whole span of 4096 units (4097
# parts) and at 3 * 101 draws for a bounded layout of 1000 units with bound
# 100. A change of the engine or of the layouts changes these.
PINNED = {
    "whole_span": [
        0.39430735545757045,
        0.6290045178804426,
        0.714269874034521,
        0.5881407609385896,
        0.7378029873502499,
        0.712116402550361,
        0.027046777408891387,
        0.6296825457039468,
        0.13697898780445428,
        0.13722939092471265,
    ],
    "bounded": [
        0.6813692756324983,
        0.069333342334577,
        0.21082745238814482,
        0.03296383528511271,
        0.4499312354256588,
        0.5150939051932433,
        0.6799364535715284,
        0.4718208970400859,
        0.8985155631446773,
        0.6221841681079674,
    ],
}


@pytest.mark.parametrize(
    ("kind", "layout", "offset"),
    [
        ("whole_span", whole_span_layout(1337, 4096), 3 * 17175675903),
        ("bounded", bounded_layout(1337, 1000, 100), 3 * 101),
    ],
    ids=["whole_span", "bounded"],
)
def test_layout_unit_values_are_pinned(kind, layout, offset):
    """Unit 3's first ten values are fixed for a given seed and layout."""
    expected = PINNED[kind]
    assert [
        state / LCG_MODULUS for state in reference_states(1337, offset, 10)
    ] == expected
    state = struct_G_random_state()
    G_random_state_for_unit(byref(state), byref(layout), 3)
    assert [G_random_double(byref(state)) for _ in range(10)] == expected
