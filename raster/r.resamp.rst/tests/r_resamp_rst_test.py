# SPDX-License-Identifier: GPL-2.0-or-later
"""Tests of r.resamp.rst"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools


@pytest.fixture
def session(tmp_path):
    """Session in an XY project, for rasters made by each test"""
    project = tmp_path / "xy_test"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        yield session


@pytest.mark.parametrize(
    ("rows", "cols", "linked"),
    [(20, 30, True), (200, 10, True), (20, 30, False)],
)
def test_data_range_in_history(session, tmp_path, rows, cols, linked):
    """The history records the range of the input, with or without a range file

    A raster linked with r.external -r has no range file, so r.resamp.rst
    computes the range itself.
    """
    tools = Tools(session=session)
    tools.g_region(s=0, n=rows, w=0, e=cols, res=1)
    tools.r_mapcalc(expression="values = float(row() * 10 + col())")
    data = tools.r_univar(map="values", format="json")
    expected = f"zmin_data={data['min']:f}, zmax_data={data['max']:f}"
    name = "values"
    if linked:
        tiff = tmp_path / "values.tif"
        tools.r_out_gdal(input="values", output=str(tiff), format="GTiff")
        tools.r_external(input=str(tiff), output="linked", flags="ro")
        name = "linked"

    tools.r_resamp_rst(input=name, elevation="resampled", ew_res=1, ns_res=1)

    assert expected in tools.r_info(map="resampled", flags="h").text
