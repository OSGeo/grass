"""Tests for rasterizing points with the dense flag

Dense mode configures its own plotting routines and never calls G_setup_plot(),
but points were always plotted with G_plot_point(), which uses the move and cont
routines that only G_setup_plot() sets. Rasterizing a point map with -d
therefore called a null function pointer and the module crashed
(https://github.com/OSGeo/grass/issues/3105).
"""

from grass.tools import Tools


def test_dense_points_match_plain_points(xy_points_session):
    """Points rasterize into the same cells with and without the dense flag

    The flag densifies lines, so for points there is nothing to densify and
    both runs have to place every point in the same cell. The coordinates in
    the fixture sit inside cells rather than on their edges, so a half cell
    shift in either direction changes the row or the column and fails here.
    """
    tools = Tools(session=xy_points_session)

    tools.v_to_rast(input="points", output="plain", use="cat", type="point")
    tools.v_to_rast(input="points", output="dense", use="cat", type="point", flags="d")

    plain = tools.r_stats(input="plain", flags="gn").text.splitlines()
    dense = tools.r_stats(input="dense", flags="gn").text.splitlines()

    # east, north and category of the centre of the cell each point landed in
    assert sorted(plain) == ["10.5 10.5 1", "20.5 20.5 2", "30.5 15.5 3"]
    assert sorted(dense) == sorted(plain)
