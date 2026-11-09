# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of t.rast.accdetect"""

import pytest

from grass.tools import Tools


def detect_occurrence(session, option, start, end, **options):
    """Detect occurrences with one limit map and return the maximum of each map."""
    tools = Tools(session=session)
    tools.t_create(output="limit", title="Limit", description="Limit dataset")
    tools.t_register(input="limit", maps=option, start=start, end=end)
    tools.t_rast_accdetect(
        input="input",
        occurrence="output",
        basename="result",
        range="0,100",
        start="2001-01-01",
        cycle="2 days",
        **{option: "limit"},
        **options,
    )
    data = tools.t_rast_list(input="output", columns="max", format="json").json["data"]
    return [row["max"] for row in data]


@pytest.mark.parametrize("option", ["minimum", "maximum"])
@pytest.mark.parametrize(
    ("start", "end"),
    [
        pytest.param("2001-01-01", "2001-01-02", id="equal"),
        pytest.param("2000-12-31", "2001-01-01 12:00:00", id="overlapped"),
    ],
)
def test_limit_map_by_relation(session_with_strds, option, start, end):
    """A limit map with a documented relation replaces the range option."""
    assert detect_occurrence(session_with_strds, option, start, end) == []


def test_unrelated_limit_map_uses_range(session_with_strds):
    """A limit map without a documented relation leaves the range option in use."""
    occurrence = detect_occurrence(
        session_with_strds, "maximum", "2002-01-01", "2002-01-02"
    )
    assert occurrence == [0]


@pytest.mark.parametrize("option", ["minimum", "maximum"])
def test_reverse_uses_limit_map_of_each_map(session_with_strds, option):
    """With the reverse flag each input map uses the limit map of its own time."""
    tools = Tools(session=session_with_strds)
    tools.r_mapcalc(expression="input_map_2 = 20")
    tools.t_register(
        input="input", maps="input_map_2", start="2001-01-02", end="2001-01-03"
    )
    occurrence = detect_occurrence(
        session_with_strds, option, "2001-01-02", "2001-01-03", flags="r"
    )
    assert occurrence == [0]
