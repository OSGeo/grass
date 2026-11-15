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
