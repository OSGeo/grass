"""!
@package gui_core.query

@brief wxGUI query dialog

Classes:
 - query::QueryDialog

(C) 2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""
import os
import sys
import wx

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

from gui_core.treeview import TreeListView
from core.treemodel import TreeModel, DictNode

class QueryDialog(wx.Dialog):
    def __init__(self, parent, data = None):
        wx.Dialog.__init__(self, parent, id = wx.ID_ANY,
                           title = _("Query results"),
                           size = (420, 400),
                           style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER)
        self.data = data

        self.panel = wx.Panel(self, id = wx.ID_ANY)
        self.mainSizer = wx.BoxSizer(wx.VERTICAL)

        helpText = wx.StaticText(self.panel, wx.ID_ANY,
                                 label=_("Right click to copy selected values to clipboard."))
        helpText.SetForegroundColour(wx.SystemSettings_GetColour(wx.SYS_COLOUR_GRAYTEXT))
        self.mainSizer.Add(item=helpText, proportion=0, flag=wx.ALL, border=5)

        self._colNames = [_("Feature"), _("Value")]
        self._model = QueryTreeBuilder(self.data, column=self._colNames[1])
        self.tree = TreeListView(model=self._model, parent=self.panel,
                                 columns=self._colNames,
                                 style=wx.TR_DEFAULT_STYLE | 
                                 wx.TR_FULL_ROW_HIGHLIGHT | wx.TR_MULTIPLE)

        self.tree.SetColumnWidth(0, 220)
        self.tree.SetColumnWidth(1, 400)
        self.tree.ExpandAll(self._model.root)
        self.tree.contextMenu.connect(self.ShowContextMenu)
        self.mainSizer.Add(item = self.tree, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)

        
        close = wx.Button(self.panel, id = wx.ID_CLOSE)
        close.Bind(wx.EVT_BUTTON, lambda event: self.Close())
        copy = wx.Button(self.panel, id = wx.ID_ANY, label = _("Copy all to clipboard"))
        copy.Bind(wx.EVT_BUTTON, self.Copy)
        self.Bind(wx.EVT_CLOSE, self.OnClose)

        hbox = wx.BoxSizer(wx.HORIZONTAL)
        hbox.AddStretchSpacer(1)
        hbox.Add(item = copy, proportion = 0, flag = wx.EXPAND | wx.RIGHT, border = 5)
        hbox.Add(item = close, proportion = 0, flag = wx.EXPAND | wx.ALL, border = 0)

        self.mainSizer.Add(item = hbox, proportion = 0, flag = wx.EXPAND | wx.ALL, border = 5)
        self.panel.SetSizer(self.mainSizer)
        self.mainSizer.Fit(self.panel)
        # for Windows
        self.SendSizeEvent()

    def SetData(self, data):
        state = self.tree.GetExpansionState()
        self.data = data
        self._model = QueryTreeBuilder(self.data, column=self._colNames[1])
        self.tree.SetModel(self._model)
        self.tree.SetExpansionState(state)

    def Copy(self, event):
        text = printResults(self._model, self._colNames[1])
        self._copyText(text)

    def ShowContextMenu(self, node):
        """!Show context menu.
        
        Menu for copying distinguishes single and multiple selection.
        """
        nodes = self.tree.GetSelected()
        if not nodes:
            return
            
        menu = wx.Menu()
        texts = []
        if len(nodes) > 1:
            values = []
            for node in nodes:
                values.append((node.label, node.data[self._colNames[1]] if node.data else ''))
            col1 = '\n'.join([val[1] for val in values if val[1]])
            col2 = '\n'.join([val[0] for val in values if val[0]])
            table = '\n'.join([val[0] + ': ' + val[1] for val in values])
            texts.append((_("Copy from '%s' column") % self._colNames[1], col1))
            texts.append((_("Copy from '%s' column") % self._colNames[0], col2))
            texts.append((_("Copy selected lines"), table))
        else:
            label1 = nodes[0].label
            texts.append((_("Copy '%s'" % self._cutLabel(label1)), label1))
            if nodes[0].data and nodes[0].data[self._colNames[1]]:
                label2 = nodes[0].data[self._colNames[1]]
                texts.insert(0, (_("Copy '%s'" % self._cutLabel(label2)), label2))
                texts.append((_("Copy line"), label1 + ': ' + label2))

        ids = []
        for text in texts:
            id = wx.NewId()
            ids.append(id)
            self.Bind(wx.EVT_MENU, lambda evt, t=text[1], id=id: self._copyText(t), id=id)
 
            menu.Append(id, text[0])
 
        # show the popup menu
        self.PopupMenu(menu)
        menu.Destroy()
        for id in ids:
            self.Unbind(wx.EVT_MENU, id=id)

    def _cutLabel(self, label):
        limit = 15
        if len(label) > limit:
            return label[:limit] + '...'
            
        return label

    def _copyText(self, text):
        """!Helper function for copying"""
        if wx.TheClipboard.Open():
            do = wx.TextDataObject()
            do.SetText(text)
            wx.TheClipboard.SetData(do)
            wx.TheClipboard.Close()

    def OnClose(self, event):
        self.Destroy()
        event.Skip()


def QueryTreeBuilder(data, column):
    """!Builds tree model from query results.
    
    @param data query results as a dictionary
    @param column column name
    
    @returns tree model
    """
    def addNode(parent, data, model):
        for k, v in data.iteritems():
            if isinstance(v, dict):
                node = model.AppendNode(parent=parent, label=k)
                addNode(parent=node, data=v, model=model)
            else:
                node = model.AppendNode(parent=parent, label=k,
                                        data={column: str(v)})

    model = TreeModel(DictNode)
    for part in data:
        addNode(parent=model.root, data=part, model=model)

    return model


def printResults(model, valueCol):
    """!Print all results to string.
    
    @param model results tree model
    @param valueCol column name with value to be printed
    """
    def printTree(node, textList, valueCol, indent=0):
        textList.append(indent*' ' + node.label + ': ' + node.data.get(valueCol, ''))
        for child in node.children:
            printTree(node=child, textList=textList, valueCol=valueCol, indent=indent + 2)
    
    textList=[]
    for child in model.root.children:
        printTree(node=child, textList=textList, valueCol=valueCol)
    return '\n'.join(textList)


def PrepareQueryResults(coordinates, result):
    """!Prepare query results as a Query dialog input.

    Adds coordinates, improves vector results tree structure.
    """
    data = []
    data.append({_("east"): coordinates[0]})
    data.append({_("north"): coordinates[1]})
    for part in result:
        if 'Map' in part:
            itemText = part['Map']
            if 'Mapset' in part:
                itemText += '@' + part['Mapset']
                del part['Mapset']
            del part['Map']
            if part:
                data.append({itemText: part})
            else:
                data.append({itemText: _("Nothing found")})
        else:
            data.append(part)
    return data


def test():
    app = wx.PySimpleApp()
    from grass.script import vector as gvect
    from grass.script import raster as grast
    testdata1 = grast.raster_what(map = ('elevation_shade@PERMANENT','landclass96'),
                                  coord = [(638509.051416,224742.348346)])

    testdata2 = gvect.vector_what(map=('firestations','bridges'),
                                  coord=(633177.897487,221352.921257), distance=10)
    
    testdata = testdata1 + testdata2
    data = PrepareQueryResults(coordinates = (638509.051416,224742.348346), result = testdata)
    frame = QueryDialog(parent = None, data = data)
    frame.ShowModal()
    frame.Destroy()
    app.MainLoop()

if __name__ == "__main__":
    test()
