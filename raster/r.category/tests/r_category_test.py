"""Tests of r.category"""

import json

import pytest

import grass.script as gs
from grass.exceptions import CalledModuleError


def test_r_category_plain_output(simple_dataset):
    """Test r.category outputs correct labels with basic input and output."""
    session = simple_dataset
    input_data = "1:trees\n2:water\n"

    gs.write_command(
        "r.category",
        map="test",
        rules="-",
        separator=":",
        stdin=input_data,
        env=session.env,
    )

    # Replacing '\r' because Windows uses '\r\n' for line endings, but we
    # want to remove the '\r' (carriage return) to standardize line endings.
    result = gs.read_command(
        "r.category", map="test", separator=",", env=session.env
    ).replace("\r", "")

    expected_output = "1,trees\n2,water\n"
    assert result == expected_output, f"Expected {expected_output}, but got {result}"


def test_r_category_default_separator(simple_dataset):
    """Test r.category outputs correct labels using the default separator."""
    session = simple_dataset
    input_data = "1\ttrees\n2\twater\n"

    gs.write_command(
        "r.category", map="test", rules="-", stdin=input_data, env=session.env
    )

    # remove the '\r' (carriage return) to standardize line endings.
    result = gs.read_command(
        "r.category", map="test", separator="tab", env=session.env
    ).replace("\r", "")

    expected_output = "1\ttrees\n2\twater\n"
    assert result == expected_output, f"Expected {expected_output}, but got {result}"


def test_r_category_with_range(simple_dataset):
    """Test r.category handles category ranges correctly with plain output."""
    session = simple_dataset
    input_data = "1:trees\n2:4:buildings\n"

    gs.write_command(
        "r.category",
        map="test_1",
        separator=":",
        rules="-",
        stdin=input_data,
        env=session.env,
    )

    # remove the '\r' (carriage return) to standardize line endings.
    result = gs.read_command(
        "r.category", map="test_1", separator=" ", env=session.env
    ).replace("\r", "")

    expected_output = "1 trees\n2 buildings\n3 buildings\n4 buildings\n"
    assert result == expected_output, f"Expected {expected_output}, but got {result}"


def test_r_category_float_map(simple_dataset):
    """Test r.category handles floating-point maps."""
    session = simple_dataset
    input_data = "0:1.5:trees\n1.5:3:buildings\n"

    gs.write_command(
        "r.category",
        map="test_d",
        separator=":",
        rules="-",
        stdin=input_data,
        env=session.env,
    )
    result = gs.read_command(
        "r.category",
        map="test_d",
        separator=" ",
        vals=[1, 1.1, 2.1, 4],
        env=session.env,
    ).replace(
        "\r", ""
    )  # remove the '\r' (carriage return) to standardize line endings.

    expected_output = "1 trees\n1.1 trees\n2.1 buildings\n4 \n"
    assert result == expected_output, f"Expected {expected_output}, but got {result}"


def test_r_category_separator_variants(simple_dataset):
    """Test r.category handles various output separators correctly."""
    session = simple_dataset
    input_data = "1\ttrees\n2\twater\n"

    gs.write_command(
        "r.category", map="test", rules="-", stdin=input_data, env=session.env
    )

    separators = ["space", ",", "XYZ", "newline", "\n&\n"]
    expected_outputs = [
        "1 trees\n2 water\n",
        "1,trees\n2,water\n",
        "1XYZtrees\n2XYZwater\n",
        "1\ntrees\n2\nwater\n",
        "1\n&\ntrees\n2\n&\nwater\n",
    ]

    for sep, expected in zip(separators, expected_outputs):
        result = gs.read_command(
            "r.category", map="test", separator=sep, env=session.env
        ).replace(
            "\r", ""
        )  # remove the '\r' (carriage return) to standardize line endings.
        assert result == expected, (
            f"Expected {expected}, but got {result} for separator '{sep}'"
        )


def test_r_category_input_separators(simple_dataset):
    """Test r.category correctly handles various input separators."""
    session = simple_dataset

    input_data = [
        "1,treesA\n2,waterA\n",
        "1,treesB\n2,waterB\n",
        "1|treesC\n2|waterC\n",
    ]
    input_separators = ["comma", ",", "|"]
    output_separators = ["space", "space", " "]
    expected_outputs = [
        "1 treesA\n2 waterA\n",
        "1 treesB\n2 waterB\n",
        "1 treesC\n2 waterC\n",
    ]

    for data, inp_sep, out_sep, expected in zip(
        input_data, input_separators, output_separators, expected_outputs
    ):
        gs.write_command(
            "r.category",
            map="test",
            separator=inp_sep,
            rules="-",
            stdin=data,
            env=session.env,
        )
        result = gs.read_command(
            "r.category", map="test", separator=out_sep, env=session.env
        ).replace(
            "\r", ""
        )  # remove the '\r' (carriage return) to standardize line endings.
        assert result == expected, (
            f"Expected {expected}, but got {result} for input separator '{inp_sep}'"
        )


def test_r_category_multiword_input(simple_dataset):
    """Test r.category handles multi-word input correctly."""
    session = simple_dataset

    input_data = [
        "1|small trees\n2|deep water\n",
        "1\tvery small trees\n2\tvery deep water\n",
    ]
    input_separators = ["|", "tab"]
    output_separators = [" ", ":"]
    expected_outputs = [
        "1 small trees\n2 deep water\n",
        "1:very small trees\n2:very deep water\n",
    ]

    for data, inp_sep, out_sep, expected in zip(
        input_data, input_separators, output_separators, expected_outputs
    ):
        gs.write_command(
            "r.category",
            map="test",
            separator=inp_sep,
            rules="-",
            stdin=data,
            env=session.env,
        )
        result = gs.read_command(
            "r.category", map="test", separator=out_sep, env=session.env
        ).replace(
            "\r", ""
        )  # remove the '\r' (carriage return) to standardize line endings.
        assert result == expected, (
            f"Expected {expected}, but got {result} for input '{data}'"
        )


def test_r_category_extreme_incorrect_values(simple_dataset):
    """Test r.category handles extreme and incorrect inputs properly."""
    session = simple_dataset

    input_data = [
        "1,trees, very green\n2,water, very deep\n",
        "1\tvery green trees\n2\tvery deep\t water\n",
        "1 very green\n2 very deep\n",
    ]
    input_separators = ["comma", "tab", " "]

    for idx, (data, inp_sep) in enumerate(zip(input_data, input_separators), start=1):
        try:
            gs.write_command(
                "r.category",
                map="test",
                separator=inp_sep,
                rules="-",
                stdin=data,
                env=session.env,
            )

            pytest.fail(
                f"Expected CalledModuleError for input {idx}, but no error occurred."
            )
        except CalledModuleError:
            pass


def test_r_category_json_output(simple_dataset):
    """Test r.category handles JSON output format correctly."""
    session = simple_dataset

    input_data = "1|trees, very green\n2|water, very deep\n"

    gs.write_command(
        "r.category",
        map="test",
        separator="|",
        rules="-",
        stdin=input_data,
        env=session.env,
    )
    result = json.loads(
        gs.read_command("r.category", map="test", output_format="json", env=session.env)
    )

    expected = [
        {"category": 1, "description": "trees, very green"},
        {"category": 2, "description": "water, very deep"},
    ]
    assert result == expected, f"Expected {expected}, but got {result}"


def test_r_category_with_json_output_color(simple_dataset):
    """Test r.category handles JSON output format correctly with color option."""
    session = simple_dataset

    input_data = "1|trees, very green\n2|water, very deep\n"
    gs.write_command(
        "r.category",
        map="test",
        separator="|",
        rules="-",
        stdin=input_data,
        env=session.env,
    )

    colors = ["rgb", "hex", "triplet", "hsv", "none"]
    expected_outputs = [
        [
            {
                "category": 1,
                "description": "trees, very green",
                "color": "rgb(68, 1, 84)",
            },
            {
                "category": 2,
                "description": "water, very deep",
                "color": "rgb(253, 231, 37)",
            },
        ],
        [
            {"category": 1, "description": "trees, very green", "color": "#440154"},
            {
                "category": 2,
                "description": "water, very deep",
                "color": "#FDE725",
            },
        ],
        [
            {"category": 1, "description": "trees, very green", "color": "68:1:84"},
            {
                "category": 2,
                "description": "water, very deep",
                "color": "253:231:37",
            },
        ],
        [
            {
                "category": 1,
                "description": "trees, very green",
                "color": "hsv(288, 98, 32)",
            },
            {
                "category": 2,
                "description": "water, very deep",
                "color": "hsv(53, 85, 99)",
            },
        ],
        [
            {"category": 1, "description": "trees, very green"},
            {
                "category": 2,
                "description": "water, very deep",
            },
        ],
    ]

    for color, expected in zip(colors, expected_outputs):
        result = json.loads(
            gs.read_command(
                "r.category",
                map="test",
                output_format="json",
                color_format=color,
                env=session.env,
            )
        )
        assert result == expected, (
            f"test failed: expected {expected} but got {result} for color option {color}"
        )


def test_r_category_with_plain_output_color(simple_dataset):
    """Test r.category handles PLAIN output format correctly with color option."""
    session = simple_dataset

    input_data = "1:trees\n2:buildings\n"
    gs.write_command(
        "r.category",
        map="test",
        separator=":",
        rules="-",
        stdin=input_data,
        env=session.env,
    )

    colors = ["rgb", "hex", "triplet", "hsv", "none"]
    expected_outputs = [
        "1 trees rgb(68, 1, 84)\n2 buildings rgb(253, 231, 37)\n",
        "1 trees #440154\n2 buildings #FDE725\n",
        "1 trees 68:1:84\n2 buildings 253:231:37\n",
        "1 trees hsv(288, 98, 32)\n2 buildings hsv(53, 85, 99)\n",
        "1 trees\n2 buildings\n",
    ]

    for color, expected in zip(colors, expected_outputs):
        result = gs.read_command(
            "r.category",
            map="test",
            separator=" ",
            color_format=color,
            env=session.env,
        ).replace(
            "\r", ""
        )  # remove the '\r' (carriage return) to standardize line endings.
        assert result == expected, (
            f"test failed: expected {expected} but got {result} for color option {color}"
        )
