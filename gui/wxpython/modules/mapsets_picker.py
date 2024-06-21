#!/usr/bin/env python3

import wx

from grass.script.setup import set_gui_path

set_gui_path()

# Imports require path to GUI code to be set.
from core.gcmd import RunCommand  # noqa: E402
from gui_core.preferences import MapsetAccess  # noqa: E402


def main():
    app = wx.App()

    dlg = MapsetAccess(parent=None)
    dlg.CenterOnScreen()

    if dlg.ShowModal() == wx.ID_OK:
        ms = dlg.GetMapsets()
        RunCommand(
            "g.mapsets", parent=None, mapset="%s" % ",".join(ms), operation="set"
        )

    dlg.Destroy()


if __name__ == "__main__":
    main()
