"""Test grass.experimental.StandaloneTools class"""

from grass.experimental.standalone import StandaloneTools


def test_key_value_parser_number(xy_dataset_session):
    """Check that numbers are parsed as numbers"""
    tools = StandaloneTools(session=xy_dataset_session)
    assert tools.g_region(flags="g").keyval["nsres"] == 1
