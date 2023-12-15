"""
@package history.browser

@brief History browser

Classes:
 - browser::HistoryBrowser

(C) 2023 by Linda Karlovska, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Linda Karlovska (Kladivova) linda.karlovska@seznam.cz
"""

import wx
import re

from core import globalvar
from core.gcmd import GError, GException
from core.utils import split
from gui_core.forms import GUI
from gui_core.treeview import CTreeView
from history.tree import HistoryBrowserTree

from grass.pydispatch.signal import Signal


class HistoryBrowser(wx.Panel):
    """History browser for executing the commands from history log.

    Signal:
        showNotification - attribute 'message'
    """

    def __init__(
        self,
        parent,
        giface,
        id=wx.ID_ANY,
        title=_("History browser"),
        name="history",
        **kwargs,
    ):
        self.parent = parent
        self._giface = giface

        self.showNotification = Signal("HistoryBrowser.showNotification")
        self.runIgnoredCmdPattern = Signal("HistoryBrowser.runIgnoredCmdPattern")
        wx.Panel.__init__(self, parent=parent, id=id, **kwargs)

        self._createTree()

        self._giface.currentMapsetChanged.connect(self.UpdateHistoryModelFromScratch)
        self._giface.updateHistory.connect(
            lambda cmd: self.UpdateHistoryModelByCommand(cmd)
        )

        self._layout()

    def _layout(self):
        """Dialog layout"""
        sizer = wx.BoxSizer(wx.VERTICAL)
        sizer.Add(
            self._tree, proportion=1, flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=5
        )

        self.SetSizerAndFit(sizer)
        self.SetAutoLayout(True)
        self.Layout()

    def _createTree(self):
        """Create tree based on the model"""
        self._model = HistoryBrowserTree()
        self._tree = self._getTreeInstance()
        self._tree.SetToolTip(_("Double-click to open the tool"))
        self._tree.selectionChanged.connect(self.OnItemSelected)
        self._tree.itemActivated.connect(lambda node: self.Run(node))

    def _getTreeInstance(self):
        return CTreeView(model=self._model.GetModel(), parent=self)

    def _getSelectedNode(self):
        selection = self._tree.GetSelected()
        if not selection:
            return None
        return selection[0]

    def _parseMapcalcCmd(self, command):
        """Parse r.mapcalc/r3.mapcalc module command

        Examples:

        r.mapcalc foo = 1 -> r.mapcalc expression=foo=1

        r.mapcalc foo=1 -> r.mapcalc expression=foo=1

        r.mapcalc expression=foo=1 -> r.mapcalc expression=foo=1

        r.mapcalc expression=foo = 1 -> r.mapcalc expression=foo=1

        r.mapcalc expression="foo = 1" -> r.mapcalc expression="foo=1"

        r.mapcalc expression="foo=1" -> r.mapcalc expression="foo=1"

        r.mapcalc expression='foo = 1' -> r.mapcalc expression='foo=1'

        r.mapcalc expression='foo=1' -> r.mapcalc expression='foo=1'


        :param str command: r.mapcalc command string

        :return str command: parsed r.mapcalc command string
        """
        flags = []
        others_params_args = []
        expression_param = "expression="
        command = command.split()

        for arg in command[:]:
            if arg.startswith("-"):
                flags.append(command.pop(command.index(arg)))
            elif "region=" in arg or "file=" in arg or "seed=" in arg:
                others_params_args.append(command.pop(command.index(arg)))

        if expression_param not in "".join(command):
            expression_param_arg = f"{expression_param}{''.join(command[1:])}"
        else:
            expression_param_arg = "".join(command[1:])

        command = " ".join(
            [
                command[0],
                *flags,
                expression_param_arg,
                *others_params_args,
            ]
        )
        return command

    def _refreshTree(self):
        self._tree.SetModel(self._model.GetModel())

    def _replaceModuleCmdSpecialFlags(self, command):
        """Replace module command special flags short version with
        full version

        Flags:

        --o -> --overwrite
        --q -> --quiet
        --v -> --verbose

        :param str command: module command string

        :return str: module command string with replaced flags
        """
        flags_regex = re.compile(r"(--o(\s+|$))|(--q(\s+|$))|(--v(\s+|$))")
        replace = {
            "--o": "--overwrite ",
            "--q": "--quiet ",
            "--v": "--verbose ",
        }
        return re.sub(
            flags_regex,
            lambda flag: replace[flag.group().strip()],
            command,
        )

    def UpdateHistoryModelFromScratch(self):
        """Update the model from scratch and refresh the tree"""
        self._model.CreateModel()
        self._refreshTree()

    def UpdateHistoryModelByCommand(self, cmd):
        """Update the model by the command and refresh the tree"""
        self._model.UpdateModel(cmd)
        self._refreshTree()

    def OnItemSelected(self, node):
        """Item selected"""
        command = node.data["command"]
        self.showNotification.emit(message=command)

    def Run(self, node=None):
        """Parse selected history command into list and launch module dialog."""
        node = node or self._getSelectedNode()
        if node:
            command = node.data["command"]
            if (
                globalvar.ignoredCmdPattern
                and re.compile(globalvar.ignoredCmdPattern).search(command)
                and "--help" not in command
                and "--ui" not in command
            ):
                self.runIgnoredCmdPattern.emit(cmd=split(command))
                return
            if re.compile(r"^r[3]?\.mapcalc").search(command):
                command = self._parseMapcalcCmd(command)
            command = self._replaceModuleCmdSpecialFlags(command)
            lst = split(command)
            try:
                GUI(parent=self, giface=self._giface).ParseCommand(lst)
            except GException as e:
                GError(
                    parent=self,
                    message=str(e),
                    caption=_("Cannot be parsed into command"),
                    showTraceback=False,
                )
