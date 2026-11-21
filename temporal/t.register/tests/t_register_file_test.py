# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Test registering maps from a file with t.register

The three-column file format is ambiguous: the last column is either an
end time or a semantic label. These tests pin down how the ambiguity is
resolved for absolute and relative time.
"""

import pytest

from grass.tools import ToolError, Tools


def create_dataset(tools, name, temporaltype):
    tools.t_create(
        type="strds",
        temporaltype=temporaltype,
        output=name,
        title="Test",
        description="Test dataset",
    )


def registered_maps(tools, dataset):
    return tools.t_rast_list(
        input=dataset,
        columns="name,start_time,end_time,semantic_label",
        format="json",
    ).json["data"]


def test_relative_interval_from_file(session_with_rasters, tmp_path):
    """An integer third column is an end time, creating intervals."""
    tools = Tools(session=session_with_rasters)
    create_dataset(tools, "relative_interval", "relative")
    map_file = tmp_path / "maps.txt"
    map_file.write_text("a|-2|-1\nb|1|2\nc|2|5\n")
    tools.t_register(
        type="raster", input="relative_interval", file=map_file, unit="days"
    )
    rows = registered_maps(tools, "relative_interval")
    assert [(row["start_time"], row["end_time"]) for row in rows] == [
        (-2, -1),
        (1, 2),
        (2, 5),
    ]
    assert all(row["semantic_label"] is None for row in rows)


def test_relative_semantic_label_from_file(session_with_rasters, tmp_path):
    """A non-integer third column is a semantic label, not an end time."""
    tools = Tools(session=session_with_rasters)
    create_dataset(tools, "relative_label", "relative")
    map_file = tmp_path / "maps.txt"
    map_file.write_text("a|0|S2_1\nb|1|S2_2\n")
    tools.t_register(type="raster", input="relative_label", file=map_file, unit="days")
    rows = registered_maps(tools, "relative_label")
    assert [(row["start_time"], row["end_time"]) for row in rows] == [
        (0, None),
        (1, None),
    ]
    assert [row["semantic_label"] for row in rows] == ["S2_1", "S2_2"]


def test_absolute_interval_from_file(session_with_rasters, tmp_path):
    """A datetime third column is an end time, creating intervals."""
    tools = Tools(session=session_with_rasters)
    create_dataset(tools, "absolute_interval", "absolute")
    map_file = tmp_path / "maps.txt"
    map_file.write_text("a|2001-01-01|2001-02-01\nb|2001-02-01|2001-03-01\n")
    tools.t_register(type="raster", input="absolute_interval", file=map_file)
    rows = registered_maps(tools, "absolute_interval")
    assert [(row["start_time"], row["end_time"]) for row in rows] == [
        ("2001-01-01 00:00:00", "2001-02-01 00:00:00"),
        ("2001-02-01 00:00:00", "2001-03-01 00:00:00"),
    ]
    assert all(row["semantic_label"] is None for row in rows)


def test_relative_increment_from_names_only_file(session_with_rasters, tmp_path):
    """A file of map names with start and increment creates intervals."""
    tools = Tools(session=session_with_rasters)
    create_dataset(tools, "relative_increment", "relative")
    map_file = tmp_path / "maps.txt"
    map_file.write_text("a\nb\nc\n")
    tools.t_register(
        type="raster",
        input="relative_increment",
        file=map_file,
        start=0,
        increment=7,
        unit="days",
        flags="i",
    )
    rows = registered_maps(tools, "relative_increment")
    assert [(row["start_time"], row["end_time"]) for row in rows] == [
        (0, 7),
        (7, 14),
        (14, 21),
    ]


def test_register_without_dataset(session_with_rasters, tmp_path):
    """Maps can be registered in the temporal database without a dataset."""
    tools = Tools(session=session_with_rasters)
    map_file = tmp_path / "maps.txt"
    map_file.write_text("a\nb\n")
    tools.t_register(type="raster", file=map_file, start=0, increment=1, unit="days")
    assert "days" in tools.r_timestamp(map="a").stdout


def test_maps_and_file_are_mutually_exclusive(session_with_rasters, tmp_path):
    """Giving both a map list and a file is an error."""
    tools = Tools(session=session_with_rasters)
    create_dataset(tools, "excl", "relative")
    map_file = tmp_path / "maps.txt"
    map_file.write_text("a\n")
    with pytest.raises(ToolError):
        tools.t_register(
            type="raster",
            input="excl",
            maps="b",
            file=map_file,
            start=0,
            unit="days",
        )


def test_relative_time_requires_unit(session_with_rasters, tmp_path):
    """Registering with relative time and no unit is an error."""
    tools = Tools(session=session_with_rasters)
    create_dataset(tools, "no_unit", "relative")
    map_file = tmp_path / "maps.txt"
    map_file.write_text("a|0|1\n")
    with pytest.raises(ToolError):
        tools.t_register(type="raster", input="no_unit", file=map_file)


def test_absolute_semantic_label_from_file(session_with_rasters, tmp_path):
    """A non-datetime third column is a semantic label, not an end time."""
    tools = Tools(session=session_with_rasters)
    create_dataset(tools, "absolute_label", "absolute")
    map_file = tmp_path / "maps.txt"
    map_file.write_text("a|2001-01-01|S2_1\nb|2001-01-02|S2_2\n")
    tools.t_register(type="raster", input="absolute_label", file=map_file)
    rows = registered_maps(tools, "absolute_label")
    assert [(row["start_time"], row["end_time"]) for row in rows] == [
        ("2001-01-01 00:00:00", None),
        ("2001-01-02 00:00:00", None),
    ]
    assert [row["semantic_label"] for row in rows] == ["S2_1", "S2_2"]


def test_lines_do_not_take_time_of_previous_line(session_with_rasters, tmp_path):
    """Lines without end time or without time keep their own times."""
    tools = Tools(session=session_with_rasters)
    create_dataset(tools, "own_times", "absolute")
    tools.r_timestamp(map="c", date="15 mar 2005")
    map_file = tmp_path / "maps.txt"
    map_file.write_text("a|2001-01-01|2001-01-05\nb|2001-01-02\nc\n")
    tools.t_register(type="raster", input="own_times", file=map_file)
    rows = registered_maps(tools, "own_times")
    assert [(row["name"], row["start_time"], row["end_time"]) for row in rows] == [
        ("a", "2001-01-01 00:00:00", "2001-01-05 00:00:00"),
        ("b", "2001-01-02 00:00:00", None),
        ("c", "2005-03-15 00:00:00", None),
    ]


def test_vector_layer_gets_timestamp_of_its_layer(session_with_rasters, tmp_path):
    """A vector layer without time in the file gets the timestamp of its layer."""
    tools = Tools(session=session_with_rasters)
    tools.v_random(output="points", npoints=1)
    tools.v_category(input="points", output="layers", option="add", layer=2)
    tools.v_timestamp(map="layers", layer=1, date="1 jan 2001")
    tools.v_timestamp(map="layers", layer=2, date="7 jul 2007")
    tools.t_create(
        type="stvds",
        temporaltype="absolute",
        output="vectors",
        title="Test",
        description="Test dataset",
    )
    map_file = tmp_path / "maps.txt"
    map_file.write_text("layers:2\n")
    tools.t_register(type="vector", input="vectors", file=map_file)
    rows = tools.t_vect_list(
        input="vectors", columns="layer,start_time", format="json"
    ).json["data"]
    assert [(row["layer"], row["start_time"]) for row in rows] == [
        ("2", "2007-07-07 00:00:00")
    ]
