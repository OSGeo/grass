############################################################################
#
# MODULE:       Test of JSON export from pygrass.Module for usage in actinia
# AUTHOR(S):    Stefan Blumentrath
# PURPOSE:      Test of JSON export from pygrass.Module for usage in actinia
# COPYRIGHT:    (C) 2024 by Stefan Blumentrath and the GRASS Development Team
#
#               This program is free software under the GNU General Public
#               License (>=v2). Read the file COPYING that comes with GRASS
#               for details.
#
#############################################################################

"""Test of JSON export from pygrass.Module for usage in actinia"""

from grass.pygrass.modules.interface import Module


def test_rinfo_simple():
    """Test if a Module can be exported to json dict"""

    mod_json_dict = Module("r.info", map="elevation", run_=False).get_json_dict()

    assert isinstance(mod_json_dict, dict)
    assert set(mod_json_dict.keys()) == {"module", "id", "inputs"}
    assert mod_json_dict["module"] == "r.info"
    assert mod_json_dict["id"].startswith("r_info_")
    assert "flags" not in mod_json_dict
    assert set(mod_json_dict["inputs"][0].keys()) == {"param", "value"}


def test_rinfo_ov():
    """Test if a Module can be exported to json dict with quiet and overwrite flags"""

    mod_json_dict = Module(
        "r.info", quiet=True, map="elevation", run_=False
    ).get_json_dict()

    assert isinstance(mod_json_dict, dict)
    assert set(mod_json_dict.keys()) == {"module", "id", "inputs", "quiet"}
    assert mod_json_dict["module"] == "r.info"
    assert mod_json_dict["id"].startswith("r_info_")
    assert "flags" not in mod_json_dict
    assert mod_json_dict["quiet"] is True
    assert isinstance(mod_json_dict["inputs"], list)
    assert set(mod_json_dict["inputs"][0].keys()) == {"param", "value"}


def test_rinfo_ov_export():
    """Test if a Module can be exported to json dict with overwrite
    and verbose flags and results exported to CSV"""

    mod_json_dict = Module(
        "r.info",
        verbose=True,
        map="elevation",
        flags="g",
        run_=False,
    ).get_json_dict(stdout_export="list")

    assert isinstance(mod_json_dict, dict)
    assert set(mod_json_dict.keys()) == {
        "module",
        "id",
        "flags",
        "inputs",
        "verbose",
        "stdout",
    }
    assert mod_json_dict["module"] == "r.info"
    assert mod_json_dict["id"].startswith("r_info_")
    assert mod_json_dict["flags"] == "g"
    assert mod_json_dict["verbose"] is True
    assert isinstance(mod_json_dict["inputs"], list)
    assert set(mod_json_dict["inputs"][0].keys()) == {"param", "value"}
    assert mod_json_dict["stdout"] == {
        "id": "stdout",
        "format": "list",
        "delimiter": "|",
    }


def test_rslopeaspect_ov_export():
    """Test if a Module can be exported to json dict with overwrite
    and verbose flags and results exported to CSV"""

    mod_json_dict = Module(
        "r.slope.aspect",
        elevation="elevation",
        slope="slope",
        aspect="aspect",
        overwrite=True,
        verbose=True,
        run_=False,
    ).get_json_dict(export="GTiff")

    assert isinstance(mod_json_dict, dict)
    assert set(mod_json_dict.keys()) == {
        "module",
        "id",
        "inputs",
        "outputs",
        "overwrite",
        "verbose",
    }
    assert mod_json_dict["module"] == "r.slope.aspect"
    assert mod_json_dict["id"].startswith("r_slope_aspect_")
    assert mod_json_dict["overwrite"] is True
    assert mod_json_dict["verbose"] is True
    assert isinstance(mod_json_dict["inputs"], list)
    assert isinstance(mod_json_dict["outputs"], list)
    assert set(mod_json_dict["outputs"][0].keys()) == {"param", "value", "export"}
    assert mod_json_dict["outputs"][0]["export"] == {
        "format": "GTiff",
        "type": "raster",
    }
