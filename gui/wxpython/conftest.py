# SPDX-License-Identifier: GPL-2.0-or-later

"""pytest fixtures for the wxPython GUI tests

The GUI modules need a GRASS session in the process environment when imported.
The tests need a display, which xvfb-run provides on Linux in CI.
"""

import os
import sys

import pytest

import grass.script as gs


def pytest_collection_modifyitems(items):
    """Mark the tests which use the wx.App to run them in their own process"""
    for item in items:
        if "wx_app" in getattr(item, "fixturenames", ()):
            item.add_marker(pytest.mark.needs_wx_app)


@pytest.fixture(scope="module")
def gui_session(tmp_path_factory):
    """Session mirrored to os.environ with the GUI directory in sys.path

    The environment, sys.path and the imported GUI modules are restored at the
    end of the module. Tests using this fixture need needs_solo_run.
    """
    project = tmp_path_factory.mktemp("gui_project") / "xy_test"
    gs.create_project(project)
    with (
        pytest.MonkeyPatch.context() as monkeypatch,
        gs.setup.init(project, env=os.environ.copy()) as session,
    ):
        for name, value in session.env.items():
            monkeypatch.setenv(name, value)
        gui_dir = session.env["GRASS_GUIWXDIR"]
        monkeypatch.syspath_prepend(gui_dir)
        try:
            yield session
        finally:
            # Third-party modules are kept, they should not be imported twice.
            prefix = os.path.normcase(gui_dir) + os.sep
            for name, module in list(sys.modules.items()):
                file = getattr(module, "__file__", None)
                if file and os.path.normcase(file).startswith(prefix):
                    del sys.modules[name]


@pytest.fixture(scope="session")
def wx_app():
    """The wx.App, kept for the whole session

    wxPython can reliably have only one application per process.
    """
    wx = pytest.importorskip("wx")
    if not wx.App.IsDisplayAvailable():
        pytest.skip("No display available for the GUI tests")
    app = wx.App()
    yield app
    # The destroyed windows are deleted by the wx cleanup at exit.
    for window in wx.GetTopLevelWindows():
        window.Destroy()
