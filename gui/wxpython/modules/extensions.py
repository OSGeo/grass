"""
@package modules.extensions

@brief GRASS Addons extensions management classes

Classes:
 - extensions::InstallExtensionWindow
 - extensions::ExtensionTreeModelBuilder
 - extensions::ManageExtensionWindow
 - extensions::CheckListExtension

(C) 2008-2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Anna Petrasova <kratochanna gmail.com>
"""

import os
import sys

import wx

from grass.script import task as gtask

from core import globalvar
from core.gcmd import GError, RunCommand, GException, GMessage
from core.utils import SetAddOnPath
from core.gthread import gThread
from core.menutree import TreeModel, ModuleNode
from gui_core.widgets import GListCtrl
from gui_core.treeview import CTreeView
from core.toolboxes import toolboxesOutdated
from gui_core.wrap import Button, StaticBox, Menu, NewId, SearchCtrl


class InstallExtensionWindow(wx.Frame):
    def __init__(
        self,
        parent,
        giface,
        id=wx.ID_ANY,
        title=_("Fetch & install extension from GRASS Addons"),
        **kwargs,
    ):
        self.parent = parent
        self._giface = giface
        self.options = dict()  # list of options

        wx.Frame.__init__(self, parent=parent, id=id, title=title, **kwargs)
        self.SetIcon(
            wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"), wx.BITMAP_TYPE_ICO)
        )

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        # self.repoBox = StaticBox(
        #     parent=self.panel, id=wx.ID_ANY, label=" %s " %
        #     _("Repository (leave empty to use the official one)"))
        self.treeBox = StaticBox(
            parent=self.panel,
            id=wx.ID_ANY,
            label=" %s " % _("List of extensions - double-click to install"),
        )

        # self.repo = TextCtrl(parent=self.panel, id=wx.ID_ANY)

        # modelBuilder loads data into tree model
        self.modelBuilder = ExtensionTreeModelBuilder()
        # tree view displays model data
        self.tree = CTreeView(parent=self.panel, model=self.modelBuilder.GetModel())

        self.search = SearchCtrl(self.panel)
        self.search.SetDescriptiveText(_("Search"))
        self.search.ShowCancelButton(True)
        # load data in different thread
        self.thread = gThread()

        self.optionBox = StaticBox(
            parent=self.panel, id=wx.ID_ANY, label=" %s " % _("Options")
        )
        task = gtask.parse_interface("g.extension")
        ignoreFlags = ["l", "c", "g", "a", "f", "t", "help", "quiet"]
        if sys.platform == "win32":
            ignoreFlags.append("d")
            ignoreFlags.append("i")

        for f in task.get_options()["flags"]:
            name = f.get("name", "")
            desc = f.get("label", "")
            if not desc:
                desc = f.get("description", "")
            if not name and not desc:
                continue
            if name in ignoreFlags:
                continue
            self.options[name] = wx.CheckBox(
                parent=self.panel, id=wx.ID_ANY, label=desc
            )
        # defaultUrl = ''  # default/official one will be used when option empty
        # self.repo.SetValue(
        #     task.get_param(
        #         value='url').get(
        #         'default',
        #         defaultUrl))

        self.statusbar = self.CreateStatusBar(number=1)

        # self.btnFetch = Button(parent=self.panel, id=wx.ID_ANY,
        #                        label=_("&Fetch"))
        # self.btnFetch.SetToolTip(_("Fetch list of available modules "
        #                            "from GRASS Addons repository"))
        self.btnClose = Button(parent=self.panel, id=wx.ID_CLOSE)
        self.btnInstall = Button(parent=self.panel, id=wx.ID_ANY, label=_("&Install"))
        self.btnInstall.SetToolTip(_("Install selected add-ons GRASS module"))
        self.btnInstall.Enable(False)
        self.btnHelp = Button(parent=self.panel, id=wx.ID_HELP)
        self.btnHelp.SetToolTip(_("Show g.extension manual page"))

        self.btnClose.Bind(wx.EVT_BUTTON, lambda evt: self.Close())
        # self.btnFetch.Bind(wx.EVT_BUTTON, self.OnFetch)
        self.btnInstall.Bind(wx.EVT_BUTTON, self.OnInstall)
        self.btnHelp.Bind(wx.EVT_BUTTON, self.OnHelp)
        self.search.Bind(wx.EVT_TEXT, lambda evt: self.Filter(evt.GetString()))
        self.search.Bind(wx.EVT_SEARCHCTRL_CANCEL_BTN, lambda evt: self.Filter(""))
        self.tree.selectionChanged.connect(self.OnItemSelected)
        self.tree.itemActivated.connect(self.OnItemActivated)
        self.tree.contextMenu.connect(self.OnContextMenu)

        wx.CallAfter(self._fetch)

        self._layout()

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        # repoSizer = wx.StaticBoxSizer(self.repoBox, wx.VERTICAL)
        # repo1Sizer = wx.BoxSizer(wx.HORIZONTAL)
        # repo1Sizer.Add(self.repo, proportion=1,
        #                flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL, border=1)
        # repo1Sizer.Add(self.btnFetch, proportion=0,
        #                flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL, border=1)
        # repoSizer.Add(repo1Sizer,
        #               flag=wx.EXPAND)

        sizer.Add(self.search, proportion=0, flag=wx.EXPAND | wx.ALL, border=3)

        treeSizer = wx.StaticBoxSizer(self.treeBox, wx.HORIZONTAL)
        treeSizer.Add(self.tree, proportion=1, flag=wx.ALL | wx.EXPAND, border=1)

        # options
        optionSizer = wx.StaticBoxSizer(self.optionBox, wx.VERTICAL)
        for key in self.options.keys():
            optionSizer.Add(self.options[key], proportion=0)

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self.btnHelp, proportion=0)
        btnSizer.AddStretchSpacer()
        btnSizer.Add(self.btnClose, proportion=0, flag=wx.RIGHT, border=5)
        btnSizer.Add(self.btnInstall, proportion=0)

        # sizer.Add(repoSizer, proportion=0,
        #           flag=wx.ALL | wx.EXPAND, border=3)
        sizer.Add(
            treeSizer,
            proportion=1,
            flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
            border=3,
        )
        sizer.Add(
            optionSizer,
            proportion=0,
            flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
            border=3,
        )
        sizer.Add(btnSizer, proportion=0, flag=wx.ALL | wx.EXPAND, border=5)

        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)

        self.Layout()

    def _getCmd(self):
        item = self.tree.GetSelected()
        if not item or "command" not in item[0].data:
            GError(_("Extension not defined"), parent=self)
            return

        name = item[0].data["command"]

        flags = list()
        for key in self.options.keys():
            if self.options[key].IsChecked():
                if len(key) == 1:
                    flags.append("-%s" % key)
                else:
                    flags.append("--%s" % key)

        # 'url=' + self.repo.GetValue().strip()]
        return ["g.extension"] + flags + ["extension={}".format(name)]

    def OnFetch(self, event):
        """Fetch list of available extensions"""
        self._fetch()

    def _fetch(self):
        """Fetch list of available extensions"""
        wx.BeginBusyCursor()
        self.SetStatusText(
            _("Fetching list of modules from GRASS-Addons (be patient)..."), 0
        )
        try:
            self.thread.Run(
                callable=self.modelBuilder.Load,
                url="",  # self.repo.GetValue().strip(),
                ondone=lambda event: self._fetchDone(),
            )
        except GException as error:
            self._fetchDone()
            GError(str(error), parent=self, showTraceback=False)

    def _fetchDone(self):
        self.tree.RefreshItems()
        nitems = len(self.modelBuilder.GetModel().SearchNodes(key="command", value="*"))
        self.SetStatusText(_("%d extensions loaded") % nitems, 0)
        wx.EndBusyCursor()

    def Filter(self, text):
        model = self.modelBuilder.GetModel()
        if text:
            model = model.Filtered(
                key=["command", "keywords", "description"], value=text
            )
            self.tree.SetModel(model)
            self.tree.ExpandAll()
        else:
            self.tree.SetModel(model)

    def OnContextMenu(self, node):
        if not hasattr(self, "popupID"):
            self.popupID = dict()
            for key in ("install", "help"):
                self.popupID[key] = NewId()

        data = node.data
        if data and "command" in data:
            self.popupMenu = Menu()
            self.popupMenu.Append(self.popupID["install"], _("Install"))
            self.Bind(wx.EVT_MENU, self.OnInstall, id=self.popupID["install"])
            self.popupMenu.AppendSeparator()
            self.popupMenu.Append(self.popupID["help"], _("Show manual page"))
            self.Bind(wx.EVT_MENU, self.OnItemHelp, id=self.popupID["help"])

            self.PopupMenu(self.popupMenu)
            self.popupMenu.Destroy()

    def OnItemActivated(self, node):
        data = node.data
        if data and "command" in data:
            self.OnInstall(event=None)

    def OnInstall(self, event):
        """Install selected extension"""
        log = self.parent.GetLogWindow()
        cmd = self._getCmd()
        if cmd:
            log.RunCmd(cmd, onDone=self.OnDone)

    def OnDone(self, event):
        if event.returncode == 0:
            if not os.getenv("GRASS_ADDON_BASE"):
                SetAddOnPath(key="BASE")

            globalvar.UpdateGRASSAddOnCommands()
            toolboxesOutdated()

    def OnItemHelp(self, event):
        item = self.tree.GetSelected()
        if not item or "command" not in item[0].data:
            return

        self._giface.Help(entry=item[0].data["command"], online=True)

    def OnHelp(self, event):
        self._giface.Help(entry="g.extension")

    def OnItemSelected(self, node):
        """Item selected"""
        data = node.data
        if data is None:
            self.SetStatusText("", 0)
            self.btnInstall.Enable(False)
        else:
            self.SetStatusText(data.get("description", ""), 0)
            self.btnInstall.Enable(True)


class ExtensionTreeModelBuilder:
    """Tree model of available extensions."""

    def __init__(self):
        self.mainNodes = dict()
        self.model = TreeModel(ModuleNode)
        for prefix in (
            "display",
            "database",
            "general",
            "imagery",
            "misc",
            "postscript",
            "paint",
            "raster",
            "raster3D",
            "sites",
            "temporal",
            "vector",
            "wxGUI",
            "other",
        ):
            node = self.model.AppendNode(parent=self.model.root, label=prefix)
            self.mainNodes[prefix] = node

    def GetModel(self):
        return self.model

    def _emptyTree(self):
        """Remove modules from tree keeping the main structure"""
        for node in self.mainNodes.values():
            for child in reversed(node.children):
                self.model.RemoveNode(child)

    def _expandPrefix(self, c):
        name = {
            "d": "display",
            "db": "database",
            "g": "general",
            "i": "imagery",
            "m": "misc",
            "ps": "postscript",
            "p": "paint",
            "r": "raster",
            "r3": "raster3D",
            "s": "sites",
            "t": "temporal",
            "v": "vector",
            "wx": "wxGUI",
            "": "other",
        }

        if c in name:
            return name[c]

        return c

    def Load(self, url, full=True):
        """Load list of extensions"""
        self._emptyTree()

        if full:
            flags = "g"
        else:
            flags = "l"
        retcode, ret, msg = RunCommand(
            "g.extension", read=True, getErrorMsg=True, url=url, flags=flags, quiet=True
        )
        if retcode != 0:
            raise GException(_("Unable to load extensions. %s") % msg)

        currentNode = None
        for line in ret.splitlines():
            if full:
                try:
                    key, value = line.split("=", 1)
                except ValueError:
                    key = "name"
                    value = line

                if key == "name":
                    try:
                        prefix, name = value.split(".", 1)
                    except ValueError:
                        prefix = ""
                        name = value
                    mainNode = self.mainNodes[self._expandPrefix(prefix)]
                    currentNode = self.model.AppendNode(parent=mainNode, label=value)
                    currentNode.data = {"command": value}
                else:
                    if currentNode is not None:
                        currentNode.data[key] = value
            else:
                try:
                    prefix, name = line.strip().split(".", 1)
                except ValueError:
                    prefix = ""
                    name = line.strip()

                if self._expandPrefix(prefix) == prefix:
                    prefix = ""
                module = prefix + "." + name
                mainNode = self.mainNodes[self._expandPrefix(prefix)]
                currentNode = self.model.AppendNode(parent=mainNode, label=module)
                currentNode.data = {
                    "command": module,
                    "keywords": "",
                    "description": "",
                }


class ManageExtensionWindow(wx.Frame):
    def __init__(
        self,
        parent,
        id=wx.ID_ANY,
        title=_("Manage installed GRASS Addons extensions"),
        **kwargs,
    ):
        self.parent = parent

        wx.Frame.__init__(self, parent=parent, id=id, title=title, **kwargs)
        self.SetIcon(
            wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"), wx.BITMAP_TYPE_ICO)
        )

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        self.extBox = StaticBox(
            parent=self.panel,
            id=wx.ID_ANY,
            label=" %s " % _("List of installed extensions"),
        )

        self.extList = CheckListExtension(parent=self.panel)

        # buttons
        self.btnUninstall = Button(
            parent=self.panel, id=wx.ID_REMOVE, label=_("Uninstall")
        )
        self.btnUninstall.SetToolTip(_("Uninstall selected Addons extensions"))
        self.btnUpdate = Button(
            parent=self.panel, id=wx.ID_REFRESH, label=_("Reinstall")
        )
        self.btnUpdate.SetToolTip(_("Reinstall selected Addons extensions"))

        self.btnClose = Button(parent=self.panel, id=wx.ID_CLOSE)

        self.btnUninstall.Bind(wx.EVT_BUTTON, self.OnUninstall)
        self.btnUpdate.Bind(wx.EVT_BUTTON, self.OnUpdate)
        self.btnClose.Bind(wx.EVT_BUTTON, lambda evt: self.Close())

        self._layout()

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)

        extSizer = wx.StaticBoxSizer(self.extBox, wx.HORIZONTAL)
        extSizer.Add(self.extList, proportion=1, flag=wx.ALL | wx.EXPAND, border=1)

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(self.btnClose, proportion=0, flag=wx.RIGHT, border=5)
        btnSizer.Add(self.btnUpdate, proportion=0, flag=wx.RIGHT, border=5)
        btnSizer.Add(self.btnUninstall, proportion=0)

        sizer.Add(extSizer, proportion=1, flag=wx.ALL | wx.EXPAND, border=3)
        sizer.Add(btnSizer, proportion=0, flag=wx.ALIGN_RIGHT | wx.ALL, border=5)

        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)

        self.Layout()

    def _getSelectedExtensions(self):
        eList = self.extList.GetExtensions()
        if not eList:
            GMessage(_("No extension selected. " "Operation canceled."), parent=self)
            return []

        return eList

    def OnUninstall(self, event):
        """Uninstall selected extensions"""
        eList = self._getSelectedExtensions()
        if not eList:
            return

        for ext in eList:
            files = RunCommand(
                "g.extension",
                parent=self,
                read=True,
                quiet=True,
                extension=ext,
                operation="remove",
            ).splitlines()
            if len(files) > 10:
                files = files[:10]
                files.append("...")
            dlg = wx.MessageDialog(
                parent=self,
                message=_(
                    "List of files to be removed:\n%(files)s\n\n"
                    "Do you want really to remove <%(ext)s> extension?"
                )
                % {"files": os.linesep.join(files), "ext": ext},
                caption=_("Remove extension"),
                style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION,
            )

            if dlg.ShowModal() == wx.ID_YES:
                RunCommand(
                    "g.extension",
                    flags="f",
                    parent=self,
                    quiet=True,
                    extension=ext,
                    operation="remove",
                )

        self.extList.LoadData()

        # update prompt
        globalvar.UpdateGRASSAddOnCommands(eList)
        toolboxesOutdated()

    def OnUpdate(self, event):
        """Update selected extensions"""
        eList = self._getSelectedExtensions()
        if not eList:
            return

        log = self.parent.GetLogWindow()

        for ext in eList:
            log.RunCmd(["g.extension", "extension=%s" % ext, "operation=add"])


class CheckListExtension(GListCtrl):
    """List of mapset/owner/group"""

    def __init__(self, parent):
        GListCtrl.__init__(self, parent)

        # load extensions
        self.InsertColumn(0, _("Extension"))
        self.LoadData()

    def LoadData(self):
        """Load data into list"""
        self.DeleteAllItems()
        for ext in RunCommand(
            "g.extension", quiet=True, parent=self, read=True, flags="a"
        ).splitlines():
            if ext:
                self.InsertItem(self.GetItemCount(), ext)

    def GetExtensions(self):
        """Get extensions to be un-installed"""
        extList = list()
        for i in range(self.GetItemCount()):
            if self.IsItemChecked(i):
                name = self.GetItemText(i)
                if name:
                    extList.append(name)

        return extList
