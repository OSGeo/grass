import os
import pytest
import grass.script as gs


@pytest.fixture
def setup_maps(tmp_path):
    """Set up a GRASS session and create test raster maps."""

    # Initialize GRASS project
    project = tmp_path / "r_clump_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        # Set the region
        gs.run_command(
            "g.region",
            n=3,
            s=0,
            e=3,
            w=0,
            res=1,
            env=session.env,
        )

        gs.mapcalc(
            "custom_map = "
            "if(row() == 1 && col() == 1, 5, "
            "if(row() == 1 && col() == 2, 5.5, "
            "if(row() == 2 && col() == 2, 5, "
            "if(row() == 3 && col() >= 2, 5.5, null()))))",
            overwrite=True,
            env=session.env,
        )

        gs.mapcalc(
            "custom_map1 = "
            "if(row() == 1 && col() == 1, 1, "
            "if(row() == 1 && col() == 2, 2, "
            "if(row() == 1 && col() == 3, 3, "
            "if(row() == 2 && col() == 1, 4, "
            "if(row() == 2 && col() == 2, 5, "
            "if(row() == 2 && col() == 3, 6, "
            "if(row() == 3 && col() == 1, 7, "
            "if(row() == 3 && col() == 2, 8, "
            "if(row() == 3 && col() == 3, 9, null())))))))))",
            overwrite=True,
            env=session.env,
        )

        yield session  # Pass the session to tests
