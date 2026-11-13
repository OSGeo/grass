# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests of v.to.rast"""

import io

import pytest

from grass.tools import Tools

POINTS = """\
1.5|1.5|3
5.5|5.5|7
8.5|2.5|12
"""


@pytest.fixture
def tools(xy_dataset_session):
    """Tools with three points whose categories are 3, 7 and 12"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(n=10, s=0, w=0, e=10, res=1)
    tools.v_in_ascii(
        input=io.StringIO(POINTS), output="points", format="point", cat=3, flags="t"
    )
    return tools


def test_categories_count_up_to_the_highest_category(tools):
    """use=cat reports the highest category as the number of categories"""
    tools.v_to_rast(input="points", output="raster", type="point", use="cat")
    info = tools.r_info(map="raster", format="json")
    assert info["max"] == 12
    assert info["ncats"] == 12


def test_categories_have_no_labels(tools):
    """use=cat still writes no per-category labels"""
    tools.v_to_rast(input="points", output="raster", type="point", use="cat")
    assert tools.r_category(map="raster", cats="3,7,12", separator="pipe").text == (
        "3|\n7|\n12|"
    )
