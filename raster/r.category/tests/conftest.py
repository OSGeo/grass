"""Fixture for r.category test"""

import os
import pytest
import grass.script as gs


@pytest.fixture
def simple_dataset(tmp_path):
    """Set up a GRASS session and create test rasters with color rules."""
    project = tmp_path / "raster_color_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command(
            "g.region",
            s=0,
            n=100,
            w=0,
            e=100,
            res=10,
            env=session.env,
        )
        gs.mapcalc("test = if(col() < 3, col(), 2)", env=session.env)
        gs.mapcalc("test_1 = if(col() < 5, col(), 4)", env=session.env)
        gs.mapcalc("test_d = if(col() < 5, col() / 2., 4.5)", env=session.env)

        yield session
