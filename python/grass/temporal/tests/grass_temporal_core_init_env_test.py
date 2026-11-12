# SPDX-License-Identifier: GPL-2.0-or-later

"""Test tgis.init() with the GRASS session passed as env."""

import os

import grass.script as gs
import grass.temporal as tgis


def test_init_with_session_only_in_env(tmp_path, monkeypatch):
    """tgis.init(env=...) works without a session in os.environ."""
    monkeypatch.delenv("GISRC", raising=False)
    project = tmp_path / "project"
    gs.create_project(project)
    with gs.setup.init(project, env=os.environ.copy()) as session:
        tgis.init(env=session.env)
        ciface = tgis.get_tgis_c_library_interface()
        assert ciface.available_mapsets() == ["PERMANENT"]
