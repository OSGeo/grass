"""Tests for rasterizing points with the dense flag

Dense mode configures its own plotting routines and never calls G_setup_plot(),
but points were always plotted with G_plot_point(), which uses the move and cont
routines that only G_setup_plot() sets. Rasterizing a point map with -d
therefore called a null function pointer and the module crashed
(https://github.com/OSGeo/grass/issues/3105).
"""

from grass.tools import Tools


def test_dense_points_match_plain_points(xy_points_session):
    """Points rasterize the same way with and without the dense flag

    The flag densifies lines, so for points there is nothing to densify and both
    runs have to produce the same cells.
    """
    tools = Tools(session=xy_points_session)

    tools.v_to_rast(input="points", output="plain", use="cat", type="point")
    tools.v_to_rast(input="points", output="dense", use="cat", type="point", flags="d")

    plain = tools.r_univar(map="plain", format="json")
    dense = tools.r_univar(map="dense", format="json")

    assert dense["n"] == 3
    assert dense["min"] == 1
    assert dense["max"] == 3
    assert dense["sum"] == 6
    assert dense["n"] == plain["n"]
    assert dense["sum"] == plain["sum"]
