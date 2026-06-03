"""Tests of r.semantic.label"""

import pytest

from grass.exceptions import CalledModuleError
from grass.tools import Tools


def test_print_missing_map_is_error(xy_raster_dataset_session_mapset):
    """operation=print on a missing map must fail, like add and remove do.

    Regression test: print previously printed an error but exited successfully,
    while add/remove exit with an error for the same missing-map condition.
    """
    tools = Tools(session=xy_raster_dataset_session_mapset)
    with pytest.raises(CalledModuleError):
        tools.r_semantic_label(operation="print", map="doesnotexist")


def test_print_existing_map_succeeds(xy_raster_dataset_session_mapset):
    """operation=print on an existing map without a label still succeeds."""
    tools = Tools(session=xy_raster_dataset_session_mapset)
    tools.r_mapcalc(expression="rast = 1")
    tools.r_semantic_label(operation="print", map="rast")
