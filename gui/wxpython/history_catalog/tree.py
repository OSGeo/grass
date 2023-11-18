"""
@package history_catalog::tree

@brief History catalog tree classes

Classes:
 - history_catalog::HistoryCatalogTree

(C) 2023 by Linda Karlovska, and the GRASS Development Team

This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Linda Karlovska (Kladivova) linda.karlovska@seznam.cz
"""

import copy

from core.treemodel import TreeModel, ModuleNode

from grass.grassdb.history import read_history


class HistoryCatalogTree:
    """Data class for the history catalog tree of executed commands."""

    def __init__(self, max_length=50):
        self.model = TreeModel(ModuleNode)
        self.max_length = max_length
        self.CreateModel()

    def CreateModel(self):
        self.model.RemoveNode(self.model.root)
        cmd_list = read_history()
        for label in cmd_list:
            self._addLabelToModel(label.strip())

    def UpdateModel(self, command):
        self._addLabelToModel(command)

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
