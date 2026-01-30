"""Tests for grass.script.pytest_utils"""

import sys
from pathlib import Path

import pytest

from grass.script.utils import parse_key_val, split

# Add local grass.script to path for development testing
# This must happen after standard imports but before pytest_utils import
script_dir = Path(__file__).parent.parent
if str(script_dir) not in sys.path:
    sys.path.insert(0, str(script_dir))

try:
    from grass.script.pytest_utils import raster_fits_univar  # noqa: E402
except ImportError:
    # Module not available, tests will be skipped
    raster_fits_univar = None

# Skip marker for tests that need pytest_utils
requires_pytest_utils = pytest.mark.skipif(
    raster_fits_univar is None, reason="grass.script.pytest_utils not available"
)


@requires_pytest_utils
class TestRasterFitsUnivar:
    """Tests for raster_fits_univar function"""

    def test_returns_tuple(self):
        """Function returns (bool, str) tuple"""
        result = raster_fits_univar("nonexistent", {"mean": 0})
        assert isinstance(result, tuple)
        assert len(result) == 2
        assert isinstance(result[0], bool)
        assert isinstance(result[1], str)

    def test_missing_map_error(self):
        """Missing map returns clear error"""
        success, msg = raster_fits_univar("nonexistent", {"mean": 0})
        assert not success
        assert "nonexistent" in msg

    @pytest.mark.parametrize(
        ("reference", "keys"),
        [
            ({"mean": 5.0}, ["mean"]),
            ({"min": 0, "max": 10}, ["min", "max"]),
            ({"mean": 5.0, "stddev": 1.0}, ["mean", "stddev"]),
        ],
    )
    def test_accepts_various_stats(self, reference, keys):
        """Function accepts different stat combinations"""
        success, msg = raster_fits_univar("test", reference)
        assert isinstance(success, bool)
        assert isinstance(msg, str)


class TestParseKeyVal:
    """Tests for parse_key_val function"""

    def test_basic(self):
        """Basic key=value parsing"""
        result = parse_key_val("a=1\nb=2")
        assert result == {"a": "1", "b": "2"}

    def test_empty(self):
        """Empty string returns empty dict"""
        assert parse_key_val("") == {}

    @pytest.mark.parametrize(
        ("input_str", "expected"),
        [
            ("a=1", {"a": "1"}),
            ("a=1\nb=2", {"a": "1", "b": "2"}),
            ("x=10\ny=20", {"x": "10", "y": "20"}),
        ],
    )
    def test_multiple(self, input_str, expected):
        """Multiple key=value pairs"""
        assert parse_key_val(input_str) == expected


class TestSplit:
    """Tests for split function (uses shlex.split)"""

    def test_basic(self):
        """Basic space-separated splitting"""
        assert split("a b c") == ["a", "b", "c"]

    def test_empty(self):
        """Empty string returns empty list"""
        assert split("") == []

    def test_quoted(self):
        """Quoted strings preserved"""
        assert split('key="value with spaces"') == ["key=value with spaces"]

    @pytest.mark.parametrize(
        ("input_str", "expected"),
        [
            ("a b", ["a", "b"]),
            ("a b c", ["a", "b", "c"]),
            ('"quoted"', ["quoted"]),
        ],
    )
    def test_various(self, input_str, expected):
        """Various input patterns"""
        assert split(input_str) == expected
