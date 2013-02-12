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

import wx
import wx.gizmos as gizmos

class QueryDialog(wx.Dialog):
    def __init__(self, parent, data = None):
        wx.Dialog.__init__(self, parent, id = wx.ID_ANY,
                           title = _("Query results"),
                           size = (420, 400),
                           style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER)
        self.data = data

        self.panel = wx.Panel(self, id = wx.ID_ANY)
        self.mainSizer = wx.BoxSizer(wx.VERTICAL)

        self.tree = gizmos.TreeListCtrl(self.panel, id = wx.ID_ANY,
                                        style = wx.TR_DEFAULT_STYLE |
                                        wx.TR_HIDE_ROOT)
        
        self.tree.AddColumn("Feature")
        self.tree.AddColumn("Value")
        self.tree.SetMainColumn(0)
        self.tree.SetColumnWidth(0, 220)
        self.tree.SetColumnWidth(1, 400)

        self.mainSizer.Add(item = self.tree, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        if self.data:
            self._load()

        close = wx.Button(self.panel, id = wx.ID_CLOSE)
        close.Bind(wx.EVT_BUTTON, lambda event: self.Close())
        copy = wx.Button(self.panel, id = wx.ID_ANY, label = _("Copy to clipboard"))
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

    def _load(self):
        self.tree.DeleteAllItems()
        self.root = self.tree.AddRoot("The Root Item")
        for part in self.data:
            self._addItem(self.root, part)

        self.tree.UnselectAll()
        self.tree.ExpandAll(self.root)

    def _print(self):
        string = []
        for part in self.data:
            self._printItem(string, '', part)
            string.append('')
        return '\n'.join(string)

    def _addItem(self, parent, data):
        for k, v in data.iteritems():
            if isinstance(v, dict):
                item = self.tree.AppendItem(parent, text = k)
                self.tree.SetItemText(item, '', 1)
                self._addItem(item, v)
            else:
                item = self.tree.AppendItem(parent, text = k)
                self.tree.SetItemText(item, str(v), 1)

    def _printItem(self, string, indent, data):
        for k, v in data.iteritems():
            if isinstance(v, dict):
                string.append(indent + k)
                self._printItem(string, indent + '    ', v)
            else:
                string.append(indent + k + ': ' + str(v))

    def SetData(self, data):
        self.data = data
        self._load()

    def Copy(self, event):
        text = self._print()
        if wx.TheClipboard.Open():
            do = wx.TextDataObject()
            do.SetText(text)
            wx.TheClipboard.SetData(do)
            wx.TheClipboard.Close()

    def OnClose(self, event):
        self.Destroy()
        event.Skip()


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
    import pprint
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
