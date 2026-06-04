"""Test t.rast3d.list functionality"""

from grass.tools import Tools


def test_t_rast3d_list_defaults(space_time_raster3d_dataset):
    """Check that the module correctly lists 3D raster maps."""
    tools = Tools(session=space_time_raster3d_dataset.session)

    result = tools.t_rast3d_list(input=space_time_raster3d_dataset.name)
    assert result.returncode == 0

    lines = [line.strip().split("|") for line in result.stdout.strip().splitlines()]
    header = lines[0]
    data_lines = [row[0] for row in lines[1:]]

    assert header == ["name", "mapset", "start_time", "end_time"]

    for map_name in space_time_raster3d_dataset.raster_names:
        assert map_name in data_lines


def test_t_rast3d_list_where_filter(space_time_raster3d_dataset):
    """Check that the where clause filters the 3D map output exactly."""
    tools = Tools(session=space_time_raster3d_dataset.session)

    result = tools.t_rast3d_list(
        input=space_time_raster3d_dataset.name, columns="name", where="name = 'vol_1'"
    )
    assert result.returncode == 0

    lines = [line.strip() for line in result.stdout.strip().splitlines()]
    data_lines = lines[1:]

    assert len(data_lines) == 1
    assert data_lines[0] == "vol_1"


def test_t_rast3d_list_order(space_time_raster3d_dataset):
    """Check that ordering the output sorts the 3D map data correctly."""
    tools = Tools(session=space_time_raster3d_dataset.session)

    result_asc = tools.t_rast3d_list(
        input=space_time_raster3d_dataset.name, columns="name", order="start_time"
    )
    assert result_asc.returncode == 0

    lines_asc = [line.strip() for line in result_asc.stdout.strip().splitlines()]
    data_lines = lines_asc[1:]

    assert data_lines == space_time_raster3d_dataset.raster_names
