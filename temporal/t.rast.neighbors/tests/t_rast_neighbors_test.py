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
    ("suffix", "expected"),
    [
        ("gran", ["result_2001_01", "result_2001_02"]),
        ("time", ["result_2001_01_01T00_00_00", "result_2001_02_01T00_00_00"]),
        ("num", ["result_00001", "result_00002"]),
        ("num%03", ["result_001", "result_002"]),
    ],
)
def test_suffix(session_in_mapset, suffix, expected):
    """The suffix option sets the names of the new maps."""
    tools = Tools(session=session_in_mapset)
    tools.t_rast_neighbors(
        input="input", output="output", basename="result", suffix=suffix
    )
    assert registered_maps(tools, "output") == expected


@pytest.mark.parametrize(
    ("semantic_labels", "expected"),
    [("input", ["S1", None]), ("method", ["S1_average", "average"])],
)
def test_semantic_labels(session_in_mapset, semantic_labels, expected):
    """The input label is copied, or combined with the method name."""
    tools = Tools(session=session_in_mapset)
    tools.t_rast_neighbors(
        input="input",
        output="output",
        basename="result",
        semantic_labels=semantic_labels,
    )
    rows = tools.t_rast_list(
        input="output", columns="semantic_label", format="json"
    ).json["data"]
    assert [row["semantic_label"] for row in rows] == expected


@pytest.mark.parametrize(
    ("flags", "expected"),
    [
        ("", ["result_2001_01", "result_2001_02"]),
        ("n", ["result_2001_01", "result_2001_02", "result_2001_03"]),
    ],
)
def test_null_maps(session_in_mapset, flags, expected):
    """Results with only null cells are registered with -n, else removed."""
    tools = Tools(session=session_in_mapset)
    tools.t_rast_neighbors(
        flags=flags, input="input", output="output", basename="result"
    )
    assert registered_maps(tools, "output") == expected
    assert tools.g_list(type="raster", pattern="result_*").text_split() == expected


@pytest.mark.parametrize(("flags", "shape"), [("", (5, 5)), ("r", (3, 3))])
def test_raster_region(session_in_mapset, flags, shape):
    """With -r, maps are processed in their own region, not the current one."""
    tools = Tools(session=session_in_mapset)
    tools.g_region(s=0, n=5, w=0, e=5, res=1)
    tools.t_rast_neighbors(
        flags=flags, input="input", output="output", basename="result"
    )
    info = tools.r_info(map="result_2001_01", format="json")
    assert (info["rows"], info["cols"]) == shape


@pytest.mark.parametrize(
    ("region", "expected"),
    [
        ({"s": 0, "n": 3}, ["result_2001_01", "result_2001_02"]),
        ({"s": 10, "n": 13}, []),
    ],
)
def test_region_relation(session_in_mapset, region, expected):
    """Only maps which overlap the current region are processed."""
    tools = Tools(session=session_in_mapset)
    tools.g_region(**region, w=0, e=3, res=1)
    tools.t_rast_neighbors(
        flags="r",
        input="input",
        output="output",
        basename="result",
        region_relation="overlaps",
    )
    rasters = tools.g_list(type="raster", pattern="result_*", format="json").json
    assert [raster["name"] for raster in rasters] == expected


@pytest.mark.parametrize(
    "options",
    [
        {"flags": "c", "size": 5},
        {"weighting_function": "gaussian", "weighting_factor": 1.5},
    ],
)
def test_neighborhood_options(session_in_mapset, options):
    """The neighborhood options are passed to r.neighbors."""
    tools = Tools(session=session_in_mapset)
    tools.t_rast_neighbors(input="input", output="output", basename="result", **options)
    tools.r_neighbors(input="jan", output="expected", **options)
    result = tools.r_univar(map="result_2001_01", format="json").json
    assert result == tools.r_univar(map="expected", format="json").json


def test_overwrite_records_command_once(session_in_mapset):
    """Overwriting replaces the dataset, so its history has one command."""
    tools = Tools(session=session_in_mapset)
    tools.t_rast_neighbors(input="input", output="output", basename="result")
    tools.t_rast_neighbors(
        input="input", output="output", basename="result", overwrite=True
    )
    assert count_history_commands(tools, "output") == 1


def test_extend(session_in_mapset):
    """Extending adds the new maps and the command to the existing dataset."""
    tools = Tools(session=session_in_mapset)
    tools.t_rast_neighbors(
        input="input",
        output="output",
        basename="result",
        where="start_time < '2001-02-01'",
    )
    assert registered_maps(tools, "output") == ["result_2001_01"]
    tools.t_rast_neighbors(
        flags="e",
        input="input",
        output="output",
        basename="result",
        where="start_time >= '2001-02-01'",
        overwrite=True,
    )
    assert registered_maps(tools, "output") == ["result_2001_01", "result_2001_02"]
    assert count_history_commands(tools, "output") == 2
