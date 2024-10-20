"""Tests of r3.colors.out"""

import grass.script as gs


def validate_json_structure(data):
    """Validate the structure and content of the JSON output."""
    assert isinstance(data, list), "Output data should be a list of entries."
    assert (
        len(data) == 8
    ), "The length of the output JSON does not match the expected value of 8."

    assert all(
        "value" in entry for entry in data
    ), "Not all entries contain the 'value' key."
    assert all(
        "rgb" in entry for entry in data
    ), "Not all entries contain the 'rgb' key."

    assert any(
        entry.get("value") == "nv" for entry in data
    ), "No entry contains 'nv' as the value."
    assert any(
        entry.get("value") == "default" for entry in data
    ), "No entry contains 'default' as the value."


def test_r3_colors_out_json(raster3_color_dataset):
    """Test r3.colors.out command for JSON output format."""
    session = raster3_color_dataset
    data = gs.parse_command(
        "r3.colors.out", map=session.raster3_names, format="json", env=session.env
    )

    validate_json_structure(data)
