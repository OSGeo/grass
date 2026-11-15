# Copyright (C) 2026 by the GRASS Development Team
# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of d.legend with a logarithmic scale"""

import os

import pytest

import grass.script as gs
from grass.tools import ToolError, Tools

# Rasters whose value ranges cover the cases a logarithmic legend has to
# handle, with label values spanning each range including its minimum.
RASTERS = {
    "positive": ("row() * col() * 10.0", "10,100,1000"),
    "between_tens": ("2.0 + 7.0 * (row() - 1) * (col() - 1) / 81", "2,5,9"),
    "from_zero": ("1000.0 * (row() - 1) * col() / 90", "0,1,10,100,1000"),
    "zero_to_one": ("(row() - 1) * (col() - 1) / 81.0", "0,0.1,0.5,1"),
    "mixed": ("2.0 * row() * col() - 100", "-98,0,10,100"),
    "negative": ("-1.0 * row() * col()", "-100,-10,-1"),
    "negative_to_zero": ("-1.0 * (row() - 1) * col()", "-90,-10,0"),
    "integer": ("int(row() * col() * 10)", "10,100,1000"),
}


@pytest.fixture(scope="module")
def session(tmp_path_factory):
    """Session in an XY project with the rasters from RASTERS"""
    project = tmp_path_factory.mktemp("d_legend") / "xy_test"
    gs.create_project(project)
    with (
        gs.setup.init(project, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        tools.g_region(s=0, n=10, w=0, e=10, res=1)
        for name, (expression, _) in RASTERS.items():
            tools.r_mapcalc(expression=f"{name} = {expression}")
            tools.r_colors(map=name, color="viridis", flags="g")
        tools.r_mapcalc(expression="constant = 5.0")
        tools.r_mapcalc(expression="constant_integer = 5")
        yield session


def render_tools(session, output):
    """Return a Tools object which renders into the given file.

    A copy of the session environment is used so the render settings do not
    leak into the shared session fixture.
    """
    env = dict(session.env)
    env["GRASS_RENDER_IMMEDIATE"] = "cairo"
    env["GRASS_RENDER_FILE"] = str(output)
    return Tools(env=env)


@pytest.mark.parametrize("raster", RASTERS)
def test_log_legend_with_label_step(session, tmp_path, raster):
    """A logarithmic legend is drawn whatever the sign of the values.

    Ranges including zero or negative values used to be a fatal error.
    """
    output = tmp_path / "legend.png"
    tools = render_tools(session, output)
    tools.d_legend(raster=raster, flags="lts", label_step=1)
    assert output.exists()


@pytest.mark.parametrize("raster", RASTERS)
def test_log_legend_with_label_values(session, tmp_path, raster):
    """Explicit label values anywhere in the range are accepted."""
    output = tmp_path / "legend.png"
    tools = render_tools(session, output)
    tools.d_legend(raster=raster, flags="lts", label_values=RASTERS[raster][1])
    assert output.exists()


def test_log_legend_differs_from_linear(session, tmp_path):
    """The logarithmic scale changes the legend for a range starting at zero."""
    linear = tmp_path / "linear.png"
    log = tmp_path / "log.png"
    render_tools(session, linear).d_legend(raster="from_zero", flags="s")
    render_tools(session, log).d_legend(raster="from_zero", flags="ls")
    assert linear.read_bytes() != log.read_bytes()


def test_log_legend_labels_range_without_power_of_ten(session, tmp_path):
    """A range between two powers of ten still gets labels with label_step.

    Without a power of ten to place labels at, the minimum and maximum are
    labeled, so the legend differs from the same legend without numbers. The
    labels are drawn with ticks, so the difference does not depend on text
    rendering, which is not available on all platforms.
    """
    bar_only = tmp_path / "bar_only.png"
    labeled = tmp_path / "labeled.png"
    render_tools(session, bar_only).d_legend(raster="between_tens", flags="lsc")
    render_tools(session, labeled).d_legend(
        raster="between_tens", flags="lst", label_step=1
    )
    assert bar_only.read_bytes() != labeled.read_bytes()


def test_log_legend_constant_raster_fails(session, tmp_path):
    """A single-value range has no logarithmic scale and is an error."""
    tools = render_tools(session, tmp_path / "legend.png")
    with pytest.raises(ToolError, match="logarithmic"):
        tools.d_legend(raster="constant", flags="l")


def test_log_legend_constant_integer_raster(session, tmp_path):
    """A single-value integer raster gets a category legend as without -l.

    The logarithmic scale applies to the smooth legend only, so the flag is
    ignored for a category legend, but a smooth legend, here forced by a wide
    box, has no scale to draw.
    """
    output = tmp_path / "legend.png"
    tools = render_tools(session, output)
    tools.d_legend(raster="constant_integer", flags="l")
    assert output.exists()
    with pytest.raises(ToolError, match="logarithmic"):
        tools.d_legend(raster="constant_integer", flags="l", at=(5, 10, 5, 95))
