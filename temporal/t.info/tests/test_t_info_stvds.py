# SPDX-License-Identifier: GPL-2.0-or-later
"""Test t.info with space time vector datasets and vector maps

The expected values capture the output of t.info as it is now, so that later
changes to the interface can be checked against the established behavior.
"""

import grass.script as gs
from grass.tools import Tools

# Keys whose values differ between runs, systems, or users. The spatial extent
# of the randomly generated vector maps is not fixed either.
VOLATILE_KEYS = (
    "creation_time",
    "modification_time",
    "creator",
    "vector_register",
    "north",
    "south",
    "east",
    "west",
)

STVDS_SHELL_KEYS = [
    "id",
    "name",
    "mapset",
    "creator",
    "temporal_type",
    "creation_time",
    "modification_time",
    "semantic_type",
    "start_time",
    "end_time",
    "granularity",
    "map_time",
    "north",
    "south",
    "east",
    "west",
    "top",
    "bottom",
    "vector_register",
    "points",
    "lines",
    "boundaries",
    "centroids",
    "faces",
    "kernels",
    "primitives",
    "nodes",
    "areas",
    "islands",
    "holes",
    "volumes",
    "number_of_maps",
]

VECTOR_SHELL_KEYS = [
    "id",
    "name",
    "mapset",
    "creator",
    "temporal_type",
    "creation_time",
    "start_time",
    "end_time",
    "north",
    "south",
    "east",
    "west",
    "top",
    "bottom",
    "is_3d",
    "points",
    "lines",
    "boundaries",
    "centroids",
    "faces",
    "kernels",
    "primitives",
    "nodes",
    "areas",
    "islands",
    "holes",
    "volumes",
    "registered_datasets",
]


def shell_info(tools, **kwargs):
    """Return the shell style output of t.info parsed into a dictionary"""
    return gs.parse_key_val(tools.t_info(flags="g", **kwargs).text)


def stable_items(info):
    """Return the parsed shell output without the keys which cannot be compared"""
    return {key: value for key, value in info.items() if key not in VOLATILE_KEYS}


def test_shell_keys(temporal_data):
    """Shell output has the expected keys in the expected order"""
    tools = Tools(session=temporal_data.session)
    info = shell_info(tools, type="stvds", input="vect_abs1")

    assert list(info.keys()) == STVDS_SHELL_KEYS


def test_shell_values(temporal_data):
    """Shell output reports the metadata of the dataset"""
    tools = Tools(session=temporal_data.session)
    info = shell_info(tools, type="stvds", input="vect_abs1")

    assert stable_items(info) == {
        "id": f"vect_abs1@{temporal_data.mapset}",
        "name": "vect_abs1",
        "mapset": temporal_data.mapset,
        "temporal_type": "absolute",
        "semantic_type": "mean",
        "start_time": "'2001-01-01 00:00:00'",
        "end_time": "'2001-03-01 00:00:00'",
        "granularity": "'1 month'",
        "map_time": "interval",
        "top": "0.0",
        "bottom": "0.0",
        "points": "40",
        "lines": "0",
        "boundaries": "0",
        "centroids": "0",
        "faces": "0",
        "kernels": "0",
        "primitives": "40",
        "nodes": "0",
        "areas": "0",
        "islands": "0",
        "holes": "0",
        "volumes": "0",
        "number_of_maps": "2",
    }
    assert info["vector_register"].startswith("vector_map_register_")


def test_plain_info(temporal_data):
    """Plain output reports the metadata in a human readable table"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(type="stvds", input="vect_abs1").text

    assert "Space Time Vector Dataset" in text
    assert f"Id: ........................ vect_abs1@{temporal_data.mapset}" in text
    assert "Granularity:................ 1 month" in text
    assert "Number of registered maps:.. 2" in text
    assert "Title:" in text
    assert "Command history:" in text


def test_vector_map_shell(temporal_data):
    """Shell output for a registered vector map reports the map metadata"""
    tools = Tools(session=temporal_data.session)
    info = shell_info(tools, type="vector", input="vect_1")

    assert list(info.keys()) == VECTOR_SHELL_KEYS
    assert stable_items(info) == {
        "id": f"vect_1@{temporal_data.mapset}",
        "name": "vect_1",
        "mapset": temporal_data.mapset,
        "temporal_type": "absolute",
        "start_time": "'2001-01-01 00:00:00'",
        "end_time": "'2001-02-01 00:00:00'",
        "top": "0.0",
        "bottom": "0.0",
        "is_3d": "0",
        "points": "20",
        "lines": "0",
        "boundaries": "0",
        "centroids": "0",
        "faces": "0",
        "kernels": "0",
        "primitives": "20",
        "nodes": "0",
        "areas": "0",
        "islands": "0",
        "holes": "0",
        "volumes": "0",
        "registered_datasets": f"vect_abs1@{temporal_data.mapset}",
    }


def test_vector_map_plain(temporal_data):
    """Plain output for a registered vector map reports the map metadata"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(type="vector", input="vect_1").text

    assert "Vector Dataset" in text
    assert "Number of points ........... 20" in text
    assert f"Registered datasets ........ vect_abs1@{temporal_data.mapset}" in text


def test_json_format_returns_valid_json(temporal_data):
    """t.info format=json outputs valid JSON (parseable with json.loads)."""
    tools = Tools(session=temporal_data.session)
    stvds_id = f"vect_abs1@{temporal_data.mapset}"
    expected = {
        "name": "vect_abs1",
        "mapset": temporal_data.mapset,
        "id": stvds_id,
        "temporal_type": "absolute",
        "proj": "XY",
        "semantic_type": "mean",
        "number_of_maps": 2,
        "title": "A vector test",
        "description": "A vector test",
        "start_time": "2001-01-01 00:00:00",
        "end_time": "2001-03-01 00:00:00",
        "faces": 0,
        "points": 40,
        "nodes": 0,
        "holes": 0,
        "islands": 0,
        "kernels": 0,
        "lines": 0,
        "boundaries": 0,
        "centroids": 0,
        "areas": 0,
        "primitives": 40,
        "volumes": 0,
        "map_time": "interval",
        "top": 0.0,
        "bottom": 0.0,
    }
    info = tools.t_info(format="json", type="stvds", input=stvds_id).json
    assert isinstance(info, dict)
    assert info | expected == info
    assert {
        "north",
        "south",
        "east",
        "west",
        "creation_time",
        "creator",
        "command",
        "modification_time",
        "tgis_db",
    } <= set(info.keys())
    expected_tgis_info_keys = {
        "creation_time",
        "dbmi_python_interface",
        "dbmi_string",
        "sql_template_path",
        "tgis_db_version",
        "tgis_version",
    }
    assert expected_tgis_info_keys <= set(info["tgis_db"].keys())


def test_json_shell_parity(temporal_data):
    """JSON output contains all keys present in shell format output."""
    tools = Tools(session=temporal_data.session)
    stvds_id = f"vect_abs1@{temporal_data.mapset}"

    shell_info = tools.t_info(format="shell", type="stvds", input=stvds_id).text

    # Parse shell output to get keys
    shell_dict = gs.parse_key_val(shell_info)
    assert isinstance(shell_dict, dict)
    shell_keys = set(shell_dict.keys())

    # Get JSON output
    json_info = tools.t_info(format="json", type="stvds", input=stvds_id).json
    assert isinstance(json_info, dict)
    json_keys = set(json_info.keys())

    # JSON should have all the keys that shell has
    assert shell_keys <= json_keys


def test_shell_format_via_option(temporal_data):
    """format=shell works as equivalent to -g flag."""
    tools = Tools(session=temporal_data.session)
    stvds_id = f"vect_abs1@{temporal_data.mapset}"

    flag_info = tools.t_info(flags="g", type="stvds", input=stvds_id).text
    option_info = tools.t_info(format="shell", type="stvds", input=stvds_id).text

    # Both should produce the same output
    assert flag_info == option_info


def test_json_format_vs_shell(temporal_data):
    """Test that format=json works equivalent format=shell (earlier -g flag)."""
    tools = Tools(session=temporal_data.session)
    stvds_id = f"vect_abs1@{temporal_data.mapset}"

    shell_info = tools.t_info(format="shell", type="stvds", input=stvds_id).text
    shell_dict = gs.parse_key_val(shell_info)

    json_dict = tools.t_info(format="json", type="stvds", input=stvds_id).json

    # Both should produce the same output
    for dict_key, dict_val in shell_dict.items():
        sanitized_value = dict_val.strip("'")
        assert dict_key in json_dict
        assert str(json_dict[dict_key]) == sanitized_value


def test_plain_format_still_works(temporal_data):
    """Default plain format output still functions correctly."""
    tools = Tools(session=temporal_data.session)
    stvds_id = f"vect_abs1@{temporal_data.mapset}"
    info = tools.t_info(format="plain", type="stvds", input=stvds_id).text
    assert "vect_abs1" in info


def test_t_info_dh_flags(temporal_data):
    """Flags -d and -h work and are included in JSON."""
    mapset, session = temporal_data.mapset, temporal_data.session
    stds_id = f"vect_abs1@{mapset}"
    tools = Tools(session=session)
    json_info = tools.t_info(format="json", type="stvds", input=stds_id).json
    system_info = tools.t_info(
        flags="d", format="shell", type="stvds", input=stds_id
    ).text
    system_info_dict = gs.parse_key_val(system_info)
    system_info_dict = {k: v.strip("'") for k, v in system_info_dict.items()}
    json_info["tgis_db"] = {k: str(v) for k, v in json_info["tgis_db"].items()}
    assert json_info["tgis_db"] | system_info_dict == json_info["tgis_db"]
    history_info = tools.t_info(
        flags="h", format="shell", type="stvds", input=stds_id
    ).text
    for line in json_info["command"].split():
        assert line in history_info


def test_t_info_map_dataset(temporal_data):
    """Vector map metadata is returned correctly."""
    mapset, session = temporal_data.mapset, temporal_data.session
    map_name = "vect_1"
    stds_name = "vect_abs1"
    expected = {
        "areas": 0,
        "bottom": 0.0,
        "boundaries": 0,
        "centroids": 0,
        "end_time": "2001-02-01 00:00:00",
        "faces": 0,
        "holes": 0,
        "id": f"{map_name}@{mapset}",
        "is_3d": 0,
        "islands": 0,
        "kernels": 0,
        "layer": None,
        "lines": 0,
        "mapset": mapset,
        "name": map_name,
        "nodes": 0,
        "points": 20,
        "primitives": 20,
        "proj": "XY",
        "registered_datasets": [
            f"{stds_name}@{mapset}",
        ],
        "start_time": "2001-01-01 00:00:00",
        "temporal_type": "absolute",
        "top": 0.0,
        "volumes": 0,
    }

    tools = Tools(session=session)
    json_info = tools.t_info(format="json", type="vector", input=map_name).json

    expected_keys = {
        "creation_time",
        "creator",
        "north",
        "south",
        "west",
        "east",
    }

    assert json_info | expected == json_info
    assert expected_keys.issubset(set(json_info.keys()))
