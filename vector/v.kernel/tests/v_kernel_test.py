# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests of v.kernel"""

import io

import pytest

from grass.tools import Tools, ToolError

POINTS = """\
0|0
10|0
20|0
30|0
"""

NETWORK = """\
VERTI:
L  2 1
 -10 0
 40 0
 1 1
"""


@pytest.fixture
def tools(xy_dataset_session):
    """Tools with points on a straight line network"""
    tools = Tools(session=xy_dataset_session)
    tools.g_region(n=50, s=-50, w=-50, e=50, res=1)
    tools.v_in_ascii(input=io.StringIO(POINTS), output="points")
    tools.v_in_ascii(input=io.StringIO(NETWORK), output="network", format="standard")
    return tools


def list_map_names(tools, map_type):
    return sorted(item["name"] for item in tools.g_list(type=map_type, format="json"))


def test_optimal_radius_without_output(tools):
    """The -q flag runs without an output name and writes no map"""
    tools.v_kernel(input="points", radius=100, flags="q")
    assert list_map_names(tools, "raster") == []


def test_optimal_radius_ignores_existing_output(tools):
    """The -q flag does not overwrite or reject an existing output map"""
    tools.r_mapcalc(expression="dens = 1")
    tools.v_kernel(input="points", output="dens", radius=100, flags="q")
    assert list_map_names(tools, "raster") == ["dens"]


def test_optimal_radius_on_network_without_output(tools):
    """The -q flag runs in network mode without a net_output name"""
    tools.v_kernel(input="points", net="network", radius=100, flags="q")
    assert list_map_names(tools, "vector") == ["network", "points"]


def test_output_required_without_q_flag(tools):
    """Without the -q flag, an output map name is still required"""
    with pytest.raises(ToolError):
        tools.v_kernel(input="points", radius=100)
