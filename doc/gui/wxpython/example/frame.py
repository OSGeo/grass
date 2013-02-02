"""!
@package example.frame

@brief Example tool for displaying raster map and related information

Classes:
 - frame::ExampleMapFrame
 - frame::ExampleInfoTextManager

(C) 2006-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import sys
import wx

# this enables to run application standalone (> python example/frame.py )
if __name__ == "__main__":
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from gui_core.mapdisp import SingleMapFrame
from mapdisp.mapwindow import BufferedWindow
from mapdisp import statusbar as sb
from core.render import Map
from core.debug import Debug
from core.gcmd import RunCommand

import grass.script as grass

from toolbars import ExampleMapToolbar, ExampleMiscToolbar, ExampleMainToolbar
from dialogs  import ExampleMapDialog

# It is possible to call grass library functions (in C) directly via ctypes
# however this is less stable. Example is available in trunk/doc/python/, ctypes
# are used in nviz, vdigit, iclass gui modules.

# from ctypes import *
# try:
#     from grass.lib.raster import *
#     haveExample = True
#     errMsg = ''
# except ImportError, e:
#     haveExample = False
#     errMsg = _("Loading raster lib failed.\n%s") % e

class ExampleMapFrame(SingleMapFrame):
    """! Main frame of example tool.
    
    Inherits from SingleMapFrame, so map is displayed in one map widow.
    In case two map windows are needed, use DoubleMapFrame from (gui_core.mapdisp).
    
    @see IClassMapFrame in iclass.frame
    """
    def __init__(self, parent = None, title = _("Example Tool"),
                 toolbars = ["MiscToolbar", "MapToolbar", "MainToolbar"],
                 size = (800, 600), name = 'exampleWindow', **kwargs):
        """!Map Frame constructor
        
        @param parent (no parent is expected)
        @param title window title
        @param toolbars list of active toolbars (default value represents all toolbars)
        @param size default size
        """
        SingleMapFrame.__init__(self, parent = parent, title = title,
                                name = name, Map = Map(), **kwargs)
        
        # Place debug message where appropriate
        # and set debug level from 1 to 5 (higher to lower level functions).
        # To enable debug mode write:
        # > g.gisenv set=WX_DEBUG=5
        Debug.msg(1, "ExampleMapFrame.__init__()")
        
        #
        # Add toolbars to aui manager
        #
        toolbarsCopy = toolbars[:]
        # workaround to have the same toolbar order on all platforms
        if sys.platform == 'win32':
            toolbarsCopy.reverse()
            
        for toolbar in toolbarsCopy:
            self.AddToolbar(toolbar)
        
        #
        # Add statusbar
        #
        
        # choose items in statusbar choice, which makes sense for your application
        self.statusbarItems = [sb.SbCoordinates,
                               sb.SbRegionExtent,
                               sb.SbCompRegionExtent,
                               sb.SbShowRegion,
                               sb.SbAlignExtent,
                               sb.SbResolution,
                               sb.SbDisplayGeometry,
                               sb.SbMapScale,
                               sb.SbGoTo,
                               sb.SbProjection]
        
        # create statusbar and its manager
        statusbar = self.CreateStatusBar(number = 4, style = 0)
        statusbar.SetStatusWidths([-5, -2, -1, -1])
        self.statusbarManager = sb.SbManager(mapframe = self, statusbar = statusbar)
        
        # fill statusbar manager
        self.statusbarManager.AddStatusbarItemsByClass(self.statusbarItems, mapframe = self, statusbar = statusbar)
        self.statusbarManager.AddStatusbarItem(sb.SbMask(self, statusbar = statusbar, position = 2))
        self.statusbarManager.AddStatusbarItem(sb.SbRender(self, statusbar = statusbar, position = 3))
        
        self.statusbarManager.Update()
        
        
        # create map window
        self.MapWindow = BufferedWindow(self, Map = self.GetMap(), frame = self, giface = self)
        
        # create whatever you want, here it is a widget for displaying raster info
        self.info = ExampleInfoTextManager(self)
        
        # add map window (and other widgets) to aui manager
        self._addPanes()
        self._mgr.Update()
        
        # default action in map toolbar
        self.OnPan(event = None)
        
        # initialize variables related to your application functionality
        self.InitVariables()
        
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        
        self.SetSize(size)
        
    def __del__(self):
        """!Destructor deletes temporary region"""
        grass.del_temp_region()
        
    def OnCloseWindow(self, event):
        """!Destroy frame"""
        self.Destroy()
        
    def IsStandalone(self):
        """!Check if application is standalone.
        
        Standalone application can work without parent.
        Parent can be e.g. Layer Manager.
        """
        if self.parent:
            return False
            
        return True
        
    def GetLayerManager(self):
        """!Returns frame parent.
        
        If IsStandalone() is True, returns None,
        otherwise retuns frame parent (Layer Manager)
        """
        return self.parent
            
    def InitVariables(self):
        """!Initialize any variables nneded by application"""
        self.currentRaster = None
        self.statitistics = dict()
        
        # use WIND_OVERRIDE region not to affect current region
        grass.use_temp_region()
        
    def _addPanes(self):
        """!Add mapwindow (and other widgets) to aui manager"""
        window = self.GetWindow()
        name = "mainWindow"
        self._mgr.AddPane(window, wx.aui.AuiPaneInfo().
                  Name(name).CentrePane().
                  Dockable(False).CloseButton(False).DestroyOnClose(True).
                  Layer(0))
                  
        window = self.info.GetControl()
        name = "infoText"
        self._mgr.AddPane(window, wx.aui.AuiPaneInfo().
                  Name(name).Caption(_("Raster Info")).MinSize((250, -1)).
                  Dockable(True).CloseButton(False).
                  Layer(0).Left())
                  
    def AddToolbar(self, name):
        """!Add defined toolbar to the window
        
        Currently known toolbars are:
         - 'ExampleMapToolbar'        - basic map toolbar
         - 'ExampleMainToolbar'       - toolbar with application specific tools
         - 'ExampleMiscToolbar'       - toolbar with common tools (help, quit, ...)
        """
        # see wx.aui.AuiPaneInfo documentation for understanding all options
        if name == "MapToolbar":
            self.toolbars[name] = ExampleMapToolbar(self)
            
            self._mgr.AddPane(self.toolbars[name],
                              wx.aui.AuiPaneInfo().
                              Name(name).Caption(_("Map Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(1).Row(1).
                              BestSize((self.toolbars[name].GetBestSize())))
                              
        if name == "MiscToolbar":
            self.toolbars[name] = ExampleMiscToolbar(self)
            
            self._mgr.AddPane(self.toolbars[name],
                              wx.aui.AuiPaneInfo().
                              Name(name).Caption(_("Misc Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(1).Row(1).
                              BestSize((self.toolbars[name].GetBestSize())))
                              
        if name == "MainToolbar":
            self.toolbars[name] = ExampleMainToolbar(self)
            
            self._mgr.AddPane(self.toolbars[name],
                              wx.aui.AuiPaneInfo().
                              Name(name).Caption(_("Main Toolbar")).
                              ToolbarPane().Top().
                              LeftDockable(False).RightDockable(False).
                              BottomDockable(False).TopDockable(True).
                              CloseButton(False).Layer(1).Row(1).
                              BestSize((self.toolbars[name].GetBestSize())))
                              
    def GetMapToolbar(self):
        """!Returns toolbar with zooming tools"""
        return self.toolbars['MapToolbar']
        
    def OnHelp(self, event):
        """!Show help page"""
        RunCommand('g.manual', entry = 'wxGUI.Example')
        
    def OnSelectRaster(self, event):
        """!Opens dialog to select raster map"""
        dlg = ExampleMapDialog(self)
        
        if dlg.ShowModal() == wx.ID_OK:
            raster = grass.find_file(name = dlg.GetRasterMap(), element = 'cell')
            if raster['fullname']:
                self.SetLayer(name = raster['fullname'])
                
        dlg.Destroy()
        
    def SetLayer(self, name):
        """!Sets layer in Map and updates statistics.
        
        @param name layer (raster) name
        """
        Debug.msg (3, "ExampleMapFrame.SetLayer(): name=%s" % name)
        
        # this simple application enables to keep only one raster
        self.GetMap().DeleteAllLayers()
        cmdlist = ['d.rast', 'map=%s' % name]
        # add layer to Map instance (core.render)
        newLayer = self.GetMap().AddLayer(ltype = 'raster', command = cmdlist, active = True,
                                          name = name, hidden = False, opacity = 1.0,
                                          render = True)
        self.GetWindow().ZoomToMap(layers = [newLayer,], render = True)
        self.currentRaster = name
        
        # change comp. region to match new raster, so that the statistics
        # are computed for the entire raster
        RunCommand('g.region',
                   rast = self.currentRaster,
                   parent = self)
        
        self.UpdateStatistics()
        
    def ComputeStatitistics(self):
        """!Computes statistics for raster map using 'r.univar' module.
        
        @return statistic in form of dictionary 
        """
        # RunCommand enables to run GRASS module
        res = RunCommand('r.univar', # module name
                         flags = 'g', # command flags
                         map = self.currentRaster, # module parameters
                         read = True) # get command output
                              
        return grass.parse_key_val(res, val_type = float)
        
    def UpdateStatistics(self):
        """!Upadate statistic information.
        
        Called after changing raster map.
        """
        stats = self.ComputeStatitistics()
        self.info.WriteStatistics(name = self.currentRaster, statDict = stats)
        
        
class ExampleInfoTextManager:
    """!Class for displaying information.
    
    Wrraper for wx.TextCtrl. 
    """
    def __init__(self, parent):
        """!Creates wx.TextCtrl for displaying information.
        """
        self.textCtrl = wx.TextCtrl(parent, id = wx.ID_ANY,
                                    style = wx.TE_MULTILINE | wx.TE_RICH2 | wx.TE_READONLY)
        self.textCtrl.SetInsertionPoint(0)
        self.textCtrl.Disable()
        self.font = self.textCtrl.GetFont()
        self.bgColor = wx.SystemSettings.GetColour(wx.SYS_COLOUR_BACKGROUND)
        
    def GetControl(self):
        """!Returns control itself."""
        return self.textCtrl
        
    def _setStyle(self, style):
        """!Sets default style of textCtrl.
        
        @param style "title"/"value"
        """
        if style == "title":
            self.font.SetWeight(wx.FONTWEIGHT_BOLD)
        elif style == "value":
            self.font.SetWeight(wx.FONTWEIGHT_NORMAL)
        else:
            return
            
        self.textCtrl.SetDefaultStyle(wx.TextAttr(colBack = self.bgColor, font = self.font))
    
    def _writeLine(self, title, value):
        """!Formats text (key, value pair) with styles."""
        self._setStyle("title")
        self.textCtrl.AppendText("%s: " % title)
        self._setStyle("value")
        self.textCtrl.AppendText("%.2f\n" % value)
        
    def _writeRasterTitle(self, name):
        """!Writes title."""
        self._setStyle("title")
        self.textCtrl.AppendText("%s\n\n" % name)
        
    def WriteStatistics(self, name, statDict):
        """!Write and format information about raster map
        
        @param name raster map name
        @param statDict dictionary containing information
        """
        self.GetControl().Clear()
        self._writeRasterTitle(name = name)
        for key, value in statDict.iteritems():
            self._writeLine(title = key, value = value)
    

def main():
    import gettext
    
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)
    
    app = wx.PySimpleApp()
    wx.InitAllImageHandlers()
    
    frame = ExampleMapFrame()
    frame.Show()
    app.MainLoop()

if __name__ == "__main__":
    main()
