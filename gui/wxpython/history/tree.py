"""
@package history.tree

@brief History browser tree

Classes:

 - browser::HistoryBrowserTree

(C) 2023 by Linda Karlovska, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Linda Karlovska (Kladivova) linda.karlovska@seznam.cz
"""

import copy

from core.treemodel import TreeModel, ModuleNode

from grass.grassdb.history import (
    read_history,
    get_current_mapset_gui_history_path,
    update_history,
)


class HistoryBrowserTree:
    """Data class for the history browser tree of executed commands."""

    def __init__(self, max_length=50):
        self._history_path = None
        self.model = TreeModel(ModuleNode)
        self.max_length = max_length
        self.CreateModel()

    def CreateModel(self):
        self.model.RemoveNode(self.model.root)
        self._history_path = get_current_mapset_gui_history_path()
        if self._history_path:
            cmd_list = read_history(self._history_path)
            for label in cmd_list:
                self.UpdateModel(label.strip())

    def DeleteEntryFromHistory(self, node):
        """Delete command from the history file

        :param object node: selected command tree node object instance
        """
        if self._history_path:
            update_history(
                history_path=self._history_path,
                update="delete",
                del_line_number=self.model.GetIndexOfNode(node)[0],
            )

    def GetModel(self):
        """Returns a deep copy of the model."""
        return copy.deepcopy(self.model)

    def UpdateModel(self, label, update="add", node=None):
        """Add/delete model node

        :param str|None label: model node label if update param arg is add
        :param str update: type of model update operation add|delete
                           model node
        :param object|None node: selected tree node object instance if
                                 update param arg is delete
        """
        if update == "add":
            data = {"command": label}
            self.model.AppendNode(
                parent=self.model.root,
                label=data["command"],
                data=data,
            )
        else:
            self.model.RemoveNode(node)
