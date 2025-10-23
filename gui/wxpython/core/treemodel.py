"""
@package core.treemodel

@brief tree structure model (used for menu, search tree)

Classes:
 - treemodel::TreeModel
 - treemodel::DictNode
 - treemodel::ModuleNode

(C) 2013-2020 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import copy

from grass.script.utils import naturally_sort


class TreeModel:
    """Class represents a tree structure with hidden root.

    TreeModel is used together with TreeView class to display results in GUI.
    The functionality is not complete, only needed methods are implemented.
    If needed, the functionality can be extended.

    >>> tree = TreeModel(DictNode)
    >>> root = tree.root
    >>> n1 = tree.AppendNode(parent=root, data={"label": "node1"})
    >>> n2 = tree.AppendNode(parent=root, data={"label": "node2"})
    >>> n11 = tree.AppendNode(parent=n1, data={"label": "node11", "xxx": 1})
    >>> n111 = tree.AppendNode(parent=n11, data={"label": "node111", "xxx": 4})
    >>> n12 = tree.AppendNode(parent=n1, data={"label": "node12", "xxx": 2})
    >>> n21 = tree.AppendNode(parent=n2, data={"label": "node21", "xxx": 1})
    >>> [node.label for node in tree.SearchNodes(key="xxx", value=1)]
    ['node11', 'node21']
    >>> [node.label for node in tree.SearchNodes(key="xxx", value=5)]
    []
    >>> tree.GetIndexOfNode(n111)
    [0, 0, 0]
    >>> tree.GetNodeByIndex((0, 1)).label
    'node12'
    >>> print(tree)
    node1
      * label : node1
      node11
        * label : node11
        * xxx : 1
        node111
          * label : node111
          * xxx : 4
      node12
        * label : node12
        * xxx : 2
    node2
      * label : node2
      node21
        * label : node21
        * xxx : 1

    """

    def __init__(self, nodeClass):
        """Constructor creates root node.

        :param nodeClass: class which is used for creating nodes
        """
        self._root = nodeClass()
        self.nodeClass = nodeClass

    @property
    def root(self):
        return self._root

    def AppendNode(self, parent, **kwargs):
        """Create node and append it to parent node.

        :param parent: parent node of the new node

        :return: new node
        """
        node = self.nodeClass(**kwargs)
        # useful for debugging deleting nodes
        # weakref.finalize(node, print, "Deleted node {}".format(label))
        parent.children.append(node)
        # weakref doesn't work out of the box when deepcopying this class
        # node.parent = weakref.proxy(parent)
        node.parent = parent
        return node

    def SearchNodes(self, parent=None, **kwargs):
        """Search nodes according to specified attributes."""
        nodes = []
        parent = parent or self.root
        self._searchNodes(node=parent, foundNodes=nodes, **kwargs)
        return nodes

    def _searchNodes(self, node, foundNodes, **kwargs):
        """Helper method for searching nodes."""
        if node.match(**kwargs):
            foundNodes.append(node)
        for child in node.children:
            self._searchNodes(node=child, foundNodes=foundNodes, **kwargs)

    def GetNodeByIndex(self, index):
        """Method used for communication between view (VirtualTree) and model.

        :param index: index of node, as defined in VirtualTree doc
                      (e.g. root ~ [], second node of a first node ~ [0, 1])
        """
        if len(index) == 0:
            return self.root
        return self._getNode(self.root, index)

    def GetIndexOfNode(self, node):
        """Method used for communication between view (VirtualTree) and model."""
        index = []
        return tuple(self._getIndex(node, index))

    def _getIndex(self, node, index):
        if node.parent:
            index.insert(0, node.parent.children.index(node))
            return self._getIndex(node.parent, index)
        return index

    def GetChildrenByIndex(self, index):
        """Method used for communication between view (VirtualTree) and model."""
        if len(index) == 0:
            return self.root.children
        node = self._getNode(self.root, index)
        return node.children

    def _getNode(self, node, index):
        if len(index) == 1:
            return node.children[index[0]]
        return self._getNode(node.children[index[0]], index[1:])

    def RemoveNode(self, node):
        """Removes node. If node is root, removes root's children, root is kept."""
        if node.parent:
            node.parent.children.remove(node)
        else:
            # node is root
            del node.children[:]

    def SortChildren(self, node):
        """Sorts children with 'natural sort' based on label."""
        if node.children:
            naturally_sort(node.children, key=lambda node: node.label)

    def Filtered(self, **kwargs):
        """Filters model based on parameters in kwargs
        that are passed to node's match function.
        Copies tree and returns a filtered copy."""

        def _filter(node):
            if node.children:
                to_remove = []
                for child in node.children:
                    match = _filter(child)
                    if not match:
                        to_remove.append(child)
                for child in reversed(to_remove):
                    fmodel.RemoveNode(child)
                if node.children:
                    return True
            return node.match(**kwargs)

        fmodel = copy.deepcopy(self)
        _filter(fmodel.root)

        return fmodel

    def GetLeafCount(self, node):
        """Returns the number of leaves in a node."""
        if node.children:
            count = 0
            for child in node.children:
                count += self.GetLeafCount(child)
            return count
        return 1

    def __str__(self):
        """Print tree."""
        text = []
        for child in self.root.children:
            child.nprint(text)
        return "\n".join(text)


class DictNode:
    """Node which has data in a form of dictionary."""

    def __init__(self, data=None):
        """Create node.

        :param data: data as dictionary or None
        """
        if not data:
            self.data = {"label": ""}
        else:
            self.data = data
        self._children = []
        self.parent = None

    @property
    def label(self):
        return self.data["label"]

    @property
    def children(self):
        return self._children

    def nprint(self, text, indent=0):
        text.append(indent * " " + self.label)
        if self.data:
            for key, value in self.data.items():
                text.append(
                    "%(indent)s* %(key)s : %(value)s"
                    % {"indent": (indent + 2) * " ", "key": key, "value": value}
                )

        if self.children:
            for child in self.children:
                child.nprint(text, indent + 2)

    def match(self, key, value) -> bool:
        """Method used for searching according to given parameters.

        :param value: dictionary value to be matched
        :param key: data dictionary key
        """
        return bool(key in self.data and self.data[key] == value)


class DictFilterNode(DictNode):
    """Node which has data in a form of dictionary and can be filtered."""

    def __init__(self, data=None):
        super().__init__(data=data)

    def match(self, method="exact", **kwargs):
        """Method used for searching according to given parameters.

        :param str method: 'exact' for exact match or
                           'filtering' for filtering by type/name
        :param kwargs key-value to be matched, filtering method uses 'type' and 'name'
        :return bool: True if an entry matching given parameters was found
        """
        if not kwargs:
            return False

        if method == "exact":
            return self._match_exact(**kwargs)
        if method == "filtering":
            return self._match_filtering(**kwargs)

    def _match_exact(self, **kwargs):
        """Match method for exact matching."""
        for key, value in kwargs.items():
            if not (key in self.data and self.data[key] == value):
                return False
        return True

    def _match_filtering(self, **kwargs) -> bool:
        """Match method for filtering."""
        if (
            "type" in kwargs
            and "type" in self.data
            and kwargs["type"] != self.data["type"]
        ):
            return False
        return not (
            "name" in kwargs
            and "name" in self.data
            and not kwargs["name"].search(self.data["name"])
        )


class ModuleNode(DictNode):
    """Node representing module."""

    def __init__(self, label=None, data=None):
        super().__init__(data=data)
        self._label = label or ""
        if not data:
            self.data = {}

    @property
    def label(self):
        return self._label

    def match(self, key, value, case_sensitive=False) -> bool:
        """Method used for searching according to command,
        keywords or description."""
        if not self.data:
            return False
        keys = [key] if isinstance(key, str) else key

        for key in keys:
            if key not in {"command", "keywords", "description"}:
                return False
            try:
                text = self.data[key]
            except KeyError:
                continue
            if not text:
                continue
            if case_sensitive:
                # start supported but unused, so testing last
                if value in text or value == "*":
                    return True
            elif value.lower() in text.lower() or value == "*":
                # this works fully only for English and requires accents
                # to be exact match (even Python 3 casefold() does not help)
                return True
        return False


def main():
    import doctest

    doctest.testmod()


if __name__ == "__main__":
    main()
