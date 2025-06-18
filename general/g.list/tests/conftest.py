import os
import grass.script as gs
import pytest


@pytest.fixture
def simple_dataset(tmp_path_factory):
    """Set up a GRASS session for the tests."""
    tmp_path = tmp_path_factory.mktemp("simple_dataset")
    project = "test_project"
    mapset = "test_1"

    # Create a test project
    gs.create_project(tmp_path, project)

    # Initialize the GRASS session
    with gs.setup.init(tmp_path / project, env=os.environ.copy()) as session:
        gs.run_command("g.region", rows=3, cols=3, env=session.env)

        # Create Mock Mapset and data
        gs.run_command(
            "g.mapset", project=project, mapset=mapset, flags="c", env=session.env
        )

        # Create a raster in this mapset
        gs.mapcalc(f"raster_{mapset} = int(row())", env=session.env)
        gs.run_command(
            "r.support",
            map=f"raster_{mapset}",
            title=f"Raster title {mapset}",
            env=session.env,
        )

        # Create a vector in this mapset
        gs.run_command(
            "v.mkgrid",
            map=f"vector_{mapset}",
            grid=[10, 10],
            type="point",
            env=session.env,
        )
        gs.run_command(
            "v.support",
            map=f"vector_{mapset}",
            map_name=f"Vector title {mapset}",
            env=session.env,
        )

        yield session
