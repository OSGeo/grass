"""pytest test fixtures for r.timestamp"""

import pytest


@pytest.fixture
def raster(session_tools):
    """A fresh raster map with no timestamp, in the current mapset"""
    name = "raster"
    session_tools.r_mapcalc(expression=f"{name} = 1")
    return name
