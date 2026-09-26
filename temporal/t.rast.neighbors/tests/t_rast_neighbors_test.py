# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of t.rast.neighbors"""

import pytest

from grass.tools import Tools


def registered_maps(tools, dataset):
    """Return the names of the maps registered in a dataset."""
    data = tools.t_rast_list(input=dataset, columns="name", format="json").json["data"]
    return [row["name"] for row in data]


def count_history_commands(tools, dataset):
    """Count the t.rast.neighbors commands in the history of a dataset."""
    history = tools.t_info(input=dataset, flags="h").text
    return sum(line.startswith("t.rast.neighbors") for line in history.splitlines())


@pytest.mark.parametrize(
    ("method", "expected"),
    [("average", [2, 12]), ("maximum", [3, 13]), ("minimum", [1, 11])],
)
def test_method(session_with_strds, method, expected):
    """The method is applied to each map, also when processing in parallel.

    A 5x5 neighborhood covers the whole 3x3 region from every cell, so all
    cells of a result have the same value.
    """
    tools = Tools(session=session_with_strds)
    tools.t_rast_neighbors(
        input="input",
        output="output",
        basename="result",
        size=5,
        method=method,
        nprocs=2,
    )
    for name, value in zip(registered_maps(tools, "output"), expected, strict=True):
        stats = tools.r_univar(map=name, format="json")
        assert (stats["min"], stats["max"]) == (value, value)


@pytest.mark.parametrize(
    ("flags", "expected"),
    [
        ("", ["result_2001_01", "result_2001_02"]),
        ("n", ["result_2001_01", "result_2001_02", "result_2001_03"]),
    ],
)
def test_null_maps(session_with_strds, flags, expected):
    """Results with only null cells are registered with -n, else removed."""
    tools = Tools(session=session_with_strds)
    tools.t_rast_neighbors(
        flags=flags, input="input", output="output", basename="result"
    )
    assert registered_maps(tools, "output") == expected
    assert tools.g_list(type="raster", pattern="result_*").text_split() == expected


def test_overwrite_records_command_once(session_with_strds):
    """Overwriting replaces the dataset, so its history has one command."""
    tools = Tools(session=session_with_strds)
    tools.t_rast_neighbors(input="input", output="output", basename="result")
    tools.t_rast_neighbors(
        input="input", output="output", basename="result", overwrite=True
    )
    assert count_history_commands(tools, "output") == 1


def test_extend_appends_command(session_with_strds):
    """Extending keeps the history and adds the extending command to it."""
    tools = Tools(session=session_with_strds)
    tools.t_rast_neighbors(
        input="input",
        output="output",
        basename="result",
        where="start_time < '2001-02-01'",
    )
    tools.t_rast_neighbors(
        flags="e",
        input="input",
        output="output",
        basename="result",
        where="start_time >= '2001-02-01'",
        overwrite=True,
    )
    assert count_history_commands(tools, "output") == 2
