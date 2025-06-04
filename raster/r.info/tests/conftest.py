import os
import pytest
import grass.script as gs


@pytest.fixture
def setup_maps(tmp_path):
    """Set up a GRASS session and create test raster maps."""

    # Initialize GRASS project
    project = tmp_path / "r_info_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        # Define a simple region
        gs.run_command(
            "g.region",
            n=100,
            s=0,
            e=100,
            w=0,
            res=1,
            env=session.env,
        )

        # Create a small raster map
        #  - Rows less than 33 get value 1
        #  - Rows between 33 and 66 get value 2
        #  - Rows 66 and above get value 3
        gs.mapcalc(
            "landuse = if(row() < 33, 1, if(row() < 66, 2, 3))",
            overwrite=True,
            env=session.env,
        )

        # Assign categories to cells
        cats = """\
        1\tForest
        2\tAgriculture
        3\tUrban"""
        gs.write_command(
            "r.category",
            map="landuse",
            rules="-",
            stdin=cats,
            env=session.env,
        )

        # Add a title to the map
        gs.run_command(
            "r.support",
            map="landuse",
            title="Sample Land-Use",
            env=session.env,
        )

        # Create a map to test multiline comments
        gs.run_command("r.mapcalc", expression="map2 = 1", env=session.env)
        gs.run_command("r.support", map="map2", history="my comment", env=session.env)
        gs.run_command("r.support", map="map2", history="my comment 2", env=session.env)

        # Create a map to test long comments
        expression = """map3 = if(row() < 50, 10, 20) + if(col() < 50, 30, 40) + if(col() < 50, 30, 40) + if(row() < 50, 10, 20)"""
        gs.run_command("r.mapcalc", expression=expression, env=session.env)

        yield session  # Pass the session to tests
