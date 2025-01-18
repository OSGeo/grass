"""Tests of v.colors.out"""

import grass.script as gs


def validate_plain_text_output(data):
    """Validate the structure and content of the plain text output."""
    assert data
    assert len(data) == 8, "The output does not match the expected number of items (8)."
    expected = {
        "1 0:191:191",
        "20.8 0:255:0",
        "40.6 255:255:0",
        "60.4 255:127:0",
        "80.2 191:127:63",
        "100 200:200:200",
        "nv 255:255:255",
        "default 255:255:255",
    }
    assert (
        expected == data.keys()
    ), f"test failed: expected {expected} but got {data.keys()}"


def test_v_colors_out_plain_output(vector_color_dataset):
    """Test v.colors.out command for plain output format."""
    session = vector_color_dataset
    data = gs.parse_command("v.colors.out", map="a", format="plain", env=session.env)
    validate_plain_text_output(data)


def test_v_colors_out_without_format_option(vector_color_dataset):
    """Test v.colors.out command without any format option."""
    session = vector_color_dataset
    data = gs.parse_command("v.colors.out", map="a", env=session.env)
    validate_plain_text_output(data)


def test_v_colors_out_with_p_flag(vector_color_dataset):
    """Test v.colors.out command with percentage values."""
    session = vector_color_dataset
    data = gs.parse_command("v.colors.out", map="a", flags="p", env=session.env)
    assert data
    assert len(data) == 8, "The output does not match the expected number of items (8)."
    expected = {
        "0% 0:191:191",
        "20% 0:255:0",
        "40% 255:255:0",
        "60% 255:127:0",
        "80% 191:127:63",
        "100% 200:200:200",
        "nv 255:255:255",
        "default 255:255:255",
    }
    assert (
        expected == data.keys()
    ), f"test failed: expected {expected} but got {data.keys()}"


def validate_common_json_structure(data):
    """Validate the common structure and content of the JSON output."""
    assert isinstance(data, list), "Output data should be a list of entries."
    assert (
        len(data) == 8
    ), "The length of the output JSON does not match the expected value of 8."


def test_v_colors_out_json_with_default_option(vector_color_dataset):
    """Test v.colors.out command for JSON output format for default color option."""
    session = vector_color_dataset
    data = gs.parse_command("v.colors.out", map="a", format="json", env=session.env)
    validate_common_json_structure(data)
    expected = [
        {"value": 1, "hex": "#00BFBF"},
        {"value": 20.8, "hex": "#00FF00"},
        {"value": 40.6, "hex": "#FFFF00"},
        {"value": 60.4, "hex": "#FF7F00"},
        {"value": 80.2, "hex": "#BF7F3F"},
        {"value": 100, "hex": "#C8C8C8"},
        {"value": "nv", "hex": "#FFFFFF"},
        {"value": "default", "hex": "#FFFFFF"},
    ]
    assert expected == data, f"test failed: expected {expected} but got {data}"


def test_v_colors_out_json_with_triplet_option(vector_color_dataset):
    """Test v.colors.out command for JSON output format for triplet color option."""
    session = vector_color_dataset
    data = gs.parse_command(
        "v.colors.out", map="a", format="json", color_format="triplet", env=session.env
    )
    validate_common_json_structure(data)
    expected = [
        {"value": 1, "triplet": "0:191:191"},
        {"value": 20.8, "triplet": "0:255:0"},
        {"value": 40.6, "triplet": "255:255:0"},
        {"value": 60.4, "triplet": "255:127:0"},
        {"value": 80.2, "triplet": "191:127:63"},
        {"value": 100, "triplet": "200:200:200"},
        {"value": "nv", "triplet": "255:255:255"},
        {"value": "default", "triplet": "255:255:255"},
    ]
    assert expected == data, f"test failed: expected {expected} but got {data}"


def test_v_colors_out_json_with_rgb_option(vector_color_dataset):
    """Test v.colors.out command for JSON output format for rgb color option."""
    session = vector_color_dataset
    data = gs.parse_command(
        "v.colors.out",
        map="a",
        format="json",
        color_format="rgb",
        env=session.env,
    )
    validate_common_json_structure(data)
    expected = [
        {"value": 1, "rgb": "rgb(0, 191, 191)"},
        {"value": 20.8, "rgb": "rgb(0, 255, 0)"},
        {"value": 40.6, "rgb": "rgb(255, 255, 0)"},
        {"value": 60.4, "rgb": "rgb(255, 127, 0)"},
        {"value": 80.2, "rgb": "rgb(191, 127, 63)"},
        {"value": 100, "rgb": "rgb(200, 200, 200)"},
        {"value": "nv", "rgb": "rgb(255, 255, 255)"},
        {"value": "default", "rgb": "rgb(255, 255, 255)"},
    ]
    assert expected == data, f"test failed: expected {expected} but got {data}"


def test_v_colors_out_json_with_hex_option(vector_color_dataset):
    """Test v.colors.out command for JSON output format for hex color option."""
    session = vector_color_dataset
    data = gs.parse_command(
        "v.colors.out",
        map="a",
        format="json",
        color_format="hex",
        env=session.env,
    )
    validate_common_json_structure(data)
    expected = [
        {"value": 1, "hex": "#00BFBF"},
        {"value": 20.8, "hex": "#00FF00"},
        {"value": 40.6, "hex": "#FFFF00"},
        {"value": 60.4, "hex": "#FF7F00"},
        {"value": 80.2, "hex": "#BF7F3F"},
        {"value": 100, "hex": "#C8C8C8"},
        {"value": "nv", "hex": "#FFFFFF"},
        {"value": "default", "hex": "#FFFFFF"},
    ]
    assert expected == data, f"test failed: expected {expected} but got {data}"


def test_v_colors_out_json_with_hsv_option(vector_color_dataset):
    """Test v.colors.out command for JSON output format for hsv color option."""
    session = vector_color_dataset
    data = gs.parse_command(
        "v.colors.out",
        map="a",
        format="json",
        color_format="hsv",
        env=session.env,
    )
    validate_common_json_structure(data)
    expected = [
        {"value": 1, "hsv": "hsv(180, 100, 74)"},
        {"value": 20.8, "hsv": "hsv(120, 100, 100)"},
        {"value": 40.6, "hsv": "hsv(60, 100, 100)"},
        {"value": 60.4, "hsv": "hsv(29, 100, 100)"},
        {"value": 80.2, "hsv": "hsv(30, 67, 74)"},
        {"value": 100, "hsv": "hsv(0, 0, 78)"},
        {"value": "nv", "hsv": "hsv(0, 0, 100)"},
        {"value": "default", "hsv": "hsv(0, 0, 100)"},
    ]
    assert expected == data, f"test failed: expected {expected} but got {data}"
