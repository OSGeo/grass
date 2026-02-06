"""Test g.extension"""

import pytest
import grass.script as gs


# This should cover both C and Python tools.
@pytest.mark.parametrize("name", ["r.stream.distance", "r.lake.series"])
def test_install(tools, name):
    """Test that a tools installs and gives help message"""
    tools.g_extension(extension=name)
    assert tools.call_cmd([name, "--help"]).stderr

    # Check that the installed extension is in the get_commands() list
    # i.e., GRASS_ADDON_BASE is accessible or not.
    commands = gs.get_commands(env=tools._original_env)[0]
    assert name in commands, f"Extension {name} not found in get_commands() list"
