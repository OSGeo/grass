"""
@package gui_core.wrap

@brief Core wrapped wxpython widgets 

Classes:
 - wrap::GSpinCtrl


(C) 2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Petrasova <kratochanna gmail.com>
"""

import wx
import wx.lib.buttons as buttons

from core.globalvar import gtk3, wxPythonPhoenix
if wxPythonPhoenix:
    import wx.adv


def BitmapFromImage(image, depth=-1):
    if wxPythonPhoenix:
        return wx.Bitmap(img=image, depth=depth)
    else:
        return wx.BitmapFromImage(image, depth=depth)


def EmptyBitmap(width, height, depth=-1):
    if wxPythonPhoenix:
        return wx.Bitmap(width=width, height=height, depth=depth)
    else:
        return wx.EmptyBitmap(width=width, height=height, depth=depth)


def EmptyImage(width, height, clear=True):
    if wxPythonPhoenix:
        return wx.Image(width=width, height=height, clear=clear)
    else:
        return wx.EmptyImage(width=width, height=height, clear=clear)


def StockCursor(cursorId):
    if wxPythonPhoenix:
        return wx.Cursor(cursorId=cursorId)
    else:
        return wx.StockCursor(cursorId)


class Window(wx.Window):
    """Wrapper around wx.Window to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.Window.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            if tip is None:
                wx.Window.UnsetToolTip(self)
            else:
                wx.Window.SetToolTip(self, tipString=tip)
        else:
            if tip is None:
                wx.Window.SetToolTip(self, tip)
            else:
                wx.Window.SetToolTipString(self, tip)


class SpinCtrl(wx.SpinCtrl):
    """Wrapper around wx.SpinCtrl to have more control
    over the widget on different platforms"""

    gtk3MinSize = 130

    def __init__(self, *args, **kwargs):
        if gtk3:
            if 'size' in kwargs:
                kwargs['size'] = wx.Size(max(self.gtk3MinSize, kwargs['size'][0]), kwargs['size'][1])
            else:
                kwargs['size'] = wx.Size(self.gtk3MinSize, -1)

        wx.SpinCtrl.__init__(self, *args, **kwargs)


class Button(wx.Button):
    """Wrapper around wx.Button to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.Button.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.Button.SetToolTip(self, tipString=tip)
        else:
            wx.Button.SetToolTipString(self, tip)


class GenBitmapButton(buttons.GenBitmapButton):
    """Wrapper around GenBitmapButton to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        buttons.GenBitmapButton.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            buttons.GenBitmapButton.SetToolTip(self, tipString=tip)
        else:
            buttons.GenBitmapButton.SetToolTipString(self, tip)


class ToggleButton(wx.ToggleButton):
    """Wrapper around wx.ToggleButton to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.ToggleButton.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.ToggleButton.SetToolTip(self, tipString=tip)
        else:
            wx.ToggleButton.SetToolTipString(self, tip)


class StaticText(wx.StaticText):
    """Wrapper around wx.StaticText to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.StaticText.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.StaticText.SetToolTip(self, tipString=tip)
        else:
            wx.StaticText.SetToolTipString(self, tip)


class StaticBox(wx.StaticBox):
    """Wrapper around wx.StaticBox to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.StaticBox.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.StaticBox.SetToolTip(self, tipString=tip)
        else:
            wx.StaticBox.SetToolTipString(self, tip)


class TextCtrl(wx.TextCtrl):
    """Wrapper around wx.TextCtrl to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.TextCtrl.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.TextCtrl.SetToolTip(self, tipString=tip)
        else:
            wx.TextCtrl.SetToolTipString(self, tip)


class SearchCtrl(wx.SearchCtrl):
    """Wrapper around wx.SearchCtrl to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.SearchCtrl.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.SearchCtrl.SetToolTip(self, tipString=tip)
        else:
            wx.SearchCtrl.SetToolTipString(self, tip)


class ListCtrl(wx.ListCtrl):
    """Wrapper around wx.ListCtrl to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.ListCtrl.__init__(self, *args, **kwargs)

    def InsertStringItem(self, index, label, imageIndex=-1):
        if wxPythonPhoenix:
            return wx.ListCtrl.InsertItem(self, index=index, label=label, imageIndex=imageIndex)
        else:
            return wx.ListCtrl.InsertStringItem(self, index=index, label=label, imageIndex=imageIndex)

    def SetStringItem(self, index, column, label, imageId=-1):
        if wxPythonPhoenix:
            return wx.ListCtrl.SetItem(self, index=index, column=column, label=label, imageId=imageId)
        else:
            return wx.ListCtrl.SetStringItem(self, index=index, col=column, label=label, imageId=imageId)


class TreeCtrl(wx.TreeCtrl):
    """Wrapper around wx.TreeCtrl to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.TreeCtrl.__init__(self, *args, **kwargs)

    def AppendItem(self, parent, text, image=-1, selImage=-1, data=None):
        if wxPythonPhoenix:
            return wx.TreeCtrl.AppendItem(self, parent, text, image, selImage, data)
        else:
            return wx.TreeCtrl.AppendItem(self, parent, text, image, selImage, wx.TreeItemData(data))

    def GetItemData(self, item):
        if wxPythonPhoenix:
            return wx.TreeCtrl.GetItemData(self, item)
        else:
            return wx.TreeCtrl.GetPyData(self, item)


class ToolBar(wx.ToolBar):
    """Wrapper around wx.ToolBar to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.ToolBar.__init__(self, *args, **kwargs)

    def AddLabelTool(self, toolId, label, bitmap, bmpDisabled=wx.NullBitmap, kind=0,
                     shortHelpString='', longHelpString='', clientData=None):
        if wxPythonPhoenix:
            return wx.ToolBar.AddTool(self, toolId=toolId, label=label, bitmap=bitmap, bmpDisabled=bmpDisabled,
                                      kind=kind, shortHelpString=shortHelpString, longHelpString=longHelpString,
                                      clientData=clientData)
        else:
            return wx.ToolBar.AddLabelTool(self, toolId, label, bitmap, bmpDisabled, kind,
                                           shortHelpString, longHelpString, clientData)

    def InsertLabelTool(self, pos, toolId, label, bitmap, bmpDisabled=wx.NullBitmap, kind=0,
                        shortHelpString='', longHelpString='', clientData=None):
        if wxPythonPhoenix:
            return wx.ToolBar.InsertTool(self, pos, toolId=toolId, label=label, bitmap=bitmap, bmpDisabled=bmpDisabled,
                                         kind=kind, shortHelp=shortHelpString, longHelp=longHelpString,
                                         clientData=clientData)
        else:
            return wx.ToolBar.InsertLabelTool(self, pos, toolId, label, bitmap, bmpDisabled, kind,
                                              shortHelpString, longHelpString, clientData)


class Menu(wx.Menu):
    """Wrapper around wx.Menu to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.Menu.__init__(self, *args, **kwargs)

    def AppendItem(self, menuItem):
        if wxPythonPhoenix:
            wx.Menu.Append(self, menuItem=menuItem)
        else:
            wx.Menu.AppendItem(self, menuItem)

    def AppendMenu(self, id, text, submenu, help=""):
        if wxPythonPhoenix:
            wx.Menu.Append(self, id=id, item=text, subMenu=submenu, helpString=help)
        else:
            wx.Menu.AppendMenu(self, id=id, text=text, submenu=submenu, help=help)


class DragImage(wx.GenericDragImage if wxPythonPhoenix else wx.DragImage):
    """Wrapper around wx.DragImage to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        super(DragImage, self).__init__(*args, **kwargs)


class PseudoDC(wx.adv.PseudoDC if wxPythonPhoenix else wx.PseudoDC):
    """Wrapper around wx.PseudoDC to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        super(PseudoDC, self).__init__(*args, **kwargs)

    def DrawRectangleRect(self, rect):
        if wxPythonPhoenix:
            super(PseudoDC, self).DrawRectangle(rect=rect)
        else:
            super(PseudoDC, self).DrawRectangleRect(rect)

    def BeginDrawing(self):
        if not wxPythonPhoenix:
            super(PseudoDC, self).BeginDrawing()

    def EndDrawing(self):
        if not wxPythonPhoenix:
            super(PseudoDC, self).EndDrawing()


class Rect(wx.Rect):
    """Wrapper around wx.Rect to have more control
    over the widget on different platforms/wxpython versions"""
    def __init__(self, *args, **kwargs):
        wx.Rect.__init__(self, *args, **kwargs)

    def ContainsXY(self, x, y):
        if wxPythonPhoenix:
            return wx.Rect.Contains(self, x=x, y=y)
        else:
            return wx.Rect.ContainsXY(self, x, y)

    def ContainsRect(self, rect):
        if wxPythonPhoenix:
            return wx.Rect.Contains(self, rect=rect)
        else:
            return wx.Rect.ContainsRect(self, rect)
