"""!
@package core.treemodel

@brief tree structure model (used for menu, search tree)

Classes:
 - treemodel::TreeModel
 - treemodel::DictNode
 - treemodel::ModuleNode

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""


class TreeModel(object):
    """!Class represents a tree structure with hidden root.
    
    TreeModel is used together with TreeView class to display results in GUI.
    The functionality is not complete, only needed methods are implemented.
    If needed, the functionality can be extended.
    
    >>> tree = TreeModel(DictNode)
    >>> root = tree.root
    >>> n1 = tree.AppendNode(parent=root, label='node1')
    >>> n2 = tree.AppendNode(parent=root, label='node2')
    >>> n11 = tree.AppendNode(parent=n1, label='node11', data={'xxx': 1})
    >>> n111 = tree.AppendNode(parent=n11, label='node111', data={'xxx': 4})
    >>> n12 = tree.AppendNode(parent=n1, label='node12', data={'xxx': 2})
    >>> n21 = tree.AppendNode(parent=n2, label='node21', data={'xxx': 1})
    >>> [node.label for node in tree.SearchNodes(key='xxx', value=1)]
    ['node11', 'node21']
    >>> [node.label for node in tree.SearchNodes(key='xxx', value=5)]
    []
    >>> tree.GetIndexOfNode(n111)
    [0, 0, 0]
    >>> tree.GetNodeByIndex((0,1)).label
    'node12'
    >>> print tree
    node1
      node11
        * xxx : 1
        node111
          * xxx : 4
      node12
        * xxx : 2
    node2
      node21
        * xxx : 1
    """
    def __init__(self, nodeClass):
        """!Constructor creates root node.

        @param nodeClass class which is used for creating nodes
        """
        self._root = nodeClass('root')
        self.nodeClass = nodeClass

    @property
    def root(self):
        return self._root

    def AppendNode(self, parent, label, data=None):
        """!Create node and append it to parent node.
        
        @param parent parent node of the new node
        @param label node label
        @param data optional node data
        
        @return new node
        """
        node = self.nodeClass(label=label, data=data)
        parent.children.append(node)
        node.parent = parent
        return node

    def SearchNodes(self, **kwargs):
        """!Search nodes according to specified attributes."""
        nodes = []
        self._searchNodes(node=self.root, foundNodes=nodes, **kwargs)
        return nodes
        
    def _searchNodes(self, node, foundNodes, **kwargs):
        """!Helper method for searching nodes."""
        if node.match(**kwargs):
            foundNodes.append(node)
        for child in node.children:
            self._searchNodes(node=child, foundNodes=foundNodes, **kwargs)

    def GetNodeByIndex(self, index):
        """!Method used for communication between view (VirtualTree) and model.

        @param index index of node, as defined in VirtualTree doc
        (e.g. root ~ [], second node of a first node ~ [0, 1])
        """
        if len(index) == 0:
            return self.root
        return self._getNode(self.root, index)
        
    def GetIndexOfNode(self, node):
        """!Method used for communication between view (VirtualTree) and model."""
        index = []
        return self._getIndex(node, index)
        
        
    def _getIndex(self, node, index):
        if node.parent:
            index.insert(0, node.parent.children.index(node))
            return self._getIndex(node.parent, index)
        return index
        
        
    def GetChildrenByIndex(self, index):
        """!Method used for communication between view (VirtualTree) and model."""
        if len(index) == 0:
            return self.root.children
        node = self._getNode(self.root, index)
        return node.children
        
    def _getNode(self, node, index):
        if len(index) == 1:
            return node.children[index[0]]
        else:
            return self._getNode(node.children[index[0]], index[1:])

    def RemoveNode(self, node):
        """!Removes node."""
        if node.parent:
            node.parent.children.remove(node)

    def __str__(self):
        """!Print tree."""
        text = []
        for child in self.root.children:
            child.nprint(text)
        return "\n".join(text)


class DictNode(object):
    """!Node which has data in a form of dictionary."""
    def __init__(self, label, data=None):
        """!Create node.

        @param label node label (displayed in GUI)
        @param data data as dictionary or None
        """

        self.label = label
        if data == None:
            self.data = dict()
        else:
            self.data = data
        self._children = []
        self.parent = None

    @property
    def children(self):
        return self._children

    def nprint(self, text, indent=0):
        text.append(indent * ' ' + self.label)
        if self.data:
            for key, value in self.data.iteritems():
                text.append("%(indent)s* %(key)s : %(value)s" % {'indent': (indent + 2) * ' ',
                                                                 'key': key,
                                                                 'value': value})

        if self.children:
            for child in self.children:
                child.nprint(text, indent + 2)

    def match(self, key, value):
        """!Method used for searching according to given parameters.

        @param value dictionary value to be matched
        @param key data dictionary key
        """
        if key in self.data and self.data[key] == value:
            return True
        return False


class ModuleNode(DictNode):
    """!Node representing module."""
    def __init__(self, label, data=None):
        super(ModuleNode, self).__init__(label=label, data=data)

    def match(self, key, value):
        """!Method used for searching according to command,
        keywords or description."""
        if not self.data:
            return False
        if key in ('command', 'keywords', 'description'):
            return len(self.data[key]) and value in self.data[key]
        
        return False
            
        
def main():
    import doctest
    doctest.testmod()


if __name__ == '__main__':
    main()
