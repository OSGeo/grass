import pytest

from grass.tools import Tools


def test_all_null_map(session_with_maps):
    """A map with only null cells has no minimum and maximum."""
    tools = Tools(session=session_with_maps)
    stats = tools.r_univar(map="all_null", format="json", nprocs=1)
    assert (stats["n"], stats["min"], stats["max"]) == (0, None, None)


def test_zone_with_only_null_cells(session_with_maps):
    """A zone with only null cells has no minimum and maximum."""
    tools = Tools(session=session_with_maps)
    stats = tools.r_univar(map="data", zones="zones", format="json", nprocs=1)
    assert [(zone["zone"], zone["n"], zone["min"], zone["max"]) for zone in stats] == [
        (1, 50, 1, 10),
        (2, 0, None, None),
    ]


@pytest.mark.parametrize(
    ("name", "value"),
    [
        ("dbl_max", 1.7976931348623157e308),
        ("minus_dbl_max", -1.7976931348623157e308),
        ("dbl_min", 2.2250738585072014e-308),
    ],
)
def test_extreme_value(session_with_maps, name, value):
    """A map of a single extreme value has it as its minimum and maximum."""
    tools = Tools(session=session_with_maps)
    stats = tools.r_univar(map=name, format="json", nprocs=1)
    assert (stats["min"], stats["max"]) == (value, value)
