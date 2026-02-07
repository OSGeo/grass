"""Test t.vect.list output formats"""

import csv
import datetime
import io
import json

import pytest

import grass.script as gs

yaml = pytest.importorskip("yaml", reason="PyYAML package not available")


@pytest.mark.needs_solo_run
def test_defaults(space_time_vector_dataset):
    """Check that the module runs with default parameters"""
    gs.run_command(
        "t.vect.list",
        input=space_time_vector_dataset.name,
        env=space_time_vector_dataset.session.env,
    )


@pytest.mark.needs_solo_run
def test_json(space_time_vector_dataset):
    """Check JSON can be parsed and contains the right values"""
    result = json.loads(
        gs.read_command(
            "t.vect.list",
            input=space_time_vector_dataset.name,
            format="json",
            env=space_time_vector_dataset.session.env,
        )
    )
    assert "data" in result
    assert "metadata" in result
    for item in result["data"]:
        for name in result["metadata"]["column_names"]:
            if name != "layer":  # layer can be None
                assert item[name], "All values should be set with the default columns"
    names = [item["name"] for item in result["data"]]
    assert names == space_time_vector_dataset.vector_names


@pytest.mark.needs_solo_run
def test_yaml(space_time_vector_dataset):
    """Check YAML can be parsed and contains the right values"""
    result = yaml.safe_load(
        gs.read_command(
            "t.vect.list",
            input=space_time_vector_dataset.name,
            format="yaml",
            env=space_time_vector_dataset.session.env,
        )
    )
    assert "data" in result
    assert "metadata" in result
    for item in result["data"]:
        for name in result["metadata"]["column_names"]:
            if name != "layer":  # layer can be None
                assert item[name], "All values should be set with the default columns"
        assert isinstance(item["start_time"], datetime.datetime)
    names = [item["name"] for item in result["data"]]
    assert names == space_time_vector_dataset.vector_names
    times = [item["start_time"] for item in result["data"]]
    assert times == space_time_vector_dataset.start_times


@pytest.mark.needs_solo_run
@pytest.mark.parametrize(
    ("separator", "delimiter"), [(None, ","), (",", ","), (";", ";"), ("tab", "\t")]
)
def test_csv(space_time_vector_dataset, separator, delimiter):
    """Check CSV can be parsed with different separators"""
    columns = ["name", "start_time"]
    text = gs.read_command(
        "t.vect.list",
        input=space_time_vector_dataset.name,
        columns=columns,
        format="csv",
        separator=separator,
        env=space_time_vector_dataset.session.env,
    )
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
    assert len(data) == len(space_time_vector_dataset.vector_names)
    for row in data:
        assert len(row) == len(columns)
