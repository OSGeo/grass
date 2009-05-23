"""
@package prompt.py

@brief GRASS prompt

Classes:
 - GPrompt
 - PromptListCtrl
 - TextCtrlAutoComplete

(C) 2009 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
"""

import sys
import shlex

import wx
import wx.lib.mixins.listctrl as listmix

import globalvar
import utils

class GPrompt:
    """Interactive GRASS prompt"""
    def __init__(self, parent):
        self.parent = parent
                
        self.panel, self.input = self.__create()
        
    def __create(self):
        """Create widget"""
        cmdprompt = wx.Panel(self.parent)
        
        label = wx.Button(parent = cmdprompt, id = wx.ID_ANY,
                          label = _("Cmd >"), size = (-1, 25))
        label.SetToolTipString(_("Click for erasing command prompt"))
        
        cmdinput = TextCtrlAutoComplete(parent = cmdprompt, id = wx.ID_ANY,
                                        value = "",
                                        style = wx.TE_LINEWRAP | wx.TE_PROCESS_ENTER,
                                        size = (-1, 25))
        
        cmdinput.SetFont(wx.Font(10, wx.FONTFAMILY_MODERN, wx.NORMAL, wx.NORMAL, 0, ''))
        
        wx.CallAfter(cmdinput.SetInsertionPoint, 0)
        
        label.Bind(wx.EVT_BUTTON,        self.OnCmdErase)
        cmdinput.Bind(wx.EVT_TEXT_ENTER, self.OnRunCmd)
        cmdinput.Bind(wx.EVT_TEXT,       self.OnUpdateStatusBar)
        
        # layout
        sizer = wx.BoxSizer(wx.HORIZONTAL)
        sizer.Add(item = label, proportion = 0,
                  flag = wx.EXPAND | wx.LEFT | wx.RIGHT |
                  wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER,
                  border = 3)
        sizer.Add(item = cmdinput, proportion = 1,
                  flag = wx.EXPAND | wx.ALL,
                  border = 1)
        
        cmdprompt.SetSizer(sizer)
        sizer.Fit(cmdprompt)
        cmdprompt.Layout()
        
        return cmdprompt, cmdinput

    def GetPanel(self):
        """Get main widget panel"""
        return self.panel
    
    def OnCmdErase(self, event):
        """Erase command prompt"""
        self.input.SetValue('')
        
    def OnRunCmd(self, event):
        """Run command"""
        cmdString = event.GetString()
        
        if self.parent.GetName() != "LayerManager":
            return
        
        if cmdString[:2] == 'd.' and not self.parent.curr_page:
            self.parent.NewDisplay(show=True)
        
        cmd = shlex.split(str(cmdString))
        if len(cmd) > 1:
            self.parent.goutput.RunCmd(cmd, switchPage = True)
        else:
            self.parent.goutput.RunCmd(cmd, switchPage = False)
        
        self.OnUpdateStatusBar(None)
        
    def OnUpdateStatusBar(self, event):
        """Update Layer Manager status bar"""
        if self.parent.GetName() != "LayerManager":
            return
        
        if event is None:
            self.parent.statusbar.SetStatusText("")
        else:
            self.parent.statusbar.SetStatusText(_("Type GRASS command and run by pressing ENTER"))
            event.Skip()
        
class PromptListCtrl(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin):
    def __init__(self, parent, id = wx.ID_ANY, pos = wx.DefaultPosition,
                 size = wx.DefaultSize, style = 0):
        wx.ListCtrl.__init__(self, parent, id, pos, size, style)
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        
class TextCtrlAutoComplete(wx.TextCtrl, listmix.ColumnSorterMixin):
    def __init__ (self, parent, id = wx.ID_ANY, choices = [], **kwargs):
        """
        Constructor works just like wx.TextCtrl except you can pass in a
        list of choices.  You can also change the choice list at any time
        by calling setChoices.
        
        Inspired by http://wiki.wxpython.org/TextCtrlAutoComplete
        """
        if kwargs.has_key('style'):
            kwargs['style'] = wx.TE_PROCESS_ENTER | kwargs['style']
        else:
            kwargs['style'] = wx.TE_PROCESS_ENTER
        
        wx.TextCtrl.__init__(self, parent, id, **kwargs)
        
        # some variables
        self._choices = choices
        self._hideOnNoMatch = True
        self._screenheight = wx.SystemSettings.GetMetric(wx.SYS_SCREEN_Y)
        
        # sort variable needed by listmix
        self.itemDataMap = dict()
        
        # widgets
        self.dropdown = wx.PopupWindow(self)
        
        # create the list and bind the events
        self.dropdownlistbox = PromptListCtrl(parent = self.dropdown,
                                              style = wx.LC_REPORT | wx.LC_SINGLE_SEL | \
                                                  wx.LC_SORT_ASCENDING | wx.LC_NO_HEADER,
                                              pos = wx.Point(0, 0))
        
        listmix.ColumnSorterMixin.__init__(self, 1)
        
        # set choices (list of GRASS modules)
        self.SetChoices(globalvar.grassCmd['all'])

        # bindings...
        self.Bind(wx.EVT_KILL_FOCUS, self.OnControlChanged, self)
        self.Bind(wx.EVT_TEXT, self.OnEnteredText, self)
        self.Bind(wx.EVT_KEY_DOWN , self.OnKeyDown, self)

        # if need drop down on left click
        ### self.Bind ( wx.EVT_LEFT_DOWN , self.onClickToggleDown, self )
        ### self.Bind ( wx.EVT_LEFT_UP , self.onClickToggleUp, self )
        self.dropdown.Bind(wx.EVT_LISTBOX , self.OnListItemSelected, self.dropdownlistbox)
        self.dropdownlistbox.Bind(wx.EVT_LEFT_DOWN, self.OnListClick)
        self.dropdownlistbox.Bind(wx.EVT_LEFT_DCLICK, self.OnListDClick)
        self.dropdownlistbox.Bind(wx.EVT_LIST_COL_CLICK, self.OnListColClick)
        
    def _updateDataList(self, choices):
        """Update data list"""
        # delete, if need, all the previous data
        if self.dropdownlistbox.GetColumnCount() != 0:
            self.dropdownlistbox.DeleteAllColumns()
            self.dropdownlistbox.DeleteAllItems()
        # and update the dict
        if choices:
            for numVal, data in enumerate(choices):
                self.itemDataMap[numVal] = data
        else:
            numVal = 0
        self.SetColumnCount(numVal)
    
    def _setListSize(self):
        """Set list size"""
        choices = self._choices
        longest = 0
        for choice in choices:
            longest = max(len(choice), longest)
        longest += 3
        itemcount = min(len( choices ), 7) + 2
        charheight = self.dropdownlistbox.GetCharHeight()
        charwidth = self.dropdownlistbox.GetCharWidth()
        self.popupsize = wx.Size(charwidth*longest, charheight*itemcount)
        self.dropdownlistbox.SetSize(self.popupsize)
        self.dropdown.SetClientSize(self.popupsize)

    def _showDropDown(self, show = True):
        """
        Eit`her display the drop down list (show = True) or hide it
        (show = False).
        """
        if show:
            size = self.dropdown.GetSize()
            width, height = self.GetSizeTuple()
            x, y = self.ClientToScreenXY(0, height)
            if size.GetWidth() != width:
                size.SetWidth(width)
                self.dropdown.SetSize(size)
                self.dropdownlistbox.SetSize(self.dropdown.GetClientSize())
            if (y + size.GetHeight()) < self._screenheight:
                self.dropdown.SetPosition(wx.Point(x, y))
            else:
                self.dropdown.SetPosition(wx.Point(x, y - height - size.GetHeight()))
        
        self.dropdown.Show(show)
    
    def _listItemVisible(self):
        """
        Moves the selected item to the top of the list ensuring it is
        always visible.
        """
        toSel = self.dropdownlistbox.GetFirstSelected()
        if toSel == -1:
            return
        self.dropdownlistbox.EnsureVisible(toSel)
    
    def _setValueFromSelected(self):
         """
         Sets the wx.TextCtrl value from the selected wx.ListCtrl item.
         Will do nothing if no item is selected in the wx.ListCtrl.
         """
         sel = self.dropdownlistbox.GetFirstSelected()
         if sel > -1:
            if self._colFetch != -1:
                col = self._colFetch
            else:
                col = self._colSearch
            itemtext = self.dropdownlistbox.GetItem(sel, col).GetText()
            ### if self._selectCallback:
            ###    dd = self.dropdownlistbox
            ###    values = [dd.GetItem(sel, x).GetText()
            ###        for x in xrange(dd.GetColumnCount())]
            ###    self._selectCallback(values)
            self.SetValue(itemtext)
            self.SetInsertionPointEnd()
            
            self._showDropDown(False)
         
    def GetListCtrl(self):
        """Method required by listmix.ColumnSorterMixin"""
        return self.dropdownlistbox
    
    def SetChoices(self, choices):
        """
        Sets the choices available in the popup wx.ListBox.
        The items will be sorted case insensitively.
        """
        self._choices = choices
        
        self.dropdownlistbox.SetWindowStyleFlag(wx.LC_REPORT | wx.LC_SINGLE_SEL | \
                                                    wx.LC_SORT_ASCENDING | wx.LC_NO_HEADER)
        if not isinstance(choices, list):
            self._choices = [ x for x in choices ]
        utils.ListSortLower(self._choices)
        
        self._updateDataList(self._choices)
        
        self.dropdownlistbox.InsertColumn(0, "")
        for num, colVal in enumerate(self._choices):
            index = self.dropdownlistbox.InsertImageStringItem(sys.maxint, colVal, -1)
            self.dropdownlistbox.SetStringItem(index, 0, colVal)
            self.dropdownlistbox.SetItemData(index, num)
        self._setListSize()
        
        # there is only one choice for both search and fetch if setting a single column:
        self._colSearch = 0
        self._colFetch = -1
        
    def OnListClick(self, evt):
        """Left mouse button pressed"""
        toSel, flag = self.dropdownlistbox.HitTest( evt.GetPosition() )
        #no values on poition, return
        if toSel == -1: return
        self.dropdownlistbox.Select(toSel)

    def OnListDClick(self, evt):
        """Mouse button double click"""
        self._setValueFromSelected()

    def OnListColClick(self, evt):
        """Left mouse button pressed on column"""
        col = evt.GetColumn()
        # reverse the sort
        if col == self._colSearch:
            self._ascending = not self._ascending
        self.SortListItems( evt.GetColumn(), ascending=self._ascending )
        self._colSearch = evt.GetColumn()
        evt.Skip()

    def OnListItemSelected(self, event):
        """Item selected"""
        self._setValueFromSelected()
        event.Skip()

    def OnEnteredText(self, event):
        """Text entered"""
        text = event.GetString()
        
        ### if self._entryCallback:
        ###    self._entryCallback()
        if not text:
            # control is empty; hide dropdown if shown:
            if self.dropdown.IsShown():
                self._showDropDown(False)
            event.Skip()
            return
        
        found = False
        choices = self._choices
        for numCh, choice in enumerate(choices):
            ### if self._matchFunction and self._matchFunction(text, choice):
            ###    found = True
            ### elif
            if choice.lower().startswith(text.lower()):
                found = True
            if found:
                self._showDropDown(True)
                item = self.dropdownlistbox.GetItem(numCh)
                toSel = item.GetId()
                self.dropdownlistbox.Select(toSel)
                break
        
        if not found:
            self.dropdownlistbox.Select(self.dropdownlistbox.GetFirstSelected(), False)
            if self._hideOnNoMatch:
                self._showDropDown(False)
        
        self._listItemVisible()
        
        event.Skip()
        
    def OnKeyDown (self, event):
        """
        Do some work when the user press on the keys: up and down:
        move the cursor left and right: move the search
        """
        skip = True
        sel = self.dropdownlistbox.GetFirstSelected()
        visible = self.dropdown.IsShown()
        KC = event.GetKeyCode()
        if KC == wx.WXK_DOWN:
            if sel < (self.dropdownlistbox.GetItemCount() - 1):
                self.dropdownlistbox.Select(sel + 1)
                self._listItemVisible()
            self._showDropDown()
            skip = False
        elif KC == wx.WXK_UP:
            if sel > 0:
                self.dropdownlistbox.Select(sel - 1)
                self._listItemVisible()
            self._showDropDown ()
            skip = False
        elif KC == wx.WXK_LEFT:
            return
        elif KC == wx.WXK_RIGHT:
            return 
        if visible:
            if event.GetKeyCode() == wx.WXK_RETURN:
                self._setValueFromSelected()
                skip = False
            if event.GetKeyCode() == wx.WXK_ESCAPE:
                self._showDropDown(False)
                skip = False
        if skip :
            event.Skip()
        
    def OnControlChanged(self, event):
        """Control changed"""
        if self.IsShown():
            self._showDropDown(False)
        
        event.Skip()
