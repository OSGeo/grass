"""Tests of grass.tools.support.ToolResult"""

import json

import pytest

from grass.tools.support import ToolResult


def test_no_text_stdout(empty_result):
    assert not empty_result.stdout
    assert empty_result.stdout is None


def test_no_text_stderr(empty_result):
    assert not empty_result.stderr
    assert empty_result.stderr is None


def test_no_text(empty_result):
    assert not empty_result.text
    assert empty_result.text is None


def test_no_text_split(empty_result):
    assert empty_result.text_split() == []


def test_no_text_split_pipe(empty_result):
    assert empty_result.text_split("|") == []


def test_no_text_comma_items(empty_result):
    assert empty_result.comma_items == []


def test_no_text_space_items(empty_result):
    assert empty_result.space_items == []


def test_no_text_keyval(empty_result):
    assert empty_result.keyval == {}


def test_no_text_json(empty_result):
    with pytest.raises(ValueError, match="No text output"):
        assert empty_result.json


def test_empty_text_stdout(empty_string_result):
    assert not empty_string_result.stdout
    assert empty_string_result.stdout is not None


def test_empty_text_stderr(empty_string_result):
    assert not empty_string_result.stderr
    assert empty_string_result.stderr is not None


def test_empty_text(empty_string_result):
    assert not empty_string_result.text
    assert empty_string_result.text is not None


def test_empty_text_split(empty_string_result):
    assert empty_string_result.text_split() == []


def test_empty_text_split_pipe(empty_string_result):
    assert empty_string_result.text_split(" ") == []


def test_empty_text_comma_items(empty_string_result):
    assert empty_string_result.comma_items == []


def test_empty_text_space_items(empty_string_result):
    assert empty_string_result.space_items == []


def test_empty_text_keyval(empty_string_result):
    assert empty_string_result.keyval == {}


def test_empty_text_json(empty_string_result):
    with pytest.raises(ValueError, match="No text output"):
        assert empty_string_result.json


def test_empty_text_keyval_empty_value():
    text = "a=\nb=xyz"
    result = ToolResult(
        name=None, command=None, kwargs=None, returncode=None, stdout=text, stderr=None
    )
    assert result.keyval == {"a": "", "b": "xyz"}


def test_empty_text_keyval_numbers():
    text = "a=1\nb=1.0"
    result = ToolResult(
        name=None, command=None, kwargs=None, returncode=None, stdout=text, stderr=None
    )
    assert result.keyval == {"a": 1, "b": 1.0}


def test_json_format_set_but_text_invalid_with_command():
    """Check invalid format, but the format is set in command"""
    text = "invalid format"
    result = ToolResult(
        name=None,
        command=["test_tool", "format=json"],
        kwargs=None,
        returncode=None,
        stdout=text,
        stderr=None,
    )
    with pytest.raises(json.JSONDecodeError):
        assert result.json


def test_json_format_set_but_text_invalid_with_kwargs():
    """Check invalid format, but the format is set in kwargs"""
    text = "invalid format"
    result = ToolResult(
        name="test_tool",
        command=None,
        kwargs={"format": "json"},
        returncode=None,
        stdout=text,
        stderr=None,
    )
    with pytest.raises(json.JSONDecodeError):
        assert result.json


def test_json_decode_error_exception_is_value_error():
    """Check that ValueError is raised when JSON decoding fails

    In the ocde, we assume that JSONDecodeError is a subclass of ValueError
    which allows users to catch both JSONDecodeError and ValueError as ValueError,
    so we test this behavior here by catching ValueError.
    """
    text = "invalid format"
    result = ToolResult(
        name="test_tool",
        command=None,
        kwargs={"format": "json"},
        returncode=None,
        stdout=text,
        stderr=None,
    )
    with pytest.raises(ValueError, match="Expecting"):
        assert result.json


def test_json_format_not_set_and_text_invalid():
    """Check format set to something else than JSON"""
    text = "invalid format"
    result = ToolResult(
        name="test_tool",
        command=None,
        kwargs={"format": "csv"},
        returncode=None,
        stdout=text,
        stderr=None,
    )
    with pytest.raises(ValueError, match=r"format.*json"):
        assert result.json


def test_json_format_correct_with_wrong_parameter():
    """Check that parameter does not influence JSON parsing"""
    text = '{"a": 1, "b": 1.0}'
    result = ToolResult(
        name="test_tool",
        command=None,
        kwargs={"format": "csv"},
        returncode=None,
        stdout=text,
        stderr=None,
    )
    assert result.json == {"a": 1, "b": 1.0}


def test_text_as_bytes():
    stdout = b"a=1\nb=1.0"
    result = ToolResult(
        name=None,
        command=None,
        kwargs=None,
        returncode=None,
        stdout=stdout,
        stderr=None,
    )
    # The stdout attribute should be untouched.
    assert result.stdout == stdout
    # The text attribute should be decoded.
    assert result.text == stdout.decode()


def test_text_strip():
    stdout = "   a=1\nb=1.0  \n"
    result = ToolResult(
        name=None,
        command=None,
        kwargs=None,
        returncode=None,
        stdout=stdout,
        stderr=None,
    )
    # The stdout attribute should be untouched.
    assert result.stdout == stdout
    # Different ways of asking the same thing.
    # The repeated access should also trigger caching, but we don't test for
    # that explicitly.
    assert result.text == "a=1\nb=1.0"
    assert result.text == result.text.strip()
    assert result.text == stdout.strip()
