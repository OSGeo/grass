# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of rand() in r.mapcalc and r3.mapcalc drawing from an exact layout

The expected values are computed with the library's random number layouts:
a row is a unit of an exact layout which draws columns * calls values, so
its stream is the part of the seed's sequence which it draws
when the rows are evaluated one after another. Each rand() call draws one
value per cell of the row, in the order in which the expressions are
evaluated. In 3D, the rows are numbered across the depths, so row r of
depth d is unit d * rows + r.
"""

from ctypes import byref
from io import StringIO

import numpy as np
import pytest

import grass.script.array as garray
from grass.lib.gis import (
    G_random_double,
    G_random_init_layout_exact,
    G_random_state_for_unit,
    struct_G_random_layout,
    struct_G_random_state,
)
from grass.tools import Tools

ROWS = 7
COLS = 5
REGION = {"n": ROWS, "s": 0, "e": COLS, "w": 0, "res": 1}


def row_values(seed, rows, cols, calls=1, depths=1):
    """Return the values of each row, one array per rand() call

    The arrays have one row per row of every depth, the depths one after
    another.
    """
    per_row = calls * cols
    layout = struct_G_random_layout()
    G_random_init_layout_exact(byref(layout), seed, depths * rows, per_row)
    state = struct_G_random_state()
    values = []
    for unit in range(depths * rows):
        G_random_state_for_unit(byref(state), byref(layout), unit)
        values.append([G_random_double(byref(state)) for i in range(per_row)])
    values = np.array(values)
    return [values[:, call * cols : (call + 1) * cols] for call in range(calls)]


def cell_values(values, low, high):
    """Return the integers which rand(low, high) makes of the values"""
    return low + (values * 2**32).astype(np.int64) % (high - low)


@pytest.mark.parametrize("nprocs", [1, 4])
def test_dcell_rows_continue_sequence(session_in_mapset, nprocs):
    """rand(0.0, 1.0) gives the values of the sequence row by row"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(expression="result = rand(0.0, 1.0)", seed=42, nprocs=nprocs)
    (expected,) = row_values(42, ROWS, COLS)
    result = garray.array("result", env=session_in_mapset.env)
    assert np.array_equal(result, expected)


@pytest.mark.parametrize("nprocs", [1, 4])
def test_cell_rows_continue_sequence(session_in_mapset, nprocs):
    """An integer rand() uses the top 32 bits of each value of the sequence"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(expression="result = rand(0, 1000)", seed=42, nprocs=nprocs)
    (values,) = row_values(42, ROWS, COLS)
    result = garray.array("result", env=session_in_mapset.env)
    assert np.array_equal(result, cell_values(values, 0, 1000))


@pytest.mark.parametrize("nprocs", [1, 4])
def test_fcell_rows_continue_sequence(session_in_mapset, nprocs):
    """A single-precision rand() rounds the values of the sequence"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(
        expression="result = rand(float(0), float(1))", seed=42, nprocs=nprocs
    )
    (values,) = row_values(42, ROWS, COLS)
    result = garray.array("result", env=session_in_mapset.env)
    assert np.array_equal(result, values.astype(np.float32))


@pytest.mark.parametrize("nprocs", [1, 4])
def test_rand_calls_in_order_of_expressions(session_in_mapset, nprocs):
    """Each rand() call of a row draws the next values of the sequence"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(
        expression="first = rand(0.0, 1.0)\nsecond = rand(0.0, 1.0)",
        seed=42,
        nprocs=nprocs,
    )
    first, second = row_values(42, ROWS, COLS, calls=2)
    env = session_in_mapset.env
    assert np.array_equal(garray.array("first", env=env), first)
    assert np.array_equal(garray.array("second", env=env), second)


@pytest.mark.parametrize("nprocs", [1, 4])
def test_rand_calls_in_order_of_file_expressions(session_in_mapset, nprocs):
    """The rand() calls of expressions read from a file draw in their order"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(
        file=StringIO(
            "first = rand(0.0, 1.0)\n"
            "second = rand(0, 1000)\n"
            "third = rand(float(0), float(1))\n"
        ),
        seed=42,
        nprocs=nprocs,
    )
    first, second, third = row_values(42, ROWS, COLS, calls=3)
    env = session_in_mapset.env
    assert np.array_equal(garray.array("first", env=env), first)
    assert np.array_equal(garray.array("second", env=env), cell_values(second, 0, 1000))
    assert np.array_equal(garray.array("third", env=env), third.astype(np.float32))


@pytest.mark.parametrize("nprocs", [1, 4])
def test_rand_calls_in_order_of_arguments(session_in_mapset, nprocs):
    """The rand() calls of one expression draw in the order of arguments"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(
        expression="result = rand(0.0, 1.0) + 2 * rand(0.0, 1.0)",
        seed=42,
        nprocs=nprocs,
    )
    first, second = row_values(42, ROWS, COLS, calls=2)
    result = garray.array("result", env=session_in_mapset.env)
    assert np.array_equal(result, first + 2 * second)


@pytest.mark.parametrize("nprocs", [1, 4])
def test_if_draws_for_both_branches(session_in_mapset, nprocs):
    """if() evaluates both branches, so both draw from the sequence"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(
        expression="result = if(rand(0, 2), rand(0.0, 1.0), rand(10.0, 11.0))",
        seed=42,
        nprocs=nprocs,
    )
    condition, then, otherwise = row_values(42, ROWS, COLS, calls=3)
    expected = np.where(
        cell_values(condition, 0, 2) != 0, then, 10.0 + otherwise * (11.0 - 10.0)
    )
    result = garray.array("result", env=session_in_mapset.env)
    assert np.array_equal(result, expected)


@pytest.mark.parametrize("nprocs", [1, 4])
def test_eval_variable_draws_once(session_in_mapset, nprocs):
    """A variable of eval() draws once per cell however often it is used"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(
        expression="result = eval(x = rand(0.0, 1.0), x + 2 * rand(0.0, 1.0) + x)",
        seed=42,
        nprocs=nprocs,
    )
    first, second = row_values(42, ROWS, COLS, calls=2)
    result = garray.array("result", env=session_in_mapset.env)
    assert np.array_equal(result, first + 2 * second + first)


@pytest.mark.parametrize("nprocs", [1, 4])
def test_r3_rows_continue_sequence(session_in_mapset, nprocs):
    """In 3D, the rows of each depth continue the sequence"""
    depths = 3
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION, t=depths, b=0, res3=1, tbres=1)
    tools.r3_mapcalc(
        expression="result = rand(0.0, 1.0) + 2 * rand(0.0, 1.0)",
        seed=42,
        nprocs=nprocs,
    )
    first, second = row_values(42, ROWS, COLS, calls=2, depths=depths)
    expected = (first + 2 * second).reshape(depths, ROWS, COLS)
    result = garray.array3d("result", env=session_in_mapset.env)
    assert np.array_equal(result, expected)


@pytest.mark.parametrize("nprocs", [2, 3, 4, 7])
def test_nprocs_gives_same_result(session_in_mapset, nprocs):
    """The result for a seed is the same as with one thread"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(n=50, s=0, e=30, w=0, res=1)
    for name, threads in [("serial", 1), ("parallel", nprocs)]:
        tools.r_mapcalc(
            expression=(
                f"{name}_a = rand(0.0, 1.0) + rand(-5, 5)\n"
                f"{name}_b = if(row() % 2, rand(1, 100), {name}_a)"
            ),
            seed=3,
            nprocs=threads,
        )
    env = session_in_mapset.env
    for suffix in ["a", "b"]:
        assert np.array_equal(
            garray.array(f"serial_{suffix}", env=env),
            garray.array(f"parallel_{suffix}", env=env),
        )


def test_same_seed_gives_same_result(session_in_mapset):
    """Two runs with the same seed give the same raster"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(expression="first = rand(0.0, 1.0)", seed=7, nprocs=4)
    tools.r_mapcalc(expression="second = rand(0.0, 1.0)", seed=7, nprocs=4)
    env = session_in_mapset.env
    assert np.array_equal(
        garray.array("first", env=env), garray.array("second", env=env)
    )


def test_different_seeds_give_different_results(session_in_mapset):
    """Two runs with different seeds differ in every cell"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**REGION)
    tools.r_mapcalc(expression="first = rand(0.0, 1.0)", seed=7)
    tools.r_mapcalc(expression="second = rand(0.0, 1.0)", seed=8)
    env = session_in_mapset.env
    assert np.all(garray.array("first", env=env) != garray.array("second", env=env))


@pytest.mark.parametrize("expression", ["rand(-3, 4)", "rand(4, -3)"])
def test_cell_range(session_in_mapset, expression):
    """Integer values lie between the bounds, the upper one excluded"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(n=20, s=0, e=20, w=0, res=1)
    tools.r_mapcalc(expression=f"result = {expression}", seed=1, nprocs=4)
    result = garray.array("result", env=session_in_mapset.env)
    assert set(np.unique(result)) == set(range(-3, 4))


@pytest.mark.parametrize("expression", ["rand(-1.0, 2.0)", "rand(2.0, -1.0)"])
def test_dcell_range(session_in_mapset, expression):
    """Floating-point values lie between the bounds, the upper one excluded"""
    tools = Tools(session=session_in_mapset)
    tools.g_region(n=20, s=0, e=20, w=0, res=1)
    tools.r_mapcalc(expression=f"result = {expression}", seed=1, nprocs=4)
    result = garray.array("result", env=session_in_mapset.env)
    assert result.min() >= -1
    assert result.max() < 2


@pytest.mark.parametrize("flags", [None, "s"])
def test_automatic_seed_is_the_reported_seed(session_in_mapset, flags):
    """An automatic seed gives the result of the seed written to the history"""
    env = session_in_mapset.env.copy()
    env["GRASS_RANDOM_SEED"] = "1234"
    tools = Tools(env=env)
    tools.g_region(**REGION)
    tools.r_mapcalc(expression="automatic = rand(0.0, 1.0)", flags=flags)
    tools.r_mapcalc(expression="given = rand(0.0, 1.0)", seed=1234)
    assert "random seed = 1234" in tools.r_info(map="automatic", flags="h").text
    assert np.array_equal(
        garray.array("automatic", env=env), garray.array("given", env=env)
    )


def test_seed_out_of_range(session_in_mapset):
    """A seed the generator cannot use is an error, not silently reduced"""
    tools = Tools(
        session=session_in_mapset, consistent_return_value=True, errors="ignore"
    )
    tools.g_region(**REGION)
    result = tools.r_mapcalc(expression="result = rand(0.0, 1.0)", seed=2**32)
    assert result.returncode == 1
    assert "4294967296" in result.stderr
    assert not tools.g_list(type="raster", pattern="result").text


@pytest.mark.parametrize("seed", ["12abc", "1.5", str(2**64)])
def test_invalid_seed(session_in_mapset, seed):
    """A seed with trailing characters or beyond a 64-bit integer is an error

    The parser accepts these for an integer option, since it reads only
    the leading digits, so r.mapcalc checks them itself.
    """
    tools = Tools(
        session=session_in_mapset, consistent_return_value=True, errors="ignore"
    )
    tools.g_region(**REGION)
    result = tools.r_mapcalc(expression="result = rand(0.0, 1.0)", seed=seed)
    assert result.returncode == 1
    assert f"Invalid random seed <{seed}>" in result.stderr
    assert not tools.g_list(type="raster", pattern="result").text
