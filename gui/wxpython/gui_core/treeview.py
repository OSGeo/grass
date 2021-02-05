"""
@package gui_core.treeview

@brief tree view for dislaying tree model (used for search tree)

Classes:
 - treeview::TreeView

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

from __future__ import print_function

import wx
from wx.lib.mixins.treemixin import VirtualTree, ExpansionState
from core.globalvar import hasAgw, wxPythonPhoenix
try:
    import wx.lib.agw.customtreectrl as CT
except ImportError:
    import wx.lib.customtreectrl as CT
if wxPythonPhoenix:
    try:
        from agw.hypertreelist import HyperTreeList as TreeListCtrl
    except ImportError: # if it's not there locally, try the wxPython lib.
        from wx.lib.agw.hypertreelist import HyperTreeList as TreeListCtrl
else:
    from wx.gizmos import TreeListCtrl

# needed just for testing
if __name__ == '__main__':
    from grass.script.setup import set_gui_path
    set_gui_path()

from core.treemodel import TreeModel, DictNode
from gui_core.wrap import CustomTreeCtrl

from grass.pydispatch.signal import Signal


class AbstractTreeViewMixin(VirtualTree):
    """Abstract tree view class for displaying tree model.

    Concrete implementation must inherit both this mixin class and a wx tree widget.
    More functionality and signals can be added if needed.

    Signals:
        selectionChanged - attribute 'node'
        itemActivated - attribute 'node'
        contextMenu - attribute 'node'
    """

    def __init__(self, model, parent, *args, **kw):
        self._model = model
        super(AbstractTreeViewMixin, self).__init__(parent=parent, *args, **kw)

        self.selectionChanged = Signal('TreeView.selectionChanged')
        self.itemActivated = Signal('TreeView.itemActivated')
        self.contextMenu = Signal('TreeView.contextMenu')

        self.Bind(wx.EVT_TREE_SEL_CHANGED, lambda evt:
                  self._emitSignal(evt.GetItem(), self.selectionChanged))
        self.Bind(wx.EVT_TREE_ITEM_ACTIVATED, lambda evt:
                  self._emitSignal(evt.GetItem(), self.itemActivated))
        self.Bind(wx.EVT_TREE_ITEM_MENU, lambda evt:
                  self._emitSignal(evt.GetItem(), self.contextMenu))

    def SetModel(self, model):
        """Set tree model and refresh.

        :param model: tree model
        """
        self._model = model
        self.RefreshItems()

    def OnGetItemText(self, index, column=0):
        """Overridden method necessary to communicate with tree model.

        :param index: index as explained in VirtualTree doc
        :param column: column index if applicable
        """
        node = self._model.GetNodeByIndex(index)
        # remove & because of & needed in menu (&Files)
        label = node.label.replace('&', '')
        return label

    def OnGetChildrenCount(self, index):
        """Overridden method necessary to communicate with tree model."""
        return len(self._model.GetChildrenByIndex(index))

    def GetSelected(self):
        """Get currently selected items.

        :return: list of nodes representing selected items (can be empty)
        """
        selected = []
        for sel in self.GetSelections():
            index = self.GetIndexOfItem(sel)
            selected.append(self._model.GetNodeByIndex(index))
        return selected

    def Select(self, node, select=True):
        """Select items.

        :param node: node representing item
        :param select: True/False to select/deselect
        """
        index = self._model.GetIndexOfNode(node)
        for i in range(len(index))[1:]:
            item = self.GetItemByIndex(index[:i])
            self.Expand(item)
            # needed for wxPython 3:
            self.EnsureVisible(item)

        item = self.GetItemByIndex(index)
        self.SelectItem(item, select)

    def ExpandNode(self, node, recursive=True):
        """Expand items.

        :param node: node representing item
        :param recursive: True/False to expand all children
        """
        index = self._model.GetIndexOfNode(node)
        item = self.GetItemByIndex(index)
        if recursive:
            self.ExpandAllChildren(item)
        else:
            self.Expand(item)
        self.EnsureVisible(item)

    def ExpandAll(self):
        """Expand all items.
        """
        def _expand(item, root=False):
            if not root:
                self.Expand(item)
            child, cookie = self.GetFirstChild(item)
            while child:
                _expand(child)
                child, cookie = self.GetNextChild(item, cookie)

        item = self.GetRootItem()
        _expand(item, True)

    def IsNodeExpanded(self, node):
        """Check if node is expanded"""
        index = self._model.GetIndexOfNode(node)
        item = self.GetItemByIndex(index)

        return self.IsExpanded(item)

    def CollapseNode(self, node, recursive=True):
        """Collapse items.

        :param node: node representing item
        :param recursive: True/False to collapse all children
        """
        index = self._model.GetIndexOfNode(node)
        item = self.GetItemByIndex(index)
        if recursive:
            self.CollapseAllChildren(item)
        else:
            self.Collapse(item)

    def RefreshNode(self, node, recursive=False):
        """Refreshes node."""
        index = self._model.GetIndexOfNode(node)
        if recursive:
            try:
                item = self.GetItemByIndex(index)
            except IndexError:
                return
            self.RefreshItemRecursively(item, index)
        else:
            self.RefreshItem(index)

    def _emitSignal(self, item, signal, **kwargs):
        """Helper method for emitting signals.

        :param item: tree item
        :param signal: signal to be emitted
        """
        if not item or not item.IsOk():
            return
        index = self.GetIndexOfItem(item)
        node = self._model.GetNodeByIndex(index)
        signal.emit(node=node, **kwargs)


class TreeView(AbstractTreeViewMixin, wx.TreeCtrl):
    """Tree view class inheriting from wx.TreeCtrl"""

    def __init__(self, model, parent, *args, **kw):
        super(TreeView, self).__init__(parent=parent, model=model, *args, **kw)
        self.RefreshItems()


class CTreeView(AbstractTreeViewMixin, CustomTreeCtrl):
    """Tree view class inheriting from wx.TreeCtrl"""

    def __init__(self, model, parent, **kw):
        if hasAgw:
            style = 'agwStyle'
        else:
            style = 'style'

        if style not in kw:
            kw[style] = CT.TR_HIDE_ROOT | CT.TR_FULL_ROW_HIGHLIGHT |\
                CT.TR_HAS_BUTTONS | CT.TR_LINES_AT_ROOT | CT.TR_SINGLE
        super(CTreeView, self).__init__(parent=parent, model=model, **kw)
        self.SetBackgroundColour(
            wx.SystemSettings().GetColour(wx.SYS_COLOUR_WINDOW))
        self.RefreshItems()


class TreeListView(AbstractTreeViewMixin, ExpansionState, TreeListCtrl):

    def __init__(self, model, parent, columns, **kw):
        self._columns = columns
        if wxPythonPhoenix and 'style' in kw:
            flags = kw['style']
            kw['agwStyle'] = flags
            del kw['style']
        super(TreeListView, self).__init__(parent=parent, model=model, **kw)
        for column in columns:
            self.AddColumn(column)
        self.SetMainColumn(0)
        self.RefreshItems()
        # to solve events inconsitency
        self.Bind(wx.EVT_TREE_ITEM_RIGHT_CLICK, lambda evt:
                  self._emitSignal(evt.GetItem(), self.contextMenu))
        self.Bind(wx.EVT_TREE_ITEM_RIGHT_CLICK, self.OnRightClick)

    def OnGetItemText(self, index, column=0):
        """Overridden method necessary to communicate with tree model.

        :param index: index as explained in VirtualTree doc
        :param column: column index if applicable
        """
        node = self._model.GetNodeByIndex(index)
        # remove & because of & needed in menu (&Files)
        if column > 0:
            return node.data.get(self._columns[column], '')
        else:
            label = node.label.replace('&', '')
            return label

    def OnRightClick(self, event):
        """Select item on right click.

        With multiple selection we don't want to deselect all items
        """
        item = event.GetItem()
        if not self.IsSelected(item):
            self.SelectItem(item)
        event.Skip()


class TreeFrame(wx.Frame):
    """Frame for testing purposes only."""

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
        print('selected items: ' +
              str([node.label for node in self.tree.GetSelected()]))

    def OnItemActivated(self, node):
        print('activated: ' + node.label)


def main():
    tree = TreeModel(DictNode)
    root = tree.root
    n1 = tree.AppendNode(parent=root, data={"label": "node1"})
    n2 = tree.AppendNode(parent=root, data={"label": "node2"})
    n3 = tree.AppendNode(parent=root, data={"label": "node3"})  # pylint: disable=W0612
    n11 = tree.AppendNode(parent=n1, data={"label": "node11", "xxx": "A"})
    n12 = tree.AppendNode(
        parent=n1, data={"label": "node12", "xxx": "B"})  # pylint: disable=W0612
    n21 = tree.AppendNode(
        parent=n2, data={"label": "node21", "xxx": "A"})  # pylint: disable=W0612
    n111 = tree.AppendNode(
        parent=n11, data={"label": "node111", "xxx": "A"})  # pylint: disable=W0612

    app = wx.App()
    frame = TreeFrame(model=tree)
#    frame.tree.Select(n111)
    frame.Show()
    app.MainLoop()


if __name__ == '__main__':
    main()
