# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Distance and nearest-value outputs, including paths without value storage."""

import numpy as np
import pytest

import grass.script as gs
import grass.script.array as ga


@pytest.fixture
def distance_tools(session_tools):
    """A rectangular region with one zero-valued source in its center."""
    session_tools.g_region(n=3, s=0, e=5, w=0, res=1)
    session_tools.r_mapcalc(
        expression="source = if(row() == 2 && col() == 3, 0, null())"
    )
    return session_tools


def assert_raster(name, expected, session):
    """Compare values and NULL cells without converting NULL to zero."""
    actual = ga.array(name, null=np.nan, env=session.env)
    np.testing.assert_array_equal(actual, expected)


@pytest.mark.parametrize(
    ("metric", "expected"),
    [
        ("euclidean", np.sqrt([[5, 2, 1, 2, 5], [4, 1, 0, 1, 4], [5, 2, 1, 2, 5]])),
        ("squared", [[5, 2, 1, 2, 5], [4, 1, 0, 1, 4], [5, 2, 1, 2, 5]]),
        ("manhattan", [[3, 2, 1, 2, 3], [2, 1, 0, 1, 2], [3, 2, 1, 2, 3]]),
        ("maximum", [[2, 1, 1, 1, 2], [2, 1, 0, 1, 2], [2, 1, 1, 1, 2]]),
    ],
)
def test_distance_only(distance_tools, session_in_mapset, metric, expected):
    """Distances to a single source match a known raster for each planar metric."""
    distance_tools.r_grow_distance(input="source", distance="distance", metric=metric)
    assert_raster("distance", expected, session_in_mapset)


@pytest.mark.parametrize("outputs", ["value", "both"])
def test_nearest_values(distance_tools, session_in_mapset, outputs):
    """Distinct labels propagate from two sources without ambiguous ties."""
    distance_tools.g_region(e=6)
    distance_tools.r_mapcalc(
        expression="labels = if(row() == 2 && col() == 1, -3.5, "
        "if(row() == 2 && col() == 6, 7.25, null()))"
    )
    kwargs = {"value": "value"}
    if outputs == "both":
        kwargs["distance"] = "distance"
    distance_tools.r_grow_distance(input="labels", metric="squared", **kwargs)
    assert_raster(
        "value",
        np.tile([-3.5, -3.5, -3.5, 7.25, 7.25, 7.25], (3, 1)),
        session_in_mapset,
    )
    if outputs == "both":
        assert_raster(
            "distance",
            [[1, 2, 5, 5, 2, 1], [0, 1, 4, 4, 1, 0], [1, 2, 5, 5, 2, 1]],
            session_in_mapset,
        )


@pytest.mark.parametrize("outputs", ["distance", "value", "both"])
def test_thresholds(distance_tools, session_in_mapset, outputs):
    """Both endpoints are retained and cells outside the interval become NULL."""
    distance_tools.r_mapcalc(
        expression="threshold_source = if(row() == 2 && col() == 3, -3.5, null())"
    )
    kwargs = {}
    if outputs != "value":
        kwargs["distance"] = "distance"
    if outputs != "distance":
        kwargs["value"] = "value"
    distance_tools.r_grow_distance(
        input="threshold_source",
        metric="squared",
        minimum_distance=1,
        maximum_distance=2,
        **kwargs,
    )
    expected = np.array(
        [
            [np.nan, 2, 1, 2, np.nan],
            [np.nan, 1, np.nan, 1, np.nan],
            [np.nan, 2, 1, 2, np.nan],
        ]
    )
    if outputs != "value":
        assert_raster("distance", expected, session_in_mapset)
    if outputs != "distance":
        assert_raster(
            "value", np.where(np.isnan(expected), np.nan, -3.5), session_in_mapset
        )


@pytest.mark.parametrize(
    ("request_value", "thresholds"),
    [(False, {}), (True, {}), (True, {"maximum_distance": 2})],
)
def test_inverted_sources(distance_tools, session_in_mapset, request_value, thresholds):
    """A positive threshold keeps the value output enabled with -n."""
    distance_tools.r_mapcalc(
        expression="inverted = if(row() == 2 && col() == 3, null(), 1)"
    )
    kwargs = {"value": "unused_value"} if request_value else {}
    distance_tools.r_grow_distance(
        input="inverted",
        distance="distance",
        metric="squared",
        flags="n",
        **kwargs,
        **thresholds,
    )
    expected = np.array(
        [[5, 2, 1, 2, 5], [4, 1, 0, 1, 4], [5, 2, 1, 2, 5]], dtype=float
    )
    if thresholds:
        expected[expected > 2] = np.nan
    assert_raster("distance", expected, session_in_mapset)
    value_exists = gs.find_file(
        "unused_value", element="cell", env=session_in_mapset.env
    )["name"]
    assert bool(value_exists) == bool(request_value and thresholds)
    if value_exists:
        assert_raster("unused_value", np.full((3, 5), np.nan), session_in_mapset)


def test_no_sources(distance_tools, session_in_mapset):
    """An input without sources produces an entirely NULL distance raster."""
    distance_tools.r_mapcalc(expression="empty = null()")
    distance_tools.r_grow_distance(input="empty", distance="distance")
    assert_raster("distance", np.full((3, 5), np.nan), session_in_mapset)
