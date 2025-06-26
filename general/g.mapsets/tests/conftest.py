"""Fixtures for Jupyter tests

Fixture for grass.jupyter.TimeSeries test

Fixture for ReprojectionRenderer test with simple GRASS location, raster, vector.
"""

import os
from types import SimpleNamespace

import grass.script as gs
import pytest

TEST_MAPSETS = ["PERMANENT", "test1", "test2", "test3"]
ACCESSIBLE_MAPSETS = ["test3", "PERMANENT"]


@pytest.fixture(scope="module")
def simple_dataset(tmp_path_factory):
    """Start a session and create a test mapsets
    Returns object with attributes about the dataset.
    """
    tmp_path = tmp_path_factory.mktemp("simple_dataset")
    project_name = "test"
    project = tmp_path / project_name
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        gs.run_command("g.proj", flags="c", epsg=26917, env=session.env)
        gs.run_command(
            "g.region",
            s=0,
            n=80,
            w=0,
            e=120,
            b=0,
            t=50,
            res=10,
            res3=10,
            env=session.env,
        )
        # Create Mock Mapsets
        for mapset in TEST_MAPSETS:
            gs.run_command(
                "g.mapset",
                project=project_name,
                mapset=mapset,
                flags="c",
                env=session.env,
            )

        yield SimpleNamespace(
            session=session, mapsets=TEST_MAPSETS, accessible_mapsets=ACCESSIBLE_MAPSETS
        )
