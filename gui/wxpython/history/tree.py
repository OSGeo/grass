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

from grass.grassdb.history import read_history, get_current_mapset_gui_history_path


class HistoryBrowserTree:
    """Data class for the history browser tree of executed commands."""

    def __init__(self, max_length=50):
        self.model = TreeModel(ModuleNode)
        self.max_length = max_length
        self.CreateModel()

    def CreateModel(self):
        self.model.RemoveNode(self.model.root)
        history_path = get_current_mapset_gui_history_path()
        if history_path:
            cmd_list = read_history(history_path)
            for label in cmd_list:
                self.UpdateModel(label.strip())

    def UpdateModel(self, label):
        data = {"command": label}
        self.model.AppendNode(parent=self.model.root, label=data["command"], data=data)

    def GetModel(self):
        """Returns a deep copy of the model."""
        return copy.deepcopy(self.model)
