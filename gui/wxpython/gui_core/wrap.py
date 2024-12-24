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

from __future__ import annotations

import sys
import wx
import wx.lib.agw.floatspin as fs
import wx.lib.colourselect as csel
import wx.lib.filebrowsebutton as filebrowse
import wx.lib.scrolledpanel as scrolled
from wx.lib import expando
from wx.lib import buttons
from wx.lib.agw.aui import tabart

try:
    import wx.lib.agw.customtreectrl as CT
except ImportError:
    import wx.lib.customtreectrl as CT

from core.globalvar import CheckWxVersion, gtk3, wxPythonPhoenix

if wxPythonPhoenix:
    import wx.adv
    from wx.adv import OwnerDrawnComboBox as OwnerDrawnComboBox_
    from wx.adv import ODCB_PAINTING_CONTROL, ODCB_PAINTING_SELECTED
    from wx.adv import BitmapComboBox as BitmapComboBox_
    from wx.adv import HyperlinkCtrl as HyperlinkCtrl_
    from wx.adv import HL_ALIGN_LEFT, HL_CONTEXTMENU

    ComboPopup = wx.ComboPopup
    wxComboCtrl = wx.ComboCtrl
else:
    import wx.combo
    from wx.combo import OwnerDrawnComboBox as OwnerDrawnComboBox_
    from wx.combo import ODCB_PAINTING_CONTROL, ODCB_PAINTING_SELECTED
    from wx.combo import BitmapComboBox as BitmapComboBox_
    from wx import HyperlinkCtrl as HyperlinkCtrl_
    from wx import HL_ALIGN_LEFT, HL_CONTEXTMENU

    ComboPopup = wx.combo.ComboPopup
    wxComboCtrl = wx.combo.ComboCtrl

if wxPythonPhoenix and CheckWxVersion([4, 0, 3, 0]):
    from wx import NewIdRef as NewId
else:
    from wx import NewId  # noqa: F401


def convertToInt(argsOrKwargs, roundVal=False):
    """Convert args, kwargs float value to int

    :param tuple/list/dict argsOrKwargs: args or kwargs
    :param bool roundVal: True if you want round float value

    return list or dict
    """
    result = {} if isinstance(argsOrKwargs, dict) else []
    j = None
    for i in argsOrKwargs:
        if isinstance(result, dict):
            i, j = argsOrKwargs[i], i
        if isinstance(i, float):
            if roundVal:
                i = round(i)
            i = int(i)
        result.update({j: i}) if j else result.append(i)
    return result


def IsDark():
    """Detects if used theme is dark.
    Wraps wx method for different versions."""

    def luminance(c):
        return (0.299 * c.Red() + 0.587 * c.Green() + 0.114 * c.Blue()) / 255

    if hasattr(wx.SystemSettings, "GetAppearance"):
        return wx.SystemSettings.GetAppearance().IsDark()

    # for older wx
    bg = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW)
    fg = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOWTEXT)
    return luminance(fg) - luminance(bg) > 0.2


def BitmapFromImage(image: wx.Image, depth=-1) -> wx.Bitmap:
    if wxPythonPhoenix:
        return wx.Bitmap(img=image, depth=depth)
    return wx.BitmapFromImage(image, depth=depth)


def ImageFromBitmap(bitmap: wx.Bitmap) -> wx.Image:
    if wxPythonPhoenix:
        return bitmap.ConvertToImage()
    return wx.ImageFromBitmap(bitmap)


def EmptyBitmap(width, height, depth=-1) -> wx.Bitmap:
    if wxPythonPhoenix:
        return wx.Bitmap(width=width, height=height, depth=depth)
    return wx.EmptyBitmap(width=width, height=height, depth=depth)


def EmptyImage(width, height, clear=True) -> wx.Image:
    if wxPythonPhoenix:
        return wx.Image(width=width, height=height, clear=clear)
    return wx.EmptyImage(width=width, height=height, clear=clear)


def StockCursor(cursorId):
    if wxPythonPhoenix:
        return wx.Cursor(cursorId=cursorId)
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
        else:  # noqa: PLR5501
            if tip is None:
                wx.Window.SetToolTip(self, tip)
            else:
                wx.Window.SetToolTipString(self, tip)


class Panel(wx.Panel):
    """Wrapper around wx.Panel to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.Panel.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.Panel.SetToolTip(self, tipString=tip)
        else:
            wx.Panel.SetToolTipString(self, tip)


class Slider(wx.Slider):
    """Wrapper around wx.Slider to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        args = convertToInt(argsOrKwargs=args)
        kwargs = convertToInt(argsOrKwargs=kwargs)

        wx.Slider.__init__(self, *args, **kwargs)

    def SetRange(self, minValue, maxValue):
        wx.Slider.SetRange(self, int(minValue), int(maxValue))

    def SetValue(self, value):
        wx.Slider.SetValue(self, int(value))


class SpinCtrl(wx.SpinCtrl):
    """Wrapper around wx.SpinCtrl to have more control
    over the widget on different platforms"""

    gtk3MinSize = 118  # optimal for SpinCtrl default param  min=1, max=100

    def __init__(self, *args, **kwargs):
        args = convertToInt(argsOrKwargs=args)
        kwargs = convertToInt(argsOrKwargs=kwargs)
        if gtk3 and "size" in kwargs and kwargs["size"][0] < self.gtk3MinSize:
            del kwargs["size"]

        wx.SpinCtrl.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.SpinCtrl.SetToolTip(self, tipString=tip)
        else:
            wx.SpinCtrl.SetToolTipString(self, tip)

    def SetRange(self, minVal, maxVal):
        wx.SpinCtrl.SetRange(self, int(minVal), int(maxVal))

    def SetValue(self, value):
        wx.SpinCtrl.SetValue(self, int(value))


class FloatSpin(fs.FloatSpin):
    """Wrapper around fs.FloatSpin to have more control
    over the widget on different platforms"""

    gtk3MinSize = 130

    def __init__(self, *args, **kwargs):
        if gtk3:
            if "size" in kwargs:
                kwargs["size"] = wx.Size(
                    max(self.gtk3MinSize, kwargs["size"][0]), kwargs["size"][1]
                )
            else:
                kwargs["size"] = wx.Size(self.gtk3MinSize, -1)

        fs.FloatSpin.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            fs.FloatSpin.SetToolTip(self, tipString=tip)
        else:
            fs.FloatSpin.SetToolTipString(self, tip)


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


class ClearButton(Button):
    """Wrapper around a Button with stock id wx.ID_CLEAR,
    to disable default key binding on certain platforms"""

    def __init__(self, *args, **kwargs):
        Button.__init__(self, *args, **kwargs)
        self.SetId(wx.ID_CLEAR)
        if sys.platform == "darwin":
            self.SetLabel(_("Clear"))
        else:
            self.SetLabel(_("&Clear"))


class CancelButton(Button):
    """Wrapper around a Button with stock id wx.ID_CANCEL, to disable
    default key binding on certain platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        Button.__init__(self, *args, **kwargs)
        self.SetId(wx.ID_CANCEL)
        if sys.platform == "darwin" and not CheckWxVersion([4, 1, 0]):
            self.SetLabel(_("Cancel"))
        else:
            self.SetLabel(_("&Cancel"))


class CloseButton(Button):
    """Wrapper around a Close labeled Button with stock id wx.ID_CANCEL
    to disable default key binding on certain platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        Button.__init__(self, *args, **kwargs)
        self.SetId(wx.ID_CANCEL)
        if sys.platform == "darwin" and not CheckWxVersion([4, 1, 0]):
            self.SetLabel(_("Close"))
        else:
            self.SetLabel(_("&Close"))


class ApplyButton(Button):
    """Wrapper around a Button with stock id wx.ID_APPLY,
    to disable default key binding on certain platforms"""

    def __init__(self, *args, **kwargs):
        Button.__init__(self, *args, **kwargs)
        self.SetId(wx.ID_APPLY)
        if sys.platform == "darwin":
            self.SetLabel(_("Apply"))
        else:
            self.SetLabel(_("&Apply"))


class RadioButton(wx.RadioButton):
    """Wrapper around wx.RadioButton to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.RadioButton.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.RadioButton.SetToolTip(self, tipString=tip)
        else:
            wx.RadioButton.SetToolTipString(self, tip)


class BitmapButton(wx.BitmapButton):
    """Wrapper around wx.BitmapButton to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.BitmapButton.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.BitmapButton.SetToolTip(self, tipString=tip)
        else:
            wx.BitmapButton.SetToolTipString(self, tip)


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
            wx.StaticText.SetToolTip(self, tip)
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


class CheckListBox(wx.CheckListBox):
    """Wrapper around wx.CheckListBox to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.CheckListBox.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.CheckListBox.SetToolTip(self, tipString=tip)
        else:
            wx.CheckListBox.SetToolTipString(self, tip)


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

    def InsertItem(self, index, label, imageIndex=-1):
        if wxPythonPhoenix:
            return wx.ListCtrl.InsertItem(
                self, index=index, label=label, imageIndex=imageIndex
            )
        return wx.ListCtrl.InsertStringItem(
            self, index=index, label=label, imageIndex=imageIndex
        )

    def SetItem(self, index, column, label, imageId=-1):
        if wxPythonPhoenix:
            return wx.ListCtrl.SetItem(
                self, index=index, column=column, label=label, imageId=imageId
            )
        return wx.ListCtrl.SetStringItem(
            self, index=index, col=column, label=label, imageId=imageId
        )

    def CheckItem(self, item, check=True):
        """Uses either deprecated listmix.CheckListCtrlMixin
        or new checkbox implementation in wx.ListCtrl since 4.1.0"""
        if hasattr(self, "HasCheckBoxes"):
            wx.ListCtrl.CheckItem(self, item, check)
        else:
            super().CheckItem(item, check)

    def IsItemChecked(self, item):
        if hasattr(self, "HasCheckBoxes"):
            return wx.ListCtrl.IsItemChecked(self, item)
        return super().IsChecked(item)


if CheckWxVersion([4, 1, 0]):

    class CheckListCtrlMixin:
        """This class pretends to be deprecated CheckListCtrlMixin mixin and
        only enables checkboxes in new versions of ListCtrl"""

        def __init__(self):
            self.EnableCheckBoxes(True)
            self.AssignImageList(wx.ImageList(16, 16), wx.IMAGE_LIST_SMALL)

else:
    import wx.lib.mixins.listctrl as listmix

    class CheckListCtrlMixin(listmix.CheckListCtrlMixin):
        """Wrapper for deprecated mixin"""

        def __init__(self):
            listmix.CheckListCtrlMixin.__init__(self)


class TreeCtrl(wx.TreeCtrl):
    """Wrapper around wx.TreeCtrl to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.TreeCtrl.__init__(self, *args, **kwargs)

    def AppendItem(self, parent, text, image=-1, selImage=-1, data=None):
        if wxPythonPhoenix:
            return wx.TreeCtrl.AppendItem(self, parent, text, image, selImage, data)
        return wx.TreeCtrl.AppendItem(
            self, parent, text, image, selImage, wx.TreeItemData(data)
        )

    def GetItemData(self, item):
        if wxPythonPhoenix:
            return wx.TreeCtrl.GetItemData(self, item)
        return wx.TreeCtrl.GetPyData(self, item)


class CustomTreeCtrl(CT.CustomTreeCtrl):
    """Wrapper around wx.lib.agw.customtreectrl to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        CT.CustomTreeCtrl.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            CT.CustomTreeCtrl.SetToolTip(self, tipString=tip)
        else:
            CT.CustomTreeCtrl.SetToolTipString(self, tip)


class ToolBar(wx.ToolBar):
    """Wrapper around wx.ToolBar to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.ToolBar.__init__(self, *args, **kwargs)

    def AddLabelTool(
        self,
        toolId,
        label,
        bitmap,
        bmpDisabled=wx.NullBitmap,
        kind=0,
        shortHelpString="",
        longHelpString="",
        clientData=None,
    ):
        if wxPythonPhoenix:
            return wx.ToolBar.AddTool(
                self,
                toolId=toolId,
                label=label,
                bitmap=bitmap,
                bmpDisabled=bmpDisabled,
                kind=kind,
                shortHelp=shortHelpString,
                longHelp=longHelpString,
                clientData=clientData,
            )
        return wx.ToolBar.AddLabelTool(
            self,
            toolId,
            label,
            bitmap,
            bmpDisabled,
            kind,
            shortHelpString,
            longHelpString,
            clientData,
        )

    def InsertLabelTool(
        self,
        pos,
        toolId,
        label,
        bitmap,
        bmpDisabled=wx.NullBitmap,
        kind=0,
        shortHelpString="",
        longHelpString="",
        clientData=None,
    ):
        if wxPythonPhoenix:
            return wx.ToolBar.InsertTool(
                self,
                pos,
                toolId=toolId,
                label=label,
                bitmap=bitmap,
                bmpDisabled=bmpDisabled,
                kind=kind,
                shortHelp=shortHelpString,
                longHelp=longHelpString,
                clientData=clientData,
            )
        return wx.ToolBar.InsertLabelTool(
            self,
            pos,
            toolId,
            label,
            bitmap,
            bmpDisabled,
            kind,
            shortHelpString,
            longHelpString,
            clientData,
        )


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
            wx.Menu.AppendSubMenu(self, submenu=submenu, text=text, help=help)
        else:
            wx.Menu.AppendMenu(self, id=id, text=text, submenu=submenu, help=help)


class DragImage(wx.GenericDragImage if wxPythonPhoenix else wx.DragImage):
    """Wrapper around wx.DragImage to have more control
    over the widget on different platforms/wxpython versions"""


class PseudoDC(wx.adv.PseudoDC if wxPythonPhoenix else wx.PseudoDC):
    """Wrapper around wx.PseudoDC to have more control
    over the widget on different platforms/wxpython versions"""

    def DrawLinePoint(self, *args, **kwargs):
        args = convertToInt(argsOrKwargs=args, roundVal=True)
        kwargs = convertToInt(argsOrKwargs=kwargs, roundVal=True)
        if wxPythonPhoenix:
            super().DrawLine(*args, **kwargs)
        else:
            super().DrawLinePoint(*args, **kwargs)

    def DrawRectangleRect(self, rect):
        if wxPythonPhoenix:
            super().DrawRectangle(rect=rect)
        else:
            super().DrawRectangleRect(rect)

    def BeginDrawing(self):
        if not wxPythonPhoenix:
            super().BeginDrawing()

    def EndDrawing(self):
        if not wxPythonPhoenix:
            super().EndDrawing()

    def DrawRectangle(self, *args, **kwargs):
        args = convertToInt(argsOrKwargs=args, roundVal=True)
        kwargs = convertToInt(argsOrKwargs=kwargs, roundVal=True)
        super().DrawRectangle(*args, **kwargs)

    def DrawBitmap(self, *args, **kwargs):
        args = convertToInt(argsOrKwargs=args, roundVal=True)
        kwargs = convertToInt(argsOrKwargs=kwargs, roundVal=True)
        super().DrawBitmap(*args, **kwargs)

    def DrawCircle(self, *args, **kwargs):
        args = convertToInt(argsOrKwargs=args, roundVal=True)
        kwargs = convertToInt(argsOrKwargs=kwargs, roundVal=True)
        super().DrawCircle(*args, **kwargs)


class ClientDC(wx.ClientDC):
    """Wrapper around wx.ClientDC to have more control
    over the widget on different platforms/wxpython versions"""

    def GetFullMultiLineTextExtent(self, string, font=None):
        if wxPythonPhoenix:
            return super().GetFullMultiLineTextExtent(string, font)
        return super().GetMultiLineTextExtent(string, font)


class Rect(wx.Rect):
    """Wrapper around wx.Rect to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        args = convertToInt(argsOrKwargs=args)
        kwargs = convertToInt(argsOrKwargs=kwargs)
        wx.Rect.__init__(self, *args, **kwargs)

    def ContainsXY(self, x, y):
        if wxPythonPhoenix:
            return wx.Rect.Contains(self, x=int(x), y=int(y))
        return wx.Rect.ContainsXY(self, int(x), int(y))

    def ContainsRect(self, rect):
        if wxPythonPhoenix:
            return wx.Rect.Contains(self, rect=rect)
        return wx.Rect.ContainsRect(self, rect)

    def OffsetXY(self, dx, dy):
        if wxPythonPhoenix:
            return wx.Rect.Offset(self, int(dx), int(dy))
        return wx.Rect.OffsetXY(self, int(dx), int(dy))


class CheckBox(wx.CheckBox):
    """Wrapper around wx.CheckBox to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.CheckBox.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.CheckBox.SetToolTip(self, tipString=tip)
        else:
            wx.CheckBox.SetToolTipString(self, tip)


class Choice(wx.Choice):
    """Wrapper around wx.Choice to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.Choice.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.Choice.SetToolTip(self, tipString=tip)
        else:
            wx.Choice.SetToolTipString(self, tip)


class TextEntryDialog(wx.TextEntryDialog):
    """Wrapper around wx.TextEntryDialog to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(
        self,
        parent,
        message,
        caption="Please enter text",
        value="",
        style=wx.OK | wx.CANCEL | wx.CENTRE,
        pos=wx.DefaultPosition,
    ):
        if wxPythonPhoenix:
            super().__init__(
                parent=parent,
                message=message,
                caption=caption,
                value=value,
                style=style,
                pos=pos,
            )
        else:
            super().__init__(
                parent=parent,
                message=message,
                caption=caption,
                defaultValue=value,
                style=style,
                pos=pos,
            )


class ColourSelect(csel.ColourSelect):
    """Wrapper around wx.lib.colourselect.ColourSelect to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        csel.ColourSelect.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            csel.ColourSelect.SetToolTip(self, tipString=tip)
        else:
            csel.ColourSelect.SetToolTipString(self, tip)


class ComboCtrl(wxComboCtrl):
    def __init__(self, *args, **kwargs):
        wxComboCtrl.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wxComboCtrl.SetToolTip(self, tipString=tip)
        else:
            wxComboCtrl.SetToolTipString(self, tip)


class Dialog(wx.Dialog):
    """Wrapper around wx.Dialog to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.Dialog.__init__(self, *args, **kwargs)


class Notebook(wx.Notebook):
    """Wrapper around NoteBook to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.Notebook.__init__(self, *args, **kwargs)


class OwnerDrawnComboBox(OwnerDrawnComboBox_):
    """Wrapper around OwnerDrawnComboBox to have more control
    over the widget on different platforms/wxpython versions"""

    ODCB_PAINTING_CONTROL = ODCB_PAINTING_CONTROL
    ODCB_PAINTING_SELECTED = ODCB_PAINTING_SELECTED

    def __init__(self, *args, **kwargs):
        OwnerDrawnComboBox_.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            OwnerDrawnComboBox_.SetToolTip(self, tipString=tip)
        else:
            OwnerDrawnComboBox_.SetToolTipString(self, tip)


class BitmapComboBox(BitmapComboBox_):
    """Wrapper around BitmapComboBox to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        BitmapComboBox_.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            BitmapComboBox_.SetToolTip(self, tipString=tip)
        else:
            BitmapComboBox_.SetToolTipString(self, tip)


class ScrolledPanel(scrolled.ScrolledPanel):
    """Wrapper around scrolled.ScrolledPanel to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        scrolled.ScrolledPanel.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            scrolled.ScrolledPanel.SetToolTip(self, tipString=tip)
        else:
            scrolled.ScrolledPanel.SetToolTipString(self, tip)


class FileBrowseButton(filebrowse.FileBrowseButton):
    """Wrapper around filebrowse.FileBrowseButton to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        filebrowse.FileBrowseButton.__init__(self, *args, **kwargs)


class DirBrowseButton(filebrowse.DirBrowseButton):
    """Wrapper around filebrowse.DirBrowseButton to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        filebrowse.DirBrowseButton.__init__(self, *args, **kwargs)


class ExpandoTextCtrl(expando.ExpandoTextCtrl):
    """Wrapper around expando.ExpandoTextCtrl to have more control
    over the widget on different platforms/wxpython versions"""

    EVT_ETC_LAYOUT_NEEDED = expando.EVT_ETC_LAYOUT_NEEDED

    def __init__(self, *args, **kwargs):
        expando.ExpandoTextCtrl.__init__(self, *args, **kwargs)


class ColourPickerCtrl(wx.ColourPickerCtrl):
    """Wrapper around wx.ColourPickerCtrl to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.ColourPickerCtrl.__init__(self, *args, **kwargs)


class ListBox(wx.ListBox):
    """Wrapper around wx.ListBox to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.ListBox.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.ListBox.SetToolTip(self, tipString=tip)
        else:
            wx.ListBox.SetToolTipString(self, tip)

    def DeselectAll(self):
        for i in range(self.GetCount()):
            self.Deselect(i)

    def SelectAll(self):
        for i in range(self.GetCount()):
            self.Select(i)


class HyperlinkCtrl(HyperlinkCtrl_):
    """Wrapper around HyperlinkCtrl to have more control
    over the widget on different platforms/wxpython versions"""

    HL_ALIGN_LEFT = HL_ALIGN_LEFT
    HL_CONTEXTMENU = HL_CONTEXTMENU

    def __init__(self, *args, **kwargs):
        HyperlinkCtrl_.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            HyperlinkCtrl_.SetToolTip(self, tipString=tip)
        else:
            HyperlinkCtrl_.SetToolTipString(self, tip)


class ComboBox(wx.ComboBox):
    """Wrapper around wx.ComboBox to have more control
    over the widget on different platforms/wxpython versions"""

    def __init__(self, *args, **kwargs):
        wx.ComboBox.__init__(self, *args, **kwargs)

    def SetToolTip(self, tip):
        if wxPythonPhoenix:
            wx.ComboBox.SetToolTip(self, tipString=tip)
        else:
            wx.ComboBox.SetToolTipString(self, tip)


class SimpleTabArt(tabart.AuiDefaultTabArt):
    """A class simplifying the appearance of AUI notebook tabs."""

    def __init__(self):
        """Default class constructor."""
        tabart.AuiDefaultTabArt.__init__(self)

    def SetDefaultColours(self, base_colour=None):
        """
        Overrides AuiDefaultTabArt.SetDefaultColours
        to get rid of gradient and to look more like
        flatnotebook tabs.
        """

        if base_colour is None:
            base_colour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_BTNFACE)

        self.SetBaseColour(base_colour)
        tab_color = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW)

        self._border_colour = wx.SystemSettings.GetColour(wx.SYS_COLOUR_BTNSHADOW)
        self._border_pen = wx.Pen(self._border_colour)
        self._background_top_colour = base_colour
        self._background_bottom_colour = base_colour
        self._tab_top_colour = tab_color
        self._tab_bottom_colour = tab_color
        self._tab_gradient_highlight_colour = tab_color
        self._tab_inactive_top_colour = base_colour
        self._tab_inactive_bottom_colour = base_colour
        self._tab_text_colour = lambda page: page.text_colour
        self._tab_disabled_text_colour = wx.SystemSettings.GetColour(
            wx.SYS_COLOUR_GRAYTEXT
        )
