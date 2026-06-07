"""Test t.rast3d.list functionality"""

import grass.script as gs
from grass.tools import Tools


def test_json_format_returns_valid_json(space_time_raster_dataset):
    """t.info format=json outputs valid JSON (parseable with json.loads)."""
    mapset, session = space_time_raster_dataset
    tools = Tools(session=session)
    strds_id = f"precip_abs1@{mapset}"
    expected = {
        "name": "precip_abs1",
        "mapset": mapset,
        "id": strds_id,
        "temporal_type": "absolute",
        "proj": "XY",
        "semantic_type": "mean",
        "north": 80,
        "south": 0,
        "east": 120,
        "west": 0,
        "top": 0.0,
        "bottom": 0.0,
        "number_of_maps": 3,
        "title": "A test",
        "description": "A test",
        "start_time": "2001-01-01 00:00:00",
        "end_time": "2001-04-01 00:00:00",
        "map_time": "interval",
        "nsres_min": 10,
        "nsres_max": 10,
        "ewres_min": 10,
        "ewres_max": 10,
        "min_min": 1.0,
        "min_max": 3.0,
        "max_min": 1.0,
        "max_max": 3.0,
        "aggregation_type": None,
        "dbmi_python_interface": "sqlite3",
    }
    info = tools.t_info(format="json", type="strds", input=strds_id).json
    assert isinstance(info, dict)
    assert info | expected == info
    assert {
        "creation_time",
        "creator",
        "command",
        "modification_time",
        "sql_template_path",
        "tgis_db",
        "dbmi_string",
    } <= set(info.keys())


def test_json_shell_parity(space_time_raster_dataset):
    """JSON output contains all keys present in shell format output."""
    # Get shell format output
    mapset, session = space_time_raster_dataset
    strds_id = f"precip_abs1@{mapset}"
    tools = Tools(session=session)

    shell_info = tools.t_info(format="shell", type="strds", input=strds_id).text

    # Parse shell output to get keys
    shell_dict = gs.parse_key_val(shell_info)
    assert isinstance(shell_dict, dict)
    shell_keys = set(shell_dict.keys())

    # Get JSON output
    json_info = tools.t_info(format="json", type="strds", input=strds_id).json
    assert isinstance(json_info, dict)
    json_keys = set(json_info.keys())

    # JSON should have all the keys that shell has
    assert shell_keys <= json_keys


def test_shell_format_via_option(space_time_raster_dataset):
    """format=shell works as equivalent to -g flag."""
    mapset, session = space_time_raster_dataset
    strds_id = f"precip_abs1@{mapset}"
    tools = Tools(session=session)

    flag_info = tools.t_info(flags="g", type="strds", input=strds_id).text
    option_info = tools.t_info(format="shell", type="strds", input=strds_id).text

    # Both should produce the same output
    assert flag_info == option_info


def test_json_format_vs_shell(space_time_raster_dataset):
    """Test that format=json works equivalent format=shell (earlier -g flag)."""
    mapset, session = space_time_raster_dataset
    strds_id = f"precip_abs1@{mapset}"
    tools = Tools(session=session)

    shell_info = tools.t_info(format="shell", type="strds", input=strds_id).text
    shell_dict = gs.parse_key_val(shell_info)

    json_dict = tools.t_info(format="json", type="strds", input=strds_id).json

    # Both should produce the same output
    for dict_key, dict_val in shell_dict.items():
        sanetized_dict_val = dict_val.strip("'")
        assert dict_key in json_dict
        assert str(json_dict[dict_key]) == sanetized_dict_val


def test_plain_format_still_works(space_time_raster_dataset):
    """Default plain format output still functions correctly."""
    mapset, session = space_time_raster_dataset
    strds_id = f"precip_abs1@{mapset}"
    tools = Tools(session=session)
    info = tools.t_info(format="plain", type="strds", input=strds_id).text
    assert "precip_abs1" in info
