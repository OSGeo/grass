"""!
@package gui_core.prompt

@brief wxGUI command prompt

Classes:
 - prompt::GPrompt
 - prompt::GPromptSTC

(C) 2009-2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Michael Barton <michael.barton@asu.edu>
@author Vaclav Petras <wenzeslaus gmail.com> (copy&paste customization)
"""

import os
import difflib
import codecs
import sys

import wx
import wx.stc

from grass.script import core as grass
from grass.script import task as gtask

from grass.pydispatch.signal import Signal

from core          import globalvar
from core          import utils
from core.gcmd     import EncodeString, DecodeString, GetRealCmd
from core.utils import _


class GPrompt(object):
    """!Abstract class for interactive wxGUI prompt
    
    Signal promptRunCmd - emitted to run command from prompt
                        - attribute 'cmd'

    See subclass GPromptPopUp and GPromptSTC.
    """
    def __init__(self, parent, menuModel, updateCmdHistory):
        self.parent = parent                 # GConsole
        self.panel  = self.parent.GetPanel()

        self.promptRunCmd = Signal('GPrompt.promptRunCmd')

        # probably only subclasses need this
        self._menuModel = menuModel

        self.mapList    = self._getListOfMaps()
        self.mapsetList = utils.ListOfMapsets()
        
        # auto complete items
        self.autoCompList   = list()
        self.autoCompFilter = None
        
        # command description (gtask.grassTask)
        self.cmdDesc   = None

        self._updateCmdHistory = updateCmdHistory
        self.cmdbuffer = self._readHistory()
        self.cmdindex  = len(self.cmdbuffer)
        
        # list of traced commands
        self.commands = list()
        
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

    def _getListOfMaps(self):
        """!Get list of maps"""
        result = dict()
        result['raster'] = grass.list_strings('rast')
        result['vector'] = grass.list_strings('vect')
        
        return result
    
    def _runCmd(self, cmdString):
        """!Run command
        
        @param cmdString command to run (given as a string)
        """
        if not cmdString:
            return

        self.commands.append(cmdString) # trace commands

        # parse command into list
        try:
            cmd = utils.split(str(cmdString))
        except UnicodeError:
            cmd = utils.split(EncodeString((cmdString)))
        cmd = map(DecodeString, cmd)

        self.promptRunCmd.emit(cmd=cmd)

        # add command to history & clean prompt
        self.UpdateCmdHistory(cmd)
        self.OnCmdErase(None)
        self.ShowStatusText('')
        
    def GetCommands(self):
        """!Get list of launched commands"""
        return self.commands
    
    def ClearCommands(self):
        """!Clear list of commands"""
        del self.commands[:]


class GPromptSTC(GPrompt, wx.stc.StyledTextCtrl):
    """!Styled wxGUI prompt with autocomplete and calltips"""    
    def __init__(self, parent, menuModel, updateCmdHistory = True, margin = False):
        GPrompt.__init__(self, parent = parent, 
                         menuModel = menuModel, updateCmdHistory = updateCmdHistory)
        wx.stc.StyledTextCtrl.__init__(self, self.panel, id = wx.ID_ANY)
        
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
        self.Bind(wx.EVT_KILL_FOCUS, self.OnKillFocus)

        # signal which requests showing of a notification
        self.showNotification = Signal('GPromptSTC.showNotification')

    def OnTextSelectionChanged(self, event):
        """!Copy selected text to clipboard and skip event.
        The same function is in GStc class (goutput.py).
        """
        wx.CallAfter(self.Copy)
        event.Skip()
        
    def OnItemChanged(self, event):
        """!Change text in statusbar 
        if the item selection in the auto-completion list is changed"""
        # list of commands
        if self.toComplete['entity'] == 'command':
            item = self.toComplete['cmd'].rpartition('.')[0] + '.' + self.autoCompList[event.GetIndex()] 
            try:
                nodes = self._menuModel.SearchNodes(key='command', value=item)
                desc = ''
                if nodes:
                    desc = nodes[0].data['description']
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
            try:
                self.cmdDesc = gtask.parse_interface(GetRealCmd(cmd))
            except IOError:
                self.cmdDesc = None

    def OnKillFocus(self, event):
        """!Hides autocomplete"""
        # hide autocomplete
        if self.AutoCompActive():
            self.AutoCompCancel()
        event.Skip()

    def SetTextAndFocus(self, text):
        pos = len(text)
        self.SetText(text)
        self.SetSelectionStart(pos)
        self.SetCurrentPos(pos)
        self.SetFocus()

    def UpdateCmdHistory(self, cmd):
        """!Update command history
        
        @param cmd command given as a list
        """
        if not self._updateCmdHistory:
            return
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
        
        try:
            splitted = utils.split(str(entry))
        except ValueError: # No closing quotation error
            return None
        if len(splitted) > 1:
            if cmd in globalvar.grassCmd:
                toComplete['cmd'] = cmd
                if entry[-1] == ' ':
                    words = entry.split(' ')
                    if any(word.startswith('-') for word in words):
                        toComplete['entity'] = 'params'
                    else:
                        toComplete['entity'] = 'params+flags'
                else:
                    # get word left from current position
                    word = self.GetWordLeft(withDelimiter = True)
                    
                    if word[0] == '=' and word[-1] == '@':
                        toComplete['entity'] = 'mapsets'
                    elif word[0] == '=':
                        # get name of parameter
                        paramName = self.GetWordLeft(withDelimiter = False, ignoredDelimiter = '=').strip('=')
                        if paramName:
                            try:
                                param = self.cmdDesc.get_param(paramName)
                            except (ValueError, AttributeError):
                                return None
                        else:
                            return None
                        
                        if param['values']:
                            toComplete['entity'] = 'param values'
                        elif param['prompt'] == 'raster' and param['element'] == 'cell':
                            toComplete['entity'] = 'raster map'
                        elif param['prompt'] == 'vector' and param['element'] == 'vector':
                            toComplete['entity'] = 'vector map'
                    elif word[0] == '-':
                        toComplete['entity'] = 'flags'
                    elif word[0] == ' ':
                        toComplete['entity'] = 'params'
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
        # complete command after pressing '.'
        if event.GetKeyCode() == 46 and not event.ShiftDown():
            self.autoCompList = list()
            entry = self.GetTextLeft()
            self.InsertText(pos, '.')
            self.CharRight()
            self.toComplete = self.EntityToComplete()
            if self.toComplete is None:
                return
            try:
                if self.toComplete['entity'] == 'command': 
                    for command in globalvar.grassCmd:
                        try:
                            if command.find(self.toComplete['cmd']) == 0:
                                dotNumber = list(self.toComplete['cmd']).count('.') 
                                self.autoCompList.append(command.split('.',dotNumber)[-1])
                        except UnicodeDecodeError, e: # TODO: fix it
                            sys.stderr.write(DecodeString(command) + ": " + unicode(e))
                            
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
            if self.toComplete and 'entity' in self.toComplete:
                if self.toComplete['entity'] == 'raster map':
                    self.autoCompList = self.mapList['raster']
                elif self.toComplete['entity'] == 'vector map':
                    self.autoCompList = self.mapList['vector']
                elif self.toComplete['entity'] == 'param values':
                    param = self.GetWordLeft(withDelimiter = False, ignoredDelimiter='=').strip(' =')
                    self.autoCompList = self.cmdDesc.get_param(param)['values']
            self.ShowList()
        
        # complete mapset ('@')
        elif event.GetKeyCode() == 50 and event.ShiftDown():
            self.autoCompList = list()
            self.InsertText(pos, '@')
            self.CharRight()
            self.toComplete = self.EntityToComplete()
            
            if self.toComplete and self.toComplete['entity'] == 'mapsets':
                self.autoCompList = self.mapsetList
            self.ShowList()
            
        # complete after pressing CTRL + Space          
        elif event.GetKeyCode() == wx.WXK_SPACE and event.ControlDown():
            self.autoCompList = list()
            self.toComplete = self.EntityToComplete()
            if self.toComplete is None:
                return 

            #complete command
            if self.toComplete['entity'] == 'command':
                for command in globalvar.grassCmd:
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
            
            if cmd not in globalvar.grassCmd:
                return
            
            info = gtask.command_info(GetRealCmd(cmd))
            
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
            except KeyError:
                txt = ''
            
            # clear current line and insert command history    
            self.DelLineLeft()
            self.DelLineRight()
            pos = self.GetCurrentPos()            
            self.InsertText(pos,txt)
            self.LineEnd()

            self.ShowStatusText('')
            
        elif event.GetKeyCode() in (wx.WXK_RETURN, wx.WXK_NUMPAD_ENTER) and \
                self.AutoCompActive() == False:
            # run command on line when <return> is pressed
            self._runCmd(self.GetCurLine()[0].strip())
                        
        elif event.GetKeyCode() == wx.WXK_SPACE:
            items = self.GetTextLeft().split()
            if len(items) == 1:
                cmd = items[0].strip()
                if cmd in globalvar.grassCmd and \
                        (not self.cmdDesc or cmd != self.cmdDesc.get_name()):
                    try:
                        self.cmdDesc = gtask.parse_interface(GetRealCmd(cmd))
                    except IOError:
                        self.cmdDesc = None
            event.Skip()
        
        else:
            event.Skip()

    def ShowStatusText(self, text):
        """!Requests showing of notification, e.g. showing in a statusbar."""
        self.showNotification.emit(message=text)
        
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
