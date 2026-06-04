"""Test t.list functionality"""

import json

import pytest

from grass.tools import Tools


def test_t_list_defaults(space_time_dataset):
    """Check that the module correctly lists datasets (strds) and maps (raster)."""
    tools = Tools(session=space_time_dataset.session)

    strds_result = tools.t_list(type="strds", columns="name")
    assert strds_result.returncode == 0

    strds_lines = [line.strip() for line in strds_result.stdout.strip().splitlines()]
    assert space_time_dataset.name in strds_lines

    # Test Map Listing (raster)
    raster_result = tools.t_list(type="raster", columns="name")

    raster_lines = [line.strip() for line in raster_result.stdout.strip().splitlines()]
    for map_name in space_time_dataset.map_names:
        assert map_name in raster_lines


def test_t_list_where_filter(space_time_dataset):
    """Check that where clause filter the output exactly."""
    tools = Tools(session=space_time_dataset.session)

    match = tools.t_list(type="strds", columns="name", where="name LIKE 'temp_%'")
    match_lines = [line.strip() for line in match.stdout.strip().splitlines()]
    assert len(match_lines) == 1
    assert match_lines[0] == space_time_dataset.name

    empty = tools.t_list(type="strds", columns="name", where="name LIKE 'land_%'")
    assert empty is None


def test_t_list_order(space_time_dataset):
    """Check that ordering the output actually sorts the data correctly."""
    tools = Tools(session=space_time_dataset.session)

    result_asc = tools.t_list(type="raster", columns="name", order="start_time")
    assert result_asc is not None

    lines_asc = [line.strip() for line in result_asc.stdout.strip().splitlines()]

    assert lines_asc == space_time_dataset.map_names


def test_t_list_json(space_time_dataset):
    """Check JSON can be parsed and matches the exact expected data."""
    tools = Tools(session=space_time_dataset.session)
    result = tools.t_list(
        type="raster", format="json", columns="name,start_time,end_time"
    )

    data = json.loads(result.stdout)

    expected = [
        {
            "name": "temp_1",
            "start_time": "2026-01-01 00:00:00",
            "end_time": "2026-02-01 00:00:00",
        },
        {
            "name": "temp_2",
            "start_time": "2026-02-01 00:00:00",
            "end_time": "2026-03-01 00:00:00",
        },
        {
            "name": "temp_3",
            "start_time": "2026-03-01 00:00:00",
            "end_time": "2026-04-01 00:00:00",
        },
    ]

    assert len(data) == len(expected)

    for i, expected_item in enumerate(expected):
        actual_item = data[i]
        for key, expected_value in expected_item.items():
            assert actual_item[key] == expected_value


@pytest.mark.parametrize("separator", [",", "|"])
def test_t_list_csv(space_time_dataset, separator):
    """Check CSV output using string matching."""
    tools = Tools(session=space_time_dataset.session)
    result = tools.t_list(
        type="raster", format="csv", columns="name,start_time", separator=separator
    )

    expected_lines = [
        f"name{separator}start_time",
        f"{space_time_dataset.map_names[0]}{separator}2026-01-01 00:00:00",
        f"{space_time_dataset.map_names[1]}{separator}2026-02-01 00:00:00",
        f"{space_time_dataset.map_names[2]}{separator}2026-03-01 00:00:00",
    ]
    expected = "\n".join(expected_lines)

    assert result.stdout.strip() == expected


@pytest.mark.parametrize("separator", [",", "|"])
def test_t_list_line(space_time_dataset, separator):
    """Check line format by matching the exact joined string in one statement."""
    tools = Tools(session=space_time_dataset.session)
    result = tools.t_list(
        type="raster", format="line", columns="name", separator=separator
    )

    expected = separator.join(space_time_dataset.map_names)

    assert result.stdout.strip() == expected


@pytest.mark.parametrize("output_format", ["json", "line", "plain"])
def test_colhead_validation(space_time_dataset, output_format):
    """Check that the column header flag (-c) is rejected for JSON and Line, but accepted for plain."""
    tools = Tools(session=space_time_dataset.session)

    result = tools.t_list(
        type="raster", format=output_format, flags="c", errors="status"
    )
    status = result.returncode if hasattr(result, "returncode") else result

    if output_format == "plain":
        assert status == 0
    else:
        assert status != 0
