"""!
@package lmgr.toolbars

@brief wxGUI Layer Manager - toolbars

Classes:
 - toolbars::LMWorkspaceToolbar
 - toolbars::LMDataToolbar
 - toolbars::LMToolsToolbar
 - toolbars::LMMiscToolbar
 - toolbars::LMVectorToolbar
 - toolbars::LMNvizToolbar

(C) 2007-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

import os
import sys

from core               import globalvar
from core.gcmd          import RunCommand
from nviz.preferences   import NvizPreferencesDialog
from gui_core.toolbars  import BaseToolbar

sys.path.append(os.path.join(globalvar.ETCWXDIR, "icons"))
from icons.icon        import Icons

class LMWorkspaceToolbar(BaseToolbar):
    """!Layer Manager `workspace` toolbar
    """
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['layerManager']
        return self._getToolbarData((('newdisplay', icons["newdisplay"],
                                      self.parent.OnNewDisplay),
                                     (None, ),
                                     ('workspaceNew', icons["workspaceNew"],
                                      self.parent.OnWorkspaceNew),
                                     ('workspaceOpen', icons["workspaceOpen"],
                                      self.parent.OnWorkspaceOpen),
                                     ('workspaceSave', icons["workspaceSave"],
                                      self.parent.OnWorkspaceSave),
                                     ))

class LMDataToolbar(BaseToolbar):
    """!Layer Manager `data` toolbar
    """
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['layerManager']
        return self._getToolbarData((('addMulti', icons["addMulti"],
                                      self.parent.OnAddMaps),
                                     ('addrast', icons["addRast"],
                                      self.parent.OnAddRaster),
                                     ('rastmisc', icons["rastMisc"],
                                      self.parent.OnAddRasterMisc),
                                     ('addvect', icons["addVect"],
                                      self.parent.OnAddVector),
                                     ('vectmisc', icons["vectMisc"],
                                      self.parent.OnAddVectorMisc),
                                     ('addgrp',  icons["addGroup"],
                                      self.parent.OnAddGroup),
                                     ('addovl',  icons["addOverlay"],
                                      self.parent.OnAddOverlay),
                                     ('delcmd',  icons["delCmd"],
                                      self.parent.OnDeleteLayer),
                                     ))

class LMToolsToolbar(BaseToolbar):
    """!Layer Manager `tools` toolbar
    """
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['layerManager']
        return self._getToolbarData((('importMap', icons["import"],
                                      self.parent.OnImportMenu),
                                     (None, ),
                                     ('mapCalc', icons["mapcalc"],
                                      self.parent.OnMapCalculator),
                                     ('georect', Icons["georectify"]["georectify"],
                                      self.parent.OnGCPManager),
                                     ('modeler', icons["modeler"],
                                      self.parent.OnGModeler),
                                     ('mapOutput', icons['mapOutput'],
                                      self.parent.OnPsMap)
                                     ))

class LMMiscToolbar(BaseToolbar):
    """!Layer Manager `misc` toolbar
    """
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['layerManager']
        return self._getToolbarData((('settings', icons["settings"],
                                      self.parent.OnPreferences),
                                     ('help', Icons["misc"]["help"],
                                      self.parent.OnHelp),
                                     ))

class LMVectorToolbar(BaseToolbar):
    """!Layer Manager `vector` toolbar
    """
    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """!Toolbar data
        """
        icons = Icons['layerManager']
        return self._getToolbarData((('vdigit', icons["vdigit"],
                                      self.parent.OnVDigit),
                                     ('attribute', icons["attrTable"],
                                      self.parent.OnShowAttributeTable),
                                     ))

class LMNvizToolbar(BaseToolbar):
    """!Nviz toolbar
    """
    def __init__(self, parent):
        self.lmgr = parent
        
        BaseToolbar.__init__(self, parent)
        
        # only one dialog can be open
        self.settingsDialog   = None
        
        self.InitToolbar(self._toolbarData())
        
        # realize the toolbar
        self.Realize()
        
    def _toolbarData(self):
        """!Toolbar data"""
        icons = Icons['nviz']
        return self._getToolbarData((("nvizCmd", icons['nvizCmd'],
                                      self.OnNvizCmd),
                                     (None, ),
                                     ("settings", icons["settings"],
                                      self.OnSettings),
                                     ("help", icons["help"],
                                      self.OnHelp))
                                    )
        
    def OnNvizCmd(self, event):
        """!Show m.nviz.image command"""
        self.lmgr.GetLayerTree().GetMapDisplay().GetWindow().OnNvizCmd()
        
    def OnHelp(self, event):
        """!Show 3D view mode help"""
        if not self.lmgr:
            RunCommand('g.manual',
                       entry = 'wxGUI.Nviz')
        else:
            log = self.lmgr.GetLogWindow()
            log.RunCmd(['g.manual',
                        'entry=wxGUI.Nviz'])
        
    def OnSettings(self, event):
        """!Show nviz notebook page"""
        if not self.settingsDialog:
            self.settingsDialog = NvizPreferencesDialog(parent = self.parent)
        self.settingsDialog.Show()
