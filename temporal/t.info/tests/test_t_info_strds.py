# SPDX-License-Identifier: GPL-2.0-or-later
"""Test t.info with space time raster datasets and raster maps

The expected values capture the output of t.info as it is now, so that later
changes to the interface can be checked against the established behavior.
"""

import pytest

import grass.script as gs
from grass.tools import Tools
from grass.tools.session_tools import ToolError

# Keys whose values differ between runs, systems, or users.
VOLATILE_KEYS = ("creation_time", "modification_time", "creator", "raster_register")

STRDS_SHELL_KEYS = [
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
    "raster_register",
    "nsres_min",
    "nsres_max",
    "ewres_min",
    "ewres_max",
    "min_min",
    "min_max",
    "max_min",
    "max_max",
    "aggregation_type",
    "number_of_semantic_labels",
    "semantic_labels",
    "number_of_maps",
]

RASTER_SHELL_KEYS = [
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
    "semantic_label",
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
    info = shell_info(tools, type="strds", input="precip_abs1")

    assert list(info.keys()) == STRDS_SHELL_KEYS


def test_shell_values(temporal_data):
    """Shell output reports the metadata of the dataset"""
    tools = Tools(session=temporal_data.session)
    info = shell_info(tools, type="strds", input="precip_abs1")

    assert stable_items(info) == {
        "id": f"precip_abs1@{temporal_data.mapset}",
        "name": "precip_abs1",
        "mapset": temporal_data.mapset,
        "temporal_type": "absolute",
        "semantic_type": "mean",
        "start_time": "'2001-01-01 00:00:00'",
        "end_time": "'2001-04-01 00:00:00'",
        "granularity": "'1 month'",
        "map_time": "interval",
        "north": "80.0",
        "south": "0.0",
        "east": "120.0",
        "west": "0.0",
        "top": "0.0",
        "bottom": "0.0",
        "nsres_min": "10.0",
        "nsres_max": "10.0",
        "ewres_min": "10.0",
        "ewres_max": "10.0",
        "min_min": "1.0",
        "min_max": "3.0",
        "max_min": "1.0",
        "max_max": "3.0",
        "aggregation_type": "None",
        "number_of_semantic_labels": "0",
        "semantic_labels": "None",
        "number_of_maps": "3",
    }
    assert info["creator"]
    assert info["raster_register"].startswith("raster_map_register_")


def test_shell_quoting(temporal_data):
    """Time values and granularity are quoted, other values are not"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(flags="g", type="strds", input="precip_abs1").text

    assert "start_time='2001-01-01 00:00:00'" in text
    assert "end_time='2001-04-01 00:00:00'" in text
    assert "granularity='1 month'" in text
    assert "north=80.0" in text
    assert "map_time=interval" in text
    assert "aggregation_type=None" in text


def test_plain_info(temporal_data):
    """Plain output reports the metadata in a human readable table"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(type="strds", input="precip_abs1").text

    assert "Space Time Raster Dataset" in text
    assert f"Id: ........................ precip_abs1@{temporal_data.mapset}" in text
    assert "Start time:................. 2001-01-01 00:00:00" in text
    assert "Granularity:................ 1 month" in text
    assert "Number of registered maps:.. 3" in text


def test_plain_reports_title_description_and_history(temporal_data):
    """Title, description and command history are in the plain output only"""
    tools = Tools(session=temporal_data.session)
    plain = tools.t_info(type="strds", input="precip_abs1").text
    shell = tools.t_info(flags="g", type="strds", input="precip_abs1").text

    assert "Title:" in plain
    assert "Description:" in plain
    assert "Command history:" in plain
    assert 't.create type="strds"' in plain

    assert "Title:" not in shell
    assert "Description:" not in shell
    assert "t.create" not in shell


def test_history_flag(temporal_data):
    """The -h flag reports title, description and the commands used"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(flags="h", type="strds", input="precip_abs1").text

    assert "# Title:" in text
    assert "# Description:" in text
    assert "# Command history:" in text
    assert 't.create type="strds"' in text
    assert "t.register" in text
    assert 'maps="prec_1,prec_2,prec_3"' in text


def test_history_flag_on_map(temporal_data):
    """The -h flag on a map reports the plain map metadata"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(flags="h", type="raster", input="prec_1").text

    assert "Raster Dataset" in text
    assert "Command history:" not in text


def test_nonascii_title_and_description(temporal_data):
    """Non-ASCII title and description are reported unchanged"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(type="strds", input="accent_ds").text

    assert "Précipitation totale" in text
    assert "Précipitation mensuelle" in text


def test_dataset_without_maps(temporal_data):
    """A dataset without registered maps reports None for the empty fields"""
    tools = Tools(session=temporal_data.session)
    info = shell_info(tools, type="strds", input="empty_ds")

    assert list(info.keys()) == STRDS_SHELL_KEYS
    assert info["start_time"] == "'None'"
    assert info["end_time"] == "'None'"
    assert info["granularity"] == "'None'"
    assert info["map_time"] == "None"
    assert info["north"] == "None"
    assert info["min_min"] == "None"
    assert info["number_of_maps"] == "None"


def test_relative_time_dataset(temporal_data):
    """A relative time dataset reports its time unit"""
    tools = Tools(session=temporal_data.session)
    info = shell_info(tools, type="strds", input="precip_rel1")

    assert info["temporal_type"] == "relative"
    assert info["unit"] == "days"
    assert info["start_time"] == "'1'"
    assert info["end_time"] == "'3'"
    assert info["granularity"] == "1"
    assert info["number_of_maps"] == "2"


def test_raster_map_shell(temporal_data):
    """Shell output for a registered raster map reports the map metadata"""
    tools = Tools(session=temporal_data.session)
    info = shell_info(tools, type="raster", input="prec_1")

    assert list(info.keys()) == RASTER_SHELL_KEYS
    assert stable_items(info) == {
        "id": f"prec_1@{temporal_data.mapset}",
        "name": "prec_1",
        "mapset": temporal_data.mapset,
        "temporal_type": "absolute",
        "start_time": "'2001-01-01 00:00:00'",
        "end_time": "'2001-02-01 00:00:00'",
        "north": "80.0",
        "south": "0.0",
        "east": "120.0",
        "west": "0.0",
        "top": "0.0",
        "bottom": "0.0",
        "datatype": "CELL",
        "cols": "12",
        "rows": "8",
        "number_of_cells": "96",
        "nsres": "10.0",
        "ewres": "10.0",
        "min": "1.0",
        "max": "1.0",
        "semantic_label": "None",
        "registered_datasets": f"precip_abs1@{temporal_data.mapset}",
    }


def test_raster_map_plain(temporal_data):
    """Plain output for a registered raster map reports the map metadata"""
    tools = Tools(session=temporal_data.session)
    text = tools.t_info(type="raster", input="prec_1").text

    assert "Raster Dataset" in text
    assert "Datatype:................... CELL" in text
    assert "Number of columns:.......... 12" in text
    assert "Number of rows:............. 8" in text
    assert "Number of cells:............ 96" in text
    assert f"Registered datasets ........ precip_abs1@{temporal_data.mapset}" in text


def test_map_registered_in_two_datasets(temporal_data):
    """A map registered in more than one dataset reports all of them"""
    tools = Tools(session=temporal_data.session)
    info = shell_info(tools, type="raster", input="prec_2")

    mapset = temporal_data.mapset
    assert info["registered_datasets"] == f"precip_abs1@{mapset},precip_abs2@{mapset}"


def test_dataset_not_found(temporal_data):
    """A dataset which is not in the temporal database is an error"""
    tools = Tools(session=temporal_data.session)

    with pytest.raises(ToolError, match="not found in"):
        tools.t_info(type="strds", input="does_not_exist")


def test_input_is_required(temporal_data):
    """Without input and without the -d flag, t.info fails"""
    tools = Tools(session=temporal_data.session)

    with pytest.raises(ToolError):
        tools.t_info(type="strds")
