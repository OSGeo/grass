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

from __future__ import print_function

import os
import sys
import getopt

# i18n is taken care of in the grass library code.
# So we need to import it before any of the GUI code.
from grass.exceptions import Usage
from grass.script.core import set_raise_on_error

from core import globalvar
from core.utils import registerPid, unregisterPid

import wx
# import adv and html before wx.App is created, otherwise
# we get annoying "Debug: Adding duplicate image handler for 'Windows bitmap file'"
# during start up, remove when not needed
import wx.adv
import wx.html
try:
    import wx.lib.agw.advancedsplash as SC
except ImportError:
    SC = None


class GMApp(wx.App):

    def __init__(self, workspace=None):
        """ Main GUI class.

        :param workspace: path to the workspace file
        """
        self.workspaceFile = workspace

        # call parent class initializer
        wx.App.__init__(self, False)

        self.locale = wx.Locale(language=wx.LANGUAGE_DEFAULT)

    def OnInit(self):
        """ Initialize all available image handlers

        :return: True
        """
        # Internal and display name of the app (if supported by/on platform)
        self.SetAppName("GRASS GIS")
        self.SetVendorName("The GRASS Development Team")

        # create splash screen
        introImagePath = os.path.join(globalvar.IMGDIR, "splash_screen.png")
        introImage = wx.Image(introImagePath, wx.BITMAP_TYPE_PNG)
        introBmp = introImage.ConvertToBitmap()
        if SC and sys.platform != 'darwin':
            # AdvancedSplash is buggy on the Mac as of 2.8.12.1
            # and raises annoying (though seemingly harmless) errors everytime
            # the GUI is started
            splash = SC.AdvancedSplash(bitmap=introBmp,
                                       timeout=2000, parent=None, id=wx.ID_ANY)
            splash.SetText(_('Starting GRASS GUI...'))
            splash.SetTextColour(wx.Colour(45, 52, 27))
            splash.SetTextFont(
                wx.Font(
                    pointSize=15,
                    family=wx.DEFAULT,
                    style=wx.NORMAL,
                    weight=wx.BOLD))
            splash.SetTextPosition((150, 430))
        else:
            if globalvar.wxPythonPhoenix:
                import wx.adv as wxadv
                wxadv.SplashScreen(
                    bitmap=introBmp,
                    splashStyle=wxadv.SPLASH_CENTRE_ON_SCREEN | wxadv.SPLASH_TIMEOUT,
                    milliseconds=2000,
                    parent=None,
                    id=wx.ID_ANY)
            else:
                wx.SplashScreen(
                    bitmap=introBmp,
                    splashStyle=wx.SPLASH_CENTRE_ON_SCREEN | wx.SPLASH_TIMEOUT,
                    milliseconds=2000,
                    parent=None,
                    id=wx.ID_ANY)

        wx.GetApp().Yield()

        # create and show main frame
        from lmgr.frame import GMFrame
        mainframe = GMFrame(parent=None, id=wx.ID_ANY,
                            workspace=self.workspaceFile)

        mainframe.Show()
        self.SetTopWindow(mainframe)

        return True

    def OnExit(self):
        """Clean up on exit"""
        unregisterPid(os.getpid())
        return super().OnExit()


def printHelp():
    """ Print program help"""
    print("Usage:", file=sys.stderr)
    print(" python wxgui.py [options]", file=sys.stderr)
    print("%sOptions:" % os.linesep, file=sys.stderr)
    print(" -w\t--workspace file\tWorkspace file to load", file=sys.stderr)
    sys.exit(1)


def process_opt(opts, args):
    """ Process command-line arguments"""
    workspaceFile = None
    for o, a in opts:
        if o in ("-h", "--help"):
            printHelp()

        elif o in ("-w", "--workspace"):
            if a != '':
                workspaceFile = str(a)
            else:
                workspaceFile = args.pop(0)

    return workspaceFile


def main(argv=None):

    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.getopt(argv[1:], "hw:",
                                       ["help", "workspace"])
        except getopt.error as msg:
            raise Usage(msg)
    except Usage as err:
        print(err.msg, file=sys.stderr)
        print(sys.stderr, "for help use --help", file=sys.stderr)
        printHelp()

    workspaceFile = process_opt(opts, args)
    app = GMApp(workspaceFile)

    # suppress wxPython logs
    q = wx.LogNull()
    set_raise_on_error(True)

    # register GUI PID
    registerPid(os.getpid())

    app.MainLoop()

if __name__ == "__main__":
    sys.exit(main())
