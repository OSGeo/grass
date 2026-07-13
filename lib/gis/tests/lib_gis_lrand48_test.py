"""Regression tests for the library drand48-family PRNG

These tests pin the exact output sequence of G_lrand48, G_mrand48 and
G_drand48 for a set of fixed seeds. Any change to the generator that alters
the produced numbers (e.g. a future reimplementation of the internal state)
is caught here, which makes this a guard for output compatibility.

The reference values below were captured from the current implementation.
"""

import pytest

from grass.lib.gis import G_drand48, G_lrand48, G_mrand48, G_srand48

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
