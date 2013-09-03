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

(C) 2007-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
"""

from core.gcmd          import RunCommand
from gui_core.toolbars  import BaseToolbar, BaseIcons
from icons.icon         import MetaIcon
from core.utils import _

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
        icons = {
            'newdisplay'    : MetaIcon(img = 'monitor-create',
                                       label = _('Start new map display')),
            'workspaceNew'  : MetaIcon(img = 'create',
                                       label = _('Create new workspace (Ctrl+N)')),
            'workspaceOpen' : MetaIcon(img = 'open',
                                       label = _('Open existing workspace file (Ctrl+O)')),
            'workspaceSave' : MetaIcon(img = 'save',
                                       label = _('Save current workspace to file (Ctrl+S)')),
            }
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
        icons = {
            'addMulti'   : MetaIcon(img = 'layer-open',
                                    label = _('Add multiple raster or vector map layers (Ctrl+Shift+L)')),
            'addRast'    : BaseIcons['addRast'].SetLabel(_("Add raster map layer (Ctrl+Shift+R)")),
            'rastMisc'   : MetaIcon(img = 'layer-raster-more',
                                    label = _('Add various raster map layers (RGB, HIS, shaded relief...)')),
            'addVect'    : BaseIcons['addVect'].SetLabel(_("Add vector map layer (Ctrl+Shift+V)")),
            'vectMisc'   : MetaIcon(img = 'layer-vector-more',
                                    label = _('Add various vector map layers (thematic, chart...)')),
            'addWS'       : MetaIcon(img = 'layer-wms-add',
                                     label = _('Add web service layer (WMS, WMTS, NASA OnEarth)')),
            'addGroup'   : MetaIcon(img = 'layer-group-add',
                                    label = _('Add group')),
            'addOverlay' : MetaIcon(img = 'layer-more',
                                    label = _('Add various overlays')),
            'delCmd'     : MetaIcon(img = 'layer-remove',
                                    label = _('Remove selected map layer(s) from layer tree')),
            }
        
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
                                     ('addovl',  icons["addOverlay"],
                                      self.parent.OnAddOverlay),
                                     ('addWS',  icons["addWS"],
                                      self.parent.OnAddWS),
                                     (None, ),
                                     ('addgrp',  icons["addGroup"],
                                      self.parent.OnAddGroup),
                                     (None, ),
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
        icons = {
            'import'  : MetaIcon(img = 'layer-import',
                                 label = _('Import/link raster or vector data')),
            'mapcalc' : MetaIcon(img = 'raster-calculator',
                                 label = _('Raster Map Calculator')),
            'modeler' : MetaIcon(img = 'modeler-main',
                                 label = _('Graphical Modeler')),
            'georectify' : MetaIcon(img = 'georectify',
                                 label = _('Georectifier')),
            'composer': MetaIcon(img = 'print-compose',
                                 label = _('Cartographic Composer')),
            }
        
        return self._getToolbarData((('importMap', icons["import"],
                                      self.parent.OnImportMenu),
                                     (None, ),
                                     ('mapCalc', icons["mapcalc"],
                                      self.parent.OnMapCalculator),
                                     ('georect', icons["georectify"],
                                      self.parent.OnGCPManager),
                                     ('modeler', icons["modeler"],
                                      self.parent.OnGModeler),
                                     ('mapOutput', icons['composer'],
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
        icons = {
            'settings'   : BaseIcons['settings'].SetLabel(_('GUI settings')),
            'help'       : BaseIcons['help'].SetLabel(_('GRASS manual')),
            }
        
        return self._getToolbarData((('settings', icons["settings"],
                                      self.parent.OnPreferences),
                                     ('help', icons["help"],
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
        icons = {
            'vdigit'     : MetaIcon(img = 'edit',
                                    label = _('Edit selected vector map')),
            'attrTable'  : MetaIcon(img = 'table',
                                    label = _('Show attribute data for selected vector map')),
            }
        
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
        icons = {
            'cmd'    : MetaIcon(img = 'script-save',
                                label = _('Generate command for m.nviz.image'),
                                desc = _('Generate command for m.nviz.image based on current state')),
            'settings' : MetaIcon(img = '3d-settings',
                                  label = _('3D view mode settings'),
                                  desc = _('Show 3D view mode settings dialog')),
            'help'   : MetaIcon(img = '3d-help',
                                label = _('Show 3D view mode manual')),
            }
        
        return self._getToolbarData((("nvizCmd", icons['cmd'],
                                      self.OnNvizCmd),
                                     (None, ),
                                     ("settings", icons["settings"],
                                      self.parent.OnNvizPreferences),
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
