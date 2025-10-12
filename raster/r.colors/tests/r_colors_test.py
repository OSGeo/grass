import pytest


@pytest.mark.parametrize("color", ["viridis", "grey", "srtm_percent"])
def test_set_color_table(session_tools, color):
    """Check that we can set color table (smoke test)"""
    session_tools.r_mapcalc(expression="raster = row() + col()")
    session_tools.r_colors(map="raster", color=color)
