"""Fixtures building a deterministic DEM for the r.geomorphon tests."""

import os

import pytest

import grass.script as gs
from grass.tools import Tools

# One hill and one pit on a flat base, so all ten landform classes appear.
DEM_EXPRESSION = (
    "200.0 + 15.0 * exp(-((x() - 13.0)^2 + (y() - 13.0)^2) / 8.0) "
    "- 15.0 * exp(-((x() - 27.0)^2 + (y() - 27.0)^2) / 8.0)"
)


@pytest.fixture(scope="module")
def geomorphon_session(tmp_path_factory):
    """Active session in an XY project with a fixed region and a generated DEM."""
    project = tmp_path_factory.mktemp("geomorphon_xy") / "project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        with Tools(session=session) as tools:
            tools.g_region(s=0, n=40, w=0, e=40, res=1)
            tools.r_mapcalc(expression=f"dem = {DEM_EXPRESSION}")
        yield session
