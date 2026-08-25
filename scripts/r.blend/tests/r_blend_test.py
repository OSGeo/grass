"""Test of r.blend"""

import pytest

from grass.tools import Tools


@pytest.mark.parametrize("nprocs", [None, 0, 1, 2])
def test_nprocs_accepted(xy_raster_dataset_session_mapset, nprocs):
    """Check that the nprocs parameter is accepted"""
    tools = Tools(session=xy_raster_dataset_session_mapset)
    tools.r_mapcalc(expression="test_1 = 1")
    tools.r_mapcalc(expression="test_2 = 2")
    tools.r_blend(first="test_1", second="test_2", output="output", nprocs=nprocs)
