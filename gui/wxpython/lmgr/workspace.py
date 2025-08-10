"""
@package lmgr::workspace

@brief Workspace manager class for creating, loading and saving workspaces

Class:
 - lmgr::WorkspaceManager

(C) 2021 by the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.
"""

import os
import tempfile

import xml.etree.ElementTree as ET

from pathlib import Path

import wx
import wx.aui

from core.settings import UserSettings
from core.gcmd import RunCommand, GError, GMessage
from core.workspace import ProcessWorkspaceFile, WriteWorkspaceFile
from core.debug import Debug
from gui_core.menu import RecentFilesMenu


class WorkspaceManager:
    """Workspace Manager for creating, loading and saving workspaces."""

    def __init__(self, lmgr, giface):
        self.lmgr = lmgr
        self.workspaceFile = None
        self._giface = giface
        self.workspaceChanged = False  # track changes in workspace
        self.loadingWorkspace = False
        self._recent_files = None

        Debug.msg(1, "WorkspaceManager.__init__()")

        self._giface.workspaceChanged.connect(self.WorkspaceChanged)

    def WorkspaceChanged(self):
        """Update window title"""
        self.workspaceChanged = True

    def New(self):
        """Create new workspace file
        Erase current workspace settings first
        """
        Debug.msg(4, "WorkspaceManager.New():")

        # start new map display if no display is available
        if not self.lmgr.currentPage:
            self.lmgr.NewDisplay()

        maptrees = [
            self.lmgr.notebookLayers.GetPage(i).maptree
            for i in range(self.lmgr.notebookLayers.GetPageCount())
        ]

        # ask user to save current settings
        if self.workspaceFile and self.workspaceChanged:
            self.Save()
        elif self.workspaceFile is None and any(tree.GetCount() for tree in maptrees):
            dlg = wx.MessageDialog(
                self.lmgr,
                message=_(
                    "Current workspace is not empty. "
                    "Do you want to store current settings "
                    "to workspace file?"
                ),
                caption=_("Create new workspace?"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.CANCEL | wx.ICON_QUESTION,
            )
            ret = dlg.ShowModal()
            if ret == wx.ID_YES:
                self.SaveAs()
            elif ret == wx.ID_CANCEL:
                dlg.Destroy()
                return

            dlg.Destroy()

        # delete all layers in map displays
        for maptree in maptrees:
            maptree.DeleteAllLayers()

        # delete all decorations
        for display in self.lmgr.GetAllMapDisplays():
            for overlayId in list(display.decorations):
                display.RemoveOverlay(overlayId)

        self.workspaceFile = None
        self.workspaceChanged = False
        self.lmgr._setTitle()

    def Open(self):
        """Open file with workspace definition"""
        dlg = wx.FileDialog(
            parent=self.lmgr,
            message=_("Choose workspace file"),
            defaultDir=str(Path.cwd()),
            wildcard=_("GRASS Workspace File (*.gxw)|*.gxw"),
        )

        filename = ""
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if filename == "":
            return

        Debug.msg(4, "WorkspaceManager.Open(): filename=%s" % filename)

        # delete current layer tree content
        self.Close()
        self.loadingWorkspace = True
        self.Load(filename)
        self.loadingWorkspace = False
        self.lmgr._setTitle()

    def _tryToSwitchMapsetFromWorkspaceFile(self, gxwXml):
        returncode, errors = RunCommand(
            "g.mapset",
            dbase=gxwXml.database,
            project=gxwXml.location,
            mapset=gxwXml.mapset,
            getErrorMsg=True,
        )
        if returncode != 0:
            # TODO: use the function from grass.py
            reason = _("Most likely the database, location or mapset does not exist")
            details = errors
            message = _(
                "Unable to change to location and mapset"
                " specified in the workspace.\n"
                "Reason: {reason}\nDetails: {details}\n\n"
                "Do you want to proceed with opening"
                " the workspace anyway?"
            ).format(**locals())
            dlg = wx.MessageDialog(
                parent=self.lmgr,
                message=message,
                caption=_("Proceed with opening of the workspace?"),
                style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
            )
            dlg.CenterOnParent()
            if dlg.ShowModal() in {wx.ID_NO, wx.ID_CANCEL}:
                return False
        else:
            # TODO: copy from ChangeLocation function
            GMessage(
                parent=self.lmgr,
                message=_(
                    "Current location is <%(loc)s>.\nCurrent mapset is <%(mapset)s>."
                )
                % {"loc": gxwXml.location, "mapset": gxwXml.mapset},
            )
        return True

    def Load(self, filename):
        """Load layer tree definition stored in GRASS Workspace XML file (gxw)
        .. todo::
            Validate against DTD
        :return: True on success
        :return: False on error
        """
        # parse workspace file
        try:
            gxwXml = ProcessWorkspaceFile(ET.parse(filename))
        except Exception as e:
            GError(
                parent=self.lmgr,
                message=_(
                    "Reading workspace file <%s> failed.\n"
                    "Invalid file, unable to parse XML document.\n"
                    "Error details: %s"
                )
                % (filename, str(e)),
            )
            return False

        if gxwXml.database and gxwXml.location and gxwXml.mapset:
            if not self._tryToSwitchMapsetFromWorkspaceFile(gxwXml):
                return False

        # the really busy part starts here (mapset change is fast)
        busy = wx.BusyInfo(_("Please wait, loading workspace..."), parent=self.lmgr)
        wx.GetApp().Yield()

        #
        # load layer manager window properties
        #
        if (
            UserSettings.Get(
                group="general", key="workspace", subkey=["posManager", "enabled"]
            )
            is False
        ):
            if gxwXml.layerManager["pos"]:
                self.lmgr.SetPosition(gxwXml.layerManager["pos"])
            if gxwXml.layerManager["size"]:
                self.lmgr.SetSize(gxwXml.layerManager["size"])
            if gxwXml.layerManager["cwd"]:
                self.lmgr.cwdPath = gxwXml.layerManager["cwd"]
                if os.path.isdir(self.lmgr.cwdPath):
                    os.chdir(self.lmgr.cwdPath)

        #
        # start map displays first (list of layers can be empty)
        #
        displayId = 0
        mapdisplay = []
        for display in gxwXml.displays:
            mapdisp = self.lmgr.NewDisplay(name=display["name"], show=False)
            mapdisplay.append(mapdisp)
            maptree = self.lmgr.notebookLayers.GetPage(displayId).maptree

            # set windows properties
            mapdisp.SetProperties(
                render=display["render"],
                mode=display["mode"],
                showCompExtent=display["showCompExtent"],
                alignExtent=display["alignExtent"],
                constrainRes=display["constrainRes"],
                projection=display["projection"]["enabled"],
            )

            if display["projection"]["enabled"]:
                if display["projection"]["epsg"]:
                    UserSettings.Set(
                        group="display",
                        key="projection",
                        subkey="epsg",
                        value=display["projection"]["epsg"],
                    )
                    if display["projection"]["proj"]:
                        UserSettings.Set(
                            group="display",
                            key="projection",
                            subkey="proj4",
                            value=display["projection"]["proj"],
                        )

            # set position and size of map display
            if not UserSettings.Get(
                group="general", key="workspace", subkey=["posDisplay", "enabled"]
            ):
                if display["pos"]:
                    mapdisp.SetPosition(display["pos"])
                if display["size"]:
                    mapdisp.SetSize(display["size"])

            # set extent if defined
            if display["extent"]:
                w, s, e, n, b, t = display["extent"]
                region = maptree.Map.region = maptree.Map.GetRegion(w=w, s=s, e=e, n=n)
                mapdisp.GetWindow().ResetZoomHistory()
                mapdisp.GetWindow().ZoomHistory(
                    region["n"], region["s"], region["e"], region["w"]
                )
            if "showStatusbar" in display and not display["showStatusbar"]:
                mapdisp.ShowStatusbar(False)
            if "showToolbars" in display and not display["showToolbars"]:
                for toolbar in mapdisp.GetToolbarNames():
                    mapdisp.RemoveToolbar(toolbar)
            if "isDocked" in display and not display["isDocked"]:
                mapdisp.OnDockUndock()

            displayId += 1
            mapdisp.Show()  # show mapdisplay
            # set render property to False to speed up loading layers
            mapdisp.mapWindowProperties.autoRender = False

        maptree = None
        selectList = []  # list of selected layers
        #
        # load list of map layers
        #
        for layer in gxwXml.layers:
            display = layer["display"]
            maptree = self.lmgr.notebookLayers.GetPage(display).maptree
            newItem = maptree.AddLayer(
                ltype=layer["type"],
                lname=layer["name"],
                lchecked=layer["checked"],
                lopacity=layer["opacity"],
                lcmd=layer["cmd"],
                lgroup=layer["group"],
                lnviz=layer["nviz"],
                lvdigit=layer["vdigit"],
                loadWorkspace=True,
            )

            if "selected" in layer:
                selectList.append((maptree, newItem, layer["selected"]))

        for maptree, layer, selected in selectList:
            if selected:
                if not layer.IsSelected():
                    maptree.SelectItem(layer, select=True)
            else:
                maptree.SelectItem(layer, select=False)

        del busy

        # set render property again when all layers are loaded
        for i, display in enumerate(gxwXml.displays):
            mapdisplay[i].mapWindowProperties.autoRender = display["render"]

            for overlay in gxwXml.overlays:
                # overlay["cmd"][0] name of command e.g. d.barscale, d.legend
                # overlay["cmd"][1:] parameters and flags
                if overlay["display"] != i:
                    continue
                if overlay["cmd"][0] == "d.legend.vect":
                    mapdisplay[i].AddLegendVect(overlay["cmd"])
                if overlay["cmd"][0] == "d.legend":
                    mapdisplay[i].AddLegendRast(overlay["cmd"])
                if overlay["cmd"][0] == "d.barscale":
                    mapdisplay[i].AddBarscale(overlay["cmd"])
                if overlay["cmd"][0] == "d.northarrow":
                    mapdisplay[i].AddArrow(overlay["cmd"])
                if overlay["cmd"][0] == "d.text":
                    mapdisplay[i].AddDtext(overlay["cmd"])

            # avoid double-rendering when loading workspace
            # mdisp.MapWindow2D.UpdateMap()
            # nviz
            if gxwXml.displays[i]["viewMode"] != "3d":
                continue
            mapdisplay[i].AddNviz()
            self.lmgr.nvizUpdateState(
                view=gxwXml.nviz_state["view"],
                iview=gxwXml.nviz_state["iview"],
                light=gxwXml.nviz_state["light"],
            )
            mapdisplay[i].MapWindow3D.constants = gxwXml.nviz_state["constants"]
            for idx, constant in enumerate(mapdisplay[i].MapWindow3D.constants):
                mapdisplay[i].MapWindow3D.AddConstant(constant, i + 1)
            for page in ("view", "light", "fringe", "constant", "cplane"):
                self.lmgr.nvizUpdatePage(page)
            self.lmgr.nvizUpdateSettings()
            mapdisplay[i].toolbars["map"].combo.SetSelection(1)

        #
        # load layout
        #
        if UserSettings.Get(group="appearance", key="singleWindow", subkey="enabled"):
            if gxwXml.layout["panes"]:
                self.lmgr.GetAuiManager().LoadPerspective(gxwXml.layout["panes"])
            if gxwXml.layout["notebook"]:
                self.lmgr.GetAuiNotebook().LoadPerspective(gxwXml.layout["notebook"])

        self.workspaceFile = filename
        self.AddFileToHistory()
        return True

    def SaveAs(self):
        """Save workspace definition to selected file"""
        dlg = wx.FileDialog(
            parent=self.lmgr,
            message=_("Choose file to save current workspace"),
            defaultDir=str(Path.cwd()),
            wildcard=_("GRASS Workspace File (*.gxw)|*.gxw"),
            style=wx.FD_SAVE,
        )

        filename = ""
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()

        if filename == "":
            return False

        # check for extension
        if filename[-4:] != ".gxw":
            filename += ".gxw"

        if os.path.exists(filename):
            dlg = wx.MessageDialog(
                self.lmgr,
                message=_(
                    "Workspace file <%s> already exists. "
                    "Do you want to overwrite this file?"
                )
                % filename,
                caption=_("Save workspace"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
            )
            if dlg.ShowModal() != wx.ID_YES:
                dlg.Destroy()
                return False

        Debug.msg(4, "WorkspaceManager.SaveAs(): filename=%s" % filename)

        self.SaveToFile(filename)
        self.workspaceFile = filename
        self.lmgr._setTitle()

    def Save(self):
        """Save file with workspace definition"""
        if self.workspaceFile:
            dlg = wx.MessageDialog(
                self.lmgr,
                message=_(
                    "Workspace file <%s> already exists. "
                    "Do you want to overwrite this file?"
                )
                % self.workspaceFile,
                caption=_("Save workspace"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
            )
            if dlg.ShowModal() == wx.ID_NO:
                dlg.Destroy()
            else:
                Debug.msg(
                    4, "WorkspaceManager.Save(): filename=%s" % self.workspaceFile
                )
                self.SaveToFile(self.workspaceFile)
                self.lmgr._setTitle()
                self.workspaceChanged = False
        else:
            self.SaveAs()

    def SaveToFile(self, filename):
        """Save layer tree layout to workspace file
        :return: True on success, False on error
        """
        with tempfile.TemporaryFile(mode="w+b") as tmpfile:
            try:
                WriteWorkspaceFile(lmgr=self.lmgr, file=tmpfile)
            except Exception as e:
                GError(
                    parent=self.lmgr,
                    message=_(
                        "Writing current settings to workspace file <%s> failed.\n"
                        "Error details: %s"
                    )
                    % (tmpfile, str(e)),
                )
                return False
            try:
                with open(filename, "wb") as mfile:
                    tmpfile.seek(0)
                    mfile.writelines(tmpfile.readlines())
            except OSError:
                GError(
                    parent=self.lmgr,
                    message=_("Unable to open file <%s> for writing.") % filename,
                )
                return False
        self.AddFileToHistory(file_path=filename)
        return True

    def CanClosePage(self, caption):
        """Ask if page with map display(s) can be closed"""
        # save changes in the workspace
        maptree = self._giface.GetLayerTree()
        if self.workspaceChanged and UserSettings.Get(
            group="manager", key="askOnQuit", subkey="enabled"
        ):
            if self.workspaceFile:
                message = _("Do you want to save changes in the workspace?")
            else:
                message = _("Do you want to store current settings to workspace file?")

            # ask user to save current settings
            if maptree.GetCount() > 0:
                dlg = wx.MessageDialog(
                    self.lmgr,
                    message=message,
                    caption=caption,
                    style=wx.YES_NO
                    | wx.YES_DEFAULT
                    | wx.CANCEL
                    | wx.ICON_QUESTION
                    | wx.CENTRE,
                )
                ret = dlg.ShowModal()
                dlg.Destroy()
                if ret == wx.ID_YES:
                    if not self.workspaceFile:
                        self.SaveAs()
                    else:
                        self.SaveToFile(self.workspaceFile)
                elif ret == wx.ID_CANCEL:
                    return False
        return True

    def Close(self):
        """Close file with workspace definition
        If workspace has been modified ask user to save the changes.
        """
        Debug.msg(4, "WorkspaceManager.Close(): file=%s" % self.workspaceFile)

        self.lmgr.DisplayCloseAll()
        self.workspaceFile = None
        self.workspaceChanged = False
        self.lmgr._setTitle()
        self.lmgr.displayIndex = 0
        self.lmgr.currentPage = None

    def CreateRecentFilesMenu(self, menu=None):
        """Create workspace recent files menu

        :param gui_core.menu.Menu menu: menu default arg is None

        :return None
        """
        if menu:
            menu_index = menu.FindMenu(_("File"))
            if menu_index == wx.NOT_FOUND:
                # try untranslated version
                menu_index = menu.FindMenu("File")
                if menu_index == wx.NOT_FOUND:
                    return
            file_menu = menu.GetMenu(menu_index)
            workspace_index = file_menu.FindItem(_("Workspace"))
            if workspace_index == wx.NOT_FOUND:
                workspace_index = file_menu.FindItem("Workspace")
                if workspace_index == wx.NOT_FOUND:
                    return
            workspace_item = file_menu.FindItemById(workspace_index)

            self._recent_files = RecentFilesMenu(
                app_name="main",
                parent_menu=workspace_item.GetSubMenu(),
                pos=0,
            )
            self._recent_files.file_requested.connect(self.OpenRecentFile)

    def AddFileToHistory(self, file_path=None):
        """Add file to history (recent files)

        :param str file_path: file path wit default arg None

        :return None
        """
        if not file_path:
            file_path = self.workspaceFile
        if self._recent_files:
            self._recent_files.AddFileToHistory(filename=file_path)

    def OpenRecentFile(self, path, file_exists, file_history):
        """Try open recent file and read content

        :param str path: file path
        :param bool file_exists: file path exists
        :param bool file_history: file history obj instance

        :return: None
        """
        if not file_exists:
            GError(
                _("File <{}> doesn't exist. It was probably moved or deleted.").format(
                    path
                ),
                parent=self.lmgr,
            )
        else:
            self.Close()
            self.loadingWorkspace = True
            self.Load(path)
            self.loadingWorkspace = False
            self.lmgr._setTitle()
            file_history.AddFileToHistory(filename=path)  # move up the list
