import os
from pathlib import Path

import pytest

import grass.script as gs
from grass.tools import Tools
from grass.grassdb.create import create_xy_project


@pytest.mark.parametrize("project_path_type", [str, Path])
def test_path_types(tmp_path, project_path_type):
    """Check that different path types are supported"""
    path = tmp_path / "project"
    create_xy_project(project_path_type(path))
    path.exists()
    with (
        gs.setup.init(path, env=os.environ.copy()) as session,
        Tools(session=session) as tools,
    ):
        region = tools.g_region(flags="p", format="json")
        assert region["crs"]["type"] == "xy"
        assert region["crs"]["type_code"] == 0
        assert region["crs"]["zone"] is None
        assert region["rows"] == 1
        assert region["cols"] == 1
