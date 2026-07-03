"""Test t.vect.univar output formats"""

import csv
import io

import pytest

from grass.tools import ToolError, Tools


@pytest.mark.needs_solo_run
def test_plain_output(space_time_vector_dataset):
    """Default plain output has a header and one pipe-separated row per map"""
    tools = Tools(session=space_time_vector_dataset.session)
    result = tools.t_vect_univar(
        input=space_time_vector_dataset.name,
        column=space_time_vector_dataset.column,
    )
    lines = result.stdout.strip().splitlines()
    assert len(lines) == 1 + len(space_time_vector_dataset.vector_names)
    header = lines[0].split("|")
    assert header[:6] == ["id", "start", "end", "n", "nmissing", "nnull"]
    first = dict(zip(header, lines[1].split("|"), strict=True))
    assert first["id"].startswith("points_1@")
    assert first["n"] == "3"
    assert float(first["min"]) == 10.0
    assert float(first["max"]) == 30.0
    assert float(first["mean"]) == 20.0


@pytest.mark.needs_solo_run
def test_plain_no_header(space_time_vector_dataset):
    """The u flag suppresses the header line"""
    tools = Tools(session=space_time_vector_dataset.session)
    result = tools.t_vect_univar(
        input=space_time_vector_dataset.name,
        column=space_time_vector_dataset.column,
        flags="u",
    )
    lines = result.stdout.strip().splitlines()
    assert len(lines) == len(space_time_vector_dataset.vector_names)
    assert lines[0].startswith("points_1@")


@pytest.mark.needs_solo_run
def test_csv_output(space_time_vector_dataset):
    """CSV output defaults to a comma separator and parses cleanly"""
    tools = Tools(session=space_time_vector_dataset.session)
    result = tools.t_vect_univar(
        input=space_time_vector_dataset.name,
        column=space_time_vector_dataset.column,
        format="csv",
    )
    rows = list(csv.DictReader(io.StringIO(result.stdout)))
    assert len(rows) == len(space_time_vector_dataset.vector_names)
    assert rows[1]["id"].startswith("points_2@")
    assert float(rows[1]["min"]) == 40.0
    assert float(rows[1]["max"]) == 60.0
    assert float(rows[1]["mean"]) == 50.0


@pytest.mark.needs_solo_run
def test_csv_rejects_long_separator(space_time_vector_dataset):
    """CSV output rejects a separator longer than one character"""
    tools = Tools(session=space_time_vector_dataset.session)
    with pytest.raises(ToolError):
        tools.t_vect_univar(
            input=space_time_vector_dataset.name,
            column=space_time_vector_dataset.column,
            format="csv",
            separator="ab",
        )


@pytest.mark.needs_solo_run
def test_json_output(space_time_vector_dataset):
    """JSON output is a list of records with typed v.univar statistics"""
    tools = Tools(session=space_time_vector_dataset.session)
    result = tools.t_vect_univar(
        input=space_time_vector_dataset.name,
        column=space_time_vector_dataset.column,
        format="json",
    )
    records = result.json
    assert len(records) == len(space_time_vector_dataset.vector_names)
    first = records[0]
    assert first["id"].startswith("points_1@")
    assert first["start"] == "2001-01-01 00:00:00"
    assert first["end"] == "2001-02-01 00:00:00"
    assert first["n"] == 3
    assert first["min"] == 10.0
    assert first["max"] == 30.0
    assert first["mean"] == 20.0


@pytest.mark.needs_solo_run
def test_json_extended_statistics(space_time_vector_dataset):
    """JSON output includes extended statistics with the e flag"""
    tools = Tools(session=space_time_vector_dataset.session)
    result = tools.t_vect_univar(
        input=space_time_vector_dataset.name,
        column=space_time_vector_dataset.column,
        format="json",
        flags="e",
    )
    first = result.json[0]
    assert first["median"] == 20.0


@pytest.mark.needs_solo_run
def test_json_rejects_separator(space_time_vector_dataset):
    """JSON output does not allow the separator option"""
    tools = Tools(session=space_time_vector_dataset.session)
    with pytest.raises(ToolError):
        tools.t_vect_univar(
            input=space_time_vector_dataset.name,
            column=space_time_vector_dataset.column,
            format="json",
            separator="comma",
        )


@pytest.mark.needs_solo_run
def test_json_rejects_no_header_flag(space_time_vector_dataset):
    """JSON output does not allow suppressing column names"""
    tools = Tools(session=space_time_vector_dataset.session)
    with pytest.raises(ToolError):
        tools.t_vect_univar(
            input=space_time_vector_dataset.name,
            column=space_time_vector_dataset.column,
            format="json",
            flags="u",
        )
