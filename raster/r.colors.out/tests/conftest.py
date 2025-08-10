"""Fixture for r.colors.out and r3.colors.out test"""

import os
import pytest
import grass.script as gs


@pytest.fixture
def raster_color_dataset(tmp_path):
    """Set up a GRASS session and create test rasters with color rules."""
    project = tmp_path / "raster_color_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command(
            "g.region",
            s=0,
            n=90,
            w=0,
            e=100,
            b=0,
            t=1,
            rows=3,
            cols=3,
            res=10,
            env=session.env,
        )
        gs.mapcalc("a = int(row())", env=session.env)
        gs.run_command("r.colors", map="a", color="elevation", env=session.env)
        yield session


@pytest.fixture
def raster3_color_dataset(tmp_path):
    """Set up a GRASS session and create test raster3 with color rules."""
    project = tmp_path / "raster3_color_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command(
            "g.region",
            s=0,
            n=100,
            w=0,
            e=100,
            b=5,
            t=50,
            tbres=10,
            res3=20,
            env=session.env,
        )
        gs.mapcalc3d("b = double(row())", env=session.env)
        gs.run_command("r3.colors", map="b", color="elevation", env=session.env)
        yield session
