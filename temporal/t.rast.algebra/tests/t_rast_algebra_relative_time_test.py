# SPDX-FileCopyrightText: 2026 GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of t.rast.algebra with relative time"""

import pytest

from grass.tools import Tools


@pytest.mark.parametrize(
    ("expression", "values"),
    [
        pytest.param("R = A * 2", [2, 4, 6], id="arithmetic"),
        pytest.param("R = A + td(A)", [2, 3, 4], id="time-delta"),
        pytest.param("R = if(A > 1, A)", [0, 2, 3], id="condition"),
    ],
)
def test_result_keeps_relative_time(session_with_relative_strds, expression, values):
    """The result of an expression on relative time input is in relative time."""
    tools = Tools(session=session_with_relative_strds)
    tools.t_rast_algebra(expression=expression, basename="r")

    info = tools.t_info(input="R", flags="g").keyval
    assert info["temporal_type"] == "relative"
    assert info["unit"] == "days"

    data = tools.t_rast_list(
        input="R", columns="start_time,end_time,max", format="json"
    ).json["data"]
    assert [row["max"] for row in data] == values
