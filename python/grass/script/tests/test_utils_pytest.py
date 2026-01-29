"""pytest tests for grass.script.utils module

These tests demonstrate pytest syntax and test utility functions
that don't require a GRASS session.

Author: Saurabh Singh
Part of: GSoC 2025 pytest migration project
"""

import pytest
from grass.script.utils import parse_key_val, split


class TestParseKeyVal:
    """Tests for parse_key_val function"""

    def test_basic_parsing(self):
        """Test basic key=value parsing"""
        result = parse_key_val("key1=value1\nkey2=value2")
        assert result == {"key1": "value1", "key2": "value2"}

    def test_empty_string(self):
        """Test parsing empty string"""
        result = parse_key_val("")
        assert result == {}

    def test_single_pair(self):
        """Test parsing single key=value pair"""
        result = parse_key_val("key=value")
        assert result == {"key": "value"}

    def test_custom_separator(self):
        """Test parsing with custom separator"""
        result = parse_key_val("key1:value1\nkey2:value2", sep=":")
        assert result == {"key1": "value1", "key2": "value2"}

    def test_whitespace_handling(self):
        """Test that whitespace is preserved in values"""
        result = parse_key_val("key=value with spaces")
        assert result == {"key": "value with spaces"}

    @pytest.mark.parametrize(
        ("input_str", "expected"),
        [
            ("a=1", {"a": "1"}),
            ("a=1\nb=2", {"a": "1", "b": "2"}),
            ("x=10\ny=20\nz=30", {"x": "10", "y": "20", "z": "30"}),
        ],
    )
    def test_multiple_inputs(self, input_str, expected):
        """Test parse_key_val with multiple inputs using parametrize"""
        result = parse_key_val(input_str)
        assert result == expected

    @pytest.mark.parametrize(
        ("input_str", "sep", "expected"),
        [
            ("a=1\nb=2", "=", {"a": "1", "b": "2"}),
            ("a:1\nb:2", ":", {"a": "1", "b": "2"}),
            ("a|1\nb|2", "|", {"a": "1", "b": "2"}),
        ],
    )
    def test_different_separators(self, input_str, sep, expected):
        """Test parse_key_val with different separators"""
        result = parse_key_val(input_str, sep=sep)
        assert result == expected


class TestSplit:
    """Tests for split function

    Note: split() uses shlex.split() for shell-like parsing,
    not simple string splitting with separators.
    """

    def test_basic_split(self):
        """Test basic space-separated splitting"""
        result = split("a b c")
        assert result == ["a", "b", "c"]

    def test_single_item(self):
        """Test splitting single item"""
        result = split("single")
        assert result == ["single"]

    def test_empty_string(self):
        """Test splitting empty string"""
        result = split("")
        assert result == []

    def test_quoted_strings(self):
        """Test split with quoted strings (shlex behavior)"""
        result = split('key="value with spaces"')
        assert result == ["key=value with spaces"]

    def test_multiple_words(self):
        """Test split with multiple words"""
        result = split("first second third")
        assert result == ["first", "second", "third"]

    @pytest.mark.parametrize(
        ("input_str", "expected"),
        [
            ("a b", ["a", "b"]),
            ("a b c", ["a", "b", "c"]),
            ("a b c d", ["a", "b", "c", "d"]),
        ],
    )
    def test_varying_lengths(self, input_str, expected):
        """Test split with varying number of items"""
        result = split(input_str)
        assert result == expected

    @pytest.mark.parametrize(
        ("input_str", "expected"),
        [
            ("simple", ["simple"]),
            ("two words", ["two", "words"]),
            ('"quoted string"', ["quoted string"]),
            ("key=value", ["key=value"]),
        ],
    )
    def test_different_inputs(self, input_str, expected):
        """Test split with different input patterns"""
        result = split(input_str)
        assert result == expected
