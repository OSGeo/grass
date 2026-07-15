# SPDX-License-Identifier: GPL-2.0-or-later
"""Test t.info with space time 3D raster datasets and 3D raster maps

The expected values capture the output of t.info as it is now, so that later
changes to the interface can be checked against the established behavior.
"""

import grass.script as gs
from grass.tools import Tools

# Keys whose values differ between runs, systems, or users.
VOLATILE_KEYS = ("creation_time", "modification_time", "creator", "raster3d_register")

STR3DS_SHELL_KEYS = [
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
    "raster3d_register",
    "tbres_min",
    "tbres_max",
    "nsres_min",
    "nsres_max",
    "ewres_min",
    "ewres_max",
    "min_min",
    "min_max",
    "max_min",
    "max_max",
    "aggregation_type",
    "number_of_maps",
]

RASTER3D_SHELL_KEYS = [
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
    "datatype",
    "cols",
    "rows",
    "number_of_cells",
    "nsres",
    "ewres",
    "min",
    "max",
    "depths",
    "tbres",
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
    info = shell_info(tools, type="str3ds", input="volume_abs1")

    assert list(info.keys()) == STR3DS_SHELL_KEYS


def test_shell_values(temporal_data):
    """Shell output reports the metadata of the dataset"""
    tools = Tools(session=temporal_data.session)
    info = shell_info(tools, type="str3ds", input="volume_abs1")

    assert stable_items(info) == {
        "id": f"volume_abs1@{temporal_data.mapset}",
        "name": "volume_abs1",
        "mapset": temporal_data.mapset,
        "temporal_type": "absolute",
        "semantic_type": "mean",
        "start_time": "'2001-01-01 00:00:00'",
        "end_time": "'2001-03-01 00:00:00'",
        "granularity": "'1 month'",
        "map_time": "interval",
        "north": "80.0",
        "south": "0.0",
        "east": "120.0",
        "west": "0.0",
        "top": "50.0",
        "bottom": "0.0",
        "tbres_min": "10.0",
        "tbres_max": "10.0",
        "nsres_min": "10.0",
        "nsres_max": "10.0",
        "ewres_min": "10.0",
        "ewres_max": "10.0",
        "min_min": "1.0",
        "min_max": "2.0",
        "max_min": "1.0",
        "max_max": "2.0",
        "aggregation_type": "None",
        "number_of_maps": "2",
    }
    assert info["raster3d_register"].startswith("raster3d_map_register_")


def test_plain_info(temporal_data):
    """Plain output reports the metadata in a human readable table"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(type="str3ds", input="volume_abs1").text

    assert "Space Time 3D Raster Dataset" in text
    assert f"Id: ........................ volume_abs1@{temporal_data.mapset}" in text
    assert "Granularity:................ 1 month" in text
    assert "Number of registered maps:.. 2" in text
    assert "Title:" in text
    assert "Command history:" in text


def test_raster3d_map_shell(temporal_data):
    """Shell output for a registered 3D raster map reports the map metadata"""
    tools = Tools(session=temporal_data.session)
    info = shell_info(tools, type="raster_3d", input="vol_1")

    assert list(info.keys()) == RASTER3D_SHELL_KEYS
    assert stable_items(info) == {
        "id": f"vol_1@{temporal_data.mapset}",
        "name": "vol_1",
        "mapset": temporal_data.mapset,
        "temporal_type": "absolute",
        "start_time": "'2001-01-01 00:00:00'",
        "end_time": "'2001-02-01 00:00:00'",
        "north": "80.0",
        "south": "0.0",
        "east": "120.0",
        "west": "0.0",
        "top": "50.0",
        "bottom": "0.0",
        "datatype": "DCELL",
        "cols": "12",
        "rows": "8",
        "number_of_cells": "480",
        "nsres": "10.0",
        "ewres": "10.0",
        "min": "1.0",
        "max": "1.0",
        "depths": "5",
        "tbres": "10.0",
        "registered_datasets": f"volume_abs1@{temporal_data.mapset}",
    }


def test_raster3d_map_plain(temporal_data):
    """Plain output for a registered 3D raster map reports the map metadata"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(type="raster_3d", input="vol_1").text

    assert "3D Raster Dataset" in text
    assert "Datatype:................... DCELL" in text
    assert "Number of columns:.......... 12" in text
    assert "Number of rows:............. 8" in text
    assert "Number of cells:............ 480" in text
    assert f"Registered datasets ........ volume_abs1@{temporal_data.mapset}" in text
