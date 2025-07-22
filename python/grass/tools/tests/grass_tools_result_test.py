"""Tests of grass.tools.support.ToolResult"""

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
