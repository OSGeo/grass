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

import os
import sys
import platform

import wx

from core               import globalvar
from core.debug         import Debug

class BaseToolbar(wx.ToolBar):
    """!Abstract toolbar class"""
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
        if self.parent.GetName() == "GCPFrame":
            return
        
        if hasattr(self.parent, 'toolbars'):
            if self.parent.GetToolbar('vdigit'):
                # update vdigit toolbar (unselect currently selected tool)
                id = self.parent.toolbars['vdigit'].GetAction(type = 'id')
                self.parent.toolbars['vdigit'].ToggleTool(id, False)
        
        if event:
            # deselect previously selected tool
            id = self.action.get('id', -1)
            if id != event.GetId():
                self.ToggleTool(self.action['id'], False)
            else:
                self.ToggleTool(self.action['id'], True)
            
            self.action['id'] = event.GetId()
            
            event.Skip()
        else:
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
