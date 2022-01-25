"""
@package lmgr.toolbars

@brief wxGUI Layer Manager - toolbars

Classes:
 - toolbars::LMWorkspaceToolbar
 - toolbars::DisplayPanelToolbar
 - toolbars::LMToolsToolbar
 - toolbars::LMMiscToolbar
 - toolbars::LMNvizToolbar


(C) 2007-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Jachym Cepicky
@author Martin Landa <landa.martin gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com>
@author Linda Kladivova <linda.kladivova gmail com>
"""

from core.gcmd import RunCommand
from gui_core.toolbars import BaseToolbar, BaseIcons
from icons.icon import MetaIcon


class LMWorkspaceToolbar(BaseToolbar):
    """Layer Manager `workspace` toolbar"""

    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Toolbar data"""
        icons = {
            "workspaceNew": MetaIcon(
                img="create", label=_("Create new workspace (Ctrl+N)")
            ),
            "workspaceOpen": MetaIcon(
                img="open", label=_("Open existing workspace file (Ctrl+O)")
            ),
            "workspaceSave": MetaIcon(
                img="save", label=_("Save current workspace to file (Ctrl+S)")
            ),
        }
        return self._getToolbarData(
            (
                (
                    ("workspaceNew", _("New workspace")),
                    icons["workspaceNew"],
                    self.parent.OnWorkspaceNew,
                ),
                (
                    ("workspaceOpen", _("Open workspace")),
                    icons["workspaceOpen"],
                    self.parent.OnWorkspaceOpen,
                ),
                (
                    ("workspaceSave", _("Save workspace")),
                    icons["workspaceSave"],
                    self.parent.OnWorkspaceSave,
                ),
            )
        )


class DisplayPanelToolbar(BaseToolbar):
    """Toolbar for display tab"""

    def __init__(self, guiparent, parent):
        BaseToolbar.__init__(self, guiparent)
        self.parent = parent

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Toolbar data"""
        icons = {
            "addMulti": MetaIcon(
                img="layer-open",
                label=_("Add multiple raster or vector map layers (Ctrl+Shift+L)"),
            ),
            "addRast": BaseIcons["addRast"].SetLabel(
                _("Add raster map layer (Ctrl+Shift+R)")
            ),
            "rastMisc": MetaIcon(
                img="layer-raster-more",
                label=_("Add various raster map layers (RGB, HIS, shaded relief...)"),
            ),
            "addVect": BaseIcons["addVect"].SetLabel(
                _("Add vector map layer (Ctrl+Shift+V)")
            ),
            "vectMisc": MetaIcon(
                img="layer-vector-more",
                label=_("Add various vector map layers (thematic, chart...)"),
            ),
            "addWS": MetaIcon(
                img="layer-wms-add",
                label=_("Add web service layer (WMS, WMTS, NASA OnEarth)"),
            ),
            "addGroup": MetaIcon(img="layer-group-add", label=_("Add group")),
            "addOverlay": MetaIcon(img="layer-more", label=_("Add various overlays")),
            "delCmd": MetaIcon(
                img="layer-remove",
                label=_("Remove selected map layer(s) from layer tree"),
            ),
            "vdigit": MetaIcon(img="edit", label=_("Edit selected vector map")),
            "attrTable": MetaIcon(
                img="table", label=_("Show attribute data for selected vector map")
            ),
        }

        return self._getToolbarData(
            (
                (
                    ("addMulti", _("Add multiple map layers")),
                    icons["addMulti"],
                    self.parent.OnAddMaps,
                ),
                (
                    ("addrast", _("Add raster")),
                    icons["addRast"],
                    self.parent.OnAddRaster,
                ),
                (
                    ("rastmisc", _("Add various raster")),
                    icons["rastMisc"],
                    self.parent.OnAddRasterMisc,
                ),
                (
                    ("addvect", _("Add vector")),
                    icons["addVect"],
                    self.parent.OnAddVector,
                ),
                (
                    ("vectmisc", _("Add various vector")),
                    icons["vectMisc"],
                    self.parent.OnAddVectorMisc,
                ),
                (
                    ("addovl", _("Add overlay")),
                    icons["addOverlay"],
                    self.parent.OnAddOverlay,
                ),
                (
                    ("addWS", _("Add web service")),
                    icons["addWS"],
                    self.parent.OnAddWS,
                ),
                (None,),
                (
                    ("addgrp", _("Add group")),
                    icons["addGroup"],
                    self.parent.OnAddGroup,
                ),
                (
                    ("delcmd", _("Delete map layer")),
                    icons["delCmd"],
                    self.parent.OnDeleteLayer,
                ),
                (None,),
                (
                    ("vdigit", _("Vector digitizer")),
                    icons["vdigit"],
                    self.parent.OnVDigit,
                ),
                (
                    ("attribute", _("Attribute table")),
                    icons["attrTable"],
                    self.parent.OnShowAttributeTable,
                ),
            )
        )


class LMToolsToolbar(BaseToolbar):
    """Layer Manager `tools` toolbar"""

    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Toolbar data"""
        icons = {
            "newdisplay": MetaIcon(
                img="monitor-create", label=_("Start new map display")
            ),
            "mapcalc": MetaIcon(
                img="raster-calculator", label=_("Raster Map Calculator")
            ),
            "modeler": MetaIcon(img="modeler-main", label=_("Graphical Modeler")),
            "georectify": MetaIcon(img="georectify", label=_("Georectifier")),
            "composer": MetaIcon(img="print-compose", label=_("Cartographic Composer")),
            "script-load": MetaIcon(
                img="script-load", label=_("Launch user-defined script")
            ),
            "python": MetaIcon(
                img="python", label=_("Open a simple Python code editor")
            ),
        }

        return self._getToolbarData(
            (
                (
                    ("newdisplay", _("New display")),
                    icons["newdisplay"],
                    self.parent.OnNewDisplay,
                ),
                (None,),
                (
                    ("mapCalc", _("Raster Map Calculator")),
                    icons["mapcalc"],
                    self.parent.OnMapCalculator,
                ),
                (
                    ("georect", _("Georectifier")),
                    icons["georectify"],
                    self.parent.OnGCPManager,
                ),
                (
                    ("modeler", _("Graphical Modeler")),
                    icons["modeler"],
                    self.parent.OnGModeler,
                ),
                (
                    ("mapOutput", _("Cartographic Composer")),
                    icons["composer"],
                    self.parent.OnPsMap,
                ),
                (None,),
                (
                    ("script-load", _("Launch user-defined script")),
                    icons["script-load"],
                    self.parent.OnRunScript,
                ),
                (
                    ("python", _("Python code editor")),
                    icons["python"],
                    self.parent.OnSimpleEditor,
                ),
            )
        )


class LMMiscToolbar(BaseToolbar):
    """Layer Manager `misc` toolbar"""

    def __init__(self, parent):
        BaseToolbar.__init__(self, parent)

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Toolbar data"""
        icons = {
            "settings": BaseIcons["settings"].SetLabel(_("GUI settings")),
            "help": BaseIcons["help"].SetLabel(_("GRASS manual")),
        }

        return self._getToolbarData(
            (
                (
                    ("settings", _("GUI settings")),
                    icons["settings"],
                    self.parent.OnPreferences,
                ),
                (
                    ("help", _("GRASS manual")),
                    icons["help"],
                    self.parent.OnHelp,
                ),
            )
        )


class LMNvizToolbar(BaseToolbar):
    """Nviz toolbar"""

    def __init__(self, parent):
        self.lmgr = parent

        BaseToolbar.__init__(self, parent)

        # only one dialog can be open
        self.settingsDialog = None

        self.InitToolbar(self._toolbarData())

        # realize the toolbar
        self.Realize()

    def _toolbarData(self):
        """Toolbar data"""
        icons = {
            "cmd": MetaIcon(
                img="script-save",
                label=_("Generate command for m.nviz.image"),
                desc=_("Generate command for m.nviz.image based on current state"),
            ),
            "settings": MetaIcon(
                img="3d-settings",
                label=_("3D view mode settings"),
                desc=_("Show 3D view mode settings dialog"),
            ),
            "help": MetaIcon(img="3d-help", label=_("Show 3D view mode manual")),
        }

        return self._getToolbarData(
            (
                (
                    ("nvizCmd", _("Generate command for m.nviz.image")),
                    icons["cmd"],
                    self.OnNvizCmd,
                ),
                (None,),
                (
                    ("settings", _("3D view settings")),
                    icons["settings"],
                    self.parent.OnNvizPreferences,
                ),
                (
                    ("help", _("3D view help")),
                    icons["help"],
                    self.OnHelp,
                ),
            )
        )

    def OnNvizCmd(self, event):
        """Show m.nviz.image command"""
        self.lmgr.GetLayerTree().GetMapDisplay().GetWindow().OnNvizCmd()

    def OnHelp(self, event):
        """Show 3D view mode help"""
        if not self.lmgr:
            RunCommand("g.manual", entry="wxGUI.nviz")
        else:
            log = self.lmgr.GetLogWindow()
            log.RunCmd(["g.manual", "entry=wxGUI.nviz"])
