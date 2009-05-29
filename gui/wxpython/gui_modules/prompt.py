"""!
@package prompt.py

@brief GRASS prompt

Classes:
 - GPrompt
 - PromptListCtrl
 - TextCtrlAutoComplete

@todo: fix TextCtrlAutoComplete to work also on Macs (missing
wx.PopupWindow())

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

from grass.script import core as grass

import globalvar
import utils
import menuform
import menudata

class GPrompt:
    """!Interactive GRASS prompt"""
    def __init__(self, parent):
        self.parent = parent # GMFrame
        
        # dictionary of modules (description, keywords, ...)
        self.modules = self.parent.menudata.GetModules()
        
        self.panel, self.input = self.__create()
        
    def __create(self):
        """!Create widget"""
        cmdprompt = wx.Panel(self.parent)
        
        #
        # search
        #
        searchTxt = wx.StaticText(parent = cmdprompt, id = wx.ID_ANY,
                                  label = _("Search:"))
        
        self.searchBy = wx.Choice(parent = cmdprompt, id = wx.ID_ANY,
                             choices = [_("description"),
                                        _("keywords")])
        
        self.search = wx.TextCtrl(parent = cmdprompt, id = wx.ID_ANY,
                             value = "", size = (-1, 25))
        
        label = wx.Button(parent = cmdprompt, id = wx.ID_ANY,
                          label = _("Cmd >"), size = (-1, 25))
        label.SetToolTipString(_("Click for erasing command prompt"))

        ### todo: fix TextCtrlAutoComplete to work also on Macs
        ### reason: missing wx.PopupWindow()
        try:
            cmdinput = TextCtrlAutoComplete(parent = cmdprompt, id = wx.ID_ANY,
                                            value = "",
                                            style = wx.TE_LINEWRAP | wx.TE_PROCESS_ENTER,
                                            size = (-1, 25))
        except NotImplementedError:
            # wx.PopupWindow may be not available in wxMac
            # see http://trac.wxwidgets.org/ticket/9377
            cmdinput = wx.TextCtrl(parent = cmdprompt, id = wx.ID_ANY,
                                   value = "",
                                   style=wx.TE_LINEWRAP | wx.TE_PROCESS_ENTER,
                                   size = (-1, 25))
            self.searchBy.Enable(False)
            self.search.Enable(False)
        
        cmdinput.SetFont(wx.Font(10, wx.FONTFAMILY_MODERN, wx.NORMAL, wx.NORMAL, 0, ''))
        
        wx.CallAfter(cmdinput.SetInsertionPoint, 0)
        
        # bidnings
        label.Bind(wx.EVT_BUTTON,        self.OnCmdErase)
        cmdinput.Bind(wx.EVT_TEXT_ENTER, self.OnRunCmd)
        cmdinput.Bind(wx.EVT_TEXT,       self.OnUpdateStatusBar)
        self.search.Bind(wx.EVT_TEXT,    self.OnSearchModule)
        
        # layout
        sizer = wx.GridBagSizer(hgap=5, vgap=5)
        sizer.AddGrowableCol(2)

        sizer.Add(item = searchTxt,
                  flag = wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL,
                  pos = (0, 0))

        sizer.Add(item = self.searchBy,
                  flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER,
                  pos = (0, 1))
        
        sizer.Add(item = self.search,
                  flag = wx.EXPAND | wx.RIGHT | wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER,
                  border = 5,
                  pos = (0, 2))
        
        sizer.Add(item = label, 
                  flag = wx.LEFT | wx.EXPAND | wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_CENTER,
                  border = 5,
                  pos = (1, 0))
        
        sizer.Add(item = cmdinput,
                  flag = wx.EXPAND | wx.RIGHT,
                  border = 5,
                  pos = (1, 1), span = (1, 2))
        
        cmdprompt.SetSizer(sizer)
        sizer.Fit(cmdprompt)
        cmdprompt.Layout()
        
        return cmdprompt, cmdinput

    def GetPanel(self):
        """!Get main widget panel"""
        return self.panel

    def GetInput(self):
        """!Get main prompt widget"""
        return self.input
    
    def OnCmdErase(self, event):
        """!Erase command prompt"""
        self.input.SetValue('')
        
    def OnRunCmd(self, event):
        """!Run command"""
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
        """!Update Layer Manager status bar"""
        if self.parent.GetName() != "LayerManager":
            return
        
        if event is None:
            self.parent.statusbar.SetStatusText("")
        else:
            self.parent.statusbar.SetStatusText(_("Type GRASS command and run by pressing ENTER"))
            event.Skip()
        
    def OnSearchModule(self, event):
        """!Search module by metadata"""
        text = event.GetString()
        if not text:
            self.input.SetChoices(globalvar.grassCmd['all'])
            return
        
        modules = []
        for module, data in self.modules.iteritems():
            if text in data['desc']:
                modules.append(module)
        
        self.parent.statusbar.SetStatusText(_("%d modules found") % len(modules))
        self.input.SetChoices(modules)
                    
class PromptListCtrl(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin):
    def __init__(self, parent, id = wx.ID_ANY, pos = wx.DefaultPosition,
                 size = wx.DefaultSize, style = 0):
        wx.ListCtrl.__init__(self, parent, id, pos, size, style)
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        
class TextCtrlAutoComplete(wx.TextCtrl, listmix.ColumnSorterMixin):
    def __init__ (self, parent, id = wx.ID_ANY, choices = [], **kwargs):
        """!Constructor works just like wx.TextCtrl except you can pass in a
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
        self._module = None # currently selected module
        self._choiceType = None # type of choice (module, params, flags, raster, vector ...)
        self._screenheight = wx.SystemSettings.GetMetric(wx.SYS_SCREEN_Y)
        
        # sort variable needed by listmix
        self.itemDataMap = dict()
        
        # widgets
        try:
            self.dropdown = wx.PopupWindow(self)
        except NotImplementedError:
            self.Destroy()
            raise NotImplementedError
        
        # create the list and bind the events
        self.dropdownlistbox = PromptListCtrl(parent = self.dropdown,
                                              style = wx.LC_REPORT | wx.LC_SINGLE_SEL | \
                                                  wx.LC_SORT_ASCENDING | wx.LC_NO_HEADER,
                                              pos = wx.Point(0, 0))
        
        listmix.ColumnSorterMixin.__init__(self, 1)
        
        # set choices (list of GRASS modules)
        self._choicesCmd = globalvar.grassCmd['all']
        self._choicesMap = dict()
        for type in ('raster', 'vector'):
            self._choicesMap[type] = grass.list_strings(type = type[:4])
        # first search for GRASS module
        self.SetChoices(self._choicesCmd)

        # bindings...
        self.Bind(wx.EVT_KILL_FOCUS, self.OnControlChanged, self)
        self.Bind(wx.EVT_TEXT, self.OnEnteredText, self)
        self.Bind(wx.EVT_KEY_DOWN , self.OnKeyDown, self)

        # if need drop down on left click
        self.dropdown.Bind(wx.EVT_LISTBOX , self.OnListItemSelected, self.dropdownlistbox)
        self.dropdownlistbox.Bind(wx.EVT_LEFT_DOWN, self.OnListClick)
        self.dropdownlistbox.Bind(wx.EVT_LEFT_DCLICK, self.OnListDClick)
        self.dropdownlistbox.Bind(wx.EVT_LIST_COL_CLICK, self.OnListColClick)
        
    def _updateDataList(self, choices):
        """!Update data list"""
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
        """!Set list size"""
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
        """!Either display the drop down list (show = True) or hide it
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
        """!Moves the selected item to the top of the list ensuring it is
        always visible.
        """
        toSel = self.dropdownlistbox.GetFirstSelected()
        if toSel == -1:
            return
        self.dropdownlistbox.EnsureVisible(toSel)
    
    def _setValueFromSelected(self):
         """!Sets the wx.TextCtrl value from the selected wx.ListCtrl item.
         Will do nothing if no item is selected in the wx.ListCtrl.
         """
         sel = self.dropdownlistbox.GetFirstSelected()
         if sel > -1:
            if self._colFetch != -1:
                col = self._colFetch
            else:
                col = self._colSearch
            itemtext = self.dropdownlistbox.GetItem(sel, col).GetText()
            
            cmd = shlex.split(str(self.GetValue()))
            if len(cmd) > 1:
                # -> append text (skip last item)
                if self._choiceType == 'param':
                    self.SetValue(' '.join(cmd[:-1]) + ' ' + itemtext + '=')
                    optType = self._module.get_param(itemtext)['prompt']
                    if optType in ('raster', 'vector'):
                        # -> raster/vector map
                        self.SetChoices(self._choicesMap[optType], optType)
                elif self._choiceType == 'flag':
                    if len(itemtext) > 1:
                        prefix = '--'
                    else:
                        prefix = '-'
                    self.SetValue(' '.join(cmd[:-1]) + ' ' + prefix + itemtext)
                elif self._choiceType in ('raster', 'vector'):
                    self.SetValue(' '.join(cmd[:-1]) + ' ' + cmd[-1].split('=', 1)[0] + '=' + itemtext)
            else:
                # -> reset text
                self.SetValue(itemtext + ' ')
            self.SetInsertionPointEnd()
            
            self._showDropDown(False)
         
    def GetListCtrl(self):
        """!Method required by listmix.ColumnSorterMixin"""
        return self.dropdownlistbox
    
    def SetChoices(self, choices, type = 'module'):
        """!Sets the choices available in the popup wx.ListBox.
        The items will be sorted case insensitively.
        """
        self._choices = choices
        self._choiceType = type
        
        self.dropdownlistbox.SetWindowStyleFlag(wx.LC_REPORT | wx.LC_SINGLE_SEL | \
                                                    wx.LC_SORT_ASCENDING | wx.LC_NO_HEADER)
        if not isinstance(choices, list):
            self._choices = [ x for x in choices ]
        if self._choiceType not in ('raster', 'vector'):
            # do not sort raster/vector maps
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
        """!Left mouse button pressed"""
        toSel, flag = self.dropdownlistbox.HitTest( evt.GetPosition() )
        #no values on poition, return
        if toSel == -1: return
        self.dropdownlistbox.Select(toSel)

    def OnListDClick(self, evt):
        """!Mouse button double click"""
        self._setValueFromSelected()

    def OnListColClick(self, evt):
        """!Left mouse button pressed on column"""
        col = evt.GetColumn()
        # reverse the sort
        if col == self._colSearch:
            self._ascending = not self._ascending
        self.SortListItems( evt.GetColumn(), ascending=self._ascending )
        self._colSearch = evt.GetColumn()
        evt.Skip()

    def OnListItemSelected(self, event):
        """!Item selected"""
        self._setValueFromSelected()
        event.Skip()

    def OnEnteredText(self, event):
        """!Text entered"""
        text = event.GetString()
        
        if not text:
            # control is empty; hide dropdown if shown:
            if self.dropdown.IsShown():
                self._showDropDown(False)
            event.Skip()
            return
        
        cmd = shlex.split(str(text))
        pattern = str(text)
        if len(cmd) > 1:
            # search for module's options
            if cmd[0] in self._choicesCmd and not self._module:
                self._module = menuform.GUI().ParseInterface(cmd = cmd)

            if self._module:
                if len(cmd[-1].split('=', 1)) == 1:
                    # new option
                    if cmd[-1][0] == '-':
                        # -> flags
                        self.SetChoices(self._module.get_list_flags(), type = 'flag')
                        pattern = cmd[-1].lstrip('-')
                    else:
                        # -> options
                        self.SetChoices(self._module.get_list_params(), type = 'param')
                        pattern = cmd[-1]
                else:
                    # value
                    pattern = cmd[-1].split('=', 1)[1]
        else:
            # search for GRASS modules
            if self._module:
                # -> switch back to GRASS modules list
                self.SetChoices(self._choicesCmd)
                self._module = None
                self._choiceType = None
        
        found = False
        choices = self._choices
        for numCh, choice in enumerate(choices):
            if choice.lower().startswith(pattern):
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
            
            if self._module and cmd[-1][-2] == '=':
                optType = self._module.get_param(cmd[-1][:-2])['prompt']
                if optType in ('raster', 'vector'):
                    # -> raster/vector map
                    self.SetChoices(self._choicesMap[optType], optType)
                                            
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
        
        if visible:
            if event.GetKeyCode() == wx.WXK_RETURN:
                self._setValueFromSelected()
                skip = False
            if event.GetKeyCode() == wx.WXK_ESCAPE:
                self._showDropDown(False)
                skip = False
        if skip:
            event.Skip()
        
    def OnControlChanged(self, event):
        """!Control changed"""
        if self.IsShown():
            self._showDropDown(False)
        
        event.Skip()
