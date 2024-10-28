"""Tests of r.colors.out"""

import grass.script as gs


def validate_plain_text_output(data):
    """Validate the structure and content of the plain text output."""
    assert data
    assert (
        len(data) == 31
    ), "The output does not match the expected number of items (31)."

    for i, key in enumerate(data.keys()):
        value, rgb_value = key.split(" ")

        assert value, "value is not present."
        assert rgb_value, "rgb color is not present."

        if i == len(data) - 2:
            assert (
                value == "nv"
            ), f"Expected 'nv' as the value for the second last item, got '{value}'."
        elif i == len(data) - 1:
            assert (
                value == "default"
            ), f"Expected 'default' as the value for the last item, got '{value}'."
        else:
            assert (
                value.isdigit()
            ), f"Invalid value '{value}' in line. Expected a number."

        # Validate RGB value format
        rgb_components = rgb_value.split(":")
        assert len(rgb_components) == 3, "Invalid RGB format."
        assert all(
            0 <= int(c) <= 255 for c in rgb_components
        ), "Invalid RGB values, Values must be between 0 and 255."


def test_r_colors_out_plain_output(raster_color_dataset):
    """Test r.colors.out command for plain output format."""
    session = raster_color_dataset
    data = gs.parse_command(
        "r.colors.out", map=session.raster_names, format="plain", env=session.env
    )

    validate_plain_text_output(data)


def test_r_colors_out_without_format_option(raster_color_dataset):
    """Test r.colors.out command without any format option."""
    session = raster_color_dataset
    data = gs.parse_command("r.colors.out", map=session.raster_names, env=session.env)

    validate_plain_text_output(data)


def validate_common_json_structure(data):
    """Validate the common structure and content of the JSON output."""
    assert isinstance(data, list), "Output data should be a list of entries."
    assert (
        len(data) == 31
    ), "The length of the output JSON does not match the expected value of 31."

    assert all(
        "value" in entry for entry in data
    ), "Not all entries contain the 'value' key."

    assert any(
        entry.get("value") == "nv" for entry in data
    ), "No entry contains 'nv' as the value."
    assert any(
        entry.get("value") == "default" for entry in data
    ), "No entry contains 'default' as the value."


def test_r_colors_out_json_with_default_flag(raster_color_dataset):
    """Test r.colors.out command for JSON output format for xterm color option."""
    session = raster_color_dataset
    data = gs.parse_command(
        "r.colors.out", map=session.raster_names, format="json", env=session.env
    )

    validate_common_json_structure(data)
    assert all(
        "RGB" in entry for entry in data
    ), "Not all entries contain the 'RGB' key."


def test_r_colors_out_json_with_rgb_flag(raster_color_dataset):
    """Test r.colors.out command for JSON output format for rgb color option."""
    session = raster_color_dataset
    data = gs.parse_command(
        "r.colors.out",
        map=session.raster_names,
        format="json",
        flags="r",
        env=session.env,
    )

    validate_common_json_structure(data)
    assert all(
        "rgb" in entry for entry in data
    ), "Not all entries contain the 'rgb' key."


def test_r_colors_out_json_with_hex_flag(raster_color_dataset):
    """Test r.colors.out command for JSON output format for hex color option."""
    session = raster_color_dataset
    data = gs.parse_command(
        "r.colors.out",
        map=session.raster_names,
        format="json",
        flags="x",
        env=session.env,
    )

    validate_common_json_structure(data)
    assert all(
        "hex" in entry for entry in data
    ), "Not all entries contain the 'hex' key."


def test_r_colors_out_json_with_hsv_flag(raster_color_dataset):
    """Test r.colors.out command for JSON output format for hsv color option."""
    session = raster_color_dataset
    data = gs.parse_command(
        "r.colors.out",
        map=session.raster_names,
        format="json",
        flags="h",
        env=session.env,
    )

    validate_common_json_structure(data)
    assert all(
        "hsv" in entry for entry in data
    ), "Not all entries contain the 'hsv' key."
