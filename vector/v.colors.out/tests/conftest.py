"""Fixture for v.colors.out test"""

import os
import pytest
import grass.script as gs


@pytest.fixture
def vector_color_dataset(tmp_path):
    """Set up a GRASS session and create test vector with color rules."""
    project = tmp_path / "vector_color_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command(
            "g.region",
            n=90,
            s=-90,
            e=180,
            w=-180,
            res=10,
            env=session.env,
        )
        gs.run_command(
            "v.mkgrid", map="a", grid=[10, 10], type="point", env=session.env
        )
        gs.run_command("v.colors", map="a", color="elevation", env=session.env)
        yield session
