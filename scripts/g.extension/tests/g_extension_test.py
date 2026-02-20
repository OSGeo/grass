"""Test g.extension (re-enabled for Windows per https://github.com/OSGeo/grass/issues/7080)."""

import pytest


# This should cover both C and Python tools.
@pytest.mark.parametrize("name", ["r.stream.distance", "r.lake.series"])
def test_install(tools, name):
    """Test that a tools installs and gives help message"""
    tools.g_extension(extension=name)
    assert tools.call_cmd([name, "--help"]).stderr
