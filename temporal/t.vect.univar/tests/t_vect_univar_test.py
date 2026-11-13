"""Test t.vect.univar output formats"""

import csv
import io
import math

import pytest

from grass.tools import ToolError, Tools


def test_plain_output(space_time_vector_dataset):
    """Default plain output has a header and one pipe-separated row per map"""
    tools = Tools(session=space_time_vector_dataset.session)
    result = tools.t_vect_univar(
        input=space_time_vector_dataset.name,
        column=space_time_vector_dataset.column,
    )
    lines = result.text_split("\n")
    assert len(lines) == 1 + len(space_time_vector_dataset.vector_names)
    header = lines[0].split("|")
    assert header[:6] == ["id", "start", "end", "n", "nmissing", "nnull"]
    first = dict(zip(header, lines[1].split("|"), strict=True))
    assert first["id"].startswith("points_1@")
    assert first["n"] == "3"
    assert float(first["min"]) == 10.0
    assert float(first["max"]) == 30.0
    assert float(first["mean"]) == 20.0


def test_plain_no_header(space_time_vector_dataset):
    """The u flag suppresses the header line"""
    tools = Tools(session=space_time_vector_dataset.session)
    result = tools.t_vect_univar(
        input=space_time_vector_dataset.name,
        column=space_time_vector_dataset.column,
        flags="u",
    )
    lines = result.text_split("\n")
    assert len(lines) == len(space_time_vector_dataset.vector_names)
    assert lines[0].startswith("points_1@")


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


def test_json_output(space_time_vector_dataset):
    """JSON output is a list of records with typed v.univar statistics"""
    tools = Tools(session=space_time_vector_dataset.session)
    result = tools.t_vect_univar(
        input=space_time_vector_dataset.name,
        column=space_time_vector_dataset.column,
        format="json",
    ).json
    assert len(result) == len(space_time_vector_dataset.vector_names)
    first = result[0]
    assert first["id"].startswith("points_1@")
    assert first["start"] == "2001-01-01 00:00:00"
    assert first["end"] == "2001-02-01 00:00:00"
    assert first["n"] == 3
    assert first["min"] == 10.0
    assert first["max"] == 30.0
    assert first["mean"] == 20.0


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


def plain_rows(result):
    """Return plain output rows as dictionaries keyed by the header"""
    lines = result.text_split("\n")
    header = lines[0].split("|")
    return [dict(zip(header, line.split("|"), strict=True)) for line in lines[1:]]


def test_plain_infinite_statistic(edge_value_dataset):
    """An infinite statistic is printed as inf, as v.univar prints it"""
    tools = Tools(session=edge_value_dataset.session)
    result = tools.t_vect_univar(
        input=edge_value_dataset.name, column=edge_value_dataset.column
    )
    zero_sum = plain_rows(result)[0]
    assert zero_sum["id"].startswith("zero_sum@")
    assert zero_sum["population_coeff_variation"] == "inf"


def test_plain_single_feature_extended(edge_value_dataset):
    """With one feature per map, the undefined statistics are printed as NaN"""
    tools = Tools(session=edge_value_dataset.session)
    result = tools.t_vect_univar(
        input=edge_value_dataset.name,
        column=edge_value_dataset.column,
        where="cat = 1",
        flags="e",
    )
    for row in plain_rows(result):
        assert row["n"] == "1"
        # printf writes NaN as nan or -nan depending on the platform.
        assert math.isnan(float(row["sample_stddev"]))


def test_plain_large_whole_number(edge_value_dataset):
    """Large whole numbers are printed as v.univar prints them"""
    tools = Tools(session=edge_value_dataset.session)
    result = tools.t_vect_univar(
        input=edge_value_dataset.name,
        column=edge_value_dataset.column,
        twhere="start_time >= '2001-02-01'",
    )
    (large,) = plain_rows(result)
    assert large["id"].startswith("large@")
    # The text of the shell output, since the number of exponent digits
    # printed by %g depends on the platform.
    univar = tools.v_univar(
        map="large", column=edge_value_dataset.column, flags="g"
    ).text
    expected = dict(line.split("=", 1) for line in univar.splitlines())
    assert large["max"] == expected["max"]
