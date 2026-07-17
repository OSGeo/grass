"""Tests of r.plane"""

import pytest

from grass.tools import Tools


def test_dip45_azimuth90_gives_z_equal_easting(xy_raster_dataset_session_mapset):
    """A 45-degree plane dipping east through the origin satisfies z == easting."""
    tools = Tools(session=xy_raster_dataset_session_mapset)
    tools.g_region(n=2, s=0, w=0, e=2, res=1)
    tools.r_plane(
        output="plane",
        dip=45,
        azimuth=90,
        easting=0,
        northing=0,
        elevation=0,
        type="DCELL",
    )
    cells = tools.r_stats(input="plane", flags="gn", format="json").json
    assert cells
    for cell in cells:
        assert cell["categories"][0]["category"] == pytest.approx(cell["east"])


def test_dip0_is_constant_elevation(xy_raster_dataset_session_mapset):
    """A horizontal plane (dip=0) equals the given elevation everywhere."""
    tools = Tools(session=xy_raster_dataset_session_mapset)
    tools.g_region(n=2, s=0, w=0, e=2, res=1)
    tools.r_plane(
        output="flat",
        dip=0,
        azimuth=0,
        easting=0,
        northing=0,
        elevation=100,
        type="DCELL",
    )
    cells = tools.r_stats(input="flat", flags="gn", format="json").json
    assert cells
    for cell in cells:
        assert cell["categories"][0]["category"] == pytest.approx(100)
