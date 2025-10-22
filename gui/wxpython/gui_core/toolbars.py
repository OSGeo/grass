"""
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
import os
from itertools import starmap

import wx
from wx.lib.agw import aui

from core import globalvar
from core.debug import Debug
from icons.icon import MetaIcon
from collections import defaultdict
from core.globalvar import IMGDIR
from gui_core.wrap import ToolBar, Menu, BitmapButton, NewId

from grass.pydispatch.signal import Signal


BaseIcons = {
    "display": MetaIcon(
        img="show", label=_("Display map"), desc=_("Re-render modified map layers only")
    ),
    "render": MetaIcon(
        img="layer-redraw",
        label=_("Render map"),
        desc=_("Force re-rendering all map layers"),
    ),
    "erase": MetaIcon(
        img="erase",
        label=_("Erase display"),
        desc=_("Erase display canvas with given background color"),
    ),
    "pointer": MetaIcon(img="pointer", label=_("Pointer")),
    "zoomIn": MetaIcon(
        img="zoom-in", label=_("Zoom in"), desc=_("Drag or click mouse to zoom")
    ),
    "zoomOut": MetaIcon(
        img="zoom-out", label=_("Zoom out"), desc=_("Drag or click mouse to unzoom")
    ),
    "zoomBack": MetaIcon(img="zoom-last", label=_("Return to previous zoom")),
    "zoomMenu": MetaIcon(
        img="zoom-more",
        label=_("Various zoom options"),
        desc=_("Zoom to default or saved region, save to named region, ..."),
    ),
    "zoomExtent": MetaIcon(img="zoom-extent", label=_("Zoom to selected map layer(s)")),
    "zoomRegion": MetaIcon(
        img="zoom-region", label=_("Zoom to computational region extent")
    ),
    "pan": MetaIcon(img="pan", label=_("Pan"), desc=_("Drag with mouse to pan")),
    "saveFile": MetaIcon(img="map-export", label=_("Save display to file")),
    "print": MetaIcon(img="print", label=_("Print display")),
    "font": MetaIcon(img="font", label=_("Select font")),
    "help": MetaIcon(img="help", label=_("Show manual")),
    "quit": MetaIcon(img="quit", label=_("Quit")),
    "addRast": MetaIcon(img="layer-raster-add", label=_("Add raster map layer")),
    "addVect": MetaIcon(img="layer-vector-add", label=_("Add vector map layer")),
    "overlay": MetaIcon(
        img="overlay-add",
        label=_("Add map elements"),
        desc=_("Overlay elements like scale and legend onto map"),
    ),
    "histogram": MetaIcon(
        img="layer-raster-histogram", label=_("Create histogram with d.histogram")
    ),
    "settings": MetaIcon(img="settings", label=_("Settings")),
    "mapDispSettings": MetaIcon(
        img="monitor-settings", label=_("Map Display Settings")
    ),
    "docking": MetaIcon(img="monitor-dock", label=_("(Un)dock")),
}


class ToolbarController:
    """Controller specialized for wx.ToolBar subclass.

    Toolbar subclasses must delegate methods to controller.
    Methods inherited from toolbar class must be delegated explicitly
    and other methods can be delegated by @c __getattr__.
    """

    def __init__(self, classObject, widget, parent, toolSwitcher):
        """
        :param classObject: toolbar class name (object, i.e. wx.Toolbar)
        :param widget: toolbar instance
        """
        self.classObject = classObject
        self.widget = widget
        self.parent = parent
        self.toolSwitcher = toolSwitcher
        self.handlers = {}
        self.data = None

    def InitToolbar(self, toolData):
        """Initialize toolbar, add tools to the toolbar"""
        for tool in toolData:
            self.CreateTool(*tool)

        self.data = toolData

    def CreateTool(self, label, bitmap, kind, shortHelp, longHelp, handler, pos=-1):
        """Add tool to the toolbar

        :param pos: if -1 add tool, if > 0 insert at given pos
        :return: id of tool
        """
        bmpDisabled = wx.NullBitmap
        tool = -1

        if isinstance(label, tuple):
            internal_label, label = label[0], label[1]
        else:
            internal_label = label

        if label:
            tool = vars(self.widget)[internal_label] = NewId()
            Debug.msg(
                3, "CreateTool(): tool=%d, label=%s bitmap=%s" % (tool, label, bitmap)
            )
            if pos < 0:
                toolWin = self.classObject.AddTool(
                    self.widget,
                    tool,
                    label,
                    bitmap,
                    bmpDisabled,
                    kind,
                    shortHelp,
                    longHelp,
                )
            else:
                toolWin = self.classObject.InsertTool(
                    self.widget,
                    pos,
                    tool,
                    label,
                    bitmap,
                    bmpDisabled,
                    kind,
                    shortHelp,
                    longHelp,
                )
            self.handlers[tool] = handler
            self.widget.Bind(wx.EVT_TOOL, handler, toolWin)
            self.widget.Bind(wx.EVT_TOOL, self.OnTool, toolWin)
        else:  # separator
            self.classObject.AddSeparator(self.widget)

        return tool

    def EnableLongHelp(self, enable):
        """Enable/disable long help

        :param enable: True for enable otherwise disable
        """
        for tool in self.data:
            if isinstance(tool[0], tuple):
                if tool[0][0] == "":  # separator
                    continue
                internal_label = tool[0][0]
            else:
                if tool[0] == "":  # separator
                    continue
                internal_label = tool[0]

            label = vars(self.widget)[internal_label]
            if enable:
                self.classObject.SetToolLongHelp(self.widget, label, tool[4])
            else:
                self.classObject.SetToolLongHelp(self.widget, label, "")

    def OnTool(self, event):
        """Tool selected"""
        if self.toolSwitcher:
            Debug.msg(3, "BaseToolbar.OnTool(): id = %s" % event.GetId())
            self.toolSwitcher.ToolChanged(event.GetId())
        event.Skip()

    def SelectTool(self, id):
        self.classObject.ToggleTool(self.widget, id, True)
        self.toolSwitcher.ToolChanged(id)

        self.handlers[id](event=None)

    def SelectDefault(self):
        """Select default tool"""
        self.SelectTool(self.widget._default)

    def FixSize(self, width):
        """Fix toolbar width on Windows

        .. todo::
            Determine why combobox causes problems here
        """
        if platform.system() == "Windows":
            size = self.classObject.GetBestSize(self.widget)
            self.classObject.SetSize(self.widget, (size[0] + width, size[1]))

    def Enable(self, tool, enable=True):
        """Enable/Disable defined tool

        :param str/tuple tool: name
        :param enable: True to enable otherwise disable tool
        """
        try:
            if isinstance(tool, tuple):
                id = getattr(self.widget, tool[0])
            else:
                id = getattr(self.widget, tool)
        except AttributeError:
            # TODO: test everything that this is not raised
            # this error was ignored for a long time
            raise AttributeError("Toolbar does not have a tool %s." % tool)
            return

        self.classObject.EnableTool(self.widget, id, enable)

    def EnableAll(self, enable=True):
        """Enable/Disable all tools

        :param enable: True to enable otherwise disable tool
        """
        for item in self.widget._toolbarData():
            if not item[0]:
                continue
            self.Enable(item[0], enable)

    def _getToolbarData(self, data):
        """Define tool"""
        return list(starmap(self._defineTool, data))

    def _defineTool(
        self, name=None, icon=None, handler=None, item=wx.ITEM_NORMAL, pos=-1
    ):
        """Define tool"""
        if name:
            return (
                name,
                icon.GetBitmap(),
                item,
                icon.GetLabel(),
                icon.GetDesc(),
                handler,
                pos,
            )
        return ("", "", "", "", "", "")  # separator

    def _onMenu(self, data):
        """Toolbar pop-up menu"""
        menu = Menu()

        for icon, handler in data:
            item = wx.MenuItem(menu, wx.ID_ANY, icon.GetLabel())
            item.SetBitmap(icon.GetBitmap(self.parent.iconsize))
            menu.AppendItem(item)
            self.widget.Bind(wx.EVT_MENU, handler, item)

        self.classObject.PopupMenu(self.widget, menu)
        menu.Destroy()

    def CreateSelectionButton(self, tooltip):
        """Add button to toolbar for selection of graphics drawing mode.

        Button must be custom (not toolbar tool) to set smaller width.
        """
        arrowPath = os.path.join(IMGDIR, "small_down_arrow.png")
        if os.path.isfile(arrowPath) and os.path.getsize(arrowPath):
            bitmap = wx.Bitmap(name=arrowPath)
        else:
            bitmap = wx.ArtProvider.GetBitmap(
                id=wx.ART_MISSING_IMAGE, client=wx.ART_TOOLBAR
            )
        button = BitmapButton(
            parent=self.widget,
            id=wx.ID_ANY,
            size=((-1, self.classObject.GetToolSize(self.widget)[1])),
            bitmap=bitmap,
            style=wx.NO_BORDER,
        )
        button.SetToolTip(tooltip)

        return button


class AuiToolbarController(ToolbarController):
    """Controller specialized for wx.lib.agw.aui.auibar.AuiToolBar subclass"""

    def _defineTool(self, name=None, icon=None, handler=None, item=wx.ITEM_NORMAL):
        """Define tool. Position is not needed since wx.lib.agw.aui.auibar.AuiToolBar
        does not have InsertTool method."""
        if name:
            return (
                name,
                icon.GetBitmap(),
                item,
                icon.GetLabel(),
                icon.GetDesc(),
                handler,
            )
        return ("", "", "", "", "", "")  # separator

    def CreateTool(self, label, bitmap, kind, shortHelp, longHelp, handler):
        """Add tool to the toolbar"""
        return super().CreateTool(label, bitmap, kind, shortHelp, longHelp, handler)


class BaseToolbar(ToolBar):
    """Abstract basic toolbar class.

    Following code shows how to create new basic toolbar:

        class MyToolbar(BaseToolbar):
            def __init__(self, parent):
                BaseToolbar.__init__(self, parent)
                self.InitToolbar(self._toolbarData())
                self.Realize()

            def _toolbarData(self):
                # e.g. ("help", _("Help")) tool short label (triangle/arrow
                # at the right side of the toolbar)
                return self._getToolbarData(
                    (
                        ("help", Icons["help"].label),
                        Icons["help"],
                        self.parent.OnHelp
                    ),
                )
    """

    def __init__(
        self, parent, toolSwitcher=None, style=wx.NO_BORDER | wx.TB_HORIZONTAL
    ):
        self.parent = parent
        wx.ToolBar.__init__(self, parent=self.parent, id=wx.ID_ANY, style=style)

        self._default = None
        self.SetToolBitmapSize(globalvar.toolbarSize)

        self.toolSwitcher = toolSwitcher
        self.controller = ToolbarController(
            classObject=ToolBar,
            widget=self,
            parent=self.parent,
            toolSwitcher=toolSwitcher,
        )

    def _toolbarData(self):
        """Toolbar data (virtual)"""
        return

    def Enable(self, tool, enable=True):
        """@copydoc ToolbarController::Enable()"""
        self.controller.Enable(tool, enable)

    def CreateTool(self, *args, **kwargs):
        """@copydoc ToolbarController::CreateTool()"""
        self.controller.CreateTool(*args, **kwargs)

    def OnTool(self, event):
        """@copydoc ToolbarController::OnTool()"""
        self.controller.OnTool(event)

    def __getattr__(self, name):
        return getattr(self.controller, name)


class AuiToolbar(aui.AuiToolBar):
    """Abstract AUI toolbar class.

    Toolbar for integration with the AUI layout system."""

    def __init__(
        self,
        parent,
        toolSwitcher=None,
        style=wx.NO_BORDER | wx.TB_HORIZONTAL,
        agwStyle=aui.AUI_TB_PLAIN_BACKGROUND | aui.AUI_TB_GRIPPER,
    ):
        self.parent = parent
        super().__init__(
            parent=self.parent, id=wx.ID_ANY, style=style, agwStyle=agwStyle
        )

        self._default = None
        self.SetToolBitmapSize(globalvar.toolbarSize)

        self.toolSwitcher = toolSwitcher
        self.controller = AuiToolbarController(
            classObject=aui.AuiToolBar,
            widget=self,
            parent=self.parent,
            toolSwitcher=toolSwitcher,
        )

    def _toolbarData(self):
        """Toolbar data (virtual)"""
        return

    def Enable(self, tool, enable=True):
        """@copydoc ToolbarController::Enable()"""
        self.controller.Enable(tool, enable)

    def CreateTool(self, *args, **kwargs):
        """@copydoc ToolbarController::CreateTool()"""
        self.controller.CreateTool(*args, **kwargs)

    def OnTool(self, event):
        """@copydoc ToolbarController::OnTool()"""
        self.controller.OnTool(event)

    def __getattr__(self, name):
        return getattr(self.controller, name)


class ToolSwitcher:
    """Class handling switching tools in toolbar and custom toggle buttons."""

    def __init__(self):
        self._groups = defaultdict(lambda: defaultdict(list))
        self._toolsGroups = defaultdict(list)

        # emitted when tool is changed
        self.toggleToolChanged = Signal("ToolSwitcher.toggleToolChanged")

    def AddToolToGroup(self, group, toolbar, tool):
        """Adds tool from toolbar to group of exclusive tools.

        :param group: name of group (e.g. 'mouseUse')
        :param toolbar: instance of toolbar
        :param tool: id of a tool from the toolbar
        """
        self._groups[group][toolbar].append(tool)
        self._toolsGroups[tool].append(group)

    def AddCustomToolToGroup(self, group, btnId, toggleHandler):
        """Adds custom tool from to group of exclusive tools (some toggle button).

        :param group: name of group (e.g. 'mouseUse')
        :param btnId: id of a tool (typically button)
        :param toggleHandler: handler to be called to switch the button
        """
        self._groups[group]["custom"].append((btnId, toggleHandler))
        self._toolsGroups[btnId].append(group)

    def RemoveCustomToolFromGroup(self, tool):
        """Removes custom tool from group.

        :param tool: id of the button
        """
        if tool not in self._toolsGroups:
            return
        for group in self._toolsGroups[tool]:
            self._groups[group]["custom"] = [
                (bid, hdlr)
                for (bid, hdlr) in self._groups[group]["custom"]
                if bid != tool
            ]

    def RemoveToolbarFromGroup(self, group, toolbar):
        """Removes toolbar from group.

        Before toolbar is destroyed, it must be removed from group, too.
        Otherwise we can expect some DeadObject errors.

        :param group: name of group (e.g. 'mouseUse')
        :param toolbar: instance of toolbar
        """
        for tb in self._groups[group]:
            if tb == toolbar:
                del self._groups[group][tb]
                break

    def IsToolInGroup(self, tool, group):
        """Checks whether a tool is in a specified group.

        :param tool: tool id
        :param group: name of group (e.g. 'mouseUse')
        """
        for group in self._toolsGroups[tool]:
            for tb in self._groups[group]:
                if tb == "custom":
                    for bid, handler in self._groups[group][tb]:
                        if tool == bid:
                            return True
                elif tb.FindById(tool):
                    return True
        return False

    def ToolChanged(self, tool):
        """When any tool/button is pressed, other tools from group must be unchecked.

        :param tool: id of a tool/button
        """
        for group in self._toolsGroups[tool]:
            for tb in self._groups[group]:
                if tb == "custom":
                    for btnId, handler in self._groups[group][tb]:
                        if btnId != tool:
                            handler(False)
                else:
                    for tl in self._groups[group][tb]:
                        if tb.FindById(tl):  # check if still exists
                            if tl != tool:
                                tb.ToggleTool(tl, False)
                            else:
                                tb.ToggleTool(tool, True)

        self.toggleToolChanged.emit(id=tool)
