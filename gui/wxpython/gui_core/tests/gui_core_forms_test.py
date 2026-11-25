# SPDX-License-Identifier: GPL-2.0-or-later

"""Tests of the dialogs generated from tool interfaces"""

import pytest

pytest.importorskip("wx")

pytestmark = pytest.mark.needs_solo_run


def test_task_frame_creates_command_from_values(gui_session, wx_app):
    """The command created by the dialog includes the values set in the dialog"""
    # The GUI is importable only with the session fixture active.
    from gui_core.forms import GUI  # noqa: PLC0415

    gui = GUI(show=True)
    gui.ParseCommand(["g.region", "n=10", "res=2", "-p"])
    frame = gui.mf
    try:
        command = frame.createCmd()
        assert command[0] == "g.region"
        assert {"-p", "n=10", "res=2"} <= set(command)
    finally:
        frame._Destroy()  # noqa: SLF001
