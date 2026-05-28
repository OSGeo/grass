import os
import pytest
import grass.script as gs


@pytest.fixture
def setup_maps(tmp_path):
    """Set up a GRASS session and create test raster maps."""

    project = tmp_path / "r_covar_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        # Set the region (3 rows x 3 cols)
        gs.run_command(
            "g.region",
            n=3,
            s=0,
            e=3,
            w=0,
            res=1,
            env=session.env,
        )

        # Create test maps
        # map1 = 1, 2, 3
        gs.mapcalc(
            "map1 = if(row() == 1, 1, if(row() == 2, 2, if(row() == 3, 3, null())))",
            overwrite=True,
            env=session.env,
        )
        # map2 = 1, null, 4
        gs.mapcalc(
            "map2 = if(row() == 1, 1, if(row() == 2, null(), if(row() == 3, 4, null())))",
            overwrite=True,
            env=session.env,
        )
        # map3 = 1, 2.5, 5
        gs.mapcalc(
            "map3 = if(row() == 1, 1, if(row() == 2, 2.5, if(row() == 3, 5, null())))",
            overwrite=True,
            env=session.env,
        )

        yield session


def clean_output(output):
    """Normalize output by stripping whitespace from each line."""
    return "\n".join([line.strip() for line in output.split("\n")])


def test_covar_basic(setup_maps):
    """Test basic covariance calculation with custom maps having different data types."""
    session = setup_maps
    output = gs.read_command(
        "r.covar",
        map="map1,map2,map3",
        env=session.env,
    )
    cleaned_output = clean_output(output)

    expected = """N = 6
1.200000 1.800000 2.400000
1.800000 2.700000 3.600000
2.400000 3.600000 4.800000
"""
    assert cleaned_output == expected


def test_corr_basic(setup_maps):
    """Test correlation calculation with -r flag"""
    session = setup_maps
    output = gs.read_command(
        "r.covar",
        map="map1,map3",
        flags="r",
        env=session.env,
    )
    cleaned_output = clean_output(output)

    expected = """N = 9
1.000000 0.989743
0.989743 1.000000
"""
    assert cleaned_output == expected


def test_zero_var(setup_maps):
    """Test correlation with identical maps"""
    session = setup_maps
    output = gs.read_command(
        "r.covar",
        map="map1,map1",
        flags="r",
        env=session.env,
    )
    cleaned_output = clean_output(output)

    expected = """N = 9
1.000000 1.000000
1.000000 1.000000
"""
    assert cleaned_output == expected
