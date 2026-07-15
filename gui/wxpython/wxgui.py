"""
@package wxgui

@brief Main Python application for GRASS wxPython GUI

Classes:
 - wxgui::GMApp

(C) 2006-2015 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton (Arizona State University)
@author Jachym Cepicky (Mendel University of Agriculture)
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (menu customization)
"""

import os
import sys
import getopt

# i18n is taken care of in the grass library code.
# So we need to import it before any of the GUI code.
from grass.exceptions import Usage
from grass.script.core import set_raise_on_error, warning, error

from core import globalvar
from core.utils import registerPid, unregisterPid
from core.settings import UserSettings

import wx

# import adv and html before wx.App is created, otherwise
# we get annoying "Debug: Adding duplicate image handler for 'Windows bitmap file'"
# during start up, remove when not needed
import wx.adv

try:
    import wx.lib.agw.advancedsplash as SC
except ImportError:
    SC = None


class GMApp(wx.App):
    def __init__(self, workspace=None):
        """Main GUI class.

        :param workspace: path to the workspace file
        """
        self.workspaceFile = workspace

        # call parent class initializer
        wx.App.__init__(self, False)

        self.locale = wx.Locale(language=wx.LANGUAGE_DEFAULT)

    def OnInit(self):
        """Initialize all available image handlers

        :return: True
        """
        # Internal and display name of the app (if supported by/on platform)
        self.SetAppName("GRASS")
        self.SetVendorName("The GRASS Development Team")

        # create splash screen
        introImagePath = os.path.join(globalvar.IMGDIR, "splash_screen.png")
        introImage = wx.Image(introImagePath, wx.BITMAP_TYPE_PNG)
        introBmp = introImage.ConvertToBitmap()
        wx.adv.SplashScreen(
            bitmap=introBmp,
            splashStyle=wx.adv.SPLASH_CENTRE_ON_SCREEN | wx.adv.SPLASH_TIMEOUT,
            milliseconds=3000,
            parent=None,
            id=wx.ID_ANY,
        )

        wx.GetApp().Yield()

        def show_main_gui():
            # create and show main frame
            single = UserSettings.Get(
                group="appearance", key="singleWindow", subkey="enabled"
            )
            if single:
                from main_window.frame import GMFrame
            else:
                from lmgr.frame import GMFrame
            try:
                mainframe = GMFrame(
                    parent=None, id=wx.ID_ANY, workspace=self.workspaceFile
                )
            except Exception as err:
                min_required_wx_version = [4, 2, 0]
                if not globalvar.CheckWxVersion(min_required_wx_version):
                    error(err)
                    warning(
                        _(
                            "Current version of wxPython {} is lower than "
                            "minimum required version {}"
                        ).format(
                            wx.__version__,
                            ".".join(map(str, min_required_wx_version)),
                        )
                    )
                else:
                    raise
            else:
                mainframe.Show()
                self.SetTopWindow(mainframe)

        wx.CallAfter(show_main_gui)

        return True

    def OnExit(self):
        """Clean up on exit"""
        unregisterPid(os.getpid())
        return super().OnExit()


def printHelp():
    """Print program help"""
    print("Usage:", file=sys.stderr)
    print(" python wxgui.py [options]", file=sys.stderr)
    print("%sOptions:" % os.linesep, file=sys.stderr)
    print(" -w\t--workspace file\tWorkspace file to load", file=sys.stderr)
    sys.exit(1)


def process_opt(opts, args):
    """Process command-line arguments"""
    workspaceFile = None
    for o, a in opts:
        if o in {"-h", "--help"}:
            printHelp()

        elif o in {"-w", "--workspace"}:
            workspaceFile = str(a) if a != "" else args.pop(0)

    return workspaceFile


def main(argv=None):
    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.getopt(argv[1:], "hw:", ["help", "workspace"])
        except getopt.error as msg:
            raise Usage(msg)
    except Usage as err:
        print(err.msg, file=sys.stderr)
        print(sys.stderr, "for help use --help", file=sys.stderr)
        printHelp()

    workspaceFile = process_opt(opts, args)
    app = GMApp(workspaceFile)

    # suppress wxPython logs
    q = wx.LogNull()  # noqa: F841
    set_raise_on_error(True)

    # register GUI PID
    registerPid(os.getpid())

    app.MainLoop()


if __name__ == "__main__":
    sys.exit(main())
