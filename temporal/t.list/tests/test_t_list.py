"""Test t.list functionality"""

import pytest

from grass.tools import Tools


@pytest.mark.needs_solo_run
def test_t_list_defaults(space_time_dataset):
    """Check that the module correctly lists datasets (strds) and maps (raster)."""
    tools = Tools(session=space_time_dataset.session)

    strds_result = tools.t_list(type="strds", columns="name")
    assert strds_result.returncode == 0

    strds_lines = [line.strip() for line in strds_result.stdout.strip().splitlines()]
    assert space_time_dataset.name in strds_lines

    # Test Map Listing (raster)
    raster_result = tools.t_list(type="raster", columns="name")

    raster_lines = [line.strip() for line in raster_result.stdout.strip().splitlines()]
    for map_name in space_time_dataset.map_names:
        assert map_name in raster_lines


@pytest.mark.needs_solo_run
def test_t_list_where_filter(space_time_dataset):
    """Check that where clause filter the output exactly."""
    tools = Tools(session=space_time_dataset.session)

    match = tools.t_list(type="strds", columns="name", where="name LIKE 'temp_%'")
    match_lines = [line.strip() for line in match.stdout.strip().splitlines()]
    assert len(match_lines) == 1
    assert match_lines[0] == space_time_dataset.name

    empty = tools.t_list(type="strds", columns="name", where="name LIKE 'land_%'")
    assert empty is None


@pytest.mark.needs_solo_run
def test_t_list_order(space_time_dataset):
    """Check that ordering the output actually sorts the data correctly."""
    tools = Tools(session=space_time_dataset.session)

    result_asc = tools.t_list(type="raster", columns="name", order="start_time")
    assert result_asc is not None

    lines_asc = [line.strip() for line in result_asc.stdout.strip().splitlines()]

    assert lines_asc == space_time_dataset.map_names
