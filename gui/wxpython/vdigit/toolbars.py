"""
@package vdigit.toolbars

@brief wxGUI vector digitizer toolbars

List of classes:
 - toolbars::VDigitToolbar

(C) 2007-2015 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Stepan Turek <stepan.turek seznam.cz> (handlers support)
"""
import wx

from grass import script as grass
from grass.pydispatch.signal import Signal

from gui_core.toolbars import BaseToolbar, BaseIcons
from gui_core.dialogs import CreateNewVector, VectorDialog
from gui_core.wrap import PseudoDC, Menu
from vdigit.preferences import VDigitSettingsDialog
from core.debug import Debug
from core.settings import UserSettings
from core.gcmd import GError, RunCommand
from icons.icon import MetaIcon
from core.giface import Notification


class VDigitToolbar(BaseToolbar):
    """Toolbar for digitization"""

    def __init__(self, parent, toolSwitcher, MapWindow, digitClass, giface, tools=[]):
        self.MapWindow = MapWindow
        self.Map = MapWindow.GetMap()  # Map class instance
        self.tools = tools
        self.digitClass = digitClass
        BaseToolbar.__init__(self, parent, toolSwitcher)
        self.digit = None
        self._giface = giface
        self.fType = None  # feature type for simple features editing

        self.editingStarted = Signal("VDigitToolbar.editingStarted")
        self.editingStopped = Signal("VDigitToolbar.editingStopped")
        self.editingBgMap = Signal("VDigitToolbar.editingBgMap")
        self.quitDigitizer = Signal("VDigitToolbar.quitDigitizer")
        layerTree = self._giface.GetLayerTree()
        if layerTree:
            self.editingStarted.connect(layerTree.StartEditing)
            self.editingStopped.connect(layerTree.StopEditing)
            self.editingBgMap.connect(layerTree.SetBgMapForEditing)

        # bind events
        self.Bind(wx.EVT_SHOW, self.OnShow)

        # currently selected map layer for editing (reference to MapLayer
        # instance)
        self.mapLayer = None
        # list of vector layers from Layer Manager (only in the current mapset)
        self.layers = []

        self.comboid = self.combo = None
        self.undo = -1
        self.redo = -1

        # only one dialog can be open
        self.settingsDialog = None

        # create toolbars (two rows optionally)
        self.InitToolbar(self._toolbarData())

        self._default = -1
        # default action (digitize new point, line, etc.)
        self.action = {"desc": "", "type": "", "id": -1}
        self._currentAreaActionType = None

        # list of available vector maps
        self.UpdateListOfLayers(updateTool=True)

        for tool in (
            "addPoint",
            "addLine",
            "addBoundary",
            "addCentroid",
            "addArea",
            "addVertex",
            "deleteLine",
            "deleteArea",
            "displayAttr",
            "displayCats",
            "editLine",
            "moveLine",
            "moveVertex",
            "removeVertex",
            "additionalTools",
        ):
            if hasattr(self, tool):
                tool = getattr(self, tool)
                self.toolSwitcher.AddToolToGroup(
                    group="mouseUse", toolbar=self, tool=tool
                )
            else:
                Debug.msg(1, "%s skipped" % tool)

        # custom button for digitization of area/boundary/centroid
        # TODO: could this be somehow generalized?
        nAreaTools = 0
        if self.tools and "addBoundary" in self.tools:
            nAreaTools += 1
        if self.tools and "addCentroid" in self.tools:
            nAreaTools += 1
        if self.tools and "addArea" in self.tools:
            nAreaTools += 1
        if nAreaTools != 1:
            self.areaButton = self.CreateSelectionButton(
                _("Select area/boundary/centroid tool")
            )
            self.areaButtonId = self.InsertControl(5, self.areaButton)
            self.areaButton.Bind(wx.EVT_BUTTON, self.OnAddAreaMenu)

        # realize toolbar
        self.Realize()
        # workaround for Mac bug. May be fixed by 2.8.8, but not before then.
        if self.combo:
            self.combo.Hide()
            self.combo.Show()

        # disable undo/redo
        if self.undo > 0:
            self.EnableTool(self.undo, False)
        if self.redo > 0:
            self.EnableTool(self.redo, False)

        self.FixSize(width=105)

    def _toolbarData(self):
        """Toolbar data"""
        data = []

        self.icons = {
            "addPoint": MetaIcon(
                img="point-create",
                label=_("Digitize new point"),
                desc=_("Left: new point"),
            ),
            "addLine": MetaIcon(
                img="line-create",
                label=_("Digitize new line"),
                desc=_(
                    "Left: new point; Ctrl+Left: undo last point; Right: close line"
                ),
            ),
            "addBoundary": MetaIcon(
                img="boundary-create",
                label=_("Digitize new boundary"),
                desc=_(
                    "Left: new point; Ctrl+Left: undo last point; Right: close line"
                ),
            ),
            "addCentroid": MetaIcon(
                img="centroid-create",
                label=_("Digitize new centroid"),
                desc=_("Left: new point"),
            ),
            "addArea": MetaIcon(
                img="polygon-create",
                label=_("Digitize new area (boundary without category)"),
                desc=_("Left: new point"),
            ),
            "addVertex": MetaIcon(
                img="vertex-create",
                label=_("Add new vertex to line or boundary"),
                desc=_("Left: Select; Ctrl+Left: Unselect; Right: Confirm"),
            ),
            "deleteLine": MetaIcon(
                img="line-delete",
                label=_(
                    "Delete selected point(s), line(s), boundary(ies) or centroid(s)"
                ),
                desc=_("Left: Select; Ctrl+Left: Unselect; Right: Confirm"),
            ),
            "deleteArea": MetaIcon(
                img="polygon-delete",
                label=_("Delete selected area(s)"),
                desc=_("Left: Select; Ctrl+Left: Unselect; Right: Confirm"),
            ),
            "displayAttr": MetaIcon(
                img="attributes-display",
                label=_("Display/update attributes"),
                desc=_("Left: Select"),
            ),
            "displayCats": MetaIcon(
                img="cats-display",
                label=_("Display/update categories"),
                desc=_("Left: Select"),
            ),
            "editLine": MetaIcon(
                img="line-edit",
                label=_("Edit selected line/boundary"),
                desc=_(
                    "Left: new point; Ctrl+Left: undo last point; Right: close line"
                ),
            ),
            "moveLine": MetaIcon(
                img="line-move",
                label=_(
                    "Move selected point(s), line(s), boundary(ies) or centroid(s)"
                ),
                desc=_("Left: Select; Ctrl+Left: Unselect; Right: Confirm"),
            ),
            "moveVertex": MetaIcon(
                img="vertex-move",
                label=_("Move selected vertex"),
                desc=_("Left: Select; Ctrl+Left: Unselect; Right: Confirm"),
            ),
            "removeVertex": MetaIcon(
                img="vertex-delete",
                label=_("Remove selected vertex"),
                desc=_("Left: Select; Ctrl+Left: Unselect; Right: Confirm"),
            ),
            "settings": BaseIcons["settings"].SetLabel(_("Digitization settings")),
            "quit": BaseIcons["quit"].SetLabel(
                label=_("Quit digitizer"), desc=_("Quit digitizer and save changes")
            ),
            "help": BaseIcons["help"].SetLabel(
                label=_("Vector Digitizer manual"),
                desc=_("Show Vector Digitizer manual"),
            ),
            "additionalTools": MetaIcon(
                img="tools",
                label=_("Additional tools " "(copy, flip, connect, etc.)"),
                desc=_("Left: Select; Ctrl+Left: Unselect; Right: Confirm"),
            ),
            "undo": MetaIcon(
                img="undo", label=_("Undo"), desc=_("Undo previous changes")
            ),
            "redo": MetaIcon(
                img="redo", label=_("Redo"), desc=_("Redo previous changes")
            ),
        }

        if not self.tools or "selector" in self.tools:
            data.append((None,))
        if not self.tools or "addPoint" in self.tools:
            data.append(
                ("addPoint", self.icons["addPoint"], self.OnAddPoint, wx.ITEM_CHECK)
            )
        if not self.tools or "addLine" in self.tools:
            data.append(
                ("addLine", self.icons["addLine"], self.OnAddLine, wx.ITEM_CHECK)
            )
        if not self.tools or "addArea" in self.tools:
            data.append(
                ("addArea", self.icons["addArea"], self.OnAddAreaTool, wx.ITEM_CHECK)
            )
        if not self.tools or "deleteLine" in self.tools:
            data.append(
                (
                    "deleteLine",
                    self.icons["deleteLine"],
                    self.OnDeleteLine,
                    wx.ITEM_CHECK,
                )
            )
        if not self.tools or "deleteArea" in self.tools:
            data.append(
                (
                    "deleteArea",
                    self.icons["deleteArea"],
                    self.OnDeleteArea,
                    wx.ITEM_CHECK,
                )
            )
        if not self.tools or "moveVertex" in self.tools:
            data.append(
                (
                    "moveVertex",
                    self.icons["moveVertex"],
                    self.OnMoveVertex,
                    wx.ITEM_CHECK,
                )
            )
        if not self.tools or "addVertex" in self.tools:
            data.append(
                ("addVertex", self.icons["addVertex"], self.OnAddVertex, wx.ITEM_CHECK)
            )
        if not self.tools or "removeVertex" in self.tools:
            data.append(
                (
                    "removeVertex",
                    self.icons["removeVertex"],
                    self.OnRemoveVertex,
                    wx.ITEM_CHECK,
                )
            )
        if not self.tools or "editLine" in self.tools:
            data.append(
                ("editLine", self.icons["editLine"], self.OnEditLine, wx.ITEM_CHECK)
            )
        if not self.tools or "moveLine" in self.tools:
            data.append(
                ("moveLine", self.icons["moveLine"], self.OnMoveLine, wx.ITEM_CHECK)
            )
        if not self.tools or "displayCats" in self.tools:
            data.append(
                (
                    "displayCats",
                    self.icons["displayCats"],
                    self.OnDisplayCats,
                    wx.ITEM_CHECK,
                )
            )
        if not self.tools or "displayAttr" in self.tools:
            data.append(
                (
                    "displayAttr",
                    self.icons["displayAttr"],
                    self.OnDisplayAttr,
                    wx.ITEM_CHECK,
                )
            )
        if not self.tools or "additionalSelf.Tools" in self.tools:
            data.append(
                (
                    "additionalTools",
                    self.icons["additionalTools"],
                    self.OnAdditionalToolMenu,
                    wx.ITEM_CHECK,
                )
            )
        if not self.tools or "undo" in self.tools or "redo" in self.tools:
            data.append((None,))
        if not self.tools or "undo" in self.tools:
            data.append(("undo", self.icons["undo"], self.OnUndo))
        if not self.tools or "redo" in self.tools:
            data.append(("redo", self.icons["redo"], self.OnRedo))
        if (
            not self.tools
            or "settings" in self.tools
            or "help" in self.tools
            or "quit" in self.tools
        ):
            data.append((None,))
        if not self.tools or "settings" in self.tools:
            data.append(("settings", self.icons["settings"], self.OnSettings))
        if not self.tools or "help" in self.tools:
            data.append(("help", self.icons["help"], self.OnHelp))
        if not self.tools or "quit" in self.tools:
            data.append(("quit", self.icons["quit"], self.OnExit))

        return self._getToolbarData(data)

    def OnTool(self, event):
        """Tool selected -> untoggles previusly selected tool in
        toolbar"""
        Debug.msg(3, "VDigitToolbar.OnTool(): id = %s" % event.GetId())
        # set cursor
        self.MapWindow.SetNamedCursor("cross")
        self.MapWindow.mouse["box"] = "point"
        self.MapWindow.mouse["use"] = "pointer"

        aId = self.action.get("id", -1)
        BaseToolbar.OnTool(self, event)

        # clear tmp canvas
        if self.action["id"] != aId or aId == -1:
            self.MapWindow.polycoords = []
            self.MapWindow.ClearLines(pdc=self.MapWindow.pdcTmp)
            if self.digit and len(self.MapWindow.digit.GetDisplay().GetSelected()) > 0:
                # cancel action
                self.MapWindow.OnMiddleDown(None)

        # set no action
        if self.action["id"] == -1:
            self.action = {"desc": "", "type": "", "id": -1}

        # set focus
        self.MapWindow.SetFocus()

    def OnAddPoint(self, event):
        """Add point to the vector map Laier"""
        Debug.msg(2, "VDigitToolbar.OnAddPoint()")
        self.action = {"desc": "addLine", "type": "point", "id": self.addPoint}
        self.MapWindow.mouse["box"] = "point"

    def OnAddLine(self, event):
        """Add line to the vector map layer"""
        Debug.msg(2, "VDigitToolbar.OnAddLine()")
        self.action = {"desc": "addLine", "type": "line", "id": self.addLine}
        self.MapWindow.mouse["box"] = "line"
        # self.MapWindow.polycoords = [] # reset temp line

    def OnAddBoundary(self, event):
        """Add boundary to the vector map layer"""
        Debug.msg(2, "VDigitToolbar.OnAddBoundary()")

        self._toggleAreaIfNeeded()

        # reset temp line
        if self.action["desc"] != "addLine" or self.action["type"] != "boundary":
            self.MapWindow.polycoords = []

        # update icon and tooltip
        self.SetToolNormalBitmap(self.addArea, self.icons["addBoundary"].GetBitmap())
        self.SetToolShortHelp(self.addArea, self.icons["addBoundary"].GetLabel())

        # set action
        self.action = {"desc": "addLine", "type": "boundary", "id": self.addArea}
        self.MapWindow.mouse["box"] = "line"
        self._currentAreaActionType = "boundary"

    def OnAddCentroid(self, event):
        """Add centroid to the vector map layer"""
        Debug.msg(2, "VDigitToolbar.OnAddCentroid()")

        self._toggleAreaIfNeeded()

        # update icon and tooltip
        self.SetToolNormalBitmap(self.addArea, self.icons["addCentroid"].GetBitmap())
        self.SetToolShortHelp(self.addArea, self.icons["addCentroid"].GetLabel())

        # set action
        self.action = {"desc": "addLine", "type": "centroid", "id": self.addArea}
        self.MapWindow.mouse["box"] = "point"
        self._currentAreaActionType = "centroid"

    def OnAddArea(self, event):
        """Add area to the vector map layer"""

        Debug.msg(2, "VDigitToolbar.OnAddArea()")

        self._toggleAreaIfNeeded()

        # update icon and tooltip
        self.SetToolNormalBitmap(self.addArea, self.icons["addArea"].GetBitmap())
        self.SetToolShortHelp(self.addArea, self.icons["addArea"].GetLabel())

        # set action
        self.action = {"desc": "addLine", "type": "area", "id": self.addArea}
        self.MapWindow.mouse["box"] = "line"
        self._currentAreaActionType = "area"

    def _toggleAreaIfNeeded(self):
        """In some cases, the area tool is not toggled, we have to do it manually."""
        if not self.GetToolState(self.addArea):
            self.ToggleTool(self.addArea, True)
            self.toolSwitcher.ToolChanged(self.addArea)

    def OnAddAreaTool(self, event):
        """Area tool activated."""
        Debug.msg(2, "VDigitToolbar.OnAddAreaTool()")

        # we need the previous id
        if (
            not self._currentAreaActionType or self._currentAreaActionType == "area"
        ):  # default action
            self.OnAddArea(event)
        elif self._currentAreaActionType == "boundary":
            self.OnAddBoundary(event)
        elif self._currentAreaActionType == "centroid":
            self.OnAddCentroid(event)

    def OnAddAreaMenu(self, event):
        """Digitize area menu (add area/boundary/centroid)"""
        menuItems = []
        if not self.tools or "addArea" in self.tools:
            menuItems.append((self.icons["addArea"], self.OnAddArea))
        if not self.fType and not self.tools or "addBoundary" in self.tools:
            menuItems.append((self.icons["addBoundary"], self.OnAddBoundary))
        if not self.fType and not self.tools or "addCentroid" in self.tools:
            menuItems.append((self.icons["addCentroid"], self.OnAddCentroid))

        self._onMenu(menuItems)

    def OnExit(self, event=None):
        """Quit digitization tool"""
        # stop editing of the currently selected map layer
        if self.mapLayer:
            self.StopEditing()

        # close dialogs if still open
        if self.settingsDialog:
            self.settingsDialog.OnCancel(None)

        # set default mouse settings
        self.parent.GetMapToolbar().SelectDefault()
        self.MapWindow.polycoords = []

        self.quitDigitizer.emit()

    def OnMoveVertex(self, event):
        """Move line vertex"""
        Debug.msg(2, "Digittoolbar.OnMoveVertex():")
        self.action = {"desc": "moveVertex", "id": self.moveVertex}
        self.MapWindow.mouse["box"] = "point"

    def OnAddVertex(self, event):
        """Add line vertex"""
        Debug.msg(2, "Digittoolbar.OnAddVertex():")
        self.action = {"desc": "addVertex", "id": self.addVertex}
        self.MapWindow.mouse["box"] = "point"

    def OnRemoveVertex(self, event):
        """Remove line vertex"""
        Debug.msg(2, "Digittoolbar.OnRemoveVertex():")
        self.action = {"desc": "removeVertex", "id": self.removeVertex}
        self.MapWindow.mouse["box"] = "point"

    def OnEditLine(self, event):
        """Edit line"""
        Debug.msg(2, "Digittoolbar.OnEditLine():")
        self.action = {"desc": "editLine", "id": self.editLine}
        self.MapWindow.mouse["box"] = "line"

    def OnMoveLine(self, event):
        """Move line"""
        Debug.msg(2, "Digittoolbar.OnMoveLine():")
        self.action = {"desc": "moveLine", "id": self.moveLine}
        self.MapWindow.mouse["box"] = "box"

    def OnDeleteLine(self, event):
        """Delete line"""
        Debug.msg(2, "Digittoolbar.OnDeleteLine():")
        self.action = {"desc": "deleteLine", "id": self.deleteLine}
        self.MapWindow.mouse["box"] = "box"

    def OnDeleteArea(self, event):
        """Delete Area"""
        Debug.msg(2, "Digittoolbar.OnDeleteArea():")
        self.action = {"desc": "deleteArea", "id": self.deleteArea}
        self.MapWindow.mouse["box"] = "box"

    def OnDisplayCats(self, event):
        """Display/update categories"""
        Debug.msg(2, "Digittoolbar.OnDisplayCats():")
        self.action = {"desc": "displayCats", "id": self.displayCats}
        self.MapWindow.mouse["box"] = "point"

    def OnDisplayAttr(self, event):
        """Display/update attributes"""
        Debug.msg(2, "Digittoolbar.OnDisplayAttr():")
        self.action = {"desc": "displayAttrs", "id": self.displayAttr}
        self.MapWindow.mouse["box"] = "point"

    def OnUndo(self, event):
        """Undo previous changes"""
        if self.digit:
            self.digit.Undo()

        event.Skip()

    def OnRedo(self, event):
        """Undo previous changes"""
        if self.digit:
            self.digit.Undo(level=1)

        event.Skip()

    def EnableUndo(self, enable=True):
        """Enable 'Undo' in toolbar

        :param enable: False for disable
        """
        self._enableTool(self.undo, enable)

    def EnableRedo(self, enable=True):
        """Enable 'Redo' in toolbar

        :param enable: False for disable
        """
        self._enableTool(self.redo, enable)

    def _enableTool(self, tool, enable):
        if not self.FindById(tool):
            return

        if enable:
            if self.GetToolEnabled(tool) is False:
                self.EnableTool(tool, True)
        else:
            if self.GetToolEnabled(tool) is True:
                self.EnableTool(tool, False)

    def GetAction(self, type="desc"):
        """Get current action info"""
        return self.action.get(type, "")

    def OnSettings(self, event):
        """Show settings dialog"""
        if self.digit is None:
            try:
                self.digit = self.MapWindow.digit = self.digitClass(
                    mapwindow=self.MapWindow
                )
            except SystemExit:
                self.digit = self.MapWindow.digit = None

        if not self.settingsDialog:
            self.settingsDialog = VDigitSettingsDialog(
                parent=self.parent, giface=self._giface
            )
            self.settingsDialog.Show()

    def OnHelp(self, event):
        """Show digitizer help page in web browser"""
        self._giface.Help("wxGUI.vdigit")

    def OnAdditionalToolMenu(self, event):
        """Menu for additional tools"""
        point = wx.GetMousePosition()
        toolMenu = Menu()

        for label, itype, handler, desc in (
            (
                _("Break selected lines/boundaries at intersection"),
                wx.ITEM_CHECK,
                self.OnBreak,
                "breakLine",
            ),
            (
                _("Connect selected lines/boundaries"),
                wx.ITEM_CHECK,
                self.OnConnect,
                "connectLine",
            ),
            (_("Copy categories"), wx.ITEM_CHECK, self.OnCopyCats, "copyCats"),
            (
                _("Copy features from (background) vector map"),
                wx.ITEM_CHECK,
                self.OnCopy,
                "copyLine",
            ),
            (_("Copy attributes"), wx.ITEM_CHECK, self.OnCopyAttrb, "copyAttrs"),
            (
                _("Feature type conversion"),
                wx.ITEM_CHECK,
                self.OnTypeConversion,
                "typeConv",
            ),
            (
                _("Flip selected lines/boundaries"),
                wx.ITEM_CHECK,
                self.OnFlip,
                "flipLine",
            ),
            (
                _("Merge selected lines/boundaries"),
                wx.ITEM_CHECK,
                self.OnMerge,
                "mergeLine",
            ),
            (
                _("Snap selected lines/boundaries (only to nodes)"),
                wx.ITEM_CHECK,
                self.OnSnap,
                "snapLine",
            ),
            (_("Split line/boundary"), wx.ITEM_CHECK, self.OnSplitLine, "splitLine"),
            (_("Query features"), wx.ITEM_CHECK, self.OnQuery, "queryLine"),
            (
                _("Z bulk-labeling of 3D lines"),
                wx.ITEM_CHECK,
                self.OnZBulk,
                "zbulkLine",
            ),
        ):
            # Add items to the menu
            item = wx.MenuItem(
                parentMenu=toolMenu, id=wx.ID_ANY, text=label, kind=itype
            )
            toolMenu.AppendItem(item)
            self.MapWindow.Bind(wx.EVT_MENU, handler, item)
            if self.action["desc"] == desc:
                item.Check(True)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.MapWindow.PopupMenu(toolMenu)
        toolMenu.Destroy()

        if self.action["desc"] == "addPoint":
            self.ToggleTool(self.additionalTools, False)

    def OnCopy(self, event):
        """Copy selected features from (background) vector map"""
        if not self.digit:
            GError(_("No vector map open for editing."), self.parent)
            return

        # select background map
        dlg = VectorDialog(
            self.parent,
            title=_("Select background vector map"),
            layerTree=self._giface.GetLayerTree(),
        )
        if dlg.ShowModal() != wx.ID_OK:
            dlg.Destroy()
            return

        mapName = dlg.GetName(full=True)
        dlg.Destroy()

        # close open background map if any
        bgMap = UserSettings.Get(
            group="vdigit", key="bgmap", subkey="value", settings_type="internal"
        )
        if bgMap:
            self.digit.CloseBackgroundMap()
            self.editingBgMap.emit(mapName=bgMap, unset=True)

        # open background map for reading
        UserSettings.Set(
            group="vdigit",
            key="bgmap",
            subkey="value",
            value=str(mapName),
            settings_type="internal",
        )
        self.digit.OpenBackgroundMap(mapName)
        self.editingBgMap.emit(mapName=mapName)

        if self.action["desc"] == "copyLine":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return

        Debug.msg(2, "Digittoolbar.OnCopy():")
        self.action = {"desc": "copyLine", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "box"

    def OnSplitLine(self, event):
        """Split line"""
        if self.action["desc"] == "splitLine":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return

        Debug.msg(2, "Digittoolbar.OnSplitLine():")
        self.action = {"desc": "splitLine", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "point"

    def OnCopyCats(self, event):
        """Copy categories"""
        if self.action["desc"] == "copyCats":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.copyCats, False)
            self.OnAddPoint(event)
            return

        Debug.msg(2, "Digittoolbar.OnCopyCats():")
        self.action = {"desc": "copyCats", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "point"

    def OnCopyAttrb(self, event):
        """Copy attributes"""
        if self.action["desc"] == "copyAttrs":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.copyCats, False)
            self.OnAddPoint(event)
            return

        Debug.msg(2, "Digittoolbar.OnCopyAttrb():")
        self.action = {"desc": "copyAttrs", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "point"

    def OnFlip(self, event):
        """Flip selected lines/boundaries"""
        if self.action["desc"] == "flipLine":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return

        Debug.msg(2, "Digittoolbar.OnFlip():")
        self.action = {"desc": "flipLine", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "box"

    def OnMerge(self, event):
        """Merge selected lines/boundaries"""
        if self.action["desc"] == "mergeLine":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return

        Debug.msg(2, "Digittoolbar.OnMerge():")
        self.action = {"desc": "mergeLine", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "box"

    def OnBreak(self, event):
        """Break selected lines/boundaries"""
        if self.action["desc"] == "breakLine":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return

        Debug.msg(2, "Digittoolbar.OnBreak():")
        self.action = {"desc": "breakLine", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "box"

    def OnSnap(self, event):
        """Snap selected features"""
        if self.action["desc"] == "snapLine":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return

        Debug.msg(2, "Digittoolbar.OnSnap():")
        self.action = {"desc": "snapLine", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "box"

    def OnConnect(self, event):
        """Connect selected lines/boundaries"""
        if self.action["desc"] == "connectLine":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return

        Debug.msg(2, "Digittoolbar.OnConnect():")
        self.action = {"desc": "connectLine", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "box"

    def OnQuery(self, event):
        """Query selected lines/boundaries"""
        if self.action["desc"] == "queryLine":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return

        Debug.msg(
            2,
            "Digittoolbar.OnQuery(): %s"
            % UserSettings.Get(group="vdigit", key="query", subkey="selection"),
        )
        self.action = {"desc": "queryLine", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "box"

    def OnZBulk(self, event):
        """Z bulk-labeling selected lines/boundaries"""
        if not self.digit.IsVector3D():
            GError(
                parent=self.parent,
                message=_("Vector map is not 3D. Operation canceled."),
            )
            return

        if self.action["desc"] == "zbulkLine":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return

        Debug.msg(2, "Digittoolbar.OnZBulk():")
        self.action = {"desc": "zbulkLine", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "line"

    def OnTypeConversion(self, event):
        """Feature type conversion

        Supported conversions:
         - point <-> centroid
         - line <-> boundary
        """
        if self.action["desc"] == "typeConv":  # select previous action
            self.ToggleTool(self.addPoint, True)
            self.ToggleTool(self.additionalTools, False)
            self.OnAddPoint(event)
            return

        Debug.msg(2, "Digittoolbar.OnTypeConversion():")
        self.action = {"desc": "typeConv", "id": self.additionalTools}
        self.MapWindow.mouse["box"] = "box"

    def OnSelectMap(self, event):
        """Select vector map layer for editing

        If there is a vector map layer already edited, this action is
        firstly terminated. The map layer is closed. After this the
        selected map layer activated for editing.
        """
        if event.GetSelection() == 0:  # create new vector map layer
            if self.mapLayer:
                openVectorMap = self.mapLayer.GetName(fullyQualified=False)["name"]
            else:
                openVectorMap = None
            dlg = CreateNewVector(
                self.parent,
                exceptMap=openVectorMap,
                giface=self._giface,
                cmd=(("v.edit", {"tool": "create"}, "map")),
                disableAdd=True,
            )

            if dlg and dlg.GetName():
                # add layer to map layer tree/map display
                mapName = dlg.GetName() + "@" + grass.gisenv()["MAPSET"]
                self._giface.GetLayerList().AddLayer(
                    ltype="vector",
                    name=mapName,
                    checked=True,
                    cmd=["d.vect", "map=%s" % mapName],
                )

                vectLayers = self.UpdateListOfLayers(updateTool=True)
                selection = vectLayers.index(mapName)

                # create table ?
                if dlg.IsChecked("table"):
                    # TODO: replace this by signal
                    # also note that starting of tools such as atm, iclass,
                    # plots etc. should be handled in some better way
                    # than starting randomly from mapdisp and lmgr
                    lmgr = self.parent.GetLayerManager()
                    if lmgr:
                        lmgr.OnShowAttributeTable(None, selection="table")
                dlg.Destroy()
            else:
                self.combo.SetValue(_("Select vector map"))
                if dlg:
                    dlg.Destroy()
                return
        else:
            selection = event.GetSelection() - 1  # first option is 'New vector map'

        # skip currently selected map
        if self.layers[selection] == self.mapLayer:
            return

        if self.mapLayer:
            # deactive map layer for editing
            self.StopEditing()

        # select the given map layer for editing
        self.StartEditing(self.layers[selection])

        event.Skip()

    def StartEditing(self, mapLayer):
        """Start editing selected vector map layer.

        :param mapLayer: MapLayer to be edited
        """
        # check if topology is available (skip for hidden - temporary
        # maps, see iclass for details)
        if (
            not mapLayer.IsHidden()
            and grass.vector_info(mapLayer.GetName())["level"] != 2
        ):
            dlg = wx.MessageDialog(
                parent=self.MapWindow,
                message=_(
                    "Topology for vector map <%s> is not available. "
                    "Topology is required by digitizer.\nDo you want to "
                    "rebuild topology (takes some time) and open the vector map "
                    "for editing?"
                )
                % mapLayer.GetName(),
                caption=_("Digitizer error"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION | wx.CENTRE,
            )
            if dlg.ShowModal() == wx.ID_YES:
                RunCommand("v.build", map=mapLayer.GetName())
            else:
                return

        # deactive layer
        self.Map.ChangeLayerActive(mapLayer, False)

        # clean map canvas
        self.MapWindow.EraseMap()

        # unset background map if needed
        if mapLayer:
            if (
                UserSettings.Get(
                    group="vdigit",
                    key="bgmap",
                    subkey="value",
                    settings_type="internal",
                )
                == mapLayer.GetName()
            ):
                UserSettings.Set(
                    group="vdigit",
                    key="bgmap",
                    subkey="value",
                    value="",
                    settings_type="internal",
                )

            self.parent.SetStatusText(
                _("Please wait, " "opening vector map <%s> for editing...")
                % mapLayer.GetName(),
                0,
            )

        self.MapWindow.pdcVector = PseudoDC()
        self.digit = self.MapWindow.digit = self.digitClass(mapwindow=self.MapWindow)

        self.mapLayer = mapLayer
        # open vector map (assume that 'hidden' map layer is temporary vector
        # map)
        if self.digit.OpenMap(mapLayer.GetName(), tmp=mapLayer.IsHidden()) is None:
            self.mapLayer = None
            self.StopEditing()
            return False

        # check feature type (only for OGR layers)
        self.fType = self.digit.GetFeatureType()
        self.EnableAll()
        self.EnableUndo(False)
        self.EnableRedo(False)

        if self.fType == "point":
            for tool in (
                self.addLine,
                self.addArea,
                self.moveVertex,
                self.addVertex,
                self.removeVertex,
                self.editLine,
            ):
                self.EnableTool(tool, False)
        elif self.fType == "linestring":
            for tool in (self.addPoint, self.addArea):
                self.EnableTool(tool, False)
        elif self.fType == "polygon":
            for tool in (self.addPoint, self.addLine):
                self.EnableTool(tool, False)
        elif self.fType:
            GError(
                parent=self,
                message=_(
                    "Unsupported feature type '%(type)s'. Unable to edit "
                    "OGR layer <%(layer)s>."
                )
                % {"type": self.fType, "layer": mapLayer.GetName()},
            )
            self.digit.CloseMap()
            self.mapLayer = None
            self.StopEditing()
            return False

        # update toolbar
        if self.combo:
            self.combo.SetValue(mapLayer.GetName())
        if "map" in self.parent.toolbars:
            self.parent.toolbars["map"].combo.SetValue(_("Vector digitizer"))

        # here was dead code to enable vdigit button in toolbar
        # with if to ignore iclass
        # some signal (DigitizerStarted) can be emitted here

        Debug.msg(4, "VDigitToolbar.StartEditing(): layer=%s" % mapLayer.GetName())

        # change cursor
        if self.MapWindow.mouse["use"] == "pointer":
            self.MapWindow.SetNamedCursor("cross")

        if not self.MapWindow.resize:
            self.MapWindow.UpdateMap(render=True)

        # respect opacity
        opacity = mapLayer.GetOpacity()

        if opacity < 1.0:
            alpha = int(opacity * 255)
            self.digit.GetDisplay().UpdateSettings(alpha=alpha)

        # emit signal
        layerTree = self._giface.GetLayerTree()
        if layerTree:
            item = layerTree.FindItemByData("maplayer", self.mapLayer)
        else:
            item = None
        self.editingStarted.emit(
            vectMap=mapLayer.GetName(), digit=self.digit, layerItem=item
        )

        return True

    def StopEditing(self):
        """Stop editing of selected vector map layer.

        :return: True on success
        :return: False on failure
        """
        item = None

        if self.combo:
            self.combo.SetValue(_("Select vector map"))

        # save changes
        if self.mapLayer:
            Debug.msg(
                4, "VDigitToolbar.StopEditing(): layer=%s" % self.mapLayer.GetName()
            )
            if (
                UserSettings.Get(group="vdigit", key="saveOnExit", subkey="enabled")
                is False
            ):
                if self.digit.GetUndoLevel() > -1:
                    dlg = wx.MessageDialog(
                        parent=self.parent,
                        message=_("Do you want to save changes " "in vector map <%s>?")
                        % self.mapLayer.GetName(),
                        caption=_("Save changes?"),
                        style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
                    )
                    if dlg.ShowModal() == wx.ID_NO:
                        # revert changes
                        self.digit.Undo(0)
                    dlg.Destroy()

            self.parent.SetStatusText(
                _(
                    "Please wait, "
                    "closing and rebuilding topology of "
                    "vector map <%s>..."
                )
                % self.mapLayer.GetName(),
                0,
            )
            self.digit.CloseMap()

            # close open background map if any
            bgMap = UserSettings.Get(
                group="vdigit", key="bgmap", subkey="value", settings_type="internal"
            )
            if bgMap:
                self.digit.CloseBackgroundMap()
                self.editingBgMap.emit(mapName=bgMap, unset=True)

            self._giface.GetProgress().SetValue(0)
            self._giface.WriteCmdLog(
                _("Editing of vector map <%s> successfully finished")
                % self.mapLayer.GetName(),
                notification=Notification.HIGHLIGHT,
            )
            # re-active layer
            layerTree = self._giface.GetLayerTree()
            if layerTree:
                item = layerTree.FindItemByData("maplayer", self.mapLayer)
                if item and layerTree.IsItemChecked(item):
                    self.Map.ChangeLayerActive(self.mapLayer, True)

        # change cursor
        self.MapWindow.SetNamedCursor("default")
        self.MapWindow.pdcVector = None

        # close dialogs
        for dialog in ("attributes", "category"):
            if self.parent.dialogs[dialog]:
                self.parent.dialogs[dialog].Close()
                self.parent.dialogs[dialog] = None

        self.digit = None
        self.MapWindow.digit = None

        self.editingStopped.emit(layerItem=item)

        self.mapLayer = None

        self.MapWindow.redrawAll = True

        return True

    def UpdateListOfLayers(self, updateTool=False):
        """Update list of available vector map layers.
        This list consists only editable layers (in the current mapset)

        :param updateTool: True to update also toolbar
        :type updateTool: bool
        """
        Debug.msg(4, "VDigitToolbar.UpdateListOfLayers(): updateTool=%d" % updateTool)

        layerNameSelected = None
        # name of currently selected layer
        if self.mapLayer:
            layerNameSelected = self.mapLayer.GetName()

        # select vector map layer in the current mapset
        layerNameList = []
        self.layers = self.Map.GetListOfLayers(
            ltype="vector", mapset=grass.gisenv()["MAPSET"]
        )

        for layer in self.layers:
            if layer.name not in layerNameList:  # do not duplicate layer
                layerNameList.append(layer.GetName())

        if updateTool:  # update toolbar
            if not self.mapLayer:
                value = _("Select vector map")
            else:
                value = layerNameSelected

            if not self.comboid:
                if not self.tools or "selector" in self.tools:
                    self.combo = wx.ComboBox(
                        self,
                        id=wx.ID_ANY,
                        value=value,
                        choices=[
                            _("New vector map"),
                        ]
                        + layerNameList,
                        size=(80, -1),
                        style=wx.CB_READONLY,
                    )
                    self.comboid = self.InsertControl(0, self.combo)
                    self.parent.Bind(wx.EVT_COMBOBOX, self.OnSelectMap, self.comboid)
            else:
                self.combo.SetItems(
                    [
                        _("New vector map"),
                    ]
                    + layerNameList
                )

            self.Realize()

        return layerNameList

    def GetLayer(self):
        """Get selected layer for editing -- MapLayer instance"""
        return self.mapLayer

    def OnShow(self, event):
        """Show frame event"""
        if event.IsShown():
            # list of available vector maps
            self.UpdateListOfLayers(updateTool=True)
