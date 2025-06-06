import os
import pytest
import grass.script as gs


@pytest.fixture
def setup_composite(tmp_path):
    """Set up GRASS session and create RGB composite map."""
    project = tmp_path / "r_composite_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        # Set the region (3 rows x 3 cols)
        gs.run_command("g.region", n=3, s=0, e=3, w=0, res=1, env=session.env)

        # Simple gradient maps for test
        gs.mapcalc("red = row() * 50", overwrite=True, env=session.env)
        gs.mapcalc("green = col() * 50", overwrite=True, env=session.env)
        gs.mapcalc("blue = 100", overwrite=True, env=session.env)

        # Create the composite map using the gradient maps for respective channels
        gs.run_command(
            "r.composite",
            red="red",
            green="green",
            blue="blue",
            output="rgb_composite",
            env=session.env,
        )

        yield session


# Test for correct composite values
def test_rgb_composite_values(setup_composite):
    """Validate the entire RGB composite map."""
    session = setup_composite

    # Get map as ASCII
    ascii_data = gs.read_command(
        "r.out.ascii", input="rgb_composite", env=session.env, flags="h"
    )

    ascii_data = [row.strip() for row in ascii_data.splitlines()]

    # Define expected RGB values row-wise
    expected_data = ["4104 4680 5000", "4100 4676 4996", "4127 4703 5023"]

    assert ascii_data == expected_data


# Test for one null row in all the color maps
def test_null_value_propagation(setup_composite):
    session = setup_composite

    # Force NULLs in the red map at second row
    # Set all maps to NULL in row 2
    gs.mapcalc(
        "red_null = if(row() == 2, null(), row() * 50)", overwrite=True, env=session.env
    )
    gs.mapcalc(
        "green_null = if(row() == 2, null(), col() * 50)",
        overwrite=True,
        env=session.env,
    )
    gs.mapcalc(
        "blue_null = if(row() == 2, null(), 100)", overwrite=True, env=session.env
    )

    # Re-run composite with modified red
    gs.run_command(
        "r.composite",
        red="red_null",
        green="green_null",
        blue="blue_null",
        output="rgb_composite_null",
        overwrite=True,
        env=session.env,
    )

    # Export to ASCII to inspect NULLs
    ascii_data = gs.read_command(
        "r.out.ascii", input="rgb_composite_null", env=session.env
    )

    # Assert that NULLs ('*') are present
    assert "*" in ascii_data, "Expected NULL values ('*') not found in composite output"
