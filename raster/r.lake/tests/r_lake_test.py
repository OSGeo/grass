import os

import pytest

import grass.script as gs
from grass.tools import Tools, ToolError


@pytest.fixture
def session_with_dem(tmp_path):
    """A GRASS session with a DEM holding two separate low areas.

    dem: a basin in the south-east corner (rows 15-20, cols 15-20) at
    elevation 5, a second low area in the north-west corner (rows 1-6,
    cols 1-6) at elevation 8, and a ridge at elevation 50 separating them.
    seedmap: a single seed cell inside the south-east basin.

    Only the south-east basin is seeded, so with water_level=10 only it
    should fill. The north-west area is below the water level but is not
    reachable from the seed.
    """
    project = tmp_path / "r_lake_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=20, s=0, e=20, w=0, res=1)
        tools.r_mapcalc(
            expression=(
                "dem = if(row() >= 15 && col() >= 15, 5, "
                "if(row() <= 6 && col() <= 6, 8, 50))"
            )
        )
        tools.r_mapcalc(
            expression="seedmap = if(row() == 18 && col() == 18, 1, null())"
        )
        yield session


def test_seed_raster_fills_only_the_seeded_basin(session_with_dem):
    """A seed raster fills only areas connected to a seed cell."""
    tools = Tools(session=session_with_dem)

    tools.r_lake(elevation="dem", seed="seedmap", water_level=10, lake="lake_out")

    # The seeded basin is 6x6 cells and nothing else should be wet.
    stats = tools.r_univar(map="lake_out", format="json").json
    assert stats["n"] == 36


def test_seed_raster_leaves_unseeded_low_area_dry(session_with_dem):
    """A low area not connected to any seed stays dry."""
    tools = Tools(session=session_with_dem)

    tools.r_lake(elevation="dem", seed="seedmap", water_level=10, lake="lake_out")

    # Cell in the middle of the unseeded north-west low area.
    queried = tools.r_what(map="lake_out", coordinates=(2.5, 17.5), format="json").json
    assert queried[0]["lake_out"]["value"] is None


def test_seed_raster_produces_a_single_water_body(session_with_dem):
    """The result contains exactly one water body when one seed is given."""
    tools = Tools(session=session_with_dem)

    tools.r_lake(elevation="dem", seed="seedmap", water_level=10, lake="lake_out")
    tools.r_clump(input="lake_out", output="clumps")

    info = tools.r_info(map="clumps", format="json").json
    assert info["max"] == 1


@pytest.fixture
def session_with_flat_terrain(tmp_path):
    """A GRASS session with flat terrain below the water level.

    The region is 50 by 50 cells with n=50, s=0, e=50, w=0 and res=1, so the
    valid cell indices are rows 0 to 49 and columns 0 to 49.
    """
    project = tmp_path / "r_lake_seed_project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tools = Tools(session=session)
        tools.g_region(n=50, s=0, e=50, w=0, res=1)
        tools.r_mapcalc(expression="terrain = 5")
        yield session


@pytest.mark.parametrize(
    "coordinates",
    [(25.5, 50.5), (25.5, 50.9), (-0.5, 25.5), (-0.9, 25.5)],
    ids=["north", "north-almost-one-cell", "west", "west-almost-one-cell"],
)
def test_seed_coordinates_less_than_a_cell_outside_are_rejected(
    session_with_flat_terrain, coordinates
):
    """Coordinates north or west of the region are outside it.

    Truncating the converted index towards zero turns the whole interval
    between -1 and 0 into row or column 0, which hides these coordinates from
    the check below zero.
    """
    tools = Tools(session=session_with_flat_terrain)

    with pytest.raises(ToolError) as error:
        tools.r_lake(
            elevation="terrain",
            water_level=10,
            lake="lake_out",
            coordinates=coordinates,
        )

    assert error.value.returncode == 1
    assert "Seed point outside the current region" in error.value.errors


@pytest.mark.parametrize("coordinates", [(25.5, 50), (0, 25.5)], ids=["north", "west"])
def test_seed_coordinates_on_north_and_west_edge_are_accepted(
    session_with_flat_terrain, coordinates
):
    """The northern and western edges belong to the first row and column."""
    tools = Tools(session=session_with_flat_terrain)

    tools.r_lake(
        elevation="terrain", water_level=10, lake="lake_out", coordinates=coordinates
    )

    stats = tools.r_univar(map="lake_out", format="json").json
    assert stats["n"] == 2500
