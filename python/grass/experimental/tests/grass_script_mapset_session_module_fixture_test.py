"""Test MapsetSession with location fixture with module scope

Location fixture with module scope needs to be the only session in
a module because the sessions are global and would thus overlap with each other
resulting in the second session ending with failure to access GISRC variable.
"""

import pytest

import grass.script as gs


@pytest.mark.parametrize("number", [1, 2, 3.1])
def test_usage_in_fixture_module_location(xy_mapset_session, number):
    """Test fixture based on location with module scope and function scope mapset

    Multiple mapsets are created in one location.
    """
    gs.run_command("r.mapcalc", expression=f"a = {number}", env=xy_mapset_session.env)
