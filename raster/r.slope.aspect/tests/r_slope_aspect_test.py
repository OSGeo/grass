from grass.tools import Tools


def test_repeated_output(xy_dataset_session):
    """Check behavior when two outputs have the same name

    This is practically undefined behavior, but here, we test the current behavior
    which is that one of the outputs wins (regardless of the order of the provided
    parameters or other in the interface).
    """
    tools = Tools(session=xy_dataset_session, consistent_return_value=True)
    name = "output_1"

    tools.r_slope_aspect(elevation="rows_raster", aspect=name, slope=name)
    stats = tools.r_univar(map=name, format="json")
    # This is slope.
    assert stats["min"] == 45
    assert stats["max"] == 45
    assert stats["sum"] == 540

    name = "output_2"
    tools.r_slope_aspect(elevation="rows_raster", dx=name, dy=name, slope=name)
    stats = tools.r_univar(map=name, format="json")
    # This is dy.
    assert stats["min"] == 1
    assert stats["max"] == 1
    assert stats["sum"] == 12
