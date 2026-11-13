# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of t.rast.accumulate"""

import pytest

from grass.tools import Tools


def create_limit_dataset(tools, dataset, maps):
    """Create a dataset with maps given as (name, start time, end time)."""
    tools.t_create(output=dataset, title=dataset, description=dataset)
    for name, start, end in maps:
        tools.t_register(input=dataset, maps=name, start=start, end=end)


def accumulate(tools, **options):
    """Accumulate the input with limit datasets and return the maximum of each map."""
    tools.t_rast_accumulate(
        input="input",
        output="output",
        basename="result",
        lower="lower",
        upper="upper",
        limits="0,100",
        method="bedd",
        start="2001-01-01",
        **options,
    )
    data = tools.t_rast_list(input="output", columns="max", format="json").json["data"]
    return [row["max"] for row in data]


@pytest.mark.parametrize(
    ("preferred", "other"),
    [
        pytest.param(
            ("2001-01-01", "2001-01-02"),
            ("2000-12-01", "2001-02-01"),
            id="equal-over-during",
        ),
        pytest.param(
            ("2000-12-31", "2001-01-01 12:00:00"),
            ("2001-01-01 06:00:00", "2001-01-01 18:00:00"),
            id="overlapped-over-contains",
        ),
    ],
)
def test_limit_maps_by_relation(session_with_strds, preferred, other):
    """Limit maps with the relation documented first replace the limits option."""
    tools = Tools(session=session_with_strds)
    create_limit_dataset(tools, "lower", [("lower_5", *preferred), ("lower_8", *other)])
    create_limit_dataset(tools, "upper", [("upper_10", *preferred)])
    assert accumulate(tools, cycle="1 day") == [5]


def test_reverse_uses_limit_maps_of_each_granule(session_with_strds):
    """With the reverse flag each granule uses the limit maps of its own time."""
    tools = Tools(session=session_with_strds)
    tools.r_mapcalc(expression="input_map_2 = 20")
    day_1 = ("2001-01-01", "2001-01-02")
    day_2 = ("2001-01-02", "2001-01-03")
    tools.t_register(input="input", maps="input_map_2", start=day_2[0], end=day_2[1])
    create_limit_dataset(tools, "lower", [("lower_5", *day_1), ("lower_8", *day_2)])
    create_limit_dataset(tools, "upper", [("upper_10", *day_2)])
    assert accumulate(tools, flags="r", cycle="2 days") == [17, 2]
