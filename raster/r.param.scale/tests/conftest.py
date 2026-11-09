"""Fixtures for the r.param.scale pytest.

Generates a small, deterministic DEM so the reference statistics are
bit-identical on every platform and GRASS version.
"""

import os

import pytest

import grass.script as gs
from grass.tools import Tools

# Deterministic polynomial DEM: pure +, -, * on integer row()/col() with
# exact decimal constants (no trig, no rand). The cubic terms make local
# curvature vary across the raster while keeping the fit well conditioned.
DEM_EXPRESSION = (
    "double(100.0 + 0.5*col() + 0.4*row() "
    "- 0.009*col()*col() + 0.0075*row()*row() + 0.0015*col()*row() "
    "+ 0.00006*col()*col()*col() - 0.00005*row()*row()*row())"
)


@pytest.fixture(scope="module")
def param_scale_session(tmp_path_factory):
    """Active session in an XY project with a fixed region and a generated DEM."""
    project = tmp_path_factory.mktemp("param_scale_xy") / "project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        with Tools(session=session) as tools:
            # Fixed, reproducible 50 x 50 region (square cells, res=1).
            tools.g_region(s=0, n=50, w=0, e=50, res=1)
            tools.r_mapcalc(expression=f"dem = {DEM_EXPRESSION}")
        yield session
