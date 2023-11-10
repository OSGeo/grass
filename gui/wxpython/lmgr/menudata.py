"""
@package lmgr.menudata

@brief wxGUI Layer Manager - menu data

Classes:
 - menudata::LayerManagerMenuData


(C) 2007-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
"""

import os
import copy

from core.treemodel import TreeModel, ModuleNode
from core.menutree import MenuTreeModelBuilder
from core.toolboxes import getMenudataFile
from core.globalvar import WXGUIDIR
from core.gcmd import GError

from grass.grassdb.checks import get_current_mapset_history_path


class LayerManagerMenuData(MenuTreeModelBuilder):
    def __init__(self, filename=None, message_handler=GError):
        if filename:
            expandAddons = False
        else:
            expandAddons = True

        fallback = os.path.join(WXGUIDIR, "xml", "menudata.xml")
        if not filename:
            filename = getMenudataFile(
                userRootFile="main_menu.xml", newFile="menudata.xml", fallback=fallback
            )
        try:
            MenuTreeModelBuilder.__init__(
                self,
                filename,
                expandAddons=expandAddons,
                message_handler=message_handler,
            )
        except (ValueError, AttributeError, TypeError):
            message_handler(
                _(
                    "Unable to parse user toolboxes XML files. "
                    "Default main menu will be loaded."
                )
            )
            fallback = os.path.join(WXGUIDIR, "xml", "menudata.xml")
            MenuTreeModelBuilder.__init__(
                self, fallback, message_handler=message_handler
            )


class LayerManagerModuleTree(MenuTreeModelBuilder):
    def __init__(self, filename=None, message_handler=GError):
        if filename:
            expandAddons = False
        else:
            expandAddons = True

        fallback = os.path.join(WXGUIDIR, "xml", "module_tree_menudata.xml")
        if not filename:
            filename = getMenudataFile(
                userRootFile="module_tree.xml",
                newFile="module_tree_menudata.xml",
                fallback=fallback,
            )
        # TODO: try-except useless?
        try:
            MenuTreeModelBuilder.__init__(
                self,
                filename,
                expandAddons=expandAddons,
                message_handler=message_handler,
            )
        except (ValueError, AttributeError, TypeError):
            message_handler(
                _(
                    "Unable to parse user toolboxes XML files. "
                    "Default module tree will be loaded."
                )
            )
            MenuTreeModelBuilder.__init__(
                self, fallback, message_handler=message_handler
            )


class HistoryModuleTree:
    """Data class for the history of executed commands."""

    def __init__(self, max_length=50):
        self.model = TreeModel(ModuleNode)
        self.max_length = max_length
        self.CreateModel()

    def CreateModel(self):
        self.model.RemoveNode(self.model.root)
        history_path = get_current_mapset_history_path()
        try:
            with open(history_path, "r") as f:
                for label in f:
                    self._addLabelToModel(label.strip())
        except Exception:
            pass

    def _trim_text(self, text):
        if len(text) > self.max_length:
            return text[: self.max_length - 3] + "..."
        else:
            return text

    def _addLabelToModel(self, label):
        data = {"label": self._trim_text(label), "command": label}
        self.model.AppendNode(parent=self.model.root, label=data["label"], data=data)

    def GetModel(self):
        """Returns a deep copy of the model."""
        return copy.deepcopy(self.model)
