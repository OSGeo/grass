# SPDX-License-Identifier: GPL-2.0-or-later

"""Regression test for tgis.init() across a GRASS session change.

tgis.init() spawns message and C-library subprocesses that capture
GISRC at spawn time. When the parent process switches to a different
project (different LOCATION_NAME or GISDBASE) and calls tgis.init()
again, those subprocesses must be restarted; otherwise they remain
bound to the previous session's GISRC, which may have been deleted.
"""

import pytest

from grass.grassdb.create import create_mapset
import grass.script as gs
import grass.temporal as tgis


@pytest.fixture
def two_projects(tmp_path):
    """Create two empty projects and yield their paths."""
    project_a = tmp_path / "a"
    project_b = tmp_path / "b"
    gs.create_project(project_a)
    create_mapset(project_a / "a")
    gs.create_project(project_b)
    create_mapset(project_b / "b")
    return project_a, project_b


def test_init_restarts_subprocesses_on_session_change(two_projects):
    """tgis.init() must respawn its subprocesses after a session change."""
    project_a, project_b = two_projects
    with gs.setup.init(project_a / "a"):
        tgis.init()
        first_ciface = tgis.get_tgis_c_library_interface()
        assert first_ciface.available_mapsets() == ["a", "PERMANENT"]

    with gs.setup.init(project_b / "b"):
        tgis.init()
        second_ciface = tgis.get_tgis_c_library_interface()

        # The C-library interface must have been replaced. Without this,
        # the subprocess is still bound to session A's deleted GISRC and
        # the call below fails.
        assert second_ciface is not first_ciface
        assert second_ciface.available_mapsets() == ["b", "PERMANENT"]
