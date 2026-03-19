import pytest
import grass.script as gs
from grass.pytest.assertions import raster_exists


def test_raster_exists(session):
    gs.run_command("r.mapcalc", expression="test_map = 1", env=session)

    raster_exists("test_map", env=session)
    with pytest.raises(AssertionError, match="Raster map 'missing_map' not found."):
        raster_exists("missing_map", env=session)
