"""!
@package wxgui

@brief Main Python application for GRASS wxPython GUI

Classes:
 - wxgui::GMApp
 - wxgui::Usage

(C) 2006-2011 by the GRASS Development Team

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

if __name__ == "__main__":
    gui_wx_path = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
    if gui_wx_path not in sys.path:
        sys.path.append(gui_wx_path)

from core import globalvar
from core.utils import _

import wx
try:
    import wx.lib.agw.advancedsplash as SC
except ImportError:
    SC = None

from lmgr.frame import GMFrame

class GMApp(wx.App):
    def __init__(self, workspace = None):
        """!Main GUI class.

        @param workspace path to the workspace file
        """
        self.workspaceFile = workspace
        
        # call parent class initializer
        wx.App.__init__(self, False)
        
        self.locale = wx.Locale(language = wx.LANGUAGE_DEFAULT)
        
    def OnInit(self):
        """!Initialize all available image handlers
        
        @return True
        """
        if not globalvar.CheckWxVersion([2, 9]):
            wx.InitAllImageHandlers()
        
        # create splash screen
        introImagePath = os.path.join(globalvar.ETCIMGDIR, "silesia_splash.png")
        introImage     = wx.Image(introImagePath, wx.BITMAP_TYPE_PNG)
        introBmp       = introImage.ConvertToBitmap()
        if SC and sys.platform != 'darwin':
            # AdvancedSplash is buggy on the Mac as of 2.8.12.1 
            # and raises annoying (though seemingly harmless) errors everytime the GUI is started
            splash = SC.AdvancedSplash(bitmap = introBmp, 
                                       timeout = 2000, parent = None, id = wx.ID_ANY)
            splash.SetText(_('Starting GRASS GUI...'))
            splash.SetTextColour(wx.Colour(45, 52, 27))
            splash.SetTextFont(wx.Font(pointSize = 15, family = wx.DEFAULT, style = wx.NORMAL,
                                       weight = wx.BOLD))
            splash.SetTextPosition((150, 430))
        else:
            wx.SplashScreen (bitmap = introBmp, splashStyle = wx.SPLASH_CENTRE_ON_SCREEN | wx.SPLASH_TIMEOUT,
                             milliseconds = 2000, parent = None, id = wx.ID_ANY)
        
        wx.Yield()
        
        # create and show main frame
        mainframe = GMFrame(parent = None, id = wx.ID_ANY,
                            workspace = self.workspaceFile)
        
        mainframe.Show()
        self.SetTopWindow(mainframe)
        
        return True

class Usage(Exception):
    def __init__(self, msg):
        self.msg = msg

def printHelp():
    """!Print program help"""
    print >> sys.stderr, "Usage:"
    print >> sys.stderr, " python wxgui.py [options]"
    print >> sys.stderr, "%sOptions:" % os.linesep
    print >> sys.stderr, " -w\t--workspace file\tWorkspace file to load"
    sys.exit(1)

def process_opt(opts, args):
    """!Process command-line arguments"""
    workspaceFile = None
    for o, a in opts:
        if o in ("-h", "--help"):
            printHelp()
            
        if o in ("-w", "--workspace"):
            if a != '':
                workspaceFile = str(a)
            else:
                workspaceFile = args.pop(0)

    return (workspaceFile,)

def main(argv = None):
    
    if argv is None:
        argv = sys.argv
    try:
        try:
            opts, args = getopt.getopt(argv[1:], "hw:",
                                       ["help", "workspace"])
        except getopt.error, msg:
            raise Usage(msg)
    
    except Usage, err:
        print >> sys.stderr, err.msg
        print >> sys.stderr, "for help use --help"
        printHelp()
    
    workspaceFile = process_opt(opts, args)[0]
    
    app = GMApp(workspaceFile)
    # suppress wxPython logs
    q = wx.LogNull()
    
    app.MainLoop()
    
if __name__ == "__main__":
    sys.exit(main())
