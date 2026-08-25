"""
@package gui_core.prompt

@brief wxGUI command prompt

Classes:
 - prompt::GPrompt
 - prompt::GPromptSTC

(C) 2009-2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Michael Barton <michael.barton@asu.edu>
@author Vaclav Petras <wenzeslaus gmail.com> (copy&paste customization)
@author Wolf Bergenheim <wolf bergenheim.net> (#962)
"""

import difflib
import sys

import wx
import wx.stc

from grass.script import core as grass
from grass.script import task as gtask

from grass.grassdb import history

from grass.pydispatch.signal import Signal

from core import globalvar
from core import utils
from core.gcmd import EncodeString, DecodeString, GError


class GPrompt:
    """Abstract class for interactive wxGUI prompt

    Signal promptRunCmd - emitted to run command from prompt
                        - attribute 'cmd'

    See subclass GPromptPopUp and GPromptSTC.
    """

    def __init__(self, parent, giface, menuModel):
        self.parent = parent  # GConsole
        self.panel = self.parent.GetPanel()

        self.promptRunCmd = Signal("GPrompt.promptRunCmd")

        # probably only subclasses need this
        self._menuModel = menuModel

        self.mapList = self._getListOfMaps()
        self.mapsetList = utils.ListOfMapsets()

        # auto complete items
        self.autoCompList = []
        self.autoCompFilter = None

        # command description (gtask.grassTask)
        self.cmdDesc = None

        # list of traced commands
        self.commands = []

        # reload map lists when needed
        if giface:
            giface.currentMapsetChanged.connect(self._reloadListOfMaps)
            giface.grassdbChanged.connect(self._reloadListOfMaps)

    def _getListOfMaps(self):
        """Get list of maps"""
        result = {}
        result["raster"] = grass.list_strings("raster")
        result["vector"] = grass.list_strings("vector")

        return result

    def _reloadListOfMaps(self):
        self.mapList = self._getListOfMaps()

    def _runCmd(self, cmdString):
        """Run command

        :param str cmdString: command to run
        """
        if not cmdString:
            return

        # parse command into list
        try:
            cmd = utils.split(str(cmdString))
        except UnicodeError:
            cmd = utils.split(EncodeString(cmdString))
        cmd = list(map(DecodeString, cmd))

        self.promptRunCmd.emit(cmd={"cmd": cmd, "cmdString": str(cmdString)})

        self.CmdErase()
        self.ShowStatusText("")

    def GetCommands(self):
        """Get list of launched commands"""
        return self.commands

    def ClearCommands(self):
        """Clear list of commands"""
        del self.commands[:]


class GPromptSTC(GPrompt, wx.stc.StyledTextCtrl):
    """Styled wxGUI prompt with autocomplete and calltips"""

    def __init__(self, parent, giface, menuModel, margin=False):
        GPrompt.__init__(self, parent=parent, giface=giface, menuModel=menuModel)
        wx.stc.StyledTextCtrl.__init__(self, self.panel, id=wx.ID_ANY)

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
        self.SetUseHorizontalScrollBar(True)

        # support light and dark mode
        bg_color = wx.SystemSettings().GetColour(wx.SYS_COLOUR_WINDOW)
        fg_color = wx.SystemSettings().GetColour(wx.SYS_COLOUR_WINDOWTEXT)
        selection_color = wx.SystemSettings().GetColour(wx.SYS_COLOUR_HIGHLIGHT)
        self.StyleSetBackground(wx.stc.STC_STYLE_DEFAULT, bg_color)
        self.StyleSetForeground(wx.stc.STC_STYLE_DEFAULT, fg_color)
        self.SetCaretForeground(fg_color)
        self.SetSelBackground(True, selection_color)
        self.StyleClearAll()

        # show hint
        self._showHint()

        # read history file
        self._loadHistory()
        if giface:
            giface.currentMapsetChanged.connect(self._loadHistory)
            giface.entryToHistoryAdded.connect(
                lambda entry: self._addEntryToCmdHistoryBuffer(entry)
            )
            giface.entryFromHistoryRemoved.connect(
                lambda index: self._removeEntryFromCmdHistoryBuffer(index)
            )
        #
        # bindings
        #
        self.Bind(wx.EVT_WINDOW_DESTROY, self.OnDestroy)
        self.Bind(wx.EVT_CHAR, self.OnChar)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyPressed)
        self.Bind(wx.stc.EVT_STC_AUTOCOMP_SELECTION, self.OnItemSelected)
        self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemChanged)
        if sys.platform != "darwin":  # unstable on Mac with wxPython 3
            self.Bind(wx.EVT_KILL_FOCUS, self.OnKillFocus)
            self.Bind(wx.EVT_SET_FOCUS, self.OnSetFocus)

        # signal which requests showing of a notification
        self.showNotification = Signal("GPromptSTC.showNotification")

        # signal to notify selected command
        self.commandSelected = Signal("GPromptSTC.commandSelected")

    def OnTextSelectionChanged(self, event):
        """Copy selected text to clipboard and skip event.
        The same function is in GStc class (goutput.py).
        """
        wx.CallAfter(self.Copy)
        event.Skip()

    def OnItemChanged(self, event):
        """Change text in statusbar
        if the item selection in the auto-completion list is changed"""
        # list of commands
        if self.toComplete["entity"] == "command":
            item = (
                self.toComplete["cmd"].rpartition(".")[0]
                + "."
                + self.autoCompList[event.GetIndex()]
            )
            try:
                nodes = self._menuModel.SearchNodes(key="command", value=item)
                desc = ""
                if nodes:
                    self.commandSelected.emit(command=item)
                    desc = nodes[0].data["description"]
            except KeyError:
                desc = ""
            self.ShowStatusText(desc)
        # list of flags
        elif self.toComplete["entity"] == "flags":
            desc = self.cmdDesc.get_flag(self.autoCompList[event.GetIndex()])[
                "description"
            ]
            self.ShowStatusText(desc)
        # list of parameters
        elif self.toComplete["entity"] == "params":
            item = self.cmdDesc.get_param(self.autoCompList[event.GetIndex()])
            desc = item["name"] + "=" + item["type"]
            if not item["required"]:
                desc = "[" + desc + "]"
            desc += ": " + item["description"]
            self.ShowStatusText(desc)
        # list of flags and commands
        elif self.toComplete["entity"] == "params+flags":
            if self.autoCompList[event.GetIndex()][0] == "-":
                desc = self.cmdDesc.get_flag(
                    self.autoCompList[event.GetIndex()].strip("-")
                )["description"]
            else:
                item = self.cmdDesc.get_param(self.autoCompList[event.GetIndex()])
                desc = item["name"] + "=" + item["type"]
                if not item["required"]:
                    desc = "[" + desc + "]"
                desc += ": " + item["description"]
            self.ShowStatusText(desc)
        else:
            self.ShowStatusText("")

    def OnItemSelected(self, event):
        """Item selected from the list"""
        lastWord = self.GetWordLeft()
        # to insert selection correctly if selected word partly matches written
        # text
        match = difflib.SequenceMatcher(None, event.GetText(), lastWord)
        matchTuple = match.find_longest_match(0, len(event.GetText()), 0, len(lastWord))

        compl = event.GetText()[matchTuple[2] :]
        text = self.GetTextLeft() + compl
        # add space or '=' at the end
        end = "="
        for char in (".", "-", "="):
            if text.split(" ")[-1].find(char) >= 0:
                end = " "

        compl += end
        text += end

        self.AddText(compl)
        pos = len(text)
        self.SetCurrentPos(pos)

        cmd = text.strip().split(" ")[0]

        if not self.cmdDesc or cmd != self.cmdDesc.get_name():
            try:
                self.cmdDesc = gtask.parse_interface(cmd)
            except OSError:
                self.cmdDesc = None

    def _showHint(self):
        """Shows usability hint"""
        self.StyleSetForeground(0, wx.SystemSettings.GetColour(wx.SYS_COLOUR_GRAYTEXT))
        self.WriteText(_("Type command here and press Enter"))
        self._hint_shown = True

    def _hideHint(self):
        """Hides usability hint"""
        if self._hint_shown:
            self.ClearAll()
            self.StyleSetForeground(
                0, wx.SystemSettings().GetColour(wx.SYS_COLOUR_WINDOWTEXT)
            )
            self._hint_shown = False

    def OnKillFocus(self, event):
        """Hides autocomplete and shows hint"""
        # hide autocomplete
        if self.AutoCompActive():
            self.AutoCompCancel()
        # show hint
        if self.IsEmpty():
            wx.CallAfter(self._showHint)
        event.Skip()

    def OnSetFocus(self, event):
        """Prepares prompt for entering commands."""
        self._hideHint()
        event.Skip()

    def SetTextAndFocus(self, text):
        pos = len(text)
        self.commandSelected.emit(command=text)
        self.SetText(text)
        self.SetSelectionStart(pos)
        self.SetCurrentPos(pos)
        self.SetFocus()

    def _loadHistory(self):
        """Load history from a history file to data structures"""
        try:
            history_path = history.get_current_mapset_gui_history_path()
            history.ensure_history_file(history_path)
            self.cmdbuffer = [
                entry["command"] for entry in history.read(history_path)
            ] or []
            self.cmdindex = len(self.cmdbuffer)
        except (OSError, ValueError) as e:
            GError(str(e))

    def _addEntryToCmdHistoryBuffer(self, entry):
        """Add entry to command history buffer.

        :param entry dict: entry with 'command' and 'command_info' keys
        command value is a string.
        """
        # create command string
        entry = entry["command"]
        # add command to history
        self.cmdbuffer.append(entry)
        # update also traced commands
        self.commands.append(entry)

        # keep command history to a manageable size
        if len(self.cmdbuffer) > 200:
            del self.cmdbuffer[0]
        self.cmdindex = len(self.cmdbuffer)

    def _removeEntryFromCmdHistoryBuffer(self, index):
        """Remove entry from command history buffer.
        :param index: index of deleted command
        """
        # remove command at the given index from history buffer
        if index < len(self.cmdbuffer):
            self.cmdbuffer.pop(index)

        # update cmd index size
        self.cmdindex = len(self.cmdbuffer)

    def EntityToComplete(self):
        """Determines which part of command (flags, parameters) should
        be completed at current cursor position"""
        entry = self.GetTextLeft()
        toComplete = {"cmd": None, "entity": None}
        try:
            cmd = entry.split()[0].strip()
        except IndexError:
            return toComplete

        try:
            splitted = utils.split(str(entry))
        except ValueError:  # No closing quotation error
            return toComplete
        if len(splitted) > 0 and cmd in globalvar.grassCmd:
            toComplete["cmd"] = cmd
            if entry[-1] == " ":
                words = entry.split(" ")
                if any(word.startswith("-") for word in words):
                    toComplete["entity"] = "params"
                else:
                    toComplete["entity"] = "params+flags"
            else:
                # get word left from current position
                word = self.GetWordLeft(withDelimiter=True)

                if word[0] == "=" and word[-1] == "@":
                    toComplete["entity"] = "mapsets"
                elif word[0] == "=":
                    # get name of parameter
                    paramName = self.GetWordLeft(
                        withDelimiter=False, ignoredDelimiter="="
                    ).strip("=")
                    if paramName:
                        try:
                            param = self.cmdDesc.get_param(paramName)
                        except (ValueError, AttributeError):
                            return toComplete
                    else:
                        return toComplete

                    if param["values"]:
                        toComplete["entity"] = "param values"
                    elif param["prompt"] == "raster" and param["element"] == "cell":
                        toComplete["entity"] = "raster map"
                    elif param["prompt"] == "vector" and param["element"] == "vector":
                        toComplete["entity"] = "vector map"
                elif word[0] == "-":
                    toComplete["entity"] = "flags"
                elif word[0] == " ":
                    toComplete["entity"] = "params"
        else:
            toComplete["entity"] = "command"
            toComplete["cmd"] = cmd

        return toComplete

    def GetWordLeft(self, withDelimiter=False, ignoredDelimiter=None):
        """Get word left from current cursor position. The beginning
        of the word is given by space or chars: .,-=

        :param withDelimiter: returns the word with the initial delimiter
        :param ignoredDelimiter: finds the word ignoring certain delimiter
        """
        textLeft = self.GetTextLeft()

        parts = []
        if ignoredDelimiter is None:
            ignoredDelimiter = ""

        for char in set(" .,-=") - set(ignoredDelimiter):
            delimiter = "" if not withDelimiter else char
            parts.append(delimiter + textLeft.rpartition(char)[2])
        return min(parts, key=lambda x: len(x))

    def ShowList(self):
        """Show sorted auto-completion list if it is not empty"""
        if len(self.autoCompList) > 0:
            self.autoCompList.sort()
            self.AutoCompShow(0, itemList=" ".join(self.autoCompList))

    def OnKeyPressed(self, event):
        """Key pressed capture special treatment for tabulator to show help"""
        pos = self.GetCurrentPos()
        if event.GetKeyCode() == wx.WXK_TAB:
            # show GRASS command calltips (to hide press 'ESC')
            entry = self.GetTextLeft()
            try:
                cmd = entry.split()[0].strip()
            except IndexError:
                cmd = ""

            if cmd not in globalvar.grassCmd:
                return

            info = gtask.command_info(cmd)

            self.CallTipSetBackground("#f4f4d1")
            self.CallTipSetForeground("BLACK")
            self.CallTipShow(pos, info["usage"] + "\n\n" + info["description"])
        elif (
            event.GetKeyCode() in {wx.WXK_RETURN, wx.WXK_NUMPAD_ENTER}
            and not self.AutoCompActive()
        ):
            # run command on line when <return> is pressed
            self._runCmd(self.GetCurLine()[0].strip())
        elif (
            event.GetKeyCode() in {wx.WXK_UP, wx.WXK_DOWN} and not self.AutoCompActive()
        ):
            # Command history using up and down
            if len(self.cmdbuffer) < 1:
                return

            self.DocumentEnd()

            # move through command history list index values
            if event.GetKeyCode() == wx.WXK_UP:
                self.cmdindex -= 1
            if event.GetKeyCode() == wx.WXK_DOWN:
                self.cmdindex += 1
            self.cmdindex = max(self.cmdindex, 0)
            self.cmdindex = min(self.cmdindex, len(self.cmdbuffer) - 1)

            try:
                # without strip causes problem on windows
                txt = self.cmdbuffer[self.cmdindex].strip()
            except KeyError:
                txt = ""

            # clear current line and insert command history
            self.DelLineLeft()
            self.DelLineRight()
            pos = self.GetCurrentPos()
            self.InsertText(pos, txt)
            self.LineEnd()

            self.ShowStatusText("")
        else:
            event.Skip()

    def OnChar(self, event):
        """Key char capture for autocompletion, calltips, and command history

        .. todo::
            event.ControlDown() for manual autocomplete
        """
        # keycodes used: "." = 46, "=" = 61, "-" = 45
        pos = self.GetCurrentPos()
        # complete command after pressing '.'
        if event.GetKeyCode() == 46:
            self.autoCompList = []
            self.InsertText(pos, ".")
            self.CharRight()
            self.toComplete = self.EntityToComplete()
            try:
                if self.toComplete["entity"] == "command":
                    for command in globalvar.grassCmd:
                        try:
                            if command.find(self.toComplete["cmd"]) == 0:
                                dotNumber = list(self.toComplete["cmd"]).count(".")
                                self.autoCompList.append(
                                    command.split(".", dotNumber)[-1]
                                )
                        except UnicodeDecodeError as error:
                            sys.stderr.write(DecodeString(command) + ": " + str(error))

            except (KeyError, TypeError):
                return
            self.ShowList()

        # complete flags after pressing '-'
        elif (
            (event.GetKeyCode() == 45)
            or event.GetKeyCode() == wx.WXK_NUMPAD_SUBTRACT
            or event.GetKeyCode() == wx.WXK_SUBTRACT
        ):
            self.autoCompList = []
            self.InsertText(pos, "-")
            self.CharRight()
            self.toComplete = self.EntityToComplete()
            if self.toComplete["entity"] == "flags" and self.cmdDesc:
                if self.GetTextLeft()[-2:] == " -":  # complete e.g. --quite
                    for flag in self.cmdDesc.get_options()["flags"]:
                        if len(flag["name"]) == 1:
                            self.autoCompList.append(flag["name"])
                else:
                    for flag in self.cmdDesc.get_options()["flags"]:
                        if len(flag["name"]) > 1:
                            self.autoCompList.append(flag["name"])
            self.ShowList()

        # complete map or values after parameter
        elif event.GetKeyCode() == 61:
            self.autoCompList = []
            self.InsertText(pos, "=")
            self.CharRight()
            self.toComplete = self.EntityToComplete()
            if self.toComplete["entity"] == "raster map":
                self.autoCompList = self.mapList["raster"]
            elif self.toComplete["entity"] == "vector map":
                self.autoCompList = self.mapList["vector"]
            elif self.toComplete["entity"] == "param values":
                param = self.GetWordLeft(
                    withDelimiter=False, ignoredDelimiter="="
                ).strip(" =")
                self.autoCompList = self.cmdDesc.get_param(param)["values"]
            self.ShowList()

        # complete mapset ('@')
        elif event.GetKeyCode() == 64:
            self.autoCompList = []
            self.InsertText(pos, "@")
            self.CharRight()
            self.toComplete = self.EntityToComplete()

            if self.toComplete["entity"] == "mapsets":
                self.autoCompList = self.mapsetList
            self.ShowList()

        # complete after pressing CTRL + Space
        elif event.GetKeyCode() == wx.WXK_SPACE and event.ControlDown():
            self.autoCompList = []
            self.toComplete = self.EntityToComplete()

            # complete command
            if self.toComplete["entity"] == "command":
                for command in globalvar.grassCmd:
                    if command.find(self.toComplete["cmd"]) == 0:
                        dotNumber = list(self.toComplete["cmd"]).count(".")
                        self.autoCompList.append(command.split(".", dotNumber)[-1])

            # complete flags in such situations (| is cursor):
            # r.colors -| ...w, q, l
            # r.colors -w| ...w, q, l
            elif self.toComplete["entity"] == "flags" and self.cmdDesc:
                for flag in self.cmdDesc.get_options()["flags"]:
                    if len(flag["name"]) == 1:
                        self.autoCompList.append(flag["name"])

            # complete parameters in such situations (| is cursor):
            # r.colors -w | ...color, map, rast, rules
            # r.colors col| ...color
            elif self.toComplete["entity"] == "params" and self.cmdDesc:
                for param in self.cmdDesc.get_options()["params"]:
                    if param["name"].find(self.GetWordLeft(withDelimiter=False)) == 0:
                        self.autoCompList.append(param["name"])

            # complete flags or parameters in such situations (| is cursor):
            # r.colors | ...-w, -q, -l, color, map, rast, rules
            # r.colors color=grey | ...-w, -q, -l, color, map, rast, rules
            elif self.toComplete["entity"] == "params+flags" and self.cmdDesc:
                self.autoCompList = []

                for param in self.cmdDesc.get_options()["params"]:
                    self.autoCompList.append(param["name"])
                for flag in self.cmdDesc.get_options()["flags"]:
                    if len(flag["name"]) == 1:
                        self.autoCompList.append("-" + flag["name"])
                    else:
                        self.autoCompList.append("--" + flag["name"])

                self.ShowList()

            # complete map or values after parameter
            # r.buffer input=| ...list of raster maps
            # r.buffer units=| ... feet, kilometers, ...
            elif self.toComplete["entity"] == "raster map":
                self.autoCompList = []
                self.autoCompList = self.mapList["raster"]
            elif self.toComplete["entity"] == "vector map":
                self.autoCompList = []
                self.autoCompList = self.mapList["vector"]
            elif self.toComplete["entity"] == "param values":
                self.autoCompList = []
                param = self.GetWordLeft(
                    withDelimiter=False, ignoredDelimiter="="
                ).strip(" =")
                self.autoCompList = self.cmdDesc.get_param(param)["values"]

            self.ShowList()

        elif event.GetKeyCode() == wx.WXK_SPACE:
            items = self.GetTextLeft().split()
            if len(items) == 1:
                cmd = items[0].strip()
                if cmd in globalvar.grassCmd and (
                    not self.cmdDesc or cmd != self.cmdDesc.get_name()
                ):
                    try:
                        self.cmdDesc = gtask.parse_interface(cmd)
                    except OSError:
                        self.cmdDesc = None
            event.Skip()

        else:
            event.Skip()

    def ShowStatusText(self, text):
        """Requests showing of notification, e.g. showing in a statusbar."""
        self.showNotification.emit(message=text)

    def GetTextLeft(self):
        """Returns all text left of the caret"""
        pos = self.GetCurrentPos()
        self.HomeExtend()
        entry = self.GetSelectedText()
        self.SetCurrentPos(pos)

        return entry

    def OnDestroy(self, event):
        """The clipboard contents can be preserved after
        the app has exited"""
        if wx.TheClipboard.IsOpened():
            wx.TheClipboard.Flush()
        event.Skip()

    def CmdErase(self):
        """Erase command prompt"""
        self.Home()
        self.DelLineRight()
