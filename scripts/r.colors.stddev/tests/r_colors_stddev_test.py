"""Tests of r.colors.stddev"""

import statistics

import pytest

from grass.tools import Tools

# data = col() over a one-row, nine-column region holds the values 1..9.
VALUES = range(1, 10)
MEAN = statistics.fmean(VALUES)  # 5.0
STDDEV = statistics.pstdev(VALUES)  # population stddev, as reported by r.univar
# The rule values are written with six-decimal formatting, so compare loosely.
TOL = 1e-3


def colors_of(session):
    """Return a Tools session holding a raster "data" with values 1..9."""
    tools = Tools(session=session)
    tools.g_region(n=1, s=0, w=0, e=9, res=1)
    tools.r_mapcalc(expression="data = col()")
    return tools


def test_default_rules_center_on_mean(xy_raster_dataset_session_mapset):
    """Default mode: white at the mean, blue at mean - 2*stddev, red at mean + 2*stddev."""
    tools = colors_of(xy_raster_dataset_session_mapset)
    tools.r_colors_stddev(map="data")
    table = tools.r_colors_out(map="data", format="json").json["table"]
    white = [e["value"] for e in table if e["color"] == "#FFFFFF"]
    blue = [e["value"] for e in table if e["color"] == "#0000FF"]
    red = [e["value"] for e in table if e["color"] == "#FF0000"]
    assert white == [pytest.approx(MEAN, abs=TOL)]
    assert blue == [pytest.approx(MEAN - 2 * STDDEV, abs=TOL)]
    assert any(v == pytest.approx(MEAN + 2 * STDDEV, abs=TOL) for v in red)


def test_bands_green_brackets_mean(xy_raster_dataset_session_mapset):
    """Banded mode (-b): the central green band spans mean +/- one stddev."""
    tools = colors_of(xy_raster_dataset_session_mapset)
    tools.r_colors_stddev(map="data", flags="b")
    table = tools.r_colors_out(map="data", format="json").json["table"]
    green = sorted(e["value"] for e in table if e["color"] == "#00FF00")
    assert green == [
        pytest.approx(MEAN - STDDEV, abs=TOL),
        pytest.approx(MEAN + STDDEV, abs=TOL),
    ]
