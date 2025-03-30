"""
@package gui_core.widgets

@brief Core GUI widgets

Classes:
 - widgets::GNotebook
 - widgets::ScrolledPanel
 - widgets::NumTextCtrl
 - widgets::FloatSlider
 - widgets::SymbolButton
 - widgets::StaticWrapText
 - widgets::BaseValidator
 - widgets::CoordinatesValidator
 - widgets::IntegerValidator
 - widgets::FloatValidator
 - widgets::EmailValidator
 - widgets::TimeISOValidator
 - widgets::MapValidator
 - widgets::NTCValidator
 - widgets::SimpleValidator
 - widgets::GenericValidator
 - widgets::GenericMultiValidator
 - widgets::LayersListValidator
 - widgets::PlacementValidator
 - widgets::GListCtrl
 - widgets::SearchModuleWidget
 - widgets::ManageSettingsWidget
 - widgets::PictureComboBox
 - widgets::ColorTablesComboBox
 - widgets::BarscalesComboBox
 - widgets::NArrowsComboBox
 - widgets::LayersList

@todo:
 - move validators to a separate file gui_core/validators.py

(C) 2008-2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
@author Enhancements by Michael Barton <michael.barton asu.edu>
@author Anna Kratochvilova <kratochanna gmail.com> (Google SoC 2011)
@author Stepan Turek <stepan.turek seznam.cz> (ManageSettingsWidget - created from
        GdalSelect)
@author Matej Krejci <matejkrejci gmail.com> (Google GSoC 2014; EmailValidator,
        TimeISOValidator)
@author Tomas Zigo <tomas.zigo slovanet.sk> (LayersListValidator, PlacementValidator)
"""

import os
import sys
import string
import re
from bisect import bisect
from datetime import datetime
from core.globalvar import wxPythonPhoenix

import wx
import wx.lib.mixins.listctrl as listmix
import wx.lib.scrolledpanel as SP
from wx.lib.stattext import GenStaticText
from wx.lib.wordwrap import wordwrap

if wxPythonPhoenix:
    import wx.adv
    from wx.adv import OwnerDrawnComboBox
else:
    import wx.combo
    from wx.combo import OwnerDrawnComboBox
try:
    import wx.lib.agw.flatnotebook as FN
except ImportError:
    import wx.lib.flatnotebook as FN
try:
    from wx.lib.buttons import ThemedGenBitmapTextButton as BitmapTextButton
except ImportError:  # not sure about TGBTButton version
    from wx.lib.buttons import GenBitmapTextButton as BitmapTextButton

if wxPythonPhoenix:
    from wx import Validator
else:
    from wx import PyValidator as Validator

from grass.script import core as grass

from grass.pydispatch.signal import Signal

from core import globalvar
from core.gcmd import GMessage, GError
from core.debug import Debug
from gui_core.wrap import (
    Button,
    SearchCtrl,
    Slider,
    StaticText,
    StaticBox,
    TextCtrl,
    Menu,
    Rect,
    EmptyBitmap,
    ListCtrl,
    NewId,
    CheckListCtrlMixin,
)


class NotebookController:
    """Provides handling of notebook page names.

    Translates page names to page indices.
    Class is aggregated in notebook subclasses.
    Notebook subclasses must delegate methods to controller.
    Methods inherited from notebook class must be delegated explicitly
    and other methods can be delegated by @c __getattr__.
    """

    def __init__(self, classObject, widget):
        """
        :param classObject: notebook class name (object, i.e. FlatNotebook)
        :param widget: notebook instance
        """
        self.notebookPages = {}
        self.classObject = classObject
        self.widget = widget
        self.highlightedTextEnd = _(" (...)")
        self.BindPageChanged()

    def BindPageChanged(self):
        """Binds page changed event."""
        self.widget.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.OnRemoveHighlight)

    def AddPage(self, *args, **kwargs):
        """Add a new page

        :param str name: use this param if notebooks has ability to
                         change position and then you must use page name
                         param arg to correctly delete notebook page.
                         If you do not use this parameter, make sure that
                         the notebooks does not have the ability to change
                         position, because in that case the deletion of
                         the page based on the position index would not
                         work correctly.
        """
        if "name" in kwargs:
            self.notebookPages[kwargs["name"]] = kwargs["page"]
            del kwargs["name"]

        self.classObject.AddPage(self.widget, *args, **kwargs)

    def InsertPage(self, *args, **kwargs):
        """Insert a new page

        :param str name: use this param if notebooks has ability to
                         change position and then you must use page name
                         param arg to correctly delete notebook page.
                         If you do not use this parameter, make sure that
                         the notebooks does not have the ability to change
                         position, because in that case the deletion of
                         the page based on the position index would not
                         work correctly.
        """
        if "name" in kwargs:
            self.notebookPages[kwargs["name"]] = kwargs["page"]
            del kwargs["name"]

        try:
            self.classObject.InsertPage(self.widget, *args, **kwargs)
        except (
            TypeError
        ):  # documentation says 'index', but certain versions of wx require 'n'
            kwargs["n"] = kwargs["index"]
            del kwargs["index"]
            self.classObject.InsertPage(self.widget, *args, **kwargs)

    def DeletePage(self, page):
        """Delete page

        :param str|int page: page name or page index position

        :return bool: True if page was deleted, False if not exists
        """
        delPageIndex = self.GetPageIndexByName(page)
        if delPageIndex == -1:
            return False
        ret = self.classObject.DeletePage(self.widget, delPageIndex)
        if ret:
            del self.notebookPages[page]
        return ret

    def RemovePage(self, page):
        """Delete page without deleting the associated window.

        :param page: name
        :return: True if page was deleted, False if not exists
        """
        delPageIndex = self.GetPageIndexByName(page)
        if delPageIndex == -1:
            return False
        ret = self.classObject.RemovePage(self.widget, delPageIndex)
        if ret:
            del self.notebookPages[page]
        return ret

    def SetSelectionByName(self, page):
        """Set active notebook page.

        :param page: name, eg. 'layers', 'output', 'search', 'pyshell', 'nviz'
                     (depends on concrete notebook instance)
        """
        idx = self.GetPageIndexByName(page)
        if self.classObject.GetSelection(self.widget) != idx:
            self.classObject.SetSelection(self.widget, idx)

            self.RemoveHighlight(idx)

    def OnRemoveHighlight(self, event):
        """Highlighted tab name should be removed."""
        page = event.GetSelection()
        self.RemoveHighlight(page)
        event.Skip()

    def RemoveHighlight(self, page):
        """Removes highlight string from notebook tab name if necessary.

        :param page: index
        """
        text = self.classObject.GetPageText(self.widget, page)
        if text.endswith(self.highlightedTextEnd):
            text = text.replace(self.highlightedTextEnd, "")
            self.classObject.SetPageText(self.widget, page, text)

    def GetPageIndexByName(self, page):
        """Get notebook page index

        :param str|int page: page name or page index position

        :return int: page index
        """
        if not self.notebookPages:
            return page
        if page not in self.notebookPages:
            return -1
        for pageIndex in range(self.classObject.GetPageCount(self.widget)):
            if self.notebookPages[page] == self.classObject.GetPage(
                self.widget, pageIndex
            ):
                break
        return pageIndex

    def HighlightPageByName(self, page):
        pageIndex = self.GetPageIndexByName(page)
        self.HighlightPage(pageIndex)

    def HighlightPage(self, index):
        if self.classObject.GetSelection(self.widget) != index:
            text = self.classObject.GetPageText(self.widget, index)
            if not text.endswith(self.highlightedTextEnd):
                text += self.highlightedTextEnd
            self.classObject.SetPageText(self.widget, index, text)

    def SetPageImage(self, page, index):
        """Sets image index for page

        :param page: page name
        :param index: image index (in wx.ImageList)
        """
        pageIndex = self.GetPageIndexByName(page)
        self.classObject.SetPageImage(self.widget, pageIndex, index)


class FlatNotebookController(NotebookController):
    """Controller specialized for FN.FlatNotebook subclasses"""

    def __init__(self, classObject, widget):
        NotebookController.__init__(self, classObject, widget)

    def BindPageChanged(self):
        self.widget.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnRemoveHighlight)

    def GetPageIndexByName(self, page):
        """Get notebook page index

        :param str|int page: page name or page index position

        :return int: page index
        """
        if not self.notebookPages:
            return page
        if page not in self.notebookPages:
            return -1

        return self.classObject.GetPageIndex(self.widget, self.notebookPages[page])

    def InsertPage(self, *args, **kwargs):
        """Insert a new page"""
        if "name" in kwargs:
            self.notebookPages[kwargs["name"]] = kwargs["page"]
            del kwargs["name"]

        kwargs["indx"] = kwargs["index"]
        del kwargs["index"]
        self.classObject.InsertPage(self.widget, *args, **kwargs)


class GNotebook(FN.FlatNotebook):
    """Generic notebook widget.

    Enables advanced style settings.
    Problems with hidden tabs. Uses system colours for active tabs.
    """

    def __init__(self, parent, style, **kwargs):
        if globalvar.hasAgw:
            FN.FlatNotebook.__init__(
                self, parent, id=wx.ID_ANY, agwStyle=style, **kwargs
            )
        else:
            FN.FlatNotebook.__init__(self, parent, id=wx.ID_ANY, style=style, **kwargs)

        self.controller = FlatNotebookController(
            classObject=FN.FlatNotebook, widget=self
        )
        self.SetActiveTabColour(wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW))
        self.SetActiveTabTextColour(
            wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOWTEXT)
        )

    def AddPage(self, *args, **kwargs):
        """@copydoc NotebookController::AddPage()"""
        self.controller.AddPage(*args, **kwargs)

    def InsertNBPage(self, *args, **kwargs):
        """@copydoc NotebookController::InsertPage()"""
        self.controller.InsertPage(*args, **kwargs)

    def DeleteNBPage(self, page):
        """@copydoc NotebookController::DeletePage()"""
        return self.controller.DeletePage(page)

    def RemoveNBPage(self, page):
        """@copydoc NotebookController::RemovePage()"""
        return self.controller.RemovePage(page)

    def SetPageImage(self, page, index):
        """Does nothing because we don't want images for this style"""

    def __getattr__(self, name):
        return getattr(self.controller, name)


class FormNotebook(wx.Notebook):
    """Notebook widget.

    Respects native look.
    """

    def __init__(self, parent, style):
        wx.Notebook.__init__(self, parent, id=wx.ID_ANY, style=style)
        self.controller = NotebookController(classObject=wx.Notebook, widget=self)

    def AddPage(self, *args, **kwargs):
        """@copydoc NotebookController::AddPage()"""
        self.controller.AddPage(*args, **kwargs)

    def InsertNBPage(self, *args, **kwargs):
        """@copydoc NotebookController::InsertPage()"""
        self.controller.InsertPage(*args, **kwargs)

    def DeleteNBPage(self, page):
        """@copydoc NotebookController::DeletePage()"""
        return self.controller.DeletePage(page)

    def RemoveNBPage(self, page):
        """@copydoc NotebookController::RemovePage()"""
        return self.controller.RemovePage(page)

    def SetPageImage(self, page, index):
        """@copydoc NotebookController::SetPageImage()"""
        return self.controller.SetPageImage(page, index)

    def __getattr__(self, name):
        return getattr(self.controller, name)


class FormListbook(wx.Listbook):
    """Notebook widget.

    Respects native look.
    """

    def __init__(self, parent, style):
        wx.Listbook.__init__(self, parent, id=wx.ID_ANY, style=style)
        self.controller = NotebookController(classObject=wx.Listbook, widget=self)

    def AddPage(self, *args, **kwargs):
        """@copydoc NotebookController::AddPage()"""
        self.controller.AddPage(*args, **kwargs)

    def InsertPage_(self, *args, **kwargs):
        """@copydoc NotebookController::InsertPage()"""
        self.controller.InsertPage(*args, **kwargs)

    def DeletePage(self, page):
        """@copydoc NotebookController::DeletePage()"""
        return self.controller.DeletePage(page)

    def RemovePage(self, page):
        """@copydoc NotebookController::RemovePage()"""
        return self.controller.RemovePage(page)

    def SetPageImage(self, page, index):
        """@copydoc NotebookController::SetPageImage()"""
        return self.controller.SetPageImage(page, index)

    def __getattr__(self, name):
        return getattr(self.controller, name)


class ScrolledPanel(SP.ScrolledPanel):
    """Custom ScrolledPanel to avoid strange behaviour concerning focus"""

    def __init__(self, parent, style=wx.TAB_TRAVERSAL):
        SP.ScrolledPanel.__init__(self, parent=parent, id=wx.ID_ANY, style=style)

    def OnChildFocus(self, event):
        pass


class NumTextCtrl(TextCtrl):
    """Class derived from wx.TextCtrl for numerical values only"""

    def __init__(self, parent, **kwargs):
        TextCtrl.__init__(
            self, parent=parent, validator=NTCValidator(flag="DIGIT_ONLY"), **kwargs
        )

    def SetValue(self, value):
        super().SetValue(str(value))

    def GetValue(self):
        val = super().GetValue()
        if val == "":
            val = "0"
        try:
            return float(val)
        except ValueError:
            val = "".join("".join(val.split("-")).split("."))
            return float(val)

    def SetRange(self, min, max):
        pass


class FloatSlider(Slider):
    """Class derived from wx.Slider for floats"""

    def __init__(self, **kwargs):
        Debug.msg(1, "FloatSlider.__init__()")
        Slider.__init__(self, **kwargs)
        self.coef = 1.0
        # init range
        self.minValueOrig = 0
        self.maxValueOrig = 1

    def SetValue(self, value):
        value *= self.coef
        if abs(value) < 1 and value != 0:
            while abs(value) < 1:
                value *= 100
                self.coef *= 100
            super().SetRange(
                self.minValueOrig * self.coef, self.maxValueOrig * self.coef
            )
        super().SetValue(value)

        Debug.msg(4, "FloatSlider.SetValue(): value = %f" % value)

    def SetRange(self, minValue, maxValue):
        self.coef = 1.0
        self.minValueOrig = minValue
        self.maxValueOrig = maxValue
        if abs(minValue) < 1 or abs(maxValue) < 1:
            while (abs(minValue) < 1 and minValue != 0) or (
                abs(maxValue) < 1 and maxValue != 0
            ):
                minValue *= 100
                maxValue *= 100
                self.coef *= 100
            super().SetValue(super().GetValue() * self.coef)
        super().SetRange(minValue, maxValue)
        Debug.msg(
            4,
            "FloatSlider.SetRange(): minValue = %f, maxValue = %f"
            % (minValue, maxValue),
        )

    def GetValue(self):
        val = super().GetValue()
        Debug.msg(4, "FloatSlider.GetValue(): value = %f" % (val / self.coef))
        return val / self.coef


class SymbolButton(BitmapTextButton):
    """Button with symbol and label."""

    def __init__(self, parent, usage, label, **kwargs):
        """Constructor

        :param parent: parent (usually wx.Panel)
        :param usage: determines usage and picture
        :param label: displayed label
        """
        size = (15, 15)
        buffer = EmptyBitmap(*size)
        BitmapTextButton.__init__(
            self, parent=parent, label=" " + label, bitmap=buffer, **kwargs
        )

        dc = wx.MemoryDC()
        dc.SelectObject(buffer)
        maskColor = wx.Colour(255, 255, 255)
        dc.SetBrush(wx.Brush(maskColor))
        dc.Clear()

        if usage == "record":
            self.DrawRecord(dc, size)
        elif usage == "stop":
            self.DrawStop(dc, size)
        elif usage == "play":
            self.DrawPlay(dc, size)
        elif usage == "pause":
            self.DrawPause(dc, size)

        if sys.platform not in {"win32", "darwin"}:
            buffer.SetMaskColour(maskColor)
        self.SetBitmapLabel(buffer)
        dc.SelectObject(wx.NullBitmap)

    def DrawRecord(self, dc, size):
        """Draw record symbol"""
        dc.SetBrush(wx.Brush(wx.Colour(255, 0, 0)))
        dc.DrawCircle(size[0] // 2, size[1] // 2, size[0] // 2)

    def DrawStop(self, dc, size):
        """Draw stop symbol"""
        dc.SetBrush(wx.Brush(wx.Colour(50, 50, 50)))
        dc.DrawRectangle(0, 0, size[0], size[1])

    def DrawPlay(self, dc, size):
        """Draw play symbol"""
        dc.SetBrush(wx.Brush(wx.Colour(0, 255, 0)))
        points = (wx.Point(0, 0), wx.Point(0, size[1]), wx.Point(size[0], size[1] // 2))
        dc.DrawPolygon(points)

    def DrawPause(self, dc, size):
        """Draw pause symbol"""
        dc.SetBrush(wx.Brush(wx.Colour(50, 50, 50)))
        dc.DrawRectangle(0, 0, 2 * size[0] // 5, size[1])
        dc.DrawRectangle(3 * size[0] // 5, 0, 2 * size[0] // 5, size[1])


class StaticWrapText(GenStaticText):
    """A Static Text widget that wraps its text to fit parents width,
    enlarging its height if necessary."""

    def __init__(self, parent, id=wx.ID_ANY, label="", margin=0, *args, **kwds):
        self._margin = margin
        self._initialLabel = label
        self.init = False
        GenStaticText.__init__(self, parent, id, label, *args, **kwds)
        self.Bind(wx.EVT_SIZE, self.OnSize)

    def DoGetBestSize(self):
        """Overridden method which reports widget's best size."""
        if not self.init:
            self.init = True
            self._updateLabel()

        parent = self.GetParent()
        newExtent = wx.ClientDC(parent).GetMultiLineTextExtent(self.GetLabel())
        # when starting, width is very small and height is big which creates
        # very high windows
        if newExtent[0] < newExtent[1]:
            return (0, 0)
        return newExtent[:2]

    def OnSize(self, event):
        self._updateLabel()
        event.Skip()

    def _updateLabel(self):
        """Calculates size of wrapped label"""
        parent = self.GetParent()
        newLabel = wordwrap(
            text=self._initialLabel,
            width=parent.GetSize()[0],
            dc=wx.ClientDC(parent),
            breakLongWords=True,
            margin=self._margin,
        )
        GenStaticText.SetLabel(self, newLabel)

    def SetLabel(self, label):
        self._initialLabel = label
        self._updateLabel()


class BaseValidator(Validator):
    def __init__(self):
        Validator.__init__(self)

        self.Bind(wx.EVT_TEXT, self.OnText)

    def OnText(self, event):
        """Do validation"""
        self._validate(win=event.GetEventObject())

        event.Skip()

    def Validate(self, parent):
        """Is called upon closing wx.Dialog"""
        win = self.GetWindow()
        return self._validate(win)

    def _validate(self, win):
        """Validate input"""
        text = win.GetValue()

        if text:
            try:
                self.type(text)
            except ValueError:
                self._notvalid()
                return False

        self._valid()
        return True

    def _notvalid(self):
        textCtrl = self.GetWindow()

        textCtrl.SetBackgroundColour("grey")
        textCtrl.Refresh()

    def _valid(self):
        textCtrl = self.GetWindow()

        sysColor = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW)
        textCtrl.SetBackgroundColour(sysColor)

        textCtrl.Refresh()
        return True

    def TransferToWindow(self):
        return True  # Prevent wxDialog from complaining.

    def TransferFromWindow(self):
        return True  # Prevent wxDialog from complaining.


class CoordinatesValidator(BaseValidator):
    """Validator for coordinates input (list of floats separated by comma)"""

    def __init__(self):
        BaseValidator.__init__(self)

    def _validate(self, win):
        """Validate input"""
        text = win.GetValue()
        if text:
            try:
                text = text.split(",")

                for t in text:
                    float(t)

                if len(text) % 2 != 0:
                    return False

            except ValueError:
                self._notvalid()
                return False

        self._valid()
        return True

    def Clone(self):
        """Clone validator"""
        return CoordinatesValidator()


class IntegerValidator(BaseValidator):
    """Validator for floating-point input"""

    def __init__(self):
        BaseValidator.__init__(self)
        self.type = int

    def Clone(self):
        """Clone validator"""
        return IntegerValidator()


class FloatValidator(BaseValidator):
    """Validator for floating-point input"""

    def __init__(self):
        BaseValidator.__init__(self)
        self.type = float

    def Clone(self):
        """Clone validator"""
        return FloatValidator()


class EmailValidator(BaseValidator):
    """Validator for email input"""

    def __init__(self):
        BaseValidator.__init__(self)

    def _validate(self, win):
        """Validate input"""
        text = win.GetValue()
        if text:
            if re.match(r"\b[\w.-]+@[\w.-]+.\w{2,4}\b", text) is None:
                self._notvalid()
                return False

        self._valid()
        return True

    def Clone(self):
        """Clone validator"""
        return EmailValidator()


class TimeISOValidator(BaseValidator):
    """Validator for time ISO format (YYYY-MM-DD) input"""

    def __init__(self):
        BaseValidator.__init__(self)

    def _validate(self, win):
        """Validate input"""
        text = win.GetValue()
        if text:
            try:
                datetime.strptime(text, "%Y-%m-%d")
            except ValueError:
                self._notvalid()
                return False

        self._valid()
        return True

    def Clone(self):
        """Clone validator"""
        return TimeISOValidator()


class NTCValidator(Validator):
    """validates input in textctrls, taken from wxpython demo"""

    def __init__(self, flag=None):
        Validator.__init__(self)
        self.flag = flag
        self.Bind(wx.EVT_CHAR, self.OnChar)

    def Clone(self):
        return NTCValidator(self.flag)

    def OnChar(self, event):
        key = event.GetKeyCode()
        if key < wx.WXK_SPACE or key == wx.WXK_DELETE or key > 255:
            event.Skip()
            return
        if self.flag == "DIGIT_ONLY" and chr(key) in string.digits + ".-":
            event.Skip()
            return
        if not wx.Validator_IsSilent():
            wx.Bell()
        # Returning without calling even.Skip eats the event before it
        # gets to the text control
        return


class SimpleValidator(Validator):
    """This validator is used to ensure that the user has entered something
    into the text object editor dialog's text field.
    """

    def __init__(self, callback):
        """Standard constructor."""
        Validator.__init__(self)
        self.callback = callback

    def Clone(self):
        """Standard cloner.

        Note that every validator must implement the Clone() method.
        """
        return SimpleValidator(self.callback)

    def Validate(self, win):
        """Validate the contents of the given text control."""
        ctrl = self.GetWindow()
        text = ctrl.GetValue()
        if len(text) == 0:
            self.callback(ctrl)
            return False
        return True

    def TransferToWindow(self):
        """Transfer data from validator to window.

        The default implementation returns False, indicating that an
        error occurred.  We simply return True, as we don't do any data
        transfer.
        """
        return True  # Prevent wxDialog from complaining.

    def TransferFromWindow(self):
        """Transfer data from window to validator.

        The default implementation returns False, indicating that an
        error occurred.  We simply return True, as we don't do any data
        transfer.
        """
        return True  # Prevent wxDialog from complaining.


class GenericValidator(Validator):
    """This validator checks condition and calls callback
    in case the condition is not fulfilled.
    """

    def __init__(self, condition, callback):
        """Standard constructor.

        :param condition: function which accepts string value and returns T/F
        :param callback: function which is called when condition is not fulfilled
        """
        Validator.__init__(self)
        self._condition = condition
        self._callback = callback

    def Clone(self):
        """Standard cloner.

        Note that every validator must implement the Clone() method.
        """
        return GenericValidator(self._condition, self._callback)

    def Validate(self, win):
        """Validate the contents of the given text control."""
        ctrl = self.GetWindow()
        text = ctrl.GetValue()
        if not self._condition(text):
            self._callback(ctrl)
            return False
        return True

    def TransferToWindow(self):
        """Transfer data from validator to window."""
        return True  # Prevent wxDialog from complaining.

    def TransferFromWindow(self):
        """Transfer data from window to validator."""
        return True  # Prevent wxDialog from complaining.


class MapValidator(GenericValidator):
    """Validator for map name input

    See G_legal_filename()
    """

    def __init__(self):
        def _mapNameValidationFailed(ctrl):
            message = _(
                "Name <%(name)s> is not a valid name for GRASS map. "
                "Please use only ASCII characters excluding %(chars)s "
                "and space."
            ) % {"name": ctrl.GetValue(), "chars": "/\"'@,=*~"}
            GError(message, caption=_("Invalid name"))

        GenericValidator.__init__(self, grass.legal_name, _mapNameValidationFailed)


class GenericMultiValidator(Validator):
    """This validator checks conditions and calls callbacks
    in case the condition is not fulfilled.
    """

    def __init__(self, checks):
        """Standard constructor.

        :param checks: list of tuples consisting of conditions (list of
        functions which accepts string value and returns T/F) and callbacks (
        list of functions which is called when condition is not fulfilled)
        """
        Validator.__init__(self)
        self._checks = checks

    def Clone(self):
        """Standard cloner.

        Note that every validator must implement the Clone() method.
        """
        return GenericMultiValidator(self._checks)

    def Validate(self, win):
        """Validate the contents of the given text control."""
        ctrl = self.GetWindow()
        text = ctrl.GetValue()
        for condition, callback in self._checks:
            if not condition(text):
                callback(ctrl)
                return False
        return True

    def TransferToWindow(self):
        """Transfer data from validator to window."""
        return True  # Prevent wxDialog from complaining.

    def TransferFromWindow(self):
        """Transfer data from window to validator."""
        return True  # Prevent wxDialog from complaining.


class SingleSymbolPanel(wx.Panel):
    """Panel for displaying one symbol.

    Changes background when selected. Assumes that parent will catch
    events emitted on mouse click. Used in gui_core::dialog::SymbolDialog.
    """

    def __init__(self, parent, symbolPath):
        """Panel constructor

        Signal symbolSelectionChanged - symbol selected
                                      - attribute 'name' (symbol name)
                                      - attribute 'doubleClick' (underlying cause)

        :param parent: parent (gui_core::dialog::SymbolDialog)
        :param symbolPath: absolute path to symbol
        """
        self.symbolSelectionChanged = Signal("SingleSymbolPanel.symbolSelectionChanged")

        wx.Panel.__init__(self, parent, id=wx.ID_ANY, style=wx.BORDER_RAISED)
        self.SetName(os.path.splitext(os.path.basename(symbolPath))[0])
        self.sBmp = wx.StaticBitmap(self, wx.ID_ANY, wx.Bitmap(symbolPath))

        self.selected = False
        self.selectColor = wx.SystemSettings.GetColour(wx.SYS_COLOUR_HIGHLIGHT)
        self.deselectColor = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW)

        sizer = wx.BoxSizer()
        sizer.Add(self.sBmp, proportion=0, flag=wx.ALL | wx.ALIGN_CENTER, border=5)
        self.SetBackgroundColour(self.deselectColor)
        self.SetMinSize(self.GetBestSize())
        self.SetSizerAndFit(sizer)

        # binding to both (staticBitmap, Panel) necessary
        self.sBmp.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        self.Bind(wx.EVT_LEFT_DCLICK, self.OnDoubleClick)
        self.sBmp.Bind(wx.EVT_LEFT_DCLICK, self.OnDoubleClick)

    def OnLeftDown(self, event):
        """Panel selected, background changes"""
        self.selected = True
        self.SetBackgroundColour(self.selectColor)
        self.Refresh()
        event.Skip()

        self.symbolSelectionChanged.emit(name=self.GetName(), doubleClick=False)

    def OnDoubleClick(self, event):
        self.symbolSelectionChanged.emit(name=self.GetName(), doubleClick=True)

    def Deselect(self):
        """Panel deselected, background changes back to default"""
        self.selected = False
        self.SetBackgroundColour(self.deselectColor)
        self.Refresh()

    def Select(self):
        """Select panel, no event emitted"""
        self.selected = True
        self.SetBackgroundColour(self.selectColor)
        self.Refresh()


class LayersListValidator(GenericValidator):
    """This validator check output map existence"""

    def __init__(self, condition, callback):
        """Standard constructor.

        :param condition: function which accepts string value and returns T/F
        :param callback: function which is called when condition is not fulfilled
        """
        GenericValidator.__init__(self, condition, callback)

    def Clone(self):
        """Standard cloner.

        Note that every validator must implement the Clone() method.
        """
        return LayersListValidator(self._condition, self._callback)

    def Validate(self, win, validate_all=False):
        """Validate output map existence"""
        mapset = grass.gisenv()["MAPSET"]
        maps = grass.list_grouped(type=self._condition)[mapset]

        # Check all selected layers
        if validate_all:
            outputs = []
            data = win.GetLayers()

            if data is None:
                return False

            for layer, output, list_id in data:
                if output in maps:
                    outputs.append(output)

            if outputs:
                win.output_map = outputs
                self._callback(layers_list=win)
                return False
        else:
            output_map = win.GetItemText(win.col, win.row)
            if output_map in maps:
                win.output_map = output_map
                self._callback(layers_list=win)
                return False
        return True


class PlacementValidator(BaseValidator):
    """Validator for placement input (list of floats separated by comma)"""

    def __init__(self, num_of_params):
        self._num_of_params = num_of_params
        super().__init__()

    def _enableDisableBtn(self, enable):
        """Enable/Disable button

        :param bool enable: Enable/Disable btn
        """
        win = self.GetWindow().GetTopLevelParent()
        for btn_id in (wx.ID_OK, wx.ID_APPLY):
            btn = win.FindWindow(id=btn_id)
            if btn:
                btn.Enable(enable)

    def _valid(self):
        super()._valid()
        self._enableDisableBtn(enable=True)

    def _notvalid(self):
        super()._notvalid()
        self._enableDisableBtn(enable=False)

    def _validate(self, win):
        """Validate input"""
        text = win.GetValue()
        if text:
            try:
                text = text.split(",")

                for t in text:
                    float(t)

                if len(text) % self._num_of_params != 0:
                    self._notvalid()
                    return False

            except ValueError:
                self._notvalid()
                return False

        self._valid()
        return True

    def Clone(self):
        """Clone validator"""
        return PlacementValidator(num_of_params=self._num_of_params)


class GListCtrl(ListCtrl, listmix.ListCtrlAutoWidthMixin, CheckListCtrlMixin):
    """Generic ListCtrl with popup menu to select/deselect all
    items"""

    def __init__(self, parent):
        self.parent = parent

        ListCtrl.__init__(self, parent, id=wx.ID_ANY, style=wx.LC_REPORT)
        CheckListCtrlMixin.__init__(self)

        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)

        self.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnPopupMenu)  # wxMSW
        self.Bind(wx.EVT_RIGHT_UP, self.OnPopupMenu)  # wxGTK

    def OnPopupMenu(self, event):
        """Show popup menu"""
        if self.GetItemCount() < 1:
            return

        if not hasattr(self, "popupDataID1"):
            self.popupDataID1 = NewId()
            self.popupDataID2 = NewId()

            self.Bind(wx.EVT_MENU, self.OnSelectAll, id=self.popupDataID1)
            self.Bind(wx.EVT_MENU, self.OnSelectNone, id=self.popupDataID2)

        # generate popup-menu
        menu = Menu()
        menu.Append(self.popupDataID1, _("Select all"))
        menu.Append(self.popupDataID2, _("Deselect all"))

        self.PopupMenu(menu)
        menu.Destroy()

    def SelectAll(self, select=True):
        """Check or uncheck all items"""
        item = -1
        while True:
            item = self.GetNextItem(item)
            if item == -1:
                break
            self.CheckItem(item, select)

    def OnSelectAll(self, event):
        """Check all items"""
        self.SelectAll(select=True)

        event.Skip()

    def OnSelectNone(self, event):
        """Uncheck items"""
        self.SelectAll(select=False)

        event.Skip()

    def GetData(self, checked=None):
        """Get list data"""
        data = []
        checkedList = []

        item = -1
        while True:
            row = []
            item = self.GetNextItem(item)
            if item == -1:
                break

            isChecked = self.IsItemChecked(item)
            if checked is not None and checked != isChecked:
                continue

            checkedList.append(isChecked)

            for i in range(self.GetColumnCount()):
                row.append(self.GetItem(item, i).GetText())

            row.append(item)
            data.append(tuple(row))

        if checked is not None:
            return tuple(data)
        return (tuple(data), tuple(checkedList))

    def LoadData(self, data=None, selectOne=True):
        """Load data into list"""
        self.DeleteAllItems()
        if data is None:
            return

        idx = 0
        for item in data:
            index = self.InsertItem(idx, str(item[0]))
            for i in range(1, self.GetColumnCount()):
                self.SetItem(index, i, item[i])
            idx += 1

        # check by default only on one item
        if len(data) == 1 and selectOne:
            self.CheckItem(index, True)


class SearchModuleWidget(wx.Panel):
    """Search module widget (used e.g. in SearchModuleWindow)

    Signals:
        moduleSelected - attribute 'name' is module name
        showSearchResult - attribute 'result' is a node (representing module)
        showNotification - attribute 'message'
    """

    def __init__(self, parent, model, showChoice=True, showTip=False, **kwargs):
        self._showTip = showTip
        self._showChoice = showChoice
        self._model = model
        self._results = []  # list of found nodes
        self._resultIndex = -1
        self._searchKeys = ["description", "keywords", "command"]
        self._oldValue = ""

        self.moduleSelected = Signal("SearchModuleWidget.moduleSelected")
        self.showSearchResult = Signal("SearchModuleWidget.showSearchResult")
        self.showNotification = Signal("SearchModuleWidget.showNotification")

        wx.Panel.__init__(self, parent=parent, id=wx.ID_ANY, **kwargs)

        #        self._box = wx.StaticBox(parent = self, id = wx.ID_ANY,
        # label = " %s " % _("Find tool - (press Enter for next match)"))

        if sys.platform == "win32":
            self._search = TextCtrl(
                parent=self, id=wx.ID_ANY, size=(-1, 25), style=wx.TE_PROCESS_ENTER
            )
        else:
            self._search = SearchCtrl(
                parent=self, id=wx.ID_ANY, size=(-1, 25), style=wx.TE_PROCESS_ENTER
            )
            self._search.SetDescriptiveText(_("Fulltext search"))
            self._search.SetToolTip(
                _("Type to search in all tools. Press Enter for next match.")
            )

        self._search.Bind(wx.EVT_TEXT, self.OnSearchModule)
        self._search.Bind(wx.EVT_TEXT_ENTER, self.OnEnter)

        if self._showTip:
            self._searchTip = StaticWrapText(
                parent=self, id=wx.ID_ANY, label="Choose a tool", size=(-1, 40)
            )

        if self._showChoice:
            self._searchChoice = wx.Choice(parent=self, id=wx.ID_ANY)
            self._searchChoice.SetItems(self._searchModule(keys=["command"], value=""))
            self._searchChoice.Bind(wx.EVT_CHOICE, self.OnSelectModule)

        self._layout()

    def _layout(self):
        """Do layout"""
        sizer = wx.BoxSizer(wx.HORIZONTAL)
        boxSizer = wx.BoxSizer(wx.VERTICAL)

        boxSizer.Add(self._search, flag=wx.EXPAND | wx.BOTTOM, border=5)
        if self._showChoice:
            hSizer = wx.BoxSizer(wx.HORIZONTAL)
            hSizer.Add(self._searchChoice, flag=wx.EXPAND | wx.BOTTOM, border=5)
            hSizer.AddStretchSpacer()
            boxSizer.Add(hSizer, flag=wx.EXPAND)
        if self._showTip:
            boxSizer.Add(self._searchTip, flag=wx.EXPAND)

        sizer.Add(boxSizer, proportion=1)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def OnEnter(self, event):
        """Process EVT_TEXT_ENTER to show search results"""
        self._showSearchResult()
        event.Skip()

    def _showSearchResult(self):
        if self._results:
            self._resultIndex += 1
            if self._resultIndex == len(self._results):
                self._resultIndex = 0
            self.showSearchResult.emit(result=self._results[self._resultIndex])

    def OnSearchModule(self, event):
        """Search module by keywords or description"""
        value = self._search.GetValue()
        if value == self._oldValue:
            event.Skip()
            return
        self._oldValue = value

        if len(value) <= 2:
            if len(value) == 0:  # reset
                commands = self._searchModule(keys=["command"], value="")
            else:
                self.showNotification.emit(
                    message=_("Searching, please type more characters.")
                )
                return
        else:
            commands = self._searchModule(keys=self._searchKeys, value=value)
        if self._showChoice:
            self._searchChoice.SetItems(commands)
            if commands:
                self._searchChoice.SetSelection(0)
                self.OnSelectModule()

        label = _("{} tools matched").format(len(commands))
        if self._showTip:
            self._searchTip.SetLabel(self._searchTip.GetLabel() + " [{}]".format(label))

        self.showNotification.emit(message=label)

        event.Skip()

    def _searchModule(self, keys, value):
        """Search modules by keys

        :param keys: list of keys
        :param value: pattern to match
        """
        nodes = set()
        for key in keys:
            nodes.update(self._model.SearchNodes(key=key, value=value))

        nodes = list(nodes)
        nodes.sort(key=lambda node: self._model.GetIndexOfNode(node))
        self._results = nodes
        self._resultIndex = -1
        return sorted([node.data["command"] for node in nodes if node.data["command"]])

    def OnSelectModule(self, event=None):
        """Module selected from choice, update command prompt"""
        cmd = self._searchChoice.GetStringSelection()
        self.moduleSelected.emit(name=cmd)

        if self._showTip:
            for module in self._results:
                if cmd == module.data["command"]:
                    self._searchTip.SetLabel(module.data["description"])
                    break

    def Reset(self):
        """Reset widget"""
        self._search.SetValue("")
        if self._showTip:
            self._searchTip.SetLabel("Choose a tool")


class ManageSettingsWidget(wx.Panel):
    """Widget which allows loading and saving settings into file."""

    def __init__(self, parent, settingsFile):
        """
        Signals:
            settingsChanged - called when users changes setting
                            - attribute 'data' with chosen setting data
            settingsSaving - called when settings are saving
                           - attribute 'name' with chosen settings name
            settingsLoaded - called when settings are loaded
                           - attribute 'settings' is dict with loaded settings
                             {nameofsetting : settingdata, ....}

        :param settingsFile: path to file, where settings will be saved and loaded from
        """
        self.settingsFile = settingsFile

        self.settingsChanged = Signal("ManageSettingsWidget.settingsChanged")
        self.settingsSaving = Signal("ManageSettingsWidget.settingsSaving")
        self.settingsLoaded = Signal("ManageSettingsWidget.settingsLoaded")

        wx.Panel.__init__(self, parent=parent, id=wx.ID_ANY)

        self.settingsBox = StaticBox(
            parent=self, id=wx.ID_ANY, label=" %s " % _("Profiles")
        )

        self.settingsChoice = wx.Choice(parent=self, id=wx.ID_ANY)
        self.settingsChoice.Bind(wx.EVT_CHOICE, self.OnSettingsChanged)
        self.btnSettingsSave = Button(parent=self, id=wx.ID_SAVE)
        self.btnSettingsSave.Bind(wx.EVT_BUTTON, self.OnSettingsSave)
        self.btnSettingsSave.SetToolTip(_("Save current settings"))
        self.btnSettingsDel = Button(parent=self, id=wx.ID_REMOVE)
        self.btnSettingsDel.Bind(wx.EVT_BUTTON, self.OnSettingsDelete)
        self.btnSettingsSave.SetToolTip(_("Delete currently selected settings"))

        # escaping with '$' character - index in self.esc_chars
        self.e_char_i = 0
        self.esc_chars = ["$", ";"]

        self._settings = self._loadSettings()  # -> self.settingsChoice.SetItems()
        self.settingsLoaded.emit(settings=self._settings)

        self.data_to_save = []

        self._layout()

        self.SetSizer(self.settingsSizer)
        self.settingsSizer.Fit(self)

    def _layout(self):
        self.settingsSizer = wx.StaticBoxSizer(self.settingsBox, wx.HORIZONTAL)
        self.settingsSizer.Add(
            StaticText(parent=self, id=wx.ID_ANY, label=_("Load:")),
            flag=wx.ALIGN_CENTER_VERTICAL | wx.RIGHT | wx.LEFT,
            border=5,
        )
        self.settingsSizer.Add(
            self.settingsChoice, proportion=1, flag=wx.EXPAND | wx.BOTTOM, border=3
        )
        self.settingsSizer.Add(
            self.btnSettingsSave, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM, border=3
        )
        self.settingsSizer.Add(self.btnSettingsDel, flag=wx.RIGHT | wx.BOTTOM, border=3)

    def OnSettingsChanged(self, event):
        """Load named settings"""
        name = event.GetString()
        if name not in self._settings:
            GError(parent=self, message=_("Settings <%s> not found") % name)
            return

        data = self._settings[name]
        self.settingsChanged.emit(data=data)

    def GetSettings(self):
        """Load named settings"""
        return self._settings.copy()

    def OnSettingsSave(self, event):
        """Save settings"""
        dlg = wx.TextEntryDialog(
            parent=self, message=_("Name:"), caption=_("Save settings")
        )
        if dlg.ShowModal() == wx.ID_OK:
            name = dlg.GetValue()
            if not name:
                GMessage(
                    parent=self, message=_("Name not given, settings is not saved.")
                )
            else:
                self.settingsSaving.emit(name=name)

            dlg.Destroy()

    def SaveSettings(self, name):
        # check if settings item already exists
        if name in self._settings:
            dlgOwt = wx.MessageDialog(
                self,
                message=_(
                    "Settings <%s> already exists. "
                    "Do you want to overwrite the settings?"
                )
                % name,
                caption=_("Save settings"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION,
            )
            if dlgOwt.ShowModal() != wx.ID_YES:
                dlgOwt.Destroy()
                return

        if self.data_to_save:
            self._settings[name] = self.data_to_save

        self._saveSettings()
        self.settingsChoice.SetStringSelection(name)

        self.data_to_save = []

    def _saveSettings(self):
        """Save settings and reload if successful"""
        if self._writeSettings() == 0:
            self._settings = self._loadSettings()

    def SetDataToSave(self, data):
        """Set data for setting, which will be saved.

        :param data: - list of strings, which will be saved
        """
        self.data_to_save = data

    def SetSettings(self, settings):
        """Set settings

        :param settings: - dict with all settings {nameofsetting : settingdata, ....}
        """
        self._settings = settings
        self._saveSettings()

    def AddSettings(self, settings):
        """Add settings

        :param settings: - dict with all settings {nameofsetting : settingdata, ....}
        """
        self._settings.update(settings)
        self._saveSettings()

    def OnSettingsDelete(self, event):
        """Save settings"""
        name = self.settingsChoice.GetStringSelection()
        if not name:
            GMessage(
                parent=self, message=_("No settings is defined. Operation canceled.")
            )
            return

        self._settings.pop(name)
        if self._writeSettings() == 0:
            self._settings = self._loadSettings()

    def _writeSettings(self):
        """Save settings into the file

        :return: 0 on success
        :return: -1 on failure
        """
        try:
            with open(self.settingsFile, "w") as fd:
                fd.write("format_version=2.0\n")
                for key, values in self._settings.items():
                    first = True
                    for v in values:
                        # escaping characters
                        for e_ch in self.esc_chars:
                            v = v.replace(e_ch, self.esc_chars[self.e_char_i] + e_ch)
                        if first:
                            # escaping characters
                            for e_ch in self.esc_chars:
                                key = key.replace(
                                    e_ch, self.esc_chars[self.e_char_i] + e_ch
                                )
                            fd.write("%s;%s;" % (key, v))
                            first = False
                        else:
                            fd.write("%s;" % (v))
                    fd.write("\n")

        except OSError:
            GError(parent=self, message=_("Unable to save settings"))
            return -1

        return 0

    def _loadSettings(self):
        """Load settings from the file

        The file is defined by self.SettingsFile.

        :return: parsed dict
        :return: empty dict on error
        """

        data = {}
        if not os.path.exists(self.settingsFile):
            return data

        try:
            with open(self.settingsFile) as fd:
                fd_lines = fd.readlines()

                if not fd_lines:
                    return data

                if fd_lines[0].strip() == "format_version=2.0":
                    data = self._loadSettings_v2(fd_lines)
                else:
                    data = self._loadSettings_v1(fd_lines)

                self.settingsChoice.SetItems(sorted(data.keys()))

        except OSError:
            return data

        self.settingsLoaded.emit(settings=data)
        return data

    def _loadSettings_v2(self, fd_lines):
        """Load settings from the file in format version 2.0

        The file is defined by self.SettingsFile.

        :return: parsed dict
        :return: empty dict on error
        """
        data = {}

        for line in fd_lines[1:]:
            try:
                lineData = []
                line = line.rstrip("\n")
                i_last_found = i_last = 0
                key = ""
                while True:
                    idx = line.find(";", i_last)
                    if idx < 0:
                        break
                    if idx != 0:
                        # find out whether it is separator
                        # $$$$; - it is separator
                        # $$$$$; - it is not separator
                        i_esc_chars = 0
                        while True:
                            if (
                                line[idx - (i_esc_chars + 1)]
                                == self.esc_chars[self.e_char_i]
                            ):
                                i_esc_chars += 1
                            else:
                                break
                        if i_esc_chars % 2 != 0:
                            i_last = idx + 1
                            continue

                    lineItem = line[i_last_found:idx]
                    # unescape characters
                    for e_ch in self.esc_chars:
                        lineItem = lineItem.replace(
                            self.esc_chars[self.e_char_i] + e_ch, e_ch
                        )
                    if i_last_found == 0:
                        key = lineItem
                    else:
                        lineData.append(lineItem)
                    i_last_found = i_last = idx + 1
                if key and lineData:
                    data[key] = lineData
            except ValueError:
                pass

        return data

    def _loadSettings_v1(self, fd_lines):
        """Load settings from the file in format version 1.0 (backward compatibility)

        The file is defined by self.SettingsFile.

        :return: parsed dict
        :return: empty dict on error
        """
        data = {}

        for line in fd_lines:
            try:
                lineData = line.rstrip("\n").split(";")
                if len(lineData) > 4:
                    # type, dsn, format, options
                    data[lineData[0]] = (
                        lineData[1],
                        lineData[2],
                        lineData[3],
                        lineData[4],
                    )
                else:
                    data[lineData[0]] = (lineData[1], lineData[2], lineData[3], "")
            except ValueError:
                pass

        return data


class PictureComboBox(OwnerDrawnComboBox):
    """Abstract class of ComboBox with pictures.

    Derived class has to specify has to specify _getPath method.
    """

    def OnDrawItem(self, dc, rect, item, flags):
        """Overridden from OwnerDrawnComboBox.

        Called to draw each item in the list.
        """
        if item == wx.NOT_FOUND:
            # painting the control, but there is no valid item selected yet
            return

        r = Rect(*rect)  # make a copy
        r.Deflate(3, 5)

        # for painting the items in the popup
        bitmap = self.GetPictureBitmap(self.GetString(item))
        if bitmap:
            dc.DrawBitmap(bitmap, r.x, r.y + (r.height - bitmap.GetHeight()) // 2)
            width = bitmap.GetWidth() + 10
        else:
            width = 0
        dc.DrawText(
            self.GetString(item),
            r.x + width,
            (r.y + 0) + (r.height - dc.GetCharHeight()) // 2,
        )

    def OnMeasureItem(self, item):
        """Overridden from OwnerDrawnComboBox, should return the height.

        Needed to display an item in the popup, or -1 for default.
        """
        return 24

    def GetPictureBitmap(self, name):
        """Returns bitmap for given picture name.

        :param str colorTable: name of color table
        """
        if not hasattr(self, "bitmaps"):
            self.bitmaps = {}

        if name in self.bitmaps:
            return self.bitmaps[name]

        path = self._getPath(name)
        if os.path.exists(path):
            bitmap = wx.Bitmap(path)
            self.bitmaps[name] = bitmap
            return bitmap
        return None


class ColorTablesComboBox(PictureComboBox):
    """ComboBox with drawn color tables (created by thumbnails.py).

    Used in r(3).colors dialog."""

    def _getPath(self, name):
        return os.path.join(
            os.getenv("GISBASE"), "docs", "html", "colortables", "%s.png" % name
        )


class BarscalesComboBox(PictureComboBox):
    """ComboBox with barscales for d.barscale."""

    def _getPath(self, name):
        return os.path.join(
            os.getenv("GISBASE"), "docs", "html", "barscales", name + ".png"
        )


class NArrowsComboBox(PictureComboBox):
    """ComboBox with north arrows for d.barscale."""

    def _getPath(self, name):
        return os.path.join(
            os.getenv("GISBASE"), "docs", "html", "northarrows", "%s.png" % name
        )


class LayersList(GListCtrl, listmix.TextEditMixin):
    """List of layers to be imported (dxf, shp...)"""

    def __init__(self, parent, columns, log=None):
        GListCtrl.__init__(self, parent)

        self.log = log
        self.row = None
        self.col = None
        self.output_map = None
        self.validate = True

        # setup mixins
        listmix.TextEditMixin.__init__(self)

        for i in range(len(columns)):
            self.InsertColumn(i, columns[i])

        width = []
        if len(columns) == 3:
            width = (65, 200)
        elif len(columns) == 4:
            width = (65, 200, 90)
        elif len(columns) == 5:
            width = (65, 180, 90, 70)

        for i in range(len(width)):
            self.SetColumnWidth(col=i, width=width[i])

    def OnLeftDown(self, event):
        """Allow editing only output name

        Code taken from TextEditMixin class.
        """
        x, y = event.GetPosition()

        colLocs = [0]
        loc = 0
        for n in range(self.GetColumnCount()):
            loc += self.GetColumnWidth(n)
            colLocs.append(loc)

        col = bisect(colLocs, x + self.GetScrollPos(wx.HORIZONTAL)) - 1

        if col == self.GetColumnCount() - 1:
            listmix.TextEditMixin.OnLeftDown(self, event)
        else:
            event.Skip()

    def GetLayers(self):
        """Get list of layers (layer name, output name, list id)"""
        layers = []

        data = self.GetData(checked=True)

        for itm in data:
            layer = itm[1]
            ftype = itm[2]
            if "/" in ftype:
                layer += "|%s" % ftype.split("/", 1)[0]
            output = itm[self.GetColumnCount() - 1]
            layers.append((layer, output, itm[-1]))

        return layers

    def ValidateOutputMapName(self):
        """Validate output map name"""
        wx.CallAfter(self.GetValidator().Validate, self)

    def OpenEditor(self, row, col):
        """Open editor"""
        self.col = col
        self.row = row
        super().OpenEditor(row, col)

    def CloseEditor(self, event=None):
        """Close editor"""
        if event:
            if event.IsCommandEvent():
                listmix.TextEditMixin.CloseEditor(self, event)
                if self.validate:
                    self.ValidateOutputMapName()
            event.Skip()
