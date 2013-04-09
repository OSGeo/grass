"""!
@package gui_core.toolbars

@brief Base classes toolbar widgets

Classes:
 - toolbars::BaseToolbar

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
"""

import platform

import wx

from core               import globalvar
from core.debug         import Debug
from icons.icon         import MetaIcon

BaseIcons = {
    'display'    : MetaIcon(img = 'show',
                            label = _('Display map'),
                            desc  =  _('Re-render modified map layers only')),
    'render'     : MetaIcon(img = 'layer-redraw',
                            label = _('Render map'),
                            desc = _('Force re-rendering all map layers')),
    'erase'      : MetaIcon(img = 'erase',
                            label = _('Erase display'),
                            desc = _('Erase display canvas with given background color')),
    'pointer'    : MetaIcon(img = 'pointer',
                            label = _('Pointer')),
    'zoomIn'     : MetaIcon(img = 'zoom-in',
                            label = _('Zoom in'),
                            desc = _('Drag or click mouse to zoom')),
    'zoomOut'    : MetaIcon(img = 'zoom-out',
                            label = _('Zoom out'),
                            desc = _('Drag or click mouse to unzoom')),
    'zoomBack'   : MetaIcon(img = 'zoom-last',
                            label = _('Return to previous zoom')),
    'zoomMenu'   : MetaIcon(img = 'zoom-more',
                            label = _('Various zoom options'),
                            desc = _('Zoom to computational, default, saved region, ...')),
    'zoomExtent' : MetaIcon(img = 'zoom-extent',
                            label = _('Zoom to selected map layer(s)')),
    'pan'        : MetaIcon(img = 'pan',
                            label = _('Pan'),
                            desc = _('Drag with mouse to pan')),
    'saveFile'   : MetaIcon(img = 'map-export',
                            label = _('Save display to graphic file')),
    'print'      : MetaIcon(img = 'print',
                            label = _('Print display')),
    'font'       : MetaIcon(img = 'font',
                            label = _('Select font')),
    'help'       : MetaIcon(img = 'help',
                            label = _('Show manual')),
    'quit'       : MetaIcon(img = 'quit',
                            label = _('Quit')),
    'addRast'    : MetaIcon(img = 'layer-raster-add',
                            label = _('Add raster map layer')),
    'addVect'    : MetaIcon(img = 'layer-vector-add',
                            label = _('Add vector map layer')),
    'overlay'    : MetaIcon(img = 'overlay-add',
                            label = _('Add map elements'),
                            desc = _('Overlay elements like scale and legend onto map')),
    'histogramD' : MetaIcon(img = 'layer-raster-histogram',
                            label = _('Create histogram with d.histogram')),
    'settings'   : MetaIcon(img = 'settings',
                            label = _("Settings")),
    }
    
class BaseToolbar(wx.ToolBar):
    """!Abstract toolbar class.
    
    Following code shows how to create new basic toolbar:

    @code
        class MyToolbar(BaseToolbar):
            def __init__(self, parent):
                BaseToolbar.__init__(self, parent)
                self.InitToolbar(self._toolbarData())
                self.Realize()

            def _toolbarData(self):
                return self._getToolbarData((("help", Icons["help"],
                                              self.parent.OnHelp),
                                              ))
    @endcode
    """
    def __init__(self, parent):
        self.parent = parent
        wx.ToolBar.__init__(self, parent = self.parent, id = wx.ID_ANY)
        
        self.action = dict()
        
        self.Bind(wx.EVT_TOOL, self.OnTool)
        
        self.SetToolBitmapSize(globalvar.toolbarSize)
        
    def InitToolbar(self, toolData):
        """!Initialize toolbar, add tools to the toolbar
        """
        for tool in toolData:
            self.CreateTool(*tool)
        
        self._data = toolData
        
    def _toolbarData(self):
        """!Toolbar data (virtual)"""
        return None
    
    def CreateTool(self, label, bitmap, kind,
                   shortHelp, longHelp, handler, pos = -1):
        """!Add tool to the toolbar
        
        @param pos if -1 add tool, if > 0 insert at given pos
        @return id of tool
        """
        bmpDisabled = wx.NullBitmap
        tool = -1
        if label:
            tool = vars(self)[label] = wx.NewId()
            Debug.msg(3, "CreateTool(): tool=%d, label=%s bitmap=%s" % \
                          (tool, label, bitmap))
            if pos < 0:
                toolWin = self.AddLabelTool(tool, label, bitmap,
                                            bmpDisabled, kind,
                                            shortHelp, longHelp)
            else:
                toolWin = self.InsertLabelTool(pos, tool, label, bitmap,
                                            bmpDisabled, kind,
                                            shortHelp, longHelp)
            self.Bind(wx.EVT_TOOL, handler, toolWin)
        else: # separator
            self.AddSeparator()
        
        return tool
    
    def EnableLongHelp(self, enable = True):
        """!Enable/disable long help
        
        @param enable True for enable otherwise disable
        """
        for tool in self._data:
            if tool[0] == '': # separator
                continue
            
            if enable:
                self.SetToolLongHelp(vars(self)[tool[0]], tool[4])
            else:
                self.SetToolLongHelp(vars(self)[tool[0]], "")
        
    def OnTool(self, event):
        """!Tool selected
        """
        
        id = self.action.get('id', -1)
        
        if event:
            # deselect previously selected tool
            if id != -1 and id != event.GetId() :
                self.ToggleTool(self.action['id'], False)
            elif id != -1:
                self.ToggleTool(self.action['id'], True)
            
            self.action['id'] = event.GetId()
            
            event.Skip()
        elif id != -1:
            # initialize toolbar
            self.ToggleTool(self.action['id'], True)
        
    def GetAction(self, type = 'desc'):
        """!Get current action info"""
        return self.action.get(type, '')
    
    def SelectDefault(self, event):
        """!Select default tool"""
        self.ToggleTool(self.defaultAction['id'], True)
        self.defaultAction['bind'](event)
        self.action = { 'id' : self.defaultAction['id'],
                        'desc' : self.defaultAction.get('desc', '') }
        
    def FixSize(self, width):
        """!Fix toolbar width on Windows
            
        @todo Determine why combobox causes problems here
        """
        if platform.system() == 'Windows':
            size = self.GetBestSize()
            self.SetSize((size[0] + width, size[1]))

    def Enable(self, tool, enable = True):
        """!Enable/Disable defined tool
        
        @param tool name
        @param enable True to enable otherwise disable tool
        """
        try:
            id = getattr(self, tool)
        except AttributeError:
            return
        
        self.EnableTool(id, enable)

    def EnableAll(self, enable = True):
        """!Enable/Disable all tools
        
        @param enable True to enable otherwise disable tool
        """
        for item in self._toolbarData():
            if not item[0]:
                continue
            self.Enable(item[0], enable)
        
    def _getToolbarData(self, data):
        """!Define tool
        """
        retData = list()
        for args in data:
            retData.append(self._defineTool(*args))
        return retData

    def _defineTool(self, name = None, icon = None, handler = None, item = wx.ITEM_NORMAL, pos = -1):
        """!Define tool
        """
        if name:
            return (name, icon.GetBitmap(),
                    item, icon.GetLabel(), icon.GetDesc(),
                    handler, pos)
        return ("", "", "", "", "", "") # separator

    def _onMenu(self, data):
        """!Toolbar pop-up menu"""
        menu = wx.Menu()
        
        for icon, handler in data:
            item = wx.MenuItem(menu, wx.ID_ANY, icon.GetLabel())
            item.SetBitmap(icon.GetBitmap(self.parent.iconsize))
            menu.AppendItem(item)
            self.Bind(wx.EVT_MENU, handler, item)
        
        self.PopupMenu(menu)
        menu.Destroy()
