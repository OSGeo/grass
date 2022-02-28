"""Test functions in grass.script.setup"""

import grass.script as gs
import grass.script.setup as grass_setup


def test_init_as_context_manager(tmp_path):
    """Check that init function return value works as a context manager"""
    location = "test"
    gs.core._create_location_xy(tmp_path, location)  # pylint: disable=protected-access
    with grass_setup.init(tmp_path / location):
        gs.run_command("g.region", flags="p")
