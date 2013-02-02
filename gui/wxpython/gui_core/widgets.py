"""!
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
 - widgets::IntegerValidator
 - widgets::FloatValidator
 - widgets::ItemTree
 - widgets::GListCtrl
 - widgets::SearchModuleWidget
 - widgets::ManageSettingsWidget

(C) 2008-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com> (Google SoC 2008/2010)
@author Enhancements by Michael Barton <michael.barton asu.edu>
@author Anna Kratochvilova <kratochanna gmail.com> (Google SoC 2011)
@author Stepan Turek <stepan.turek seznam.cz> (ManageSettingsWidget - created from GdalSelect)
"""

import os
import sys
import string

import wx
import wx.lib.mixins.listctrl as listmix
import wx.lib.scrolledpanel as SP
try:
    import wx.lib.agw.flatnotebook   as FN
except ImportError:
    import wx.lib.flatnotebook   as FN
try:
    from wx.lib.buttons import ThemedGenBitmapTextButton as BitmapTextButton
except ImportError: # not sure about TGBTButton version
    from wx.lib.buttons import GenBitmapTextButton as BitmapTextButton
try:
    import wx.lib.agw.customtreectrl as CT
except ImportError:
    import wx.lib.customtreectrl as CT

from core        import globalvar
from core.gcmd   import GMessage
from core.debug  import Debug
from core.events import gShowNotification

from wx.lib.newevent import NewEvent
wxSymbolSelectionChanged, EVT_SYMBOL_SELECTION_CHANGED  = NewEvent()

# ManageSettingsWidget
wxOnSettingsLoaded, EVT_SETTINGS_LOADED = NewEvent()
wxOnSettingsChanged, EVT_SETTINGS_CHANGED = NewEvent()
wxOnSettingsSaving, EVT_SETTINGS_SAVING = NewEvent()

class NotebookController:
    """!Provides handling of notebook page names.

    Translates page names to page indices.
    Class is aggregated in notebook subclasses.
    Notebook subclasses must delegate methods to controller.
    Methods inherited from notebook class must be delegated explicitly
    and other methods can be delegated by @c __getattr__.
    """
    def __init__(self, classObject, widget):
        """!        
        @param classObject notebook class name (object, i.e. FlatNotebook)
        @param widget notebook instance
        """
        self.notebookPages = {}
        self.classObject = classObject
        self.widget = widget
        self.highlightedTextEnd = _(" (...)")
        self.BindPageChanged()

    def BindPageChanged(self):
        """!Binds page changed event."""
        self.widget.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.OnRemoveHighlight)

    def AddPage(self, **kwargs):
        """!Add a new page
        """
        if 'name' in kwargs:
            self.notebookPages[kwargs['name']] = kwargs['page']
            del kwargs['name']

        self.classObject.AddPage(self.widget, **kwargs)

    def InsertPage(self, **kwargs):
        """!Insert a new page
        """
        if 'name' in kwargs:
            self.notebookPages[kwargs['name']] = kwargs['page']
            del kwargs['name']
        self.classObject.InsertPage(self.widget, **kwargs)

    def DeletePage(self, page):
        """!Delete page

        @param page name
        @return True if page was deleted, False if not exists
        """
        delPageIndex = self.GetPageIndexByName(page)
        if delPageIndex != -1:
            ret = self.classObject.DeletePage(self.widget, delPageIndex)
            if ret:
                del self.notebookPages[page]
            return ret
        else:
            return False

    def RemovePage(self, page):
        """!Delete page without deleting the associated window.

        @param page name
        @return True if page was deleted, False if not exists
        """
        delPageIndex = self.GetPageIndexByName(page)
        if delPageIndex != -1:
            ret = self.classObject.RemovePage(self.widget, delPageIndex)
            if ret:
                del self.notebookPages[page]
            return ret
        else:
            return False

    def SetSelectionByName(self, page):
        """!Set active notebook page.

        @param page name, eg. 'layers', 'output', 'search', 'pyshell', 'nviz'
        (depends on concrete notebook instance)
        """
        idx = self.GetPageIndexByName(page)
        if self.classObject.GetSelection(self.widget) != idx:
            self.classObject.SetSelection(self.widget, idx)

            self.RemoveHighlight(idx)

    def OnRemoveHighlight(self, event):
        """!Highlighted tab name should be removed."""
        page = event.GetSelection()
        self.RemoveHighlight(page)

    def RemoveHighlight(self, page):
        """!Removes highlight string from notebook tab name if necessary.

        @param page index
        """
        text = self.classObject.GetPageText(self.widget, page)
        if text.endswith(self.highlightedTextEnd):
            text = text.replace(self.highlightedTextEnd, '')
            self.classObject.SetPageText(self.widget, page, text)

    def GetPageIndexByName(self, page):
        """!Get notebook page index
        
        @param page name
        """
        if page not in self.notebookPages:
            return -1
        for pageIndex in range(self.classObject.GetPageCount(self.widget)):
            if self.notebookPages[page] == self.classObject.GetPage(self.widget, pageIndex):
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
        """!Sets image index for page

        @param page page name
        @param index image index (in wx.ImageList)
        """
        pageIndex = self.GetPageIndexByName(page)
        self.classObject.SetPageImage(self.widget, pageIndex, index)


class FlatNotebookController(NotebookController):
    """!Controller specialized for FN.FlatNotebook subclasses"""
    def __init__(self, classObject, widget):
        NotebookController.__init__(self, classObject, widget)

    def BindPageChanged(self):
        self.widget.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnRemoveHighlight)

    def GetPageIndexByName(self, page):
        """!Get notebook page index
        
        @param page name
        """
        if page not in self.notebookPages:
            return -1
        
        return self.classObject.GetPageIndex(self.widget, self.notebookPages[page])


class GNotebook(FN.FlatNotebook):
    """!Generic notebook widget.

    Enables advanced style settings.
    Problems with hidden tabs and does not respect system colors (native look).
    """
    def __init__(self, parent, style, **kwargs):
        if globalvar.hasAgw:
            FN.FlatNotebook.__init__(self, parent, id = wx.ID_ANY, agwStyle = style, **kwargs)
        else:
            FN.FlatNotebook.__init__(self, parent, id = wx.ID_ANY, style = style, **kwargs)
        
        self.controller = FlatNotebookController(classObject = FN.FlatNotebook, widget = self)

    def AddPage(self, **kwargs):
        """! @copydoc NotebookController::AddPage()"""
        self.controller.AddPage(**kwargs)

    def InsertPage(self, **kwargs):
        """! @copydoc NotebookController::InsertPage()"""
        self.controller.InsertPage(**kwargs)

    def DeletePage(self, page):
        """! @copydoc NotebookController::DeletePage()"""
        return self.controller.DeletePage(page)

    def RemovePage(self, page):
        """! @copydoc NotebookController::RemovePage()"""
        return self.controller.RemovePage(page)

    def SetPageImage(self, page, index):
        """!Does nothing because we don't want images for this style"""
        pass

    def __getattr__(self, name):
        return getattr(self.controller, name)

class FormNotebook(wx.Notebook):
    """!Notebook widget.

    Respects native look.
    """
    def __init__(self, parent, style):
        wx.Notebook.__init__(self, parent, id = wx.ID_ANY, style = style)
        self.controller = NotebookController(classObject = wx.Notebook, widget = self)

    def AddPage(self, **kwargs):
        """!@copydoc NotebookController::AddPage()"""
        self.controller.AddPage(**kwargs)

    def InsertPage(self, **kwargs):
        """! @copydoc NotebookController::InsertPage()"""
        self.controller.InsertPage(**kwargs)

    def DeletePage(self, page):
        """ @copydoc NotebookController::DeletePage()"""
        return self.controller.DeletePage(page)

    def RemovePage(self, page):
        """ @copydoc NotebookController::RemovePage()"""
        return self.controller.RemovePage(page)

    def SetPageImage(self, page, index):
        """! @copydoc NotebookController::SetPageImage()"""
        return self.controller.SetPageImage(page, index)

    def __getattr__(self, name):
        return getattr(self.controller, name)


class FormListbook(wx.Listbook):
    """!Notebook widget.

    Respects native look.
    """
    def __init__(self, parent, style):
        wx.Listbook.__init__(self, parent, id = wx.ID_ANY, style = style)
        self.controller = NotebookController(classObject = wx.Listbook, widget = self)
            
    def AddPage(self, **kwargs):
        """!@copydoc NotebookController::AddPage()"""
        self.controller.AddPage(**kwargs)

    def InsertPage(self, **kwargs):
        """! @copydoc NotebookController::InsertPage()"""
        self.controller.InsertPage(**kwargs)

    def DeletePage(self, page):
        """ @copydoc NotebookController::DeletePage()"""
        return self.controller.DeletePage(page)

    def RemovePage(self, page):
        """ @copydoc NotebookController::RemovePage()"""
        return self.controller.RemovePage(page)

    def SetPageImage(self, page, index):
        """! @copydoc NotebookController::SetPageImage()"""
        return self.controller.SetPageImage(page, index)

    def __getattr__(self, name):
        return getattr(self.controller, name)


class ScrolledPanel(SP.ScrolledPanel):
    """!Custom ScrolledPanel to avoid strange behaviour concerning focus"""
    def __init__(self, parent, style = wx.TAB_TRAVERSAL):
        SP.ScrolledPanel.__init__(self, parent = parent, id = wx.ID_ANY, style = style)

    def OnChildFocus(self, event):
        pass
        
class NumTextCtrl(wx.TextCtrl):
    """!Class derived from wx.TextCtrl for numerical values only"""
    def __init__(self, parent,  **kwargs):
##        self.precision = kwargs.pop('prec')
        wx.TextCtrl.__init__(self, parent = parent,
            validator = NTCValidator(flag = 'DIGIT_ONLY'), **kwargs)
        
            
    def SetValue(self, value):
        super(NumTextCtrl, self).SetValue( str(value))
        
    def GetValue(self):
        val = super(NumTextCtrl, self).GetValue()
        if val == '':
            val = '0'
        try:
            return float(val)
        except ValueError:
            val = ''.join(''.join(val.split('-')).split('.'))
            return float(val)
        
    def SetRange(self, min, max):
        pass
   
class FloatSlider(wx.Slider):
    """!Class derived from wx.Slider for floats"""
    def __init__(self, **kwargs):
        Debug.msg(1, "FloatSlider.__init__()")
        wx.Slider.__init__(self, **kwargs)
        self.coef = 1.
        #init range
        self.minValueOrig = 0
        self.maxValueOrig = 1
        
    def SetValue(self, value):
        value *= self.coef 
        if abs(value) < 1 and value != 0:
            while abs(value) < 1:
                value *= 100
                self.coef *= 100
            super(FloatSlider, self).SetRange(self.minValueOrig * self.coef, self.maxValueOrig * self.coef)
        super(FloatSlider, self).SetValue(value)
        
        Debug.msg(4, "FloatSlider.SetValue(): value = %f" % value)
        
    def SetRange(self, minValue, maxValue):
        self.coef = 1.
        self.minValueOrig = minValue
        self.maxValueOrig = maxValue
        if abs(minValue) < 1 or abs(maxValue) < 1:
            while (abs(minValue) < 1 and minValue != 0) or (abs(maxValue) < 1 and maxValue != 0):
                minValue *= 100
                maxValue *= 100
                self.coef *= 100
            super(FloatSlider, self).SetValue(super(FloatSlider, self).GetValue() * self.coef)
        super(FloatSlider, self).SetRange(minValue, maxValue)
        Debug.msg(4, "FloatSlider.SetRange(): minValue = %f, maxValue = %f" % (minValue, maxValue))
            
    def GetValue(self):
        val = super(FloatSlider, self).GetValue()
        Debug.msg(4, "FloatSlider.GetValue(): value = %f" % (val/self.coef))
        return val/self.coef
        
        
class SymbolButton(BitmapTextButton):
    """!Button with symbol and label."""
    def __init__(self, parent, usage, label, **kwargs):
        """!Constructor
        
        @param parent parent (usually wx.Panel)
        @param usage determines usage and picture
        @param label displayed label
        """
        size = (15, 15)
        buffer = wx.EmptyBitmap(*size)
        BitmapTextButton.__init__(self, parent = parent, label = " " + label, bitmap = buffer, **kwargs)
        
        dc = wx.MemoryDC()
        dc.SelectObject(buffer)
        maskColor = wx.Color(255, 255, 255)
        dc.SetBrush(wx.Brush(maskColor))
        dc.Clear()
        
        if usage == 'record':
            self.DrawRecord(dc, size)
        elif usage == 'stop':
            self.DrawStop(dc, size)
        elif usage == 'play':
            self.DrawPlay(dc, size)
        elif usage == 'pause':
            self.DrawPause(dc, size)

        if sys.platform != "win32":
            buffer.SetMaskColour(maskColor)
        self.SetBitmapLabel(buffer)
        dc.SelectObject(wx.NullBitmap)
        
    def DrawRecord(self, dc, size):
        """!Draw record symbol"""
        dc.SetBrush(wx.Brush(wx.Color(255, 0, 0)))
        dc.DrawCircle(size[0]/2, size[1] / 2, size[0] / 2)
        
    def DrawStop(self, dc, size):
        """!Draw stop symbol"""
        dc.SetBrush(wx.Brush(wx.Color(50, 50, 50)))
        dc.DrawRectangle(0, 0, size[0], size[1])
        
    def DrawPlay(self, dc, size):
        """!Draw play symbol"""
        dc.SetBrush(wx.Brush(wx.Color(0, 255, 0)))
        points = (wx.Point(0, 0), wx.Point(0, size[1]), wx.Point(size[0], size[1] / 2))
        dc.DrawPolygon(points)
        
    def DrawPause(self, dc, size):
        """!Draw pause symbol"""
        dc.SetBrush(wx.Brush(wx.Color(50, 50, 50)))
        dc.DrawRectangle(0, 0, 2 * size[0] / 5, size[1])
        dc.DrawRectangle(3 * size[0] / 5, 0, 2 * size[0] / 5, size[1])

class StaticWrapText(wx.StaticText):
    """!A Static Text field that wraps its text to fit its width,
    enlarging its height if necessary.
    """
    def __init__(self, parent, id = wx.ID_ANY, label = '', *args, **kwds):
        self.parent        = parent
        self.originalLabel = label
        
        wx.StaticText.__init__(self, parent, id, label = '', *args, **kwds)
        
        self.SetLabel(label)
        self.Bind(wx.EVT_SIZE, self.OnResize)
    
    def SetLabel(self, label):
        self.originalLabel = label
        self.wrappedSize = None
        self.OnResize(None)

    def OnResize(self, event):
        if not getattr(self, "resizing", False):
            self.resizing = True
            newSize = wx.Size(self.parent.GetSize().width - 50,
                              self.GetSize().height)
            if self.wrappedSize != newSize:
                wx.StaticText.SetLabel(self, self.originalLabel)
                self.Wrap(newSize.width)
                self.wrappedSize = newSize
                
                self.SetSize(self.wrappedSize)
            del self.resizing

class BaseValidator(wx.PyValidator):
    def __init__(self):
        wx.PyValidator.__init__(self)
        
        self.Bind(wx.EVT_TEXT, self.OnText) 

    def OnText(self, event):
        """!Do validation"""
        self.Validate()
        
        event.Skip()
        
    def Validate(self):
        """Validate input"""
        textCtrl = self.GetWindow()
        text = textCtrl.GetValue()

        if text:
            try:
                self.type(text)
            except ValueError:
                textCtrl.SetBackgroundColour("grey")
                textCtrl.SetFocus()
                textCtrl.Refresh()
                return False
        
        sysColor = wx.SystemSettings_GetColour(wx.SYS_COLOUR_WINDOW)
        textCtrl.SetBackgroundColour(sysColor)
        
        textCtrl.Refresh()
        
        return True

    def TransferToWindow(self):
        return True # Prevent wxDialog from complaining.
    
    def TransferFromWindow(self):
        return True # Prevent wxDialog from complaining.

class IntegerValidator(BaseValidator):
    """!Validator for floating-point input"""
    def __init__(self):
        BaseValidator.__init__(self)
        self.type = int
        
    def Clone(self):
        """!Clone validator"""
        return IntegerValidator()

class FloatValidator(BaseValidator):
    """!Validator for floating-point input"""
    def __init__(self):
        BaseValidator.__init__(self)
        self.type = float
        
    def Clone(self):
        """!Clone validator"""
        return FloatValidator()

class NTCValidator(wx.PyValidator):
    """!validates input in textctrls, taken from wxpython demo"""
    def __init__(self, flag = None):
        wx.PyValidator.__init__(self)
        self.flag = flag
        self.Bind(wx.EVT_CHAR, self.OnChar)

    def Clone(self):
        return NTCValidator(self.flag)

    def OnChar(self, event):
        key = event.GetKeyCode()
        if key < wx.WXK_SPACE or key == wx.WXK_DELETE or key > 255:
            event.Skip()
            return
        if self.flag == 'DIGIT_ONLY' and chr(key) in string.digits + '.-':
            event.Skip()
            return
        if not wx.Validator_IsSilent():
            wx.Bell()
        # Returning without calling even.Skip eats the event before it
        # gets to the text control
        return  

class SimpleValidator(wx.PyValidator):
    """ This validator is used to ensure that the user has entered something
        into the text object editor dialog's text field.
    """
    def __init__(self, callback):
        """ Standard constructor.
        """
        wx.PyValidator.__init__(self)
        self.callback = callback

    def Clone(self):
        """ Standard cloner.

        Note that every validator must implement the Clone() method.
        """
        return SimpleValidator(self.callback)

    def Validate(self, win):
        """ Validate the contents of the given text control.
        """
        ctrl = self.GetWindow()
        text = ctrl.GetValue()
        if len(text) == 0:
            self.callback(ctrl)
            return False
        else:
            return True

    def TransferToWindow(self):
        """ Transfer data from validator to window.

        The default implementation returns False, indicating that an error
        occurred.  We simply return True, as we don't do any data transfer.
        """
        return True # Prevent wxDialog from complaining.


    def TransferFromWindow(self):
        """ Transfer data from window to validator.

            The default implementation returns False, indicating that an error
            occurred.  We simply return True, as we don't do any data transfer.
        """
        return True # Prevent wxDialog from complaining.

class ItemTree(CT.CustomTreeCtrl):
    def __init__(self, parent, id = wx.ID_ANY,
                 ctstyle = CT.TR_HIDE_ROOT | CT.TR_FULL_ROW_HIGHLIGHT | CT.TR_HAS_BUTTONS |
                 CT.TR_LINES_AT_ROOT | CT.TR_SINGLE, **kwargs):
        if globalvar.hasAgw:
            super(ItemTree, self).__init__(parent, id, agwStyle = ctstyle, **kwargs)
        else:
            super(ItemTree, self).__init__(parent, id, style = ctstyle, **kwargs)
        
        self.root = self.AddRoot(_("Menu tree"))
        self.itemsMarked = [] # list of marked items
        self.itemSelected = None

    def SearchItems(self, element, value):
        """!Search item 

        @param element element index (see self.searchBy)
        @param value

        @return list of found tree items
        """
        items = list()
        if not value:
            return items
        
        item = self.GetFirstChild(self.root)[0]
        self._processItem(item, element, value, items)
        
        self.itemsMarked  = items
        self.itemSelected = None
        
        return items
    
    def _processItem(self, item, element, value, listOfItems):
        """!Search items (used by SearchItems)
        
        @param item reference item
        @param listOfItems list of found items
        """
        while item and item.IsOk():
            subItem = self.GetFirstChild(item)[0]
            if subItem:
                self._processItem(subItem, element, value, listOfItems)
            data = self.GetPyData(item)
            
            if data and element in data and \
                    value.lower() in data[element].lower():
                listOfItems.append(item)
            
            item = self.GetNextSibling(item)
            
    def GetSelected(self):
        """!Get selected item"""
        return self.itemSelected

    def OnShowItem(self, event):
        """!Highlight first found item in menu tree"""
        if len(self.itemsMarked) > 0:
            if self.GetSelected():
                self.ToggleItemSelection(self.GetSelected())
                idx = self.itemsMarked.index(self.GetSelected()) + 1
            else:
                idx = 0
            try:
                self.ToggleItemSelection(self.itemsMarked[idx])
                self.itemSelected = self.itemsMarked[idx]
                self.EnsureVisible(self.itemsMarked[idx])
            except IndexError:
                self.ToggleItemSelection(self.itemsMarked[0]) # reselect first item
                self.EnsureVisible(self.itemsMarked[0])
                self.itemSelected = self.itemsMarked[0]
        else:
            for item in self.root.GetChildren():
                self.Collapse(item)
            itemSelected = self.GetSelection()
            if itemSelected:
                self.ToggleItemSelection(itemSelected)
            self.itemSelected = None

class SingleSymbolPanel(wx.Panel):
    """!Panel for displaying one symbol.
    
    Changes background when selected. Assumes that parent will catch
    events emitted on mouse click. Used in gui_core::dialog::SymbolDialog.
    """
    def __init__(self, parent, symbolPath):
        """!Panel constructor
        
        @param parent parent (gui_core::dialog::SymbolDialog)
        @param symbolPath absolute path to symbol
        """
        wx.Panel.__init__(self, parent, id = wx.ID_ANY, style = wx.BORDER_RAISED)
        self.SetName(os.path.splitext(os.path.basename(symbolPath))[0])
        self.sBmp = wx.StaticBitmap(self, wx.ID_ANY, wx.Bitmap(symbolPath))

        self.selected = False
        self.selectColor = wx.SystemSettings.GetColour(wx.SYS_COLOUR_HIGHLIGHT)
        self.deselectColor = wx.SystemSettings.GetColour(wx.SYS_COLOUR_WINDOW)
        
        sizer = wx.BoxSizer()
        sizer.Add(item = self.sBmp, proportion = 0, flag = wx.ALL | wx.ALIGN_CENTER, border = 5)
        self.SetBackgroundColour(self.deselectColor)
        self.SetMinSize(self.GetBestSize())
        self.SetSizerAndFit(sizer)
        
        # binding to both (staticBitmap, Panel) necessary
        self.sBmp.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        self.Bind(wx.EVT_LEFT_DOWN, self.OnLeftDown)
        self.Bind(wx.EVT_LEFT_DCLICK, self.OnDoubleClick)
        self.sBmp.Bind(wx.EVT_LEFT_DCLICK, self.OnDoubleClick)
        
    def OnLeftDown(self, event):
        """!Panel selected, background changes"""
        self.selected = True
        self.SetBackgroundColour(self.selectColor)
        event.Skip()
        
        event = wxSymbolSelectionChanged(name = self.GetName(), doubleClick = False)
        wx.PostEvent(self.GetParent(), event)
        
    def OnDoubleClick(self, event):
        event = wxSymbolSelectionChanged(name = self.GetName(), doubleClick = True)
        wx.PostEvent(self.GetParent(), event)
        
    def Deselect(self):
        """!Panel deselected, background changes back to default"""
        self.selected = False
        self.SetBackgroundColour(self.deselectColor)
        
    def Select(self):
        """!Select panel, no event emitted"""
        self.selected = True
        self.SetBackgroundColour(self.selectColor)
        
class GListCtrl(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin, listmix.CheckListCtrlMixin):
    """!Generic ListCtrl with popup menu to select/deselect all
    items"""
    def __init__(self, parent):
        self.parent = parent
        
        wx.ListCtrl.__init__(self, parent, id = wx.ID_ANY,
                             style = wx.LC_REPORT)
        listmix.CheckListCtrlMixin.__init__(self)
        
        # setup mixins
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        
        self.Bind(wx.EVT_COMMAND_RIGHT_CLICK, self.OnPopupMenu) #wxMSW
        self.Bind(wx.EVT_RIGHT_UP,            self.OnPopupMenu) #wxGTK

    def LoadData(self):
        """!Load data into list"""
        pass

    def OnPopupMenu(self, event):
        """!Show popup menu"""
        if self.GetItemCount() < 1:
            return
        
        if not hasattr(self, "popupDataID1"):
            self.popupDataID1 = wx.NewId()
            self.popupDataID2 = wx.NewId()
            
            self.Bind(wx.EVT_MENU, self.OnSelectAll,  id = self.popupDataID1)
            self.Bind(wx.EVT_MENU, self.OnSelectNone, id = self.popupDataID2)
        
        # generate popup-menu
        menu = wx.Menu()
        menu.Append(self.popupDataID1, _("Select all"))
        menu.Append(self.popupDataID2, _("Deselect all"))
        
        self.PopupMenu(menu)
        menu.Destroy()

    def OnSelectAll(self, event):
        """!Select all items"""
        item = -1
        
        while True:
            item = self.GetNextItem(item)
            if item == -1:
                break
            self.CheckItem(item, True)
        
        event.Skip()
        
    def OnSelectNone(self, event):
        """!Deselect items"""
        item = -1
        
        while True:
            item = self.GetNextItem(item, wx.LIST_STATE_SELECTED)
            if item == -1:
                break
            self.CheckItem(item, False)
        
        event.Skip()


gModuleSelected, EVT_MODULE_SELECTED = NewEvent()


class SearchModuleWidget(wx.Panel):
    """!Search module widget (used in SearchModuleWindow)"""
    def __init__(self, parent, modulesData, id = wx.ID_ANY,
                 showChoice = True, showTip = False, **kwargs):
        self.showTip = showTip
        self.showChoice = showChoice
        self.modulesData = modulesData

        wx.Panel.__init__(self, parent = parent, id = id, **kwargs)

        self._searchDict = { _('description') : 'description',
                             _('command') : 'command',
                             _('keywords') : 'keywords' }
        
        self.box = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                label = " %s " % _("Find module - (press Enter for next match)"))

        self.searchBy = wx.Choice(parent = self, id = wx.ID_ANY)
        items = [_('description'), _('keywords'), _('command')]
        datas = ['description', 'keywords', 'command']
        for item, data in zip(items, datas):
            self.searchBy.Append(item = item, clientData = data)
        self.searchBy.SetSelection(0)

        self.search = wx.SearchCtrl(parent = self, id = wx.ID_ANY,
                                    size = (-1, 25), style = wx.TE_PROCESS_ENTER)
        self.search.Bind(wx.EVT_TEXT, self.OnSearchModule)

        if self.showTip:
            self.searchTip = StaticWrapText(parent = self, id = wx.ID_ANY,
                                            size = (-1, 35))

        if self.showChoice:
            self.searchChoice = wx.Choice(parent = self, id = wx.ID_ANY)
            self.searchChoice.SetItems(self.modulesData.GetCommandItems())
            self.searchChoice.Bind(wx.EVT_CHOICE, self.OnSelectModule)

        self._layout()

    def _layout(self):
        """!Do layout"""
        sizer = wx.StaticBoxSizer(self.box, wx.HORIZONTAL)
        gridSizer = wx.GridBagSizer(hgap = 3, vgap = 3)
        
        gridSizer.Add(item = self.searchBy,
                      flag = wx.ALIGN_CENTER_VERTICAL, pos = (0, 0))
        gridSizer.Add(item = self.search,
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, pos = (0, 1))
        row = 1
        if self.showChoice:
            gridSizer.Add(item = self.searchChoice,
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, pos = (row, 0), span = (1, 2))
            row += 1
        if self.showTip:
            gridSizer.Add(item = self.searchTip,
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND, pos = (row, 0), span = (1, 2))
            row += 1

        gridSizer.AddGrowableCol(1)

        sizer.Add(item = gridSizer, proportion = 1)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def GetCtrl(self):
        """!Get SearchCtrl widget"""
        return self.search

    def GetSelection(self):
        """!Get selected element"""
        selection = self.searchBy.GetStringSelection()

        return self._searchDict[selection]

    def SetSelection(self, i):
        """!Set selection element"""
        self.searchBy.SetSelection(i)

    def OnSearchModule(self, event):
        """!Search module by keywords or description"""

        text = event.GetEventObject().GetValue()
        if not text:
            self.modulesData.SetFilter()
            mList = self.modulesData.GetCommandItems()
            if self.showChoice:
                self.searchChoice.SetItems(mList)
            label = _("%d modules found") % len(mList)
        else:
            findIn = self.searchBy.GetClientData(self.searchBy.GetSelection())
            modules, nFound = self.modulesData.FindModules(text = text, findIn = findIn)
            self.modulesData.SetFilter(modules)
            if self.showChoice:
                self.searchChoice.SetItems(self.modulesData.GetCommandItems())
                self.searchChoice.SetSelection(0)
            label = _("%d modules match") % nFound

        if self.showTip:
            self.searchTip.SetLabel(label)

        newEvent = gShowNotification(self.GetId(), message = label)
        wx.PostEvent(self, newEvent)

        event.Skip()

    def OnSelectModule(self, event):
        """!Module selected from choice, update command prompt"""
        cmd  = event.GetString().split(' ', 1)[0]

        moduleEvent = gModuleSelected(name = cmd)
        wx.PostEvent(self, moduleEvent)

        desc = self.modulesData.GetCommandDesc(cmd)
        if self.showTip:
            self.searchTip.SetLabel(desc)

    def Reset(self):
        """!Reset widget"""
        self.searchBy.SetSelection(0)
        self.search.SetValue('')
        if self.showTip:
            self.searchTip.SetLabel('')

class ManageSettingsWidget(wx.Panel):
    """!Widget which allows loading and saving settings into file."""
    def __init__(self, parent, settingsFile, id = wx.ID_ANY):
        """
        Events:
            EVT_SETTINGS_CHANGED - called when users changes setting
                                - event object has attribute 'data', with chosen setting data
            EVT_SETTINGS_SAVING - called when settings are saving
                                - If you bind instance of ManageSettingsWidget with this event,
                                  you can use SetDataToSave method to set data for save and then call 
                                  Skip() to save the data.
                                  If you do not call Skip(), the data will not be saved. 
            EVT_SETTINGS_LOADED - called when settings are loaded
                                - event object has attribute 'settings', which is dict with loaded settings
                                  {nameofsetting : settingdata, ....}

        @param settingsFile - path to file, where settings will be saved and loaded from
        """
        self.settingsFile = settingsFile

        wx.Panel.__init__(self, parent = parent, id = wx.ID_ANY)

        self.settingsBox = wx.StaticBox(parent = self, id = wx.ID_ANY,
                                        label = " %s " % _("Settings"))
        
        self.settingsChoice = wx.Choice(parent = self, id = wx.ID_ANY)
        self.settingsChoice.Bind(wx.EVT_CHOICE, self.OnSettingsChanged)
        self.btnSettingsSave = wx.Button(parent = self, id = wx.ID_SAVE)
        self.btnSettingsSave.Bind(wx.EVT_BUTTON, self.OnSettingsSave)
        self.btnSettingsSave.SetToolTipString(_("Save current settings"))
        self.btnSettingsDel = wx.Button(parent = self, id = wx.ID_REMOVE)
        self.btnSettingsDel.Bind(wx.EVT_BUTTON, self.OnSettingsDelete)
        self.btnSettingsSave.SetToolTipString(_("Delete currently selected settings"))

        self.Bind(EVT_SETTINGS_SAVING, self.OnSettingsSaving)

        # escaping with '$' character - index in self.esc_chars
        self.e_char_i = 0
        self.esc_chars = ['$', ';']

        self._settings = self._loadSettings() # -> self.settingsChoice.SetItems()
        event = wxOnSettingsLoaded(settings = self._settings)
        wx.PostEvent(self, event)

        self.data_to_save = []

        self._layout()

    def _layout(self):

        settingsSizer = wx.StaticBoxSizer(self.settingsBox, wx.HORIZONTAL)
        settingsSizer.Add(item = wx.StaticText(parent = self,
                                               id = wx.ID_ANY,
                                               label = _("Load settings:")),
                          flag = wx.ALIGN_CENTER_VERTICAL | wx.RIGHT,
                          border  = 5)
        settingsSizer.Add(item = self.settingsChoice,
                          proportion = 1,
                          flag = wx.EXPAND)
        settingsSizer.Add(item = self.btnSettingsSave,
                          flag = wx.LEFT | wx.RIGHT,
                          border = 5)
        settingsSizer.Add(item = self.btnSettingsDel,
                          flag = wx.RIGHT,
                          border = 5)

        self.SetSizer(settingsSizer)
        settingsSizer.Fit(self)

    def OnSettingsChanged(self, event):
        """!Load named settings"""
        name = event.GetString()
        if name not in self._settings:
            GError(parent = self,
                   message = _("Settings <%s> not found") % name)
            return

        data = self._settings[name]
        event = wxOnSettingsChanged(data = data)
        wx.PostEvent(self, event)

    def OnSettingsSave(self, event):
        """!Save settings"""
        dlg = wx.TextEntryDialog(parent = self,
                                 message = _("Name:"),
                                 caption = _("Save settings"))
        if dlg.ShowModal() != wx.ID_OK:
            return
        
        # check required params
        if not dlg.GetValue():
            GMessage(parent = self,
                     message = _("Name not given, settings is not saved."))
            return

        name = dlg.GetValue()

        event = wxOnSettingsSaving(name = name, dlg = dlg)
        wx.PostEvent(self, event)
  
    def OnSettingsSaving(self, event):
        # check if settings item already exists
        if event.name in self._settings:
            dlgOwt = wx.MessageDialog(self, message = _("Settings <%s> already exists. "
                                                        "Do you want to overwrite the settings?") % event.name,
                                      caption = _("Save settings"), style = wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
            if dlgOwt.ShowModal() != wx.ID_YES:
                dlgOwt.Destroy()
                return

        if self.data_to_save:
            self._settings[event.name] = self.data_to_save

        self.SaveSettings()
        self.settingsChoice.SetStringSelection(event.name)

        self.data_to_save = []
        event.dlg.Destroy()
 
    def SaveSettings(self):
        """!Save settings"""
        if self._saveSettings() == 0:
            self._settings = self._loadSettings()

    def SetDataToSave(self, data):
        """!Set data for setting, which will be saved.

        @param data - list of strings, which will be saved
        """
        self.data_to_save = data

    def SetSettings(self, settings):
        """!Set settings

        @param settings - dict with all settigs {nameofsetting : settingdata, ....}
        """
        self._settings = settings
        self.SaveSettings()

    def OnSettingsDelete(self, event):
        """!Save settings
        """
        name = self.settingsChoice.GetStringSelection()
        if not name:
            GMessage(parent = self,
                     message = _("No settings is defined. Operation canceled."))
            return
        
        self._settings.pop(name)
        if self._saveSettings() == 0:
            self._settings = self._loadSettings()
        
    def _saveSettings(self):
        """!Save settings into the file

        @return 0 on success
        @return -1 on failure
        """
        try:
            fd = open(self.settingsFile, 'w')
            fd.write('format_version=2.0\n')
            for key, values in self._settings.iteritems():
                first = True
                for v in values:
                    # escaping characters
                    for e_ch in self.esc_chars:
                        v = v.replace(e_ch, self.esc_chars[self.e_char_i] + e_ch)
                    if first:
                        # escaping characters
                        for e_ch in self.esc_chars:
                            key = key.replace(e_ch, self.esc_chars[self.e_char_i] + e_ch)
                        fd.write('%s;%s;' % (key, v))
                        first = False
                    else:
                        fd.write('%s;' % (v))
                fd.write('\n')

        except IOError:
            GError(parent = self,
                   message = _("Unable to save settings"))
            return -1
        fd.close()
        
        return 0

    def _loadSettings(self):
        """!Load settings from the file

        The file is defined by self.SettingsFile.
        
        @return parsed dict
        @return empty dict on error
        """

        data = dict()
        if not os.path.exists(self.settingsFile):
            return data

        try:
            fd = open(self.settingsFile, 'r')
        except IOError:
            return data

        fd_lines = fd.readlines()

        if not fd_lines:
            fd.close()
            return data

        if fd_lines[0].strip() == 'format_version=2.0':
            data = self._loadSettings_v2(fd_lines)
        else:
            data = self._loadSettings_v1(fd_lines)

        self.settingsChoice.SetItems(sorted(data.keys()))
        fd.close()

        event = wxOnSettingsLoaded(settings = data)
        wx.PostEvent(self, event)

        return data

    def _loadSettings_v2(self, fd_lines):
        """Load settings from the file in format version 2.0

        The file is defined by self.SettingsFile.
        
        @return parsed dict
        @return empty dict on error
        """
        data = dict()
        
        for line in fd_lines[1:]:
            try:
                lineData = []
                line = line.rstrip('\n')
                i_last_found = i_last = 0
                key = ''
                while True:
                    idx = line.find(';', i_last)
                    if idx < 0:
                        break
                    elif idx != 0:

                        # find out whether it is separator
                        # $$$$; - it is separator
                        # $$$$$; - it is not separator
                        i_esc_chars = 0
                        while True:
                            if line[idx - (i_esc_chars + 1)] == self.esc_chars[self.e_char_i]:
                                i_esc_chars += 1
                            else: 
                                break
                        if i_esc_chars%2 != 0:
                            i_last = idx + 1
                            continue

                    lineItem = line[i_last_found : idx]
                    # unescape characters
                    for e_ch in self.esc_chars:
                        lineItem = lineItem.replace(self.esc_chars[self.e_char_i] + e_ch, e_ch)
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
        """!Load settings from the file in format version 1.0 (backward compatibility)

        The file is defined by self.SettingsFile.
        
        @return parsed dict
        @return empty dict on error
        """
        data = dict()
      
        for line in fd_lines:
            try:
                lineData = line.rstrip('\n').split(';')
                if len(lineData) > 4:
                    # type, dsn, format, options
                    data[lineData[0]] = (lineData[1], lineData[2], lineData[3], lineData[4])
                else:
                    data[lineData[0]] = (lineData[1], lineData[2], lineData[3], '')
            except ValueError:
                pass
        
        return data
