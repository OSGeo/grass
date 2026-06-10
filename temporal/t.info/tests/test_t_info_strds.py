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
    }
    expected_tgis_info_keys = {
        "creation_time",
        "dbmi_python_interface",
        "dbmi_string",
        "sql_template_path",
        "tgis_db_version",
        "tgis_version",
    }
    info = tools.t_info(format="json", type="strds", input=strds_id).json
    assert isinstance(info, dict)
    assert info | expected == info
    assert set(info.keys()).issuperset(
        {
            "creation_time",
            "creator",
            "command",
            "modification_time",
            "tgis_db",
        }
    )
    assert expected_tgis_info_keys == set(info["tgis_db"].keys())


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

    for shell_key in shell_keys:
        assert shell_dict[shell_key].strip("'") == str(json_info[shell_key])


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


def test_t_info_dh_flags(space_time_raster_dataset):
    """Default plain format output still functions correctly."""
    mapset, session = space_time_raster_dataset
    strds_id = f"precip_abs1@{mapset}"
    tools = Tools(session=session)
    json_info = tools.t_info(format="json", type="strds", input=strds_id).json
    system_info = tools.t_info(
        flags="d", format="shell", type="strds", input=strds_id
    ).text
    system_info_dict = gs.parse_key_val(system_info)
    system_info_dict = {k: v.strip("'") for k, v in system_info_dict.items()}
    json_info["tgis_db"] = {k: str(v) for k, v in json_info["tgis_db"].items()}
    assert json_info["tgis_db"] | system_info_dict == json_info["tgis_db"]
    history_info = tools.t_info(
        flags="h", format="shell", type="strds", input=strds_id
    ).text
    for line in json_info["command"].split():
        assert line in history_info


def test_t_info_map_dataset(space_time_raster_dataset):
    """Raster map metadata is returned correctly."""
    mapset, session = space_time_raster_dataset
    stds_name = "precip_abs1"
    map_name = "prec_1"
    expected = {
        "bottom": 0.0,
        "cols": 8,
        "datatype": "CELL",
        "east": 120.0,
        "end_time": "2001-02-01 00:00:00",
        "ewres": 10.0,
        "id": f"{map_name}@{mapset}",
        "mapset": mapset,
        "max": 1.0,
        "min": 1.0,
        "name": map_name,
        "north": 80.0,
        "nsres": 10.0,
        "number_of_cells": 96,
        "proj": "XY",
        "registered_datasets": [
            f"{stds_name}@{mapset}",
        ],
        "rows": 12,
        "south": 0.0,
        "start_time": "2001-01-01 00:00:00",
        "temporal_type": "absolute",
        "top": 0.0,
        "west": 0.0,
    }

    tools = Tools(session=session)
    json_info = tools.t_info(format="json", type="raster", input=map_name).json

    expected_keys = {
        "creation_time",
        "creator",
    }

    assert json_info | expected == json_info
    assert expected_keys.issubset(set(json_info.keys()))
