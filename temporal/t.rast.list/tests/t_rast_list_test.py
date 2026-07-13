"""Test t.rast.list output formats"""

import csv
import datetime
import io

import pytest

try:
    import yaml
except ImportError:
    yaml = None

from grass.tools import Tools


@pytest.mark.needs_solo_run
def test_defaults(space_time_raster_dataset):
    """Check that the module runs with default parameters"""
    tools = Tools(session=space_time_raster_dataset.session)
    tools.t_rast_list(input=space_time_raster_dataset.name)


@pytest.mark.needs_solo_run
@pytest.mark.parametrize(
    ("separator", "delimiter"),
    [(None, ","), (",", ","), ("pipe", "|")],
)
def test_line(space_time_raster_dataset, separator, delimiter):
    """Line format can be parsed and contains full names by default"""
    tools = Tools(session=space_time_raster_dataset.session)
    names = (
        tools.t_rast_list(
            input=space_time_raster_dataset.name,
            format="line",
            separator=separator,
        ).text_split(delimiter)
    )
    assert names == space_time_raster_dataset.full_raster_names


@pytest.mark.needs_solo_run
def test_json(space_time_raster_dataset):
    """Check JSON can be parsed and contains the right values"""
    tools = Tools(session=space_time_raster_dataset.session)
    result = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        format="json",
    ).json
    assert "data" in result
    assert "metadata" in result
    for item in result["data"]:
        for name in result["metadata"]["column_names"]:
            assert item[name], "All values should be set with the default columns"
    names = [item["name"] for item in result["data"]]
    assert names == space_time_raster_dataset.raster_names


@pytest.mark.skipif(yaml is None, reason="PyYAML package not available")
@pytest.mark.needs_solo_run
def test_yaml(space_time_raster_dataset):
    """Check YAML can be parsed and contains the right values"""
    tools = Tools(session=space_time_raster_dataset.session)
    result = yaml.safe_load(
        tools.t_rast_list(
            input=space_time_raster_dataset.name,
            format="yaml",
        ).text
    )
    assert "data" in result
    assert "metadata" in result
    for item in result["data"]:
        for name in result["metadata"]["column_names"]:
            assert item[name], "All values should be set with the default columns"
        assert isinstance(item["start_time"], datetime.datetime)
    names = [item["name"] for item in result["data"]]
    assert names == space_time_raster_dataset.raster_names
    times = [item["start_time"] for item in result["data"]]
    assert times == space_time_raster_dataset.start_times


@pytest.mark.needs_solo_run
@pytest.mark.parametrize(
    ("separator", "delimiter"),
    [(None, ","), (",", ","), (";", ";"), ("tab", "\t"), ("pipe", "|")],
)
def test_csv(space_time_raster_dataset, separator, delimiter):
    """Check CSV can be parsed with different separators"""
    tools = Tools(session=space_time_raster_dataset.session)
    columns = ["name", "start_time"]
    text = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        columns=columns,
        format="csv",
        separator=separator,
    ).text
    io_string = io.StringIO(text)
    reader = csv.DictReader(
        io_string,
        delimiter=delimiter,
        quotechar='"',
        doublequote=True,
        lineterminator="\n",
        strict=True,
    )
    data = list(reader)
    assert len(data) == len(space_time_raster_dataset.raster_names)
    for row in data:
        assert len(row) == len(columns)


@pytest.mark.needs_solo_run
def test_columns_list(space_time_raster_dataset):
    """Check that specific columns are returned correctly for the list method"""
    tools = Tools(session=space_time_raster_dataset.session)
    # All relevant columns from the interface.
    columns = [
        "id",
        "name",
        "semantic_label",
        "creator",
        "mapset",
        "temporal_type",
        "creation_time",
        "start_time",
        "end_time",
        "north",
        "south",
        "west",
        "east",
        "nsres",
        "ewres",
        "cols",
        "rows",
        "number_of_cells",
        "min",
        "max",
    ]
    result = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        method="list",
        columns=columns,
        format="json",
    ).json
    data = result["data"]
    assert len(data) == len(space_time_raster_dataset.raster_names)
    for row in data:
        assert len(row) == len(columns)


@pytest.mark.needs_solo_run
def test_columns_delta_gran(space_time_raster_dataset):
    """Check that specific columns are returned correctly for the gran method"""
    tools = Tools(session=space_time_raster_dataset.session)
    # All relevant columns from the interface.
    columns = [
        "id",
        "name",
        "mapset",
        "start_time",
        "end_time",
        "interval_length",
        "distance_from_begin",
    ]
    result = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        method="gran",
        columns=columns,
        format="json",
    ).json
    data = result["data"]
    assert len(data) == len(space_time_raster_dataset.raster_names)
    for row in data:
        assert len(row) == len(columns)


@pytest.mark.needs_solo_run
def test_json_empty_result(space_time_raster_dataset):
    """Check JSON is generated for no returned values"""
    tools = Tools(session=space_time_raster_dataset.session)
    result = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        format="json",
        where="FALSE",
    )
    assert "data" in result
    assert "metadata" in result
    assert len(result["data"]) == 0


@pytest.mark.needs_solo_run
@pytest.mark.parametrize("output_format", ["plain", "line"])
def test_plain_empty_result(space_time_raster_dataset, output_format):
    """Check module fails with non-zero return code for empty result"""
    tools = Tools(session=space_time_raster_dataset.session)
    return_code = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        format=output_format,
        where="FALSE",
        errors="status",
    )
    assert return_code != 0


@pytest.mark.needs_solo_run
@pytest.mark.parametrize("output_format", ["csv", "plain"])
def test_no_header_accepted(space_time_raster_dataset, output_format):
    """Check that the no column names flag is accepted"""
    tools = Tools(session=space_time_raster_dataset.session)
    tools.t_rast_list(
        input=space_time_raster_dataset.name,
        format=output_format,
    )


@pytest.mark.needs_solo_run
@pytest.mark.parametrize(
    "output_format",
    [
        "json",
        pytest.param(
            "yaml",
            marks=pytest.mark.skipif(
                yaml is None, reason="PyYAML package not available"
            ),
        ),
    ],
)
def test_no_header_rejected(space_time_raster_dataset, output_format):
    """Check that the no column names flag is rejected"""
    tools = Tools(session=space_time_raster_dataset.session)
    return_code = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        format=output_format,
        flags="u",
        errors="status",
    )
    assert return_code != 0


@pytest.mark.needs_solo_run
@pytest.mark.parametrize(
    "output_format",
    [
        "json",
        pytest.param(
            "yaml",
            marks=pytest.mark.skipif(
                yaml is None, reason="PyYAML package not available"
            ),
        ),
    ],
)
def test_separator_rejected(space_time_raster_dataset, output_format):
    """Check that the separator option is rejected"""
    tools = Tools(session=space_time_raster_dataset.session)
    returncode = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        format=output_format,
        separator=",",
        errors="status",
    )
    assert returncode != 0


@pytest.mark.needs_solo_run
@pytest.mark.parametrize("method", ["delta", "deltagaps", "gran"])
def test_other_methods_json(space_time_raster_dataset, method):
    """Test methods other than list"""
    tools = Tools(session=space_time_raster_dataset.session)
    result = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        format="json",
        method=method,
    ).json
    assert "data" in result
    assert "metadata" in result
    for item in result["data"]:
        assert item["interval_length"] >= 0
        assert item["distance_from_begin"] >= 0
    names = [item["name"] for item in result["data"]]
    assert names == space_time_raster_dataset.raster_names


@pytest.mark.needs_solo_run
def test_gran_json(space_time_raster_dataset):
    """Test granularity method"""
    tools = Tools(session=space_time_raster_dataset.session)
    result = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        format="json",
        method="gran",
        gran="15 days",
    ).json
    assert "data" in result
    assert "metadata" in result
    for item in result["data"]:
        assert item["interval_length"] >= 0
        assert item["distance_from_begin"] >= 0
        assert (
            item["name"] in space_time_raster_dataset.raster_names
            or item["name"] is None
        )
    assert len(result["data"]) > len(space_time_raster_dataset.raster_names), (
        "There should be more entries because of finer granularity"
    )


@pytest.mark.needs_solo_run
def test_gran_where_json(space_time_raster_dataset):
    """Test granularity method with where condition"""
    tools = Tools(session=space_time_raster_dataset.session)
    result = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        format="json",
        method="gran",
        where="True",
        gran="15 days",
    ).json
    assert "data" in result
    assert "metadata" in result
    for item in result["data"]:
        assert item["interval_length"] == 15.0
        assert item["distance_from_begin"] >= 0
        assert (
            item["name"] in space_time_raster_dataset.raster_names
            or item["name"] is None
        )
    assert len(result["data"]) > len(space_time_raster_dataset.raster_names), (
        "There should be more entries because of finer granularity"
    )
    assert len(result["data"]) == 13


@pytest.mark.needs_solo_run
def test_gran_where_larger_json(space_time_raster_dataset):
    """Test granularity method with where condition and lager granuarity"""
    tools = Tools(session=space_time_raster_dataset.session)
    result = tools.t_rast_list(
        input=space_time_raster_dataset.name,
        format="json",
        method="gran",
        where="True",
        gran="2 months",
    ).json
    assert "data" in result
    assert "metadata" in result
    for item in result["data"]:
        assert item["interval_length"] >= 58.0
        assert item["distance_from_begin"] >= 0
        assert (
            item["name"] in space_time_raster_dataset.raster_names
            or item["name"] is None
        )
    assert len(result["data"]) < len(space_time_raster_dataset.raster_names), (
        "There should be fewer entries because of finer granularity."
    )
    assert len(result["data"]) == 3
