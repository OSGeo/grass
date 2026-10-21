from grass.tools import Tools


def test_all_null_map(session_with_maps):
    """A map with only null cells has no minimum and maximum."""
    tools = Tools(session=session_with_maps)
    stats = tools.r_univar(map="all_null", format="json").json
    assert (stats["n"], stats["min"], stats["max"]) == (0, None, None)


def test_zone_with_only_null_cells(session_with_maps):
    """A zone with only null cells has no minimum and maximum."""
    tools = Tools(session=session_with_maps)
    stats = tools.r_univar(map="data", zones="zones", format="json").json
    assert [(zone["zone"], zone["n"], zone["min"], zone["max"]) for zone in stats] == [
        (1, 50, 1, 10),
        (2, 0, None, None),
    ]
