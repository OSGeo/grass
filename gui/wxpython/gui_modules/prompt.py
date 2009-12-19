"""!
@package prompt.py

@brief wxGUI prompt

Classes:
 - GPromptPopUp
 - PromptListCtrl
 - TextCtrlAutoComplete
 - GPromptSTC

@todo: reduce size of STC prompt to about 3 lines

(C) 2009 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
@author Michael Barton <michael.barton@asu.edu>
"""

import os
import sys
import shlex

import wx
import wx.stc
import wx.lib.mixins.listctrl as listmix

from grass.script import core as grass

import globalvar
import menudata
import gcmd

class GPromptPopUp:
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
                                  label = _("Find module:"))
        
        self.searchBy = wx.Choice(parent = cmdprompt, id = wx.ID_ANY,
                             choices = [_("description"),
                                        _("keywords")])
        winHeight = self.searchBy.GetSize()[1]

        self.search = wx.TextCtrl(parent = cmdprompt, id = wx.ID_ANY,
                             value = "", size = (-1, 25))
        
        label = wx.Button(parent = cmdprompt, id = wx.ID_ANY,
                          label = _("&Cmd >"), size = (-1, winHeight))
        label.SetToolTipString(_("Click for erasing command prompt"))

        ### todo: fix TextCtrlAutoComplete to work also on Macs
        ### reason: missing wx.PopupWindow()
        try:
            cmdinput = TextCtrlAutoComplete(parent = cmdprompt, id = wx.ID_ANY,
                                            value = "",
                                            style = wx.TE_LINEWRAP | wx.TE_PROCESS_ENTER,
                                            size = (-1, winHeight),
                                            statusbar = self.parent.statusbar)
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
        sizer.AddGrowableRow(1)
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

    def __checkKey(self, text, keywords):
        """!Check if text is in keywords"""
        found = 0
        keys = text.split(',')
        if len(keys) > 1: # -> multiple keys
            for k in keys[:-1]:
                k = k.strip()
                for key in keywords: 
                    if k == key: # full match
                        found += 1
                        break
            k = keys[-1].strip()
            for key in keywords:
                if k in key: # partial match
                    found +=1
                    break
        else:
            for key in keywords:
                if text in key: # partial match
                    found +=1
                    break
        
        if found == len(keys):
            return True
        
        return False
    
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
            if self.searchBy.GetSelection() == 0: # -> description
                if text in data['desc']:
                    modules.append(module)
            else: # -> keywords
                if self.__checkKey(text, data['keywords']):
                    modules.append(module)
        
        self.parent.statusbar.SetStatusText(_("%d modules found") % len(modules))
        self.input.SetChoices(modules)
                    
class PromptListCtrl(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin):
    def __init__(self, parent, id = wx.ID_ANY, pos = wx.DefaultPosition,
                 size = wx.DefaultSize, style = 0):
        wx.ListCtrl.__init__(self, parent, id, pos, size, style)
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        
class TextCtrlAutoComplete(wx.ComboBox, listmix.ColumnSorterMixin):
    def __init__ (self, parent, statusbar,
                  id = wx.ID_ANY, choices = [], **kwargs):
        """!Constructor works just like wx.TextCtrl except you can pass in a
        list of choices.  You can also change the choice list at any time
        by calling setChoices.
        
        Inspired by http://wiki.wxpython.org/TextCtrlAutoComplete
        """
        self.statusbar = statusbar
        
        if kwargs.has_key('style'):
            kwargs['style'] = wx.TE_PROCESS_ENTER | kwargs['style']
        else:
            kwargs['style'] = wx.TE_PROCESS_ENTER
        
        wx.ComboBox.__init__(self, parent, id, **kwargs)
        
        # some variables
        self._choices = choices
        self._hideOnNoMatch = True
        self._module = None      # currently selected module
        self._choiceType = None  # type of choice (module, params, flags, raster, vector ...)
        self._screenheight = wx.SystemSettings.GetMetric(wx.SYS_SCREEN_Y)
        self._historyItem = 0   # last item
        
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
        
        self.SetMinSize(self.GetSize())
        # read history
        self.SetHistoryItems()
        
        # bindings...
        self.Bind(wx.EVT_KILL_FOCUS, self.OnControlChanged)
        self.Bind(wx.EVT_TEXT, self.OnEnteredText)
        self.Bind(wx.EVT_KEY_DOWN , self.OnKeyDown)
        ### self.Bind(wx.EVT_LEFT_DOWN, self.OnClick)

        # if need drop down on left click
        self.dropdown.Bind(wx.EVT_LISTBOX , self.OnListItemSelected, self.dropdownlistbox)
        self.dropdownlistbox.Bind(wx.EVT_LEFT_DOWN, self.OnListClick)
        self.dropdownlistbox.Bind(wx.EVT_LEFT_DCLICK, self.OnListDClick)
        self.dropdownlistbox.Bind(wx.EVT_LIST_COL_CLICK, self.OnListColClick)

        self.Bind(wx.EVT_COMBOBOX, self.OnCommandSelect)
        
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

    def _setModule(self, name):
        """!Set module's choices (flags, parameters)""" 
        # get module's description
        if name in self._choicesCmd and not self._module:
            try:
                self._module = menuform.GUI().ParseInterface(cmd = [name])
            except IOError:
                self._module = None
             
        # set choices (flags)
        self._choicesMap['flag'] = self._module.get_list_flags()
        for idx in range(len(self._choicesMap['flag'])):
            item = self._choicesMap['flag'][idx]
            desc = self._module.get_flag(item)['label']
            if not desc:
                desc = self._module.get_flag(item)['description']
            
            self._choicesMap['flag'][idx] = '%s (%s)' % (item, desc)
        
        # set choices (parameters)
        self._choicesMap['param'] = self._module.get_list_params()
        for idx in range(len(self._choicesMap['param'])):
            item = self._choicesMap['param'][idx]
            desc = self._module.get_param(item)['label']
            if not desc:
                desc = self._module.get_param(item)['description']
            
            self._choicesMap['param'][idx] = '%s (%s)' % (item, desc)
    
    def _setValueFromSelected(self):
         """!Sets the wx.TextCtrl value from the selected wx.ListCtrl item.
         Will do nothing if no item is selected in the wx.ListCtrl.
         """
         sel = self.dropdownlistbox.GetFirstSelected()
         if sel < 0:
             return
         
         if self._colFetch != -1:
             col = self._colFetch
         else:
             col = self._colSearch
         itemtext = self.dropdownlistbox.GetItem(sel, col).GetText()
         
         cmd = shlex.split(str(self.GetValue()))
         if len(cmd) > 0 and cmd[0] in self._choicesCmd:
             # -> append text (skip last item)
             if self._choiceType == 'param':
                 itemtext = itemtext.split(' ')[0]
                 self.SetValue(' '.join(cmd) + ' ' + itemtext + '=')
                 optType = self._module.get_param(itemtext)['prompt']
                 if optType in ('raster', 'vector'):
                     # -> raster/vector map
                     self.SetChoices(self._choicesMap[optType], optType)
             elif self._choiceType == 'flag':
                 itemtext = itemtext.split(' ')[0]
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
             
             # define module
             self._setModule(itemtext)
             
             # use parameters as default choices
             self._choiceType = 'param'
             self.SetChoices(self._choicesMap['param'], type = 'param')
         
         self.SetInsertionPointEnd()
         
         self._showDropDown(False)
         
    def GetListCtrl(self):
        """!Method required by listmix.ColumnSorterMixin"""
        return self.dropdownlistbox
    
    def SetHistoryItems(self):
        """!Read history file and update combobox items"""
        env = grass.gisenv()
        try:
            fileHistory = open(os.path.join(env['GISDBASE'],
                                            env['LOCATION_NAME'],
                                            env['MAPSET'],
                                            '.bash_history'), 'r')
        except IOError:
            self.SetItems([])
            return
        
        try:
            hist = []
            for line in fileHistory.readlines():
                hist.append(line.replace('\n', ''))
            
            self.SetItems(hist)
        finally:
            fileHistory.close()
            return
        
        self.SetItems([])
        
    def SetChoices(self, choices, type = 'module'):
        """!Sets the choices available in the popup wx.ListBox.
        The items will be sorted case insensitively.

        @param choices list of choices
        @param type type of choices (module, param, flag, raster, vector)
        """
        self._choices = choices
        self._choiceType = type
        
        self.dropdownlistbox.SetWindowStyleFlag(wx.LC_REPORT | wx.LC_SINGLE_SEL |
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

    def OnClick(self, event):
        """Left mouse button pressed"""
        sel = self.dropdownlistbox.GetFirstSelected()
        if not self.dropdown.IsShown():
            if sel > -1:
                self.dropdownlistbox.Select(sel)
            else:
                self.dropdownlistbox.Select(0)
            self._listItemVisible()
            self._showDropDown()
        else:
            self.dropdown.Hide()
        
    def OnCommandSelect(self, event):
        """!Command selected from history"""
        self._historyItem = event.GetSelection() - len(self.GetItems())
        self.SetFocus()
        
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
        
        try:
            cmd = shlex.split(str(text))
        except ValueError, e:
            self.statusbar.SetStatusText(str(e))
            cmd = text.split(' ')
        pattern = str(text)
        
        if len(cmd) > 0 and cmd[0] in self._choicesCmd and not self._module:
            self._setModule(cmd[0])
        elif len(cmd) > 1 and cmd[0] in self._choicesCmd:
            if self._module:
                if len(cmd[-1].split('=', 1)) == 1:
                    # new option
                    if cmd[-1][0] == '-':
                        # -> flags
                        self.SetChoices(self._choicesMap['flag'], type = 'flag')
                        pattern = cmd[-1].lstrip('-')
                    else:
                        # -> options
                        self.SetChoices(self._choicesMap['param'], type = 'param')
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
        
        self._choiceType
        self._choicesMap
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
                if self._module and '=' not in cmd[-1]:
                    message = ''
                    if cmd[-1][0] == '-': # flag
                        message = _("Warning: flag <%s> not found in '%s'") % \
                            (cmd[-1][1:], self._module.name)
                    else: # option
                        message = _("Warning: option <%s> not found in '%s'") % \
                            (cmd[-1], self._module.name)
                    self.statusbar.SetStatusText(message)
        
        if self._module and len(cmd[-1]) == 2 and cmd[-1][-2] == '=':
            optType = self._module.get_param(cmd[-1][:-2])['prompt']
            if optType in ('raster', 'vector'):
                # -> raster/vector map
                self.SetChoices(self._choicesMap[optType], optType)
        
        self._listItemVisible()
        
        event.Skip()
        
    def OnKeyDown (self, event):
        """!Do some work when the user press on the keys: up and down:
        move the cursor left and right: move the search
        """
        skip = True
        sel = self.dropdownlistbox.GetFirstSelected()
        visible = self.dropdown.IsShown()
        KC = event.GetKeyCode()
        
        if KC == wx.WXK_RIGHT:
            # right -> show choices
            if sel < (self.dropdownlistbox.GetItemCount() - 1):
                self.dropdownlistbox.Select(sel + 1)
                self._listItemVisible()
            self._showDropDown()
            skip = False
        elif KC == wx.WXK_UP:
            if visible:
                if sel > 0:
                    self.dropdownlistbox.Select(sel - 1)
                    self._listItemVisible()
                self._showDropDown()
                skip = False
            else:
                self._historyItem -= 1
                try:
                    self.SetValue(self.GetItems()[self._historyItem])
                except IndexError:
                    self._historyItem += 1
        elif KC == wx.WXK_DOWN:
            if visible:
                if sel < (self.dropdownlistbox.GetItemCount() - 1):
                    self.dropdownlistbox.Select(sel + 1)
                    self._listItemVisible()
                self._showDropDown()
                skip = False
            else:
                if self._historyItem < -1:
                    self._historyItem += 1
                    self.SetValue(self.GetItems()[self._historyItem])
        
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

class GPromptSTC(wx.stc.StyledTextCtrl):
    """!Styled GRASS prompt with autocomplete and calltips"""    
    def __init__(self, parent, id, onRun, margin=False, wrap=None):
        wx.stc.StyledTextCtrl.__init__(self, parent, id)
        self.parent = parent
        self.SetUndoCollection(True)        
        self.RunCmd = onRun
        
        #
        # styles
        #                
        self.SetWrapMode(True)
        
        #
        # create command and map lists for autocompletion
        #
        self.AutoCompSetIgnoreCase(False) 
        
        self.rastlist = []
        self.vectlist = []
        self.imglist = []
        self.r3list = []
        self.dblist = []
        self.genlist = []
        self.displist = []
        
        #
        # Get available GRASS commands and parse into lists by command type for autocomplete
        #
        for item in globalvar.grassCmd['all']:
            if len(item.split('.')) > 1:
                start,end = item.split('.',1)
                if start == 'r': self.rastlist.append(end)
                elif start == 'v': self.vectlist.append(end)
                elif start == 'i': self.imglist.append(end)
                elif start == 'r3': self.r3list.append(end)
                elif start == 'db': self.dblist.append(end)
                elif start == 'g': self.genlist.append(end)
                elif start == 'd': self.displist.append(end)

        self.rastlist.sort()
        self.vectlist.sort()
        self.imglist.sort()
        self.r3list.sort()
        self.dblist.sort()
        self.genlist.sort()
        self.displist.sort()
                        
        #
        # Create lists of element types and possible arguments for autocomplete
        #
        self.datatypes = []
        self.maplists = {}
        self.maptype = ''
        self.datatypes = ['rast',
                        'rast3d',
                        'vect',
                        'oldvect',
                        'asciivect',
                        'labels',
                        'region',
                        'region3d',
                        'group',
                        '3dview']

        self.drastcmd = ['d.rast',
                        'd.rgb',
                        'd.his',
                        'd.rast.arrow',
                        'd.rast.num']
                    
        self.dvectcmd = ['d.vect',
                        'd.vect.chart'
                        'd.thematic.area',
                        'd.vect.thematic']
        
        self.rastargs = ['map',
                        'input',
                        'rast',
                        'raster',
                        'red',
                        'green',
                        'blue',
                        'h_map',
                        'i_map',
                        's_map',
                        'hue_input',
                        'intensity_input',
                        'saturation_input',
                        'red_input',
                        'green_input',
                        'blue_input']
                        
        self.__getfiles()

        #
        # command history buffer
        #
        self.cmdbuffer = []
        self.cmdindex = 0

        #
        # line margins
        #
        # TODO print number only from cmdlog
        self.SetMarginWidth(1, 0)
        self.SetMarginWidth(2, 0)
        if margin:
            self.SetMarginType(0, wx.stc.STC_MARGIN_NUMBER)
            self.SetMarginWidth(0, 30)
        else:
            self.SetMarginWidth(0, 0)

        #
        # miscellaneous
        #
        self.SetViewWhiteSpace(False)
#        self.SetTabWidth(4)
        self.SetUseTabs(False)
        self.UsePopUp(True)
        self.SetSelBackground(True, "#FFFF00")
        self.SetUseHorizontalScrollBar(True)

        #
        # bindings
        #
        self.Bind(wx.EVT_WINDOW_DESTROY, self.OnDestroy)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyPressed)
 
    def __getfiles(self):   
        """!Get accessible files for autocomplete"""
        for item in self.datatypes:
            mlist = grass.read_command("g.mlist", "m", type=item).splitlines()
            mlist.sort()
            self.maplists[item] = mlist
            
    def OnKeyPressed(self, event):
        """!Key press capture for autocompletion, calltips, and command history"""
        
        #keycodes used: "." = 46, "=" = 61, "," = 44 
        line = ''
        entry = ''
        usage = ''
        cmdtype = ''
        cmdname = ''
        cmd = ''
                            
        # CAN CHANGE: event.ControlDown() for manual autocomplete
        
        if event.GetKeyCode() == 46 and not event.ShiftDown():
            #GRASS command autocomplete when "." is pressed after r,v,i,g,db, or d
            listcmds = []
            pos = self.GetCurrentPos()
            self.InsertText(pos,'.')
            self.CharRight()
            
            entry = self.GetTextLeft()
            if entry not in ['r.','v.','i.','g.','db.','d.']:
                return

            if entry == 'r.': listcmds = self.rastlist
            elif entry == 'v.': listcmds = self.vectlist
            elif entry == 'i.': listcmds = self.imglist
            elif entry == 'r3.': listcmds = self.r3list
            elif entry == 'db.': listcmds = self.dblist
            elif entry == 'g.': listcmds = self.genlist
            elif entry == 'd.': listcmds = self.displist

            if listcmds == []:
                return
            else:
                self.AutoCompShow(0, " ".join(listcmds))                    
            
        elif event.GetKeyCode() == wx.WXK_TAB:
            #GRASS command calltips
                        
            #Must be a command to the left somewhere
            pos = self.GetCurrentPos()
            entry = self.GetTextLeft()
            cmd = entry.split()[0].strip()
            if cmd not in globalvar.grassCmd['all']:
                return
            
            usage, description = self.GetCommandUsage(cmd)
                                        
            self.CallTipSetBackground("PALE GREEN")
            self.CallTipSetForeground("BLACK")
            self.CallTipShow(pos, usage+'\n\n'+description)
            
        elif (event.GetKeyCode() == wx.WXK_SPACE and event.ControlDown()) or \
            event.GetKeyCode() == 61 or event.GetKeyCode() == 44:
            #Autocompletion for map/data file name entry after '=', ',', or manually
            
            pos = self.GetCurrentPos()
            entry = self.GetTextLeft()
            if event.GetKeyCode() != 44:
                self.maptype = ''
            arg = ''
            cmdtype = ''
            cmdname = ''
            cmd = ''

            if entry.strip()[0:2] in ['r.','v.','i.','g.','db.','d.']:
                cmdtype =  entry.strip()[0]
                cmd = entry.split()[0].strip()
                if cmd in globalvar.grassCmd['all']:
                    cmdname = cmd.split('.')[1]
                else:
                    #No complete GRASS command found
                    cmd = ''
                    cmdname = ''
            elif entry.strip()[0:4] == 'nviz':
                cmdtype = ''
                cmdname = cmd = 'nviz'
            else:
                #No partial or complete GRASS command found
                return

            cmdargs = entry.strip('=')
            try:
                arg = cmdargs.rsplit(' ',1)[1]
            except:
                arg = ''
                
            if event.GetKeyCode() == 61:
                # autocompletion after '='
                # insert the '=' and move to after the '=', ready for a map name
                self.InsertText(pos,'=')
                self.CharRight()

                maplist = []
                self.maptype = ''

                # what kind of map/data type is desired?
                if (((cmdtype in ['r', 'i'] or cmd in self.drastcmd) and arg in self.rastargs) or
                  ((cmd=='nviz' or cmdtype=='r3') and arg in ['elevation','color']) or
                  arg in ['rast', 'raster']):
                    self.maptype = 'rast'
                elif (((cmdtype=='v' or cmd in self.dvectcmd) and arg in ['map', 'input']) or
                  (cmdtype=='r3' and arg=='input') or
                  arg in ['vect', 'vector', 'points']):
                    self.maptype = 'vect'
                elif ((cmdtype=='r3' and arg in ['map', 'input']) or
                  (cmdtype=='nviz' and arg=='volume') or arg=='rast3d'):
                    self.maptype = 'rast3d'
                elif arg=='labels':
                    self.maptype ='labels'
                elif arg=='region':
                    self.maptype ='region'
                elif arg=='region3d':
                    self.maptype ='region3d'
                elif arg=='group':
                    self.maptype ='group'
                elif arg=='3dview':
                    self.maptype ='3dview'
                
            elif event.GetKeyCode() == 44:
                # autocompletion after ','
                # if comma is pressed, use the same maptype as previous for multiple map entries
                
                # insert the comma and move to after the comma ready for a map name
                self.InsertText(pos,',')
                self.CharRight()
                
                #must apply to an entry where '=[string]' has already been entered
                if '=' not in arg:
                    return

            elif event.GetKeyCode() == wx.WXK_SPACE and event.ControlDown():
                # manual autocompletion
                # map entries without arguments (as in r.info [mapname]) use ctrl-shift
                
                maplist = []
                if cmdtype=='r' or cmdtype=='i':
                    self.maptype = 'rast'
                elif cmdtype=='v':
                    self.maptype = 'vect'
                elif cmdtype=='r3':
                    self.maptype = 'rast3d'
                    
            if self.maptype == '': 
                return
            else:
                maplist = self.maplists[self.maptype]
                self.AutoCompShow(0, " ".join(maplist))
                        
        elif event.GetKeyCode() in [wx.WXK_UP,wx.WXK_DOWN] and event.ControlDown():
            # Command history using ctrl-up and ctrl-down   
            
            if self.cmdbuffer == []: return
            txt = ''

            self.DocumentEnd()
            
            # move through command history list index values
            if event.GetKeyCode() == wx.WXK_UP:
                self.cmdindex = self.cmdindex - 1
            if event.GetKeyCode() == wx.WXK_DOWN:
                self.cmdindex = self.cmdindex + 1
            if self.cmdindex < 0:
                self.cmdindex = 0
            if self.cmdindex > len(self.cmdbuffer) - 1:
                self.cmdindex = len(self.cmdbuffer) - 1
            
            try:
                txt = self.cmdbuffer[self.cmdindex]
            except:
                pass
                
            # clear current line and insert command history    
            self.DelLineLeft()
            self.DelLineRight()
            pos = self.GetCurrentPos()            
            self.InsertText(pos,txt)
            self.LineEnd()
            
        elif event.GetKeyCode() == wx.WXK_RETURN and self.AutoCompActive() == False:
            # Run command on line when <return> is pressed    
            
            # find the command to run
            line = str(self.GetCurLine()[0]).strip()
            if len(line) == 0:
                return
            
            # parse command into list
            # TODO: shell commands should probably be passed as string           
            cmd = shlex.split(str(line))
            
            #send the command list to the processor 
            self.RunCmd(cmd)
                            
            #add command to history    
            self.cmdbuffer.append(line)
            
            #keep command history to a managable size
            if len(self.cmdbuffer) > 200:
                del self.cmdbuffer[0]
            self.cmdindex = len(self.cmdbuffer)

        else:
            event.Skip()

    def GetTextLeft(self):
        """!Returns all text left of the caret"""
        entry = ''
        pos = self.GetCurrentPos()
        self.HomeExtend()
        entry = self.GetSelectedText().strip()
        self.SetCurrentPos(pos)
        
        return entry

    def GetCommandUsage(self, command):
        """!Returns command syntax by running command help"""
        usage = ''
        description = ''

        ret, out  = gcmd.RunCommand(command, 'help', getErrorMsg = True)
               
        if ret == 0:
            cmdhelp = out.splitlines()
            addline = False
            helplist = []
            description = ''
            for line in cmdhelp:
                if "Usage:" in line:
                    addline = True
                    continue
                elif "Flags:" in line:
                    addline = False
                    break
                elif addline == True:
                    line = line.strip()
                    helplist.append(line)

            for line in cmdhelp:
                if "Description:" in line:
                    addline = True
                    continue
                elif "Keywords:" in line:
                    addline = False
                    break
                elif addline == True:
                    description += (line + ' ')
                
            description = description.strip()

            for line in helplist:
                usage += line + '\n'

            return usage.strip(), description
        else:
            return ''   

    def OnDestroy(self, evt):
        """!The clipboard contents can be preserved after
        the app has exited"""
        
        wx.TheClipboard.Flush()
        evt.Skip()
    
    def OnCmdErase(self, event):
        """!Erase command prompt"""
        self.Home()
        self.DelLineRight()
        
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
