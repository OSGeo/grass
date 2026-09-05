import os

import pytest

import grass.script as gs
from grass.tools import Tools


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
