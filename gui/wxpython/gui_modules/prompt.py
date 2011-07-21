"""!
@package prompt.py

@brief wxGUI command prompt

Classes:
 - PromptListCtrl
 - TextCtrlAutoComplete
 - GPrompt
 - GPromptPopUp
 - GPromptSTC

(C) 2009-2011 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Martin Landa <landa.martin gmail.com>
@author Michael Barton <michael.barton@asu.edu>
@author Vaclav Petras <wenzeslaus gmail.com> (copy&paste customization)
"""

import os
import sys
import copy
import difflib
import codecs

import wx
import wx.stc
import wx.lib.mixins.listctrl as listmix

from grass.script import core as grass
from grass.script import task as gtask

import globalvar
import menudata
import menuform
import gcmd
import utils

class PromptListCtrl(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin):
    """!PopUp window used by GPromptPopUp"""
    def __init__(self, parent, id = wx.ID_ANY, pos = wx.DefaultPosition,
                 size = wx.DefaultSize, style = 0):
        wx.ListCtrl.__init__(self, parent, id, pos, size, style)
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        
class TextCtrlAutoComplete(wx.ComboBox, listmix.ColumnSorterMixin):
    """!Auto complete text area used by GPromptPopUp"""
    def __init__ (self, parent, statusbar,
                  id = wx.ID_ANY, choices = [], **kwargs):
        """!Constructor works just like wx.TextCtrl except you can pass in a
        list of choices.  You can also change the choice list at any time
        by calling setChoices.
        
        Inspired by http://wiki.wxpython.org/TextCtrlAutoComplete
        """
        self.statusbar = statusbar
        
        if 'style' in kwargs:
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
        
        # bindings...
        self.Bind(wx.EVT_KILL_FOCUS, self.OnControlChanged)
        self.Bind(wx.EVT_TEXT, self.OnEnteredText)
        self.Bind(wx.EVT_TEXT_ENTER, self.OnRunCmd)
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
                self._module = gtask.parse_interface(name)
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
         
         cmd = utils.split(str(self.GetValue()))
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
            cmd = utils.split(str(text))
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
                        message = _("Warning: flag <%(flag)s> not found in '%(module)s'") % \
                            { 'flag' : cmd[-1][1:], 'module' : self._module.name }
                    else: # option
                        message = _("Warning: option <%(param)s> not found in '%(module)s'") % \
                            { 'param' : cmd[-1], 'module' : self._module.name }
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

class GPrompt(object):
    """!Abstract class for interactive wxGUI prompt

    See subclass GPromptPopUp and GPromptSTC.
    """
    def __init__(self, parent):
        self.parent = parent                 # GMConsole
        self.panel  = self.parent.GetPanel()
        
        if self.parent.parent.GetName() not in ("LayerManager", "Modeler"):
            self.standAlone = True
        else:
            self.standAlone = False
        
        # dictionary of modules (description, keywords, ...)
        if not self.standAlone:
            if self.parent.parent.GetName() == 'Modeler':
                self.moduleDesc = menudata.ManagerData().GetModules()
            else:
                self.moduleDesc = parent.parent.menubar.GetData().GetModules()
            self.moduleList = self._getListOfModules()
            self.mapList = self._getListOfMaps()
        else:
            self.moduleDesc = self.moduleList = self.mapList = None
        
        # auto complete items
        self.autoCompList   = list()
        self.autoCompFilter = None
        
        # command description (gtask.grassTask)
        self.cmdDesc = None
        self.cmdbuffer = self._readHistory()
        self.cmdindex = len(self.cmdbuffer)
        
    def _readHistory(self):
        """!Get list of commands from history file"""
        hist = list()
        env = grass.gisenv()
        try:
            fileHistory = codecs.open(os.path.join(env['GISDBASE'],
                                                   env['LOCATION_NAME'],
                                                   env['MAPSET'],
                                                   '.bash_history'),
                                      encoding = 'utf-8', mode = 'r', errors='replace')
        except IOError:
            return hist
        
        try:
            for line in fileHistory.readlines():
                hist.append(line.replace('\n', ''))
        finally:
            fileHistory.close()
        
        return hist

    def GetCommandDesc(self, cmd):
        """!Get description for given command"""
        if cmd in self.moduleDesc:
            return self.moduleDesc[cmd]['desc']
        
        return ''
    
    def GetCommandItems(self):
        """!Get list of available commands"""
        items = list()
        
        if self.autoCompFilter is not None:
            mList = self.autoCompFilter
        else:
            mList = self.moduleList
            
        if not mList:
            return items
        
        prefixes = mList.keys()
        prefixes.sort()
        
        for prefix in prefixes:
            for command in mList[prefix]:
                name = prefix + '.' + command
                items.append(name)
                
        items.sort()
        
        return items
    
    def _getListOfModules(self):
        """!Get list of modules"""
        result = dict()
        for module in globalvar.grassCmd['all']:
            try:
                group, name = module.split('.',1)
            except ValueError:
                continue # TODO
            
            if group not in result:
                result[group] = list()
            result[group].append(name)
            
            # for better auto-completion: 
            # not only result['r']={...,'colors.out',...}, but also result['r.colors']={'out',...}
            for i in range(len(name.split('.'))-1):
                group = '.'.join([group,name.split('.',1)[0]])
                name = name.split('.',1)[1]
                if group not in result:
                    result[group] = list()
                result[group].append(name)
      
        # sort list of names
        for group in result.keys():
            result[group].sort()
        
        return result
    
    def _getListOfMaps(self):
        """!Get list of maps"""
        result = dict()
        result['raster'] = grass.list_strings('rast')
        result['vector'] = grass.list_strings('vect')
        
        return result

    def OnRunCmd(self, event):
        """!Run command"""
        cmdString = event.GetString()
        
        if self.standAlone:
            return
        
        if cmdString[:2] == 'd.' and not self.parent.curr_page:
            self.parent.NewDisplay(show=True)
        
        cmd = utils.split(cmdString)
        if len(cmd) > 1:
            self.parent.RunCmd(cmd, switchPage = True)
        else:
            self.parent.RunCmd(cmd, switchPage = False)
        
        self.OnUpdateStatusBar(None)
        
    def OnUpdateStatusBar(self, event):
        """!Update Layer Manager status bar"""
        if self.standAlone:
            return
        
        if event is None:
            self.parent.parent.statusbar.SetStatusText("")
        else:
            self.parent.parent.statusbar.SetStatusText(_("Type GRASS command and run by pressing ENTER"))
            event.Skip()
        
    def GetPanel(self):
        """!Get main widget panel"""
        return self.panel

    def GetInput(self):
        """!Get main prompt widget"""
        return self.input
    
    def SetFilter(self, data, module = True):
        """!Set filter

        @param module True to filter modules, otherwise data
        """
        if module:
            if data:
                self.moduleList = data
            else:
                self.moduleList = self._getListOfModules()
        else:
            if data:
                self.dataList = data
            else:
                self.dataList = self._getListOfMaps()
        
class GPromptPopUp(GPrompt, TextCtrlAutoComplete):
    """!Interactive wxGUI prompt - popup version"""
    def __init__(self, parent):
        GPrompt.__init__(self, parent)
        
        ### todo: fix TextCtrlAutoComplete to work also on Macs
        ### reason: missing wx.PopupWindow()
        try:
            TextCtrlAutoComplete.__init__(self, parent = self.panel, id = wx.ID_ANY,
                                          value = "",
                                          style = wx.TE_LINEWRAP | wx.TE_PROCESS_ENTER,
                                          statusbar = self.parent.parent.statusbar)
            self.SetItems(self._readHistory())
        except NotImplementedError:
            # wx.PopupWindow may be not available in wxMac
            # see http://trac.wxwidgets.org/ticket/9377
            wx.TextCtrl.__init__(parent = self.panel, id = wx.ID_ANY,
                                 value = "",
                                 style=wx.TE_LINEWRAP | wx.TE_PROCESS_ENTER,
                                 size = (-1, 25))
            self.searchBy.Enable(False)
            self.search.Enable(False)
        
        self.SetFont(wx.Font(10, wx.FONTFAMILY_MODERN, wx.NORMAL, wx.NORMAL, 0, ''))
        
        wx.CallAfter(self.SetInsertionPoint, 0)
        
        # bidnings
        self.Bind(wx.EVT_TEXT_ENTER, self.OnRunCmd)
        self.Bind(wx.EVT_TEXT,       self.OnUpdateStatusBar)
        
    def OnCmdErase(self, event):
        """!Erase command prompt"""
        self.input.SetValue('')

class GPromptSTC(GPrompt, wx.stc.StyledTextCtrl):
    """!Styled wxGUI prompt with autocomplete and calltips"""    
    def __init__(self, parent, id = wx.ID_ANY, margin = False):
        GPrompt.__init__(self, parent)
        wx.stc.StyledTextCtrl.__init__(self, self.panel, id)
        
        #
        # styles
        #                
        self.SetWrapMode(True)
        self.SetUndoCollection(True)        
        
        #
        # create command and map lists for autocompletion
        #
        self.AutoCompSetIgnoreCase(False) 
        
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
        self.SetUseTabs(False)
        self.UsePopUp(True)
        self.SetSelBackground(True, "#FFFF00")
        self.SetUseHorizontalScrollBar(True)
        
        #
        # bindings
        #
        self.Bind(wx.EVT_WINDOW_DESTROY, self.OnDestroy)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyPressed)
        self.Bind(wx.stc.EVT_STC_AUTOCOMP_SELECTION, self.OnItemSelected)
        self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemChanged)
        
    def OnTextSelectionChanged(self, event):
        """!Copy selected text to clipboard and skip event.
        The same function is in GMStc class (goutput.py).
        """
        self.Copy()
        event.Skip()
        
    def OnItemChanged(self, event):
        """!Change text in statusbar 
        if the item selection in the auto-completion list is changed"""
        # list of commands
        if self.toComplete['entity'] == 'command':
            item = self.toComplete['cmd'].rpartition('.')[0] + '.' + self.autoCompList[event.GetIndex()] 
            try:
                desc = self.moduleDesc[item]['desc']        
            except KeyError:
                desc = '' 
            self.ShowStatusText(desc)
        # list of flags    
        elif self.toComplete['entity'] == 'flags':
            desc = self.cmdDesc.get_flag(self.autoCompList[event.GetIndex()])['description']
            self.ShowStatusText(desc)
        # list of parameters
        elif self.toComplete['entity'] == 'params':
            item = self.cmdDesc.get_param(self.autoCompList[event.GetIndex()])
            desc = item['name'] + '=' + item['type']
            if not item['required']:
                desc = '[' + desc + ']'
            desc += ': ' + item['description']
            self.ShowStatusText(desc)
        # list of flags and commands       
        elif self.toComplete['entity'] == 'params+flags':
            if self.autoCompList[event.GetIndex()][0] == '-':
                desc = self.cmdDesc.get_flag(self.autoCompList[event.GetIndex()].strip('-'))['description']
            else:
                item = self.cmdDesc.get_param(self.autoCompList[event.GetIndex()])
                desc = item['name'] + '=' + item['type']
                if not item['required']:
                    desc = '[' + desc + ']'
                desc += ': ' + item['description']
            self.ShowStatusText(desc)
        else:
            self.ShowStatusText('')
            
    def OnItemSelected(self, event):
        """!Item selected from the list"""
        lastWord = self.GetWordLeft()
        # to insert selection correctly if selected word partly matches written text
        match = difflib.SequenceMatcher(None, event.GetText(), lastWord)
        matchTuple = match.find_longest_match(0, len(event.GetText()), 0, len(lastWord))
    
        compl = event.GetText()[matchTuple[2]:]
        text = self.GetTextLeft() + compl
        # add space or '=' at the end
        end = '='
        for char in ('.','-','='):
            if text.split(' ')[-1].find(char) >= 0:
                end = ' '
        
        compl += end
        text += end

        self.AddText(compl)
        pos = len(text)
        self.SetCurrentPos(pos)
        
        cmd = text.strip().split(' ')[0]

        if not self.cmdDesc or cmd != self.cmdDesc.get_name():
            if cmd in ('r.mapcalc', 'r3.mapcalc'):
                self.parent.parent.OnMapCalculator(event = None, cmd = [cmd])
                # add command to history & clean prompt
                self.UpdateCmdHistory([cmd])
                self.OnCmdErase(None)
            else:
                try:
                    self.cmdDesc = gtask.parse_interface(cmd)
                except IOError:
                    self.cmdDesc = None
        
    def UpdateCmdHistory(self, cmd):
        """!Update command history
        
        @param cmd command given as a list
        """
        # add command to history    
        self.cmdbuffer.append(' '.join(cmd))
        
        # keep command history to a managable size
        if len(self.cmdbuffer) > 200:
            del self.cmdbuffer[0]
        self.cmdindex = len(self.cmdbuffer)
        
    def EntityToComplete(self):
        """!Determines which part of command (flags, parameters) should
        be completed at current cursor position"""
        entry = self.GetTextLeft()
        toComplete = dict()
        try:
            cmd = entry.split()[0].strip()
        except IndexError:
            return None
        
        if len(entry.split(' ')) > 1:
            if cmd in globalvar.grassCmd['all']:
                toComplete['cmd'] = cmd
                if entry[-1] == ' ':
                    words = entry.split(' ')
                    if any(word.startswith('-') for word in words):
                        toComplete['entity'] = 'params'
                        return toComplete
                    else:
                        toComplete['entity'] = 'params+flags'
                        return toComplete
                    
                else:
                    #get word left from current position
                    word = self.GetWordLeft(withDelimiter = True)
                    if word[0] == '=':
                        # get name of parameter
                        paramName = self.GetWordLeft(withDelimiter = False, ignoredDelimiter = '=').strip('=')
                        if paramName:
                            try:
                                param = self.cmdDesc.get_param(paramName)
                            except (ValueError, AttributeError):
                                return
                        else:
                            return

                        if param['values']:
                            toComplete['entity'] = 'param values'
                            return toComplete
                        elif param['prompt'] == 'raster' and param['element'] == 'cell':
                            toComplete['entity'] = 'raster map'
                            return toComplete
                        elif param['prompt'] == 'vector' and param['element'] == 'vector':
                            toComplete['entity'] = 'vector map'
                            return toComplete
                    elif word[0] == '-':
                        toComplete['entity'] = 'flags'
                        return toComplete
                    elif word[0] == ' ':
                        toComplete['entity'] = 'params'
                        return toComplete
                                       
            else:
                return None
        else:
            toComplete['entity'] = 'command'
            toComplete['cmd'] = cmd
            return toComplete
    
    def GetWordLeft(self, withDelimiter = False, ignoredDelimiter = None):
        """!Get word left from current cursor position. The beginning
        of the word is given by space or chars: .,-= 
        
        @param withDelimiter returns the word with the initial delimeter
        @param ignoredDelimiter finds the word ignoring certain delimeter
        """
        textLeft = self.GetTextLeft()
        
        parts = list()
        if ignoredDelimiter is None:
            ignoredDelimiter = ''
        
        for char in set(' .,-=') - set(ignoredDelimiter):
            if not withDelimiter:
                delimiter = ''
            else:
                delimiter = char
            parts.append(delimiter + textLeft.rpartition(char)[2])
        return min(parts, key=lambda x: len(x))
         
    def ShowList(self):
        """!Show sorted auto-completion list if it is not empty"""
        if len(self.autoCompList) > 0:
            self.autoCompList.sort()
            self.AutoCompShow(lenEntered = 0, itemList = ' '.join(self.autoCompList))    
        
    def OnKeyPressed(self, event):
        """!Key press capture for autocompletion, calltips, and command history

        @todo event.ControlDown() for manual autocomplete
        """
        # keycodes used: "." = 46, "=" = 61, "-" = 45 
        pos = self.GetCurrentPos()
        #complete command after pressing '.'
        if event.GetKeyCode() == 46 and not event.ShiftDown():
            self.autoCompList = list()
            entry = self.GetTextLeft()
            self.InsertText(pos, '.')
            self.CharRight()
            self.toComplete = self.EntityToComplete()
            try:
                if self.toComplete['entity'] == 'command': 
                    self.autoCompList = self.moduleList[entry.strip()]
            except (KeyError, TypeError):
                return
            self.ShowList()

        # complete flags after pressing '-'       
        elif event.GetKeyCode() == 45 and not event.ShiftDown(): 
            self.autoCompList = list()
            entry = self.GetTextLeft()
            self.InsertText(pos, '-')
            self.CharRight()
            self.toComplete = self.EntityToComplete()
            if self.toComplete['entity'] == 'flags' and self.cmdDesc:
                if self.GetTextLeft()[-2:] == ' -': # complete e.g. --quite
                    for flag in self.cmdDesc.get_options()['flags']:
                        if len(flag['name']) == 1:
                            self.autoCompList.append(flag['name'])
                else:
                    for flag in self.cmdDesc.get_options()['flags']:
                        if len(flag['name']) > 1:
                            self.autoCompList.append(flag['name'])            
            self.ShowList()
            
        # complete map or values after parameter
        elif event.GetKeyCode() == 61 and not event.ShiftDown():
            self.autoCompList = list()
            self.InsertText(pos, '=')
            self.CharRight()
            self.toComplete = self.EntityToComplete()
            if self.toComplete:
                if self.toComplete['entity'] == 'raster map':
                    self.autoCompList = self.mapList['raster']
                elif self.toComplete['entity'] == 'vector map':
                    self.autoCompList = self.mapList['vector']
                elif self.toComplete['entity'] == 'param values':
                    param = self.GetWordLeft(withDelimiter = False, ignoredDelimiter='=').strip(' =')
                    self.autoCompList = self.cmdDesc.get_param(param)['values']
            self.ShowList()
            
        # complete after pressing CTRL + Space          
        elif event.GetKeyCode() == wx.WXK_SPACE and event.ControlDown():
            self.autoCompList = list()
            self.toComplete = self.EntityToComplete()
            if self.toComplete is None:
                return 

            #complete command
            if self.toComplete['entity'] == 'command':
                for command in globalvar.grassCmd['all']:
                    if command.find(self.toComplete['cmd']) == 0:
                        dotNumber = list(self.toComplete['cmd']).count('.') 
                        self.autoCompList.append(command.split('.',dotNumber)[-1])
                
            
            # complete flags in such situations (| is cursor):
            # r.colors -| ...w, q, l
            # r.colors -w| ...w, q, l  
            elif self.toComplete['entity'] == 'flags' and self.cmdDesc:
                for flag in self.cmdDesc.get_options()['flags']:
                    if len(flag['name']) == 1:
                        self.autoCompList.append(flag['name'])
                    
            # complete parameters in such situations (| is cursor):
            # r.colors -w | ...color, map, rast, rules
            # r.colors col| ...color
            elif self.toComplete['entity'] == 'params' and self.cmdDesc:
                for param in self.cmdDesc.get_options()['params']:
                    if param['name'].find(self.GetWordLeft(withDelimiter=False)) == 0:
                        self.autoCompList.append(param['name'])           
            
            # complete flags or parameters in such situations (| is cursor):
            # r.colors | ...-w, -q, -l, color, map, rast, rules
            # r.colors color=grey | ...-w, -q, -l, color, map, rast, rules
            elif self.toComplete['entity'] == 'params+flags' and self.cmdDesc:
                self.autoCompList = list()
                
                for param in self.cmdDesc.get_options()['params']:
                    self.autoCompList.append(param['name'])
                for flag in self.cmdDesc.get_options()['flags']:
                    if len(flag['name']) == 1:
                        self.autoCompList.append('-' + flag['name'])
                    else:
                        self.autoCompList.append('--' + flag['name'])
                    
                self.ShowList() 
                   
            # complete map or values after parameter  
            # r.buffer input=| ...list of raster maps
            # r.buffer units=| ... feet, kilometers, ...   
            elif self.toComplete['entity'] == 'raster map':
                self.autoCompList = list()
                self.autoCompList = self.mapList['raster']
            elif self.toComplete['entity'] == 'vector map':
                self.autoCompList = list()
                self.autoCompList = self.mapList['vector']
            elif self.toComplete['entity'] == 'param values':
                self.autoCompList = list()
                param = self.GetWordLeft(withDelimiter = False, ignoredDelimiter='=').strip(' =')
                self.autoCompList = self.cmdDesc.get_param(param)['values']
                
            self.ShowList()

        elif event.GetKeyCode() == wx.WXK_TAB:
            # show GRASS command calltips (to hide press 'ESC')
            entry = self.GetTextLeft()
            try:
                cmd = entry.split()[0].strip()
            except IndexError:
                cmd = ''
            
            if cmd not in globalvar.grassCmd['all']:
                return
            
            info = gtask.command_info(cmd)
            
            self.CallTipSetBackground("#f4f4d1")
            self.CallTipSetForeground("BLACK")
            self.CallTipShow(pos, info['usage'] + '\n\n' + info['description'])
            
            
        elif event.GetKeyCode() in [wx.WXK_UP, wx.WXK_DOWN] and \
                 not self.AutoCompActive():
            # Command history using up and down   
            if len(self.cmdbuffer) < 1:
                return
            
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
                txt = ''
            
            # clear current line and insert command history    
            self.DelLineLeft()
            self.DelLineRight()
            pos = self.GetCurrentPos()            
            self.InsertText(pos,txt)
            self.LineEnd()
            self.parent.parent.statusbar.SetStatusText('')
            
        elif event.GetKeyCode() == wx.WXK_RETURN and \
                self.AutoCompActive() == False:
            # run command on line when <return> is pressed
            
            if self.parent.GetName() == "ModelerDialog":
                self.parent.OnOk(None)
                return
            
            # find the command to run
            line = self.GetCurLine()[0].strip()
            if len(line) == 0:
                return
            
            # parse command into list
            try:
                cmd = utils.split(str(line))
            except UnicodeError:
                cmd = utils.split(utils.EncodeString((line)))
            cmd = map(utils.DecodeString, cmd)
            
            #  send the command list to the processor 
            if cmd[0] in ('r.mapcalc', 'r3.mapcalc') and len(cmd) == 1:
                self.parent.parent.OnMapCalculator(event = None, cmd = cmd)
            else:
                self.parent.RunCmd(cmd)
            
            # add command to history & clean prompt
            self.UpdateCmdHistory(cmd)
            self.OnCmdErase(None)
            self.parent.parent.statusbar.SetStatusText('')
            
        elif event.GetKeyCode() == wx.WXK_SPACE:
            items = self.GetTextLeft().split()
            if len(items) == 1:
                cmd = items[0].strip()
                if cmd in globalvar.grassCmd['all'] and \
                        cmd != 'r.mapcalc' and \
                        (not self.cmdDesc or cmd != self.cmdDesc.get_name()):
                    try:
                        self.cmdDesc = gtask.parse_interface(cmd)
                    except IOError:
                        self.cmdDesc = None
            event.Skip()
        
        else:
            event.Skip()

    def ShowStatusText(self, text):
        """!Sets statusbar text, if it's too long, it is cut off"""
        maxLen = self.parent.parent.statusbar.GetFieldRect(0).GetWidth()/ 7 # any better way?
        if len(text) < maxLen:
            self.parent.parent.statusbar.SetStatusText(text)
        else:
            self.parent.parent.statusbar.SetStatusText(text[:maxLen]+'...')
        
        
    def GetTextLeft(self):
        """!Returns all text left of the caret"""
        pos = self.GetCurrentPos()
        self.HomeExtend()
        entry = self.GetSelectedText()
        self.SetCurrentPos(pos)
        
        return entry
    
    def OnDestroy(self, event):
        """!The clipboard contents can be preserved after
        the app has exited"""
        wx.TheClipboard.Flush()
        event.Skip()

    def OnCmdErase(self, event):
        """!Erase command prompt"""
        self.Home()
        self.DelLineRight()
