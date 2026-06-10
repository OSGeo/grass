"""Test t.rast3d.list functionality"""

import datetime
import json

import pytest

try:
    import yaml
except ImportError:
    yaml = None

from grass.tools import Tools


def test_t_rast3d_list_defaults(space_time_raster3d_dataset):
    """Check that the module correctly lists 3D raster maps."""
    tools = Tools(session=space_time_raster3d_dataset.session)

    result = tools.t_rast3d_list(input=space_time_raster3d_dataset.name)
    assert result.returncode == 0

    lines = [line.split("|") for line in result.text.splitlines()]
    header = lines[0]
    data_lines = [row[0] for row in lines[1:]]

    assert header == ["name", "mapset", "start_time", "end_time"]

    for map_name in space_time_raster3d_dataset.raster_names:
        assert map_name in data_lines


def test_t_rast3d_list_where_filter(space_time_raster3d_dataset):
    """Check that the where clause filters the 3D map output exactly."""
    tools = Tools(session=space_time_raster3d_dataset.session)

    result = tools.t_rast3d_list(
        input=space_time_raster3d_dataset.name, columns="name", where="name = 'vol_1'"
    )
    assert result.returncode == 0

    lines = result.text.splitlines()
    data_lines = lines[1:]

    assert len(data_lines) == 1
    assert data_lines[0] == "vol_1"


def test_t_rast3d_list_order(space_time_raster3d_dataset):
    """Check that ordering the output sorts the 3D map data correctly."""
    tools = Tools(session=space_time_raster3d_dataset.session)

    result_asc = tools.t_rast3d_list(
        input=space_time_raster3d_dataset.name, columns="name", order="start_time"
    )
    assert result_asc.returncode == 0

    lines_asc = result_asc.text.splitlines()
    data_lines = lines_asc[1:]

    assert data_lines == space_time_raster3d_dataset.raster_names


def test_json_format(space_time_raster3d_dataset):
    """Check JSON can be parsed and matches the expected data."""
    tools = Tools(session=space_time_raster3d_dataset.session)
    result = tools.t_rast3d_list(input=space_time_raster3d_dataset.name, format="json")

    parsed = json.loads(result.text)

    expected = [
        {
            "name": "vol_1",
            "mapset": "PERMANENT",
            "start_time": "2001-01-01 00:00:00",
            "end_time": "2001-02-01 00:00:00",
        },
        {
            "name": "vol_2",
            "mapset": "PERMANENT",
            "start_time": "2001-02-01 00:00:00",
            "end_time": "2001-03-01 00:00:00",
        },
        {
            "name": "vol_3",
            "mapset": "PERMANENT",
            "start_time": "2001-03-01 00:00:00",
            "end_time": "2001-04-01 00:00:00",
        },
        {
            "name": "vol_4",
            "mapset": "PERMANENT",
            "start_time": "2001-04-01 00:00:00",
            "end_time": "2001-05-01 00:00:00",
        },
        {
            "name": "vol_5",
            "mapset": "PERMANENT",
            "start_time": "2001-05-01 00:00:00",
            "end_time": "2001-06-01 00:00:00",
        },
        {
            "name": "vol_6",
            "mapset": "PERMANENT",
            "start_time": "2001-06-01 00:00:00",
            "end_time": "2001-07-01 00:00:00",
        },
    ]

    assert len(parsed["data"]) == len(expected)

    for i, expected_item in enumerate(expected):
        actual_item = parsed["data"][i]
        for key, expected_value in expected_item.items():
            assert actual_item[key] == expected_value


@pytest.mark.skipif(yaml is None, reason="PyYAML package not available")
def test_yaml_format(space_time_raster3d_dataset):
    """Check YAML structure and types."""
    tools = Tools(session=space_time_raster3d_dataset.session)
    result = tools.t_rast3d_list(input=space_time_raster3d_dataset.name, format="yaml")

    parsed = yaml.safe_load(result.text)

    assert "data" in parsed
    assert "metadata" in parsed

    for item in parsed["data"]:
        for name in parsed["metadata"]["column_names"]:
            assert item[name], "All values should be set with the default columns"
        assert isinstance(item["start_time"], datetime.datetime)

    names = [item["name"] for item in parsed["data"]]
    assert names == space_time_raster3d_dataset.raster_names

    times = [item["start_time"] for item in parsed["data"]]
    expected_times = [datetime.datetime(2001, i + 1, 1) for i in range(len(names))]
    assert times == expected_times


@pytest.mark.parametrize("separator", [",", "|"])
def test_csv_format(space_time_raster3d_dataset, separator):
    """Check CSV output using string matching."""
    tools = Tools(session=space_time_raster3d_dataset.session)
    result = tools.t_rast3d_list(
        input=space_time_raster3d_dataset.name, format="csv", separator=separator
    )

    expected_lines = [
        f'"name"{separator}"mapset"{separator}"start_time"{separator}"end_time"',
        f'"vol_1"{separator}"PERMANENT"{separator}"2001-01-01 00:00:00"{separator}"2001-02-01 00:00:00"',
        f'"vol_2"{separator}"PERMANENT"{separator}"2001-02-01 00:00:00"{separator}"2001-03-01 00:00:00"',
        f'"vol_3"{separator}"PERMANENT"{separator}"2001-03-01 00:00:00"{separator}"2001-04-01 00:00:00"',
        f'"vol_4"{separator}"PERMANENT"{separator}"2001-04-01 00:00:00"{separator}"2001-05-01 00:00:00"',
        f'"vol_5"{separator}"PERMANENT"{separator}"2001-05-01 00:00:00"{separator}"2001-06-01 00:00:00"',
        f'"vol_6"{separator}"PERMANENT"{separator}"2001-06-01 00:00:00"{separator}"2001-07-01 00:00:00"',
    ]
    expected = "\n".join(expected_lines)

    assert result.text == expected


@pytest.mark.parametrize("separator", [",", "|"])
def test_line_format(space_time_raster3d_dataset, separator):
    """Check line format by matching the string in one statement."""
    tools = Tools(session=space_time_raster3d_dataset.session)
    result = tools.t_rast3d_list(
        input=space_time_raster3d_dataset.name, format="line", separator=separator
    )

    expected = separator.join(
        [f"{name}@PERMANENT" for name in space_time_raster3d_dataset.raster_names]
    )

    assert result.text == expected


def test_method_comma(space_time_raster3d_dataset):
    """Verify the comma method correctly works."""
    tools = Tools(session=space_time_raster3d_dataset.session)
    result = tools.t_rast3d_list(input=space_time_raster3d_dataset.name, method="comma")

    expected = ",".join(
        [f"{name}@PERMANENT" for name in space_time_raster3d_dataset.raster_names]
    )

    assert result.text == expected


@pytest.mark.parametrize("output_format", ["json", "yaml", "csv", "plain"])
def test_suppress_header_validation(space_time_raster3d_dataset, output_format):
    """Check that the suppress column flag (-s) is rejected for specific formats."""
    tools = Tools(session=space_time_raster3d_dataset.session)

    result = tools.t_rast3d_list(
        input=space_time_raster3d_dataset.name,
        format=output_format,
        flags="s",
        errors="status",
    )

    status = result.returncode if hasattr(result, "returncode") else result

    if output_format in {"plain", "csv"}:
        assert status == 0
    else:
        assert status != 0
