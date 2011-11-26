"""!
@package mapdisp.toolbars

@brief Map display frame - toolbars

Classes:
 - toolbars::MapToolbar

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
"""

import wx

from gui_core.toolbars import BaseToolbar
from icons.icon        import Icons
from nviz.main         import haveNviz
from vdigit.main       import haveVDigit

class MapToolbar(BaseToolbar):
    """!Map Display toolbar
    """
    def __init__(self, parent, mapcontent):
        """!Map Display constructor

        @param parent reference to MapFrame
        @param mapcontent reference to render.Map (registred by MapFrame)
        """
        self.mapcontent = mapcontent # render.Map
        BaseToolbar.__init__(self, parent = parent) # MapFrame
        
        self.InitToolbar(self._toolbarData())
        
        # optional tools
        choices = [ _('2D view'), ]
        self.toolId = { '2d' : 0 }
        if self.parent.GetLayerManager():
            log = self.parent.GetLayerManager().GetLogWindow()
        
        if haveNviz:
            choices.append(_('3D view'))
            self.toolId['3d'] = 1
        else:
            from nviz.main import errorMsg
            log.WriteCmdLog(_('3D view mode not available'))
            log.WriteWarning(_('Reason: %s') % str(errorMsg))
            log.WriteLog(_('Note that the wxGUI\'s 3D view mode is currently disabled '
                           'on MS Windows (hopefully this will be fixed soon). '
                           'Please keep an eye out for updated versions of GRASS. '
                           'In the meantime you can use "NVIZ" from the File menu.'), wrap = 60)
            
            self.toolId['3d'] = -1

        if haveVDigit:
            choices.append(_('Digitize'))
            if self.toolId['3d'] > -1:
                self.toolId['vdigit'] = 2
            else:
                self.toolId['vdigit'] = 1
        else:
            from vdigit.main import errorMsg
            log.WriteCmdLog(_('Vector digitizer not available'))
            log.WriteWarning(_('Reason: %s') % errorMsg)
            log.WriteLog(_('Note that the wxGUI\'s vector digitizer is currently disabled '
                           '(hopefully this will be fixed soon). '
                           'Please keep an eye out for updated versions of GRASS. '
                           'In the meantime you can use "v.digit" from the Develop Vector menu.'), wrap = 60)
            
            self.toolId['vdigit'] = -1
        
        self.combo = wx.ComboBox(parent = self, id = wx.ID_ANY,
                                 choices = choices,
                                 style = wx.CB_READONLY, size = (110, -1))
        self.combo.SetSelection(0)
        
        self.comboid = self.AddControl(self.combo)
        self.parent.Bind(wx.EVT_COMBOBOX, self.OnSelectTool, self.comboid)
        
        # realize the toolbar
        self.Realize()
        
        # workaround for Mac bug. May be fixed by 2.8.8, but not before then.
        self.combo.Hide()
        self.combo.Show()
        
        self.action = { 'id' : self.pointer }
        self.defaultAction = { 'id' : self.pointer,
                               'bind' : self.parent.OnPointer }
        
        self.OnTool(None)
        
        self.EnableTool(self.zoomback, False)
        
        self.FixSize(width = 90)
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = Icons['displayWindow']
        return self._getToolbarData((('displaymap', icons['display'],
                                      self.parent.OnDraw),
                                     ('rendermap', icons['render'],
                                      self.parent.OnRender),
                                     ('erase', icons['erase'],
                                      self.parent.OnErase),
                                     (None, ),
                                     ('pointer', icons['pointer'],
                                      self.parent.OnPointer,
                                      wx.ITEM_CHECK),
                                     ('query', icons['query'],
                                      self.parent.OnQuery,
                                      wx.ITEM_CHECK),
                                     ('pan', icons['pan'],
                                      self.parent.OnPan,
                                      wx.ITEM_CHECK),
                                     ('zoomin', icons['zoomIn'],
                                      self.parent.OnZoomIn,
                                      wx.ITEM_CHECK),
                                     ('zoomout', icons['zoomOut'],
                                      self.parent.OnZoomOut,
                                      wx.ITEM_CHECK),
                                     ('zoomextent', icons['zoomExtent'],
                                      self.parent.OnZoomToMap),
                                     ('zoomback', icons['zoomBack'],
                                      self.parent.OnZoomBack),
                                     ('zoommenu', icons['zoomMenu'],
                                      self.parent.OnZoomMenu),
                                     (None, ),
                                     ('analyze', icons['analyze'],
                                      self.parent.OnAnalyze),
                                     (None, ),
                                     ('dec', icons['overlay'],
                                      self.parent.OnDecoration),
                                     (None, ),
                                     ('savefile', icons['saveFile'],
                                      self.parent.SaveToFile),
                                     ('printmap', icons['print'],
                                      self.parent.PrintMenu),
                                     (None, ))
                                    )
    def InsertTool(self, data):
        """!Insert tool to toolbar
        
        @param data toolbar data"""
        data = self._getToolbarData(data)
        for tool in data:
            self.CreateTool(*tool)
        self.Realize()
        
        self.parent._mgr.GetPane('mapToolbar').BestSize(self.GetBestSize())
        self.parent._mgr.Update()
        
    def RemoveTool(self, tool):
        """!Remove tool from toolbar
        
        @param tool tool id"""
        self.DeleteTool(tool)
        
        self.parent._mgr.GetPane('mapToolbar').BestSize(self.GetBestSize())
        self.parent._mgr.Update()
        
    def ChangeToolsDesc(self, mode2d):
        """!Change description of zoom tools for 2D/3D view"""
        if mode2d:
            set = 'displayWindow'
        else:
            set = 'nviz'
        for i, data in enumerate(self._data):
            for tool, toolname in (('zoomin', 'zoomIn'),('zoomout', 'zoomOut')):
                if data[0] == tool:
                    tmp = list(data)
                    tmp[4] = Icons[set][toolname].GetDesc()
                    self._data[i] = tuple(tmp)
                
    def OnSelectTool(self, event):
        """!Select / enable tool available in tools list
        """
        tool =  event.GetSelection()
        
        if tool == self.toolId['2d']:
            self.ExitToolbars()
            self.Enable2D(True)
            self.ChangeToolsDesc(mode2d = True)            
        
        elif tool == self.toolId['3d'] and \
                not (self.parent.MapWindow3D and self.parent.IsPaneShown('3d')):
            self.ExitToolbars()
            self.parent.AddNviz()
            
        elif tool == self.toolId['vdigit'] and \
                not self.parent.GetToolbar('vdigit'):
            self.ExitToolbars()
            self.parent.AddToolbar("vdigit")
            self.parent.MapWindow.SetFocus()
        
    def ExitToolbars(self):
        if self.parent.GetToolbar('vdigit'):
            self.parent.toolbars['vdigit'].OnExit()
        if self.parent.GetLayerManager().IsPaneShown('toolbarNviz'):
            self.parent.RemoveNviz()
        
    def Enable2D(self, enabled):
        """!Enable/Disable 2D display mode specific tools"""
        for tool in (self.zoommenu,
                     self.analyze,
                     self.printmap):
            self.EnableTool(tool, enabled)
