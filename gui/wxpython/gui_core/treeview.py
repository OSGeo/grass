"""!
@package gui_core.treeview

@brief tree view for dislaying tree model (used for search tree)

Classes:
 - treeview::TreeView

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import os
import sys
import wx
from wx.lib.mixins.treemixin import VirtualTree, ExpansionState
try:
    import wx.lib.agw.customtreectrl as CT
except ImportError:
    import wx.lib.customtreectrl as CT
import wx.gizmos as gizmos

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from core.treemodel import TreeModel, DictNode

from grass.pydispatch.signal import Signal


class AbstractTreeViewMixin(VirtualTree):
    """!Abstract tree view class for displaying tree model.

    Concrete implementation must inherit both this mixin class and a wx tree widget.
    More functionality and signals can be added if needed.

    Signals:
        selectionChanged - attribute 'node'
        itemActivated - attribute 'node'
    """
    def __init__(self, model, parent, *args, **kw):
        self._model = model
        super(AbstractTreeViewMixin, self).__init__(parent=parent, *args, **kw)
        self.RefreshItems()

        self.selectionChanged = Signal('TreeView.selectionChanged')
        self.itemActivated = Signal('TreeView.itemActivated')

        self.Bind(wx.EVT_TREE_SEL_CHANGED, lambda evt:
                                           self._emitSignal(evt.GetItem(), self.selectionChanged))
        self.Bind(wx.EVT_TREE_ITEM_ACTIVATED, lambda evt:
                                           self._emitSignal(evt.GetItem(), self.itemActivated))

    def SetModel(self, model):
        """!Set tree model and refresh.
        
        @param model tree model        
        """
        self._model = model
        self.RefreshItems()

    def OnGetItemText(self, index, column=0):
        """!Overridden method necessary to communicate with tree model.

        @param index index as explained in VirtualTree doc
        @param column column index if applicable
        """
        node = self._model.GetNodeByIndex(index)
        # remove & because of & needed in menu (&Files)
        label = node.label.replace('&', '')
        return label

    def OnGetChildrenCount(self, index):
        """!Overridden method necessary to communicate with tree model."""
        return len(self._model.GetChildrenByIndex(index))

    def GetSelected(self):
        """!Get currently selected items.

        @return list of nodes representing selected items (can be empty)
        """
        selected = []
        for sel in self.GetSelections():
            index = self.GetIndexOfItem(sel)
            selected.append(self._model.GetNodeByIndex(index))
        return selected

    def Select(self, node, select=True):
        """!Select items.

        @param node node representing item
        @param select True/False to select/deselect
        """
        index = self._model.GetIndexOfNode(node)
        for i in range(len(index))[1:]:
            item = self.GetItemByIndex(index[:i])
            self.Expand(item)

        item = self.GetItemByIndex(index)
        self.SelectItem(item, select)

    def _emitSignal(self, item, signal):
        """!Helper method for emitting signals.

        @param item tree item
        @param signal signal to be emitted
        """
        if not item or not item.IsOk():
            return
        index = self.GetIndexOfItem(item)
        node = self._model.GetNodeByIndex(index)
        signal.emit(node = node)


class TreeView(AbstractTreeViewMixin, wx.TreeCtrl):
    """!Tree view class inheriting from wx.TreeCtrl"""
    def __init__(self, model, parent, *args, **kw):
        super(TreeView, self).__init__(parent=parent, model=model, *args, **kw)

class CTreeView(AbstractTreeViewMixin, CT.CustomTreeCtrl):
    """!Tree view class inheriting from wx.TreeCtrl"""
    def __init__(self, model, parent, **kw):
        if 'agwStyle' not in kw:
            kw['agwStyle'] = CT.TR_HIDE_ROOT | CT.TR_FULL_ROW_HIGHLIGHT |\
                             CT.TR_HAS_BUTTONS | CT.TR_LINES_AT_ROOT | CT.TR_SINGLE
        super(CTreeView, self).__init__(parent=parent, model=model, **kw)
        
class TreeListView(AbstractTreeViewMixin, ExpansionState, gizmos.TreeListCtrl):
    def __init__(self, model, parent, columns, **kw):
        self._columns = columns
        super(TreeListView, self).__init__(parent=parent, model=model, **kw)
        for column in columns:
            self.AddColumn(column)
        self.SetMainColumn(0)
        # refresh again
        self.RefreshItems()

    def OnGetItemText(self, index, column=0):
        """!Overridden method necessary to communicate with tree model.

        @param index index as explained in VirtualTree doc
        @param column column index if applicable
        """
        node = self._model.GetNodeByIndex(index)
        # remove & because of & needed in menu (&Files)
        if column > 0:
            return node.data.get(self._columns[column], '')
        else:
            label = node.label.replace('&', '')
            return label


class TreeFrame(wx.Frame):
    """!Frame for testing purposes only."""
    def __init__(self, model=None):
        wx.Frame.__init__(self, None, title='Test tree')

        panel = wx.Panel(self)
#        self.tree = TreeListView(model=model, parent=panel, columns=['col1', 'xxx'])
#        self.tree = TreeView(model=model, parent=panel)
        self.tree = CTreeView(model=model, parent=panel)
        self.tree.selectionChanged.connect(self.OnSelChanged)
        self.tree.itemActivated.connect(self.OnItemActivated)
        self.tree.SetMinSize((150, 300))

        szr = wx.BoxSizer(wx.VERTICAL)
        szr.Add(self.tree, 1, wx.ALIGN_CENTER)
        panel.SetSizerAndFit(szr)
        szr.SetSizeHints(self)

    def OnSelChanged(self):
        print 'selected items: ' + \
              str([node.label for node in self.tree.GetSelected()])
        
    def OnItemActivated(self, node):
        print 'activated: ' + node.label


def main():
    tree = TreeModel(DictNode)
    root = tree.root
    n1 = tree.AppendNode(parent=root, label='node1')
    n2 = tree.AppendNode(parent=root, label='node2')
    n3 = tree.AppendNode(parent=root, label='node3') # pylint: disable=W0612
    n11 = tree.AppendNode(parent=n1, label='node11', data={'xxx': 'A'})
    n12 = tree.AppendNode(parent=n1, label='node12', data={'xxx': 'B'}) # pylint: disable=W0612
    n21 = tree.AppendNode(parent=n2, label='node21', data={'xxx': 'A'}) # pylint: disable=W0612
    n111 = tree.AppendNode(parent=n11, label='node111', data={'xxx': 'A'}) # pylint: disable=W0612


    app = wx.PySimpleApp()
    frame = TreeFrame(model=tree)
#    frame.tree.Select(n111)
    frame.Show()
    app.MainLoop()


if __name__ == '__main__':
    main()
