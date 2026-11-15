"""Tests of --json CLI"""

import pytest

from grass.exceptions import CalledModuleError


def test_raster_input_output(tools):
    """Check basic case with input and output raster"""
    result = tools.call_cmd(
        ["r.slope.aspect", "elevation=dem", "slope=slope1", "aspect=aspect1", "--json"]
    )
    # Let's consider the id non-deterministic and don't test its value.
    # We still test its presence. (...and need a separate test for the ids.)
    assert "id" in result.json
    del result.json["id"]
    # Because we test all parameter values, this needs an update anytime the tool's
    # parameter change.
    assert result.json == {
        "module": "r.slope.aspect",
        "inputs": [
            {"param": "elevation", "value": "dem"},
            {"param": "format", "value": "degrees"},
            {"param": "precision", "value": "FCELL"},
            {"param": "zscale", "value": "1.0"},
            {"param": "min_slope", "value": "0.0"},
            {"param": "nprocs", "value": "0"},
            {"param": "memory", "value": "300"},
        ],
        "outputs": [
            {"param": "slope", "value": "slope1"},
            {"param": "aspect", "value": "aspect1"},
        ],
    }


def test_tiff_input_output(tools):
    """Check input with URL and export format.

    This is based on specification (examples) in the C source code.
    """
    result = tools.call_cmd(
        [
            "r.slope.aspect",
            "elevation=elevation_a@https://example.com/data/elev_ned_30m.tif",
            "slope=slope_a+GTiff",
            "aspect=aspect_a+GTiff",
            "--json",
        ]
    )
    del result.json["id"]
    assert result.json == {
        "module": "r.slope.aspect",
        "inputs": [
            {
                "import_descr": {
                    "source": "https://example.com/data/elev_ned_30m.tif",
                    "type": "raster",
                },
                "param": "elevation",
                "value": "elevation_a",
            },
            {"param": "format", "value": "degrees"},
            {"param": "precision", "value": "FCELL"},
            {"param": "zscale", "value": "1.0"},
            {"param": "min_slope", "value": "0.0"},
            {"param": "nprocs", "value": "0"},
            {"param": "memory", "value": "300"},
        ],
        "outputs": [
            {
                "export": {"format": "GTiff", "type": "raster"},
                "param": "slope",
                "value": "slope_a",
            },
            {
                "export": {"format": "GTiff", "type": "raster"},
                "param": "aspect",
                "value": "aspect_a",
            },
        ],
    }


def test_output_text_file(tools):
    """Check output text file export.

    This is based on specification (examples) in the C source code.
    """
    result = tools.call_cmd(
        [
            "v.out.ascii",
            "input=hospitals@PERMANENT",
            "output=myfile+TXT",
            "--json",
        ]
    )
    del result.json["id"]
    # Unlike the format specification (examples) in the code, mapset is not passed to
    # the inputs.
    assert result.json == {
        "module": "v.out.ascii",
        "inputs": [
            {"param": "input", "value": "hospitals"},
            {"param": "layer", "value": "1"},
            {"param": "type", "value": "point,line,boundary,centroid,area,face,kernel"},
            {"param": "format", "value": "point"},
            {"param": "separator", "value": "pipe"},
            {"param": "precision", "value": "8"},
        ],
        "outputs": [
            {
                "export": {"format": "TXT", "type": "file"},
                "param": "output",
                "value": "$file::myfile",
            }
        ],
    }


def test_flag(tools):
    """Check a single flag.

    This is based on specification (examples) in the C source code.
    """
    result = tools.call_cmd(["v.info", "map=hospitals@PERMANENT", "-c", "--json"])
    del result.json["id"]
    assert result.json == {
        "module": "v.info",
        "flags": "c",
        "inputs": [
            {"param": "map", "value": "hospitals"},
            {"param": "layer", "value": "1"},
        ],
    }


def test_json_escape_parameter_value_with_backslash(tools):
    """Check single backslash"""
    value = r"\word"
    result = tools.call_cmd(["d.text", f"text={value}", "--json"])
    for item in result["inputs"]:
        if item["param"] == "text":
            assert item["value"] == value


def test_json_escape_parameter_value(tools):
    """Check multiple backslashes and other characters"""
    value = r'\\\\\\::\t"Heading"\t::///'
    result = tools.call_cmd(["d.text", f"text={value}", "--json"])
    for item in result["inputs"]:
        if item["param"] == "text":
            assert item["value"] == value


def test_json_escape_raster_input_parameter(tools):
    """Raster input in need of escaping"""
    value = r"path\with\backslashes\data"
    result = tools.call_cmd(["r.univar", f"map={value}", "--json"])
    print(result.text)
    for item in result["inputs"]:
        if item["param"] == "map":
            assert item["value"] == value


def test_json_escape_vector_output_parameter(tools):
    """Vector output in need of escaping"""
    value = r"path\with\backslashes\data"
    result = tools.call_cmd(["v.random", f"output={value}", "npoints=1", "--json"])
    print(result.text)
    for item in result["inputs"]:
        if item["param"] == "map":
            assert item["value"] == value


def test_missing_required(tools):
    """Missing required parameter should produce an error

    With --json, the tools behave the same as without when the parser detects
    an issue with parameters.
    """
    with pytest.raises(CalledModuleError, match="output"):
        tools.call_cmd(["v.random", "npoints=1", "--json"])


def test_two_at_sign_characters(tools):
    """Multiple at signs are not allowed

    The implementation does not assign any meaning to two or more at signs
    in the parameter.
    """
    with pytest.raises(CalledModuleError, match="@"):
        tools.call_cmd(["r.univar", "map=dem@PERMANENT@https://example.com", "--json"])
    # Order of components should not matter; both are wrong.
    with pytest.raises(CalledModuleError, match="@"):
        tools.call_cmd(["r.univar", "map=dem@https://example.com@PERMANENT", "--json"])
    # Mapset name or virtual OGR mapset are still wrong.
    with pytest.raises(CalledModuleError, match="@"):
        tools.call_cmd(["r.univar", "map=dem@mapset_a@https://example.com", "--json"])
    with pytest.raises(CalledModuleError, match="@"):
        tools.call_cmd(["r.univar", "map=dem@OGR@https://example.com", "--json"])
