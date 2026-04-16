"""Tests for v.geometry."""

import math

import pytest

from grass.tools import Tools


def test_area_metrics(session):
    """Area-family metrics on a 2x2 grid of 50x50 squares.

    Exercises area, perimeter, compactness, fractal_dimension, bbox in
    one call so the merged-record and key-rename behaviors are both
    covered without paying for extra subprocess spawns.
    """
    tools = Tools(session=session.session)
    result = tools.v_geometry(
        map="grid", metric="area,perimeter,compactness,fractal_dimension,bbox"
    )
    assert len(result["records"]) == 4
    expected_compactness = 200.0 / (2.0 * math.sqrt(math.pi * 2500.0))
    for record in result["records"]:
        assert record["area"] == pytest.approx(2500.0)
        assert record["perimeter"] == pytest.approx(200.0)
        assert record["compactness"] == pytest.approx(expected_compactness, rel=1e-6)
        assert "compact" not in record
        assert "fractal_dimension" in record
        assert "fd" not in record
        assert {"north", "south", "east", "west"} <= set(record.keys())
    assert result["units"]["area"] == "square meters"
    assert result["units"]["perimeter"] == "meters"


def test_area_hectares(session):
    tools = Tools(session=session.session)
    result = tools.v_geometry(map="grid", metric="area", units="hectares")
    for record in result["records"]:
        assert record["area"] == pytest.approx(0.25)
    assert result["units"]["area"] == "hectares"


def test_line_metrics(session):
    """Line-family metrics on a straight line from (0,0) to (100,100)."""
    tools = Tools(session=session.session)
    result = tools.v_geometry(map="line", metric="length,sinuosity,azimuth")
    record = result["records"][0]
    assert record["length"] == pytest.approx(math.sqrt(2.0) * 100.0, rel=1e-6)
    # A straight line has sinuosity 1.
    assert record["sinuosity"] == pytest.approx(1.0, rel=1e-6)
    assert "sinuous" not in record
    assert "azimuth" in record


def test_count_totals_flag(session):
    tools = Tools(session=session.session)
    result = tools.v_geometry(map="points", metric="count", flags="c")
    assert result["totals"]["count"] == 3


def test_coordinates(session):
    tools = Tools(session=session.session)
    result = tools.v_geometry(map="points", metric="coordinates")
    coords = {(record["x"], record["y"]) for record in result["records"]}
    assert coords == {(10.0, 20.0), (30.0, 40.0), (50.0, 60.0)}


def test_multiple_metrics_totals(session):
    """Totals from different metrics are merged."""
    tools = Tools(session=session.session)
    result = tools.v_geometry(
        map="grid", metric="area,count", type="centroid", flags="c"
    )
    assert result["totals"]["area"] == pytest.approx(10000.0)
    assert result["totals"]["count"] == 4


def test_per_metric_units(session):
    """Each metric gets its own unit via positional correspondence."""
    tools = Tools(session=session.session)
    result = tools.v_geometry(
        map="grid", metric="area,perimeter", units="hectares,kilometers"
    )
    assert result["units"]["area"] == "hectares"
    assert result["units"]["perimeter"] == "kilometers"
    for record in result["records"]:
        assert record["area"] == pytest.approx(0.25)
        assert record["perimeter"] == pytest.approx(0.2)


def test_partial_units(session):
    """Fewer units than metrics: extra metrics use defaults."""
    tools = Tools(session=session.session)
    result = tools.v_geometry(map="grid", metric="area,perimeter", units="hectares")
    assert result["units"]["area"] == "hectares"
    assert result["units"]["perimeter"] == "meters"


def test_plain_format(session):
    tools = Tools(session=session.session)
    result = tools.v_geometry(map="grid", metric="area", format="plain")
    lines = result.stdout.splitlines()
    assert lines[0] == "category|area"
    assert len(lines) == 5


def test_csv_format(session):
    tools = Tools(session=session.session)
    result = tools.v_geometry(map="grid", metric="area", format="csv")
    lines = result.stdout.splitlines()
    assert lines[0] == "category,area"
    assert len(lines) == 5


def test_csv_rejects_multichar_separator(session):
    tools = Tools(session=session.session)
    with pytest.raises(Exception, match="CSV separator"):
        tools.v_geometry(map="grid", metric="area", format="csv", separator="--")


def test_mixed_feature_types_rejected(session):
    """Mixing metrics from different feature-type families fails cleanly."""
    tools = Tools(session=session.session)
    with pytest.raises(Exception, match="different feature types"):
        tools.v_geometry(map="grid", metric="area,length")
