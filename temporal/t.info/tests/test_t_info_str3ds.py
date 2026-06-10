"""Test t.rast3d.list functionality"""

import grass.script as gs
from grass.tools import Tools


def test_json_format_returns_valid_json(space_time_raster3d_dataset):
    """t.info format=json outputs valid JSON (parseable with json.loads)."""
    mapset, session = space_time_raster3d_dataset
    tools = Tools(session=session)
    str3ds_id = f"volume_abs1@{mapset}"
    expected = {
        "aggregation_type": None,
        "bottom": 0.0,
        "description": "A test with sequential files",
        "east": 120.0,
        "end_time": "2001-07-01 00:00:00",
        "ewres_max": 10.0,
        "ewres_min": 10.0,
        "granularity": "1 month",
        "id": f"volume_abs1@{mapset}",
        "map_time": "interval",
        "mapset": "tinfo_mapset",
        "max_max": 647.0,
        "max_min": 298.0,
        "min_max": 2.0,
        "min_min": 0.0,
        "name": "volume_abs1",
        "north": 80.0,
        "nsres_max": 10.0,
        "nsres_min": 10.0,
        "number_of_maps": 6,
        "proj": "XY",
        "semantic_type": "mean",
        "south": 0.0,
        "start_time": "2001-01-01 00:00:00",
        "tbres_max": 10.0,
        "tbres_min": 10.0,
        "temporal_type": "absolute",
        "title": "A test with sequential files",
        "top": 50.0,
        "west": 0.0,
    }

    info = tools.t_info(format="json", type="str3ds", input=str3ds_id).json
    assert isinstance(info, dict)
    assert info | expected == info


def test_json_shell_parity(space_time_raster3d_dataset):
    """JSON output contains all keys present in shell format output."""
    # Get shell format output
    mapset, session = space_time_raster3d_dataset
    str3ds_id = f"volume_abs1@{mapset}"
    tools = Tools(session=session)

    shell_info = tools.t_info(format="shell", type="str3ds", input=str3ds_id).text

    # Parse shell output to get keys
    shell_dict = gs.parse_key_val(shell_info)
    assert isinstance(shell_dict, dict)
    shell_keys = set(shell_dict.keys())

    # Get JSON output
    json_info = tools.t_info(format="json", type="str3ds", input=str3ds_id).json
    assert isinstance(json_info, dict)
    json_keys = set(json_info.keys())

    # JSON should have all the keys that shell has
    assert shell_keys <= json_keys


def test_shell_format_via_option(space_time_raster3d_dataset):
    """format=shell works as equivalent to -g flag."""
    mapset, session = space_time_raster3d_dataset
    str3ds_id = f"volume_abs1@{mapset}"
    tools = Tools(session=session)

    flag_info = tools.t_info(flags="g", type="str3ds", input=str3ds_id).text
    option_info = tools.t_info(format="shell", type="str3ds", input=str3ds_id).text

    # Both should produce the same output
    assert flag_info == option_info


def test_json_format_vs_shell(space_time_raster3d_dataset):
    """Test that format=json works equivalent format=shell (earlier -g flag)."""
    mapset, session = space_time_raster3d_dataset
    str3ds_id = f"volume_abs1@{mapset}"
    tools = Tools(session=session)

    shell_info = tools.t_info(format="shell", type="str3ds", input=str3ds_id).text
    shell_dict = gs.parse_key_val(shell_info)

    json_dict = tools.t_info(format="json", type="str3ds", input=str3ds_id).json

    # Both should produce the same output
    for dict_key, dict_val in shell_dict.items():
        sanetized_dict_val = dict_val.strip("'")
        assert dict_key in json_dict
        assert str(json_dict[dict_key]) == sanetized_dict_val


def test_plain_format_still_works(space_time_raster3d_dataset):
    """Default plain format output still functions correctly."""
    mapset, session = space_time_raster3d_dataset
    str3ds_id = f"volume_abs1@{mapset}"
    tools = Tools(session=session)
    info = tools.t_info(format="plain", type="str3ds", input=str3ds_id).text
    assert "volume_abs1" in info


def test_t_info_map_dataset(space_time_raster3d_dataset):
    """Raster3D map metadata is returned correctly."""
    mapset, session = space_time_raster3d_dataset
    stds_name = "volume_abs1"
    map_name = "vol_1"
    expected = {
        "bottom": 0.0,
        "cols": 8,
        "datatype": "DCELL",
        "depths": 5,
        "east": 120.0,
        "end_time": "2001-02-01 00:00:00",
        "ewres": 10.0,
        "id": f"{map_name}@{mapset}",
        "mapset": mapset,
        "max": 549.0,
        "min": 0.0,
        "name": "vol_1",
        "north": 80.0,
        "nsres": 10.0,
        "number_of_cells": 480,
        "proj": "XY",
        "registered_datasets": [
            f"{stds_name}@{mapset}",
        ],
        "rows": 12,
        "south": 0.0,
        "start_time": "2001-01-01 00:00:00",
        "tbres": 10.0,
        "temporal_type": "absolute",
        "top": 50.0,
        "west": 0.0,
    }

    tools = Tools(session=session)
    json_info = tools.t_info(format="json", type="raster_3d", input=map_name).json

    expected_keys = {
        "creation_time",
        "creator",
    }

    assert json_info | expected == json_info
    assert expected_keys.issubset(set(json_info.keys()))
