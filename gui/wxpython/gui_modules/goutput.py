"""
@package goutput

@brief Command output log widget

Classes:
 - GMConsole
 - GMStc
 - GMStdout
 - GMStderr

(C) 2007-2008 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Michael Barton (Arizona State University)
Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import textwrap
import time

import wx
import wx.stc

import globalvar
import gcmd
from debug import Debug as Debug

class GMConsole(wx.Panel):
    """
    Create and manage output console for commands entered on the
    GIS Manager command line.
    """
    def __init__(self, parent, id=wx.ID_ANY, margin=False, pageid=0,
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.TAB_TRAVERSAL | wx.FULL_REPAINT_ON_RESIZE):
        wx.Panel.__init__(self, parent, id, pos, size, style)

        # initialize variables
        self.Map             = None
        self.parent          = parent # GMFrame
        self.cmdThreads      = {}     # cmdThread : cmdPID
        self.lineWidth       = 80
        self.pageid          = pageid

        # progress bar
        self.console_progressbar = wx.Gauge(parent=self, id=wx.ID_ANY,
                                            range=100, pos=(110, 50), size=(-1, 25),
                                            style=wx.GA_HORIZONTAL)

        # text control for command output
        self.cmd_output = GMStc(parent=self, id=wx.ID_ANY, margin=margin,
                                wrap=None) 
        
        # redirect
        self.cmd_stdout = GMStdout(self.cmd_output)
        ### sys.stdout = self.cmd_stdout
        self.cmd_stderr = GMStderr(self.cmd_output,
                                   self.console_progressbar,
                                   self.parent.notebook,
                                   pageid)

        # buttons
        self.console_clear = wx.Button(parent=self, id=wx.ID_CLEAR)
        self.console_save  = wx.Button(parent=self, id=wx.ID_SAVE)
        self.Bind(wx.EVT_BUTTON, self.ClearHistory, self.console_clear)
        self.Bind(wx.EVT_BUTTON, self.SaveHistory,  self.console_save)

        # output control layout
        boxsizer1 = wx.BoxSizer(wx.VERTICAL)
        gridsizer1 = wx.GridSizer(rows=1, cols=2, vgap=0, hgap=0)
        boxsizer1.Add(item=self.cmd_output, proportion=1,
                      flag=wx.EXPAND | wx.ADJUST_MINSIZE, border=0)
        gridsizer1.Add(item=self.console_clear, proportion=0,
                       flag=wx.ALIGN_CENTER_HORIZONTAL | wx.ADJUST_MINSIZE, border=0)
        gridsizer1.Add(item=self.console_save, proportion=0,
                       flag=wx.ALIGN_CENTER_HORIZONTAL | wx.ADJUST_MINSIZE, border=0)


        boxsizer1.Add(item=gridsizer1, proportion=0,
                      flag=wx.EXPAND | wx.ALIGN_CENTRE_VERTICAL | wx.TOP | wx.BOTTOM,
                      border=5)
        boxsizer1.Add(item=self.console_progressbar, proportion=0,
                      flag=wx.EXPAND | wx.ADJUST_MINSIZE | wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)

        boxsizer1.Fit(self)
        boxsizer1.SetSizeHints(self)

        # set up event handler for any command thread results
        gcmd.EVT_RESULT(self, self.OnResult)
        
        # layout
        self.SetAutoLayout(True)
        self.SetSizer(boxsizer1)

    def Redirect(self):
        """Redirect stderr

        @return True redirected
        @return False failed
        """
        if Debug.get_level() == 0:
            # don't redirect when debugging is enabled
            sys.stderr = self.cmd_stderr
            
            return True

        return False

    def WriteLog(self, line, style=None, wrap=None):
        """Generic method for writing log message in 
        given style

        @param line text line
        @param style text style (see GMStc)
        @param stdout write to stdout or stderr
        """
        if not style:
            style = self.cmd_output.StyleDefault

        self.cmd_output.GotoPos(self.cmd_output.GetEndStyled())
        p1 = self.cmd_output.GetCurrentPos()
        
        # fill space
        if len(line) < self.lineWidth:
            diff = 80 - len(line) 
            line += diff * ' '

        self.cmd_output.AddTextWrapped(line, wrap=wrap) # adds os.linesep
        self.cmd_output.EnsureCaretVisible()
        p2 = self.cmd_output.GetCurrentPos()
        self.cmd_output.StartStyling(p1, 0xff)
        self.cmd_output.SetStyling(p2 - p1, style)

    def WriteCmdLog(self, line, pid=None):
        """Write out line in selected style"""
        if pid:
            line = '(' + str(pid) + ') ' + line

        self.WriteLog(line, self.cmd_output.StyleCommand)

    def RunCmd(self, command):
        """
        Run in GUI GRASS (or other) commands typed into
        console command text widget, and send stdout output to output
        text widget.

        Command is transformed into a list for processing.

        TODO: Display commands (*.d) are captured and
        processed separately by mapdisp.py. Display commands are
        rendered in map display widget that currently has
        the focus (as indicted by mdidx).
        """
        
        # map display window available ?
        try:
            curr_disp = self.parent.curr_page.maptree.mapdisplay
            self.Map = curr_disp.GetRender()
        except:
            curr_disp = None

        if len(self.GetListOfCmdThreads()) > 0:
            # only one running command enabled (per GMConsole instance)
            busy = wx.BusyInfo(message=_("Unable to run the command, another command is running..."),
                               parent=self)
            wx.Yield()
            time.sleep(3)
            busy.Destroy()
            return  None

        # command given as a string ?
        try:
            cmdlist = command.strip().split(' ')
        except:
            cmdlist = command

        if cmdlist[0] in globalvar.grassCmd['all']:
            # send GRASS command without arguments to GUI command interface
            # except display commands (they are handled differently)
            if cmdlist[0][0:2] == "d.":
                #
                # display GRASS commands
                #
                try:
                    layertype = {'d.rast'         : 'raster',
                                 'd.rgb'          : 'rgb',
                                 'd.his'          : 'his',
                                 'd.shaded'       : 'shaded',
                                 'd.legend'       : 'rastleg',
                                 'd.rast.arrow'   : 'rastarrow',
                                 'd.rast.num'     : 'rastnum',
                                 'd.vect'         : 'vector',
                                 'd.vect.thematic': 'thememap',
                                 'd.vect.chart'   : 'themechart',
                                 'd.grid'         : 'grid',
                                 'd.geodesic'     : 'geodesic',
                                 'd.rhumbline'    : 'rhumb',
                                 'd.labels'       : 'labels'}[cmdlist[0]]
                except KeyError:
                    wx.MessageBox(message=_("Command '%s' not yet implemented.") % cmdlist[0])
                    return None

                # add layer into layer tree
                self.parent.curr_page.maptree.AddLayer(ltype=layertype,
                                                       lcmd=cmdlist)

            else:
                #
                # other GRASS commands (r|v|g|...)
                #
                if hasattr(self.parent, "curr_page"):
                    # change notebook page only for Layer Manager
                    if self.parent.notebook.GetSelection() != 1:
                        self.parent.notebook.SetSelection(1)

                # activate computational region (set with g.region)
                # for all non-display commands.
                tmpreg = os.getenv("GRASS_REGION")
                os.unsetenv("GRASS_REGION")
                if len(cmdlist) == 1:
                    import menuform
                    # process GRASS command without argument
                    menuform.GUI().ParseCommand(cmdlist, parentframe=self)
                else:
                    # process GRASS command with argument
                    cmdPID = len(self.cmdThreads.keys())+1
                    self.WriteCmdLog('%s' % ' '.join(cmdlist), pid=cmdPID)
                    
                    grassCmd = gcmd.Command(cmdlist, wait=False,
                                            stdout=self.cmd_stdout,
                                            stderr=self.cmd_stderr)
                    
                    self.cmdThreads[grassCmd.cmdThread] = { 'cmdPID' : cmdPID }
                    
                    return grassCmd
                # deactivate computational region and return to display settings
                if tmpreg:
                    os.environ["GRASS_REGION"] = tmpreg
        else:
            # Send any other command to the shell. Send output to
            # console output window
            if hasattr(self.parent, "curr_page"):
                # change notebook page only for Layer Manager
                if self.parent.notebook.GetSelection() != 1:
                    self.parent.notebook.SetSelection(1)

            # if command is not a GRASS command, treat it like a shell command
            try:
                generalCmd = gcmd.Command(cmdlist,
                                          stdout=self.cmd_stdout,
                                          stderr=self.cmd_stderr)
            except gcmd.CmdError, e:
                print >> sys.stderr, e

        return None

    def ClearHistory(self, event):
        """Clear history of commands"""
        self.cmd_output.ClearAll()
        self.console_progressbar.SetValue(0)

    def SaveHistory(self, event):
        """Save history of commands"""
        self.history = self.cmd_output.GetSelectedText()
        if self.history == '':
            self.history = self.cmd_output.GetText()

        # add newline if needed
        if len(self.history) > 0 and self.history[-1] != os.linesep:
            self.history += os.linesep

        wildcard = "Text file (*.txt)|*.txt"
        dlg = wx.FileDialog(
            self, message=_("Save file as..."), defaultDir=os.getcwd(),
            defaultFile="grass_cmd_history.txt", wildcard=wildcard,
            style=wx.SAVE|wx.FD_OVERWRITE_PROMPT)

        # Show the dialog and retrieve the user response. If it is the OK response,
        # process the data.
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()

            output = open(path, "w")
            output.write(self.history)
            output.close()

        dlg.Destroy()

    def GetListOfCmdThreads(self, onlyAlive=True):
        """Return list of command threads)"""
        list = []
        for t in self.cmdThreads.keys():
            Debug.msg (4, "GMConsole.GetListOfCmdThreads(): name=%s, alive=%s" %
                       (t.getName(), t.isAlive()))
            if onlyAlive and not t.isAlive():
                continue
            list.append(t)

        return list

    def OnResult(self, event):
        """Show result status"""
        
        if event.cmdThread.aborted:
            # Thread aborted (using our convention of None return)
            self.WriteLog(_('Please note that the data are left in incosistent stage '
                            'and can be corrupted'), self.cmd_output.StyleWarning)
            self.WriteCmdLog(_('Command aborted'),
                             pid=self.cmdThreads[event.cmdThread]['cmdPID'])
        else:
            try:
                # Process results here
                self.WriteCmdLog(_('Command finished (%d sec)') % (time.time() - event.cmdThread.startTime),
                                 pid=self.cmdThreads[event.cmdThread]['cmdPID'])
            except KeyError:
                # stopped deamon
                pass
                
        self.console_progressbar.SetValue(0) # reset progress bar on '0%'

        # updated command dialog
        if hasattr(self.parent.parent, "btn_run"):
            dialog = self.parent.parent

            if hasattr(self.parent.parent, "btn_abort"):
                dialog.btn_abort.Enable(False)

            if hasattr(self.parent.parent, "btn_cancel"):
                dialog.btn_cancel.Enable(True)

            if hasattr(self.parent.parent, "btn_clipboard"):
                dialog.btn_clipboard.Enable(True)

            if hasattr(self.parent.parent, "btn_help"):
                dialog.btn_help.Enable(True)

            dialog.btn_run.Enable(True)
            
            if dialog.get_dcmd is None and \
                   dialog.closebox.IsChecked():
                time.sleep(1)
                dialog.Close()

class GMStdout:
    """GMConsole standard output

    Based on FrameOutErr.py

    Name:      FrameOutErr.py
    Purpose:   Redirecting stdout / stderr
    Author:    Jean-Michel Fauth, Switzerland
    Copyright: (c) 2005-2007 Jean-Michel Fauth
    Licence:   GPL
    """
    def __init__(self, gmstc):
        self.gmstc  = gmstc

    def write(self, s):
        if len(s) == 0:
            return
        s = s.replace('\n', os.linesep)
        for line in s.split(os.linesep):
            p1 = self.gmstc.GetCurrentPos() # get caret position
            self.gmstc.AddTextWrapped(line, wrap=None) # no wrapping && adds os.linesep
            # self.gmstc.EnsureCaretVisible()
            p2 = self.gmstc.GetCurrentPos()
            self.gmstc.StartStyling(p1, 0xff)
            self.gmstc.SetStyling(p2 - p1 + 1, self.gmstc.StyleOutput)

class GMStderr(object):
    """GMConsole standard error output

    Based on FrameOutErr.py

    Name:      FrameOutErr.py
    Purpose:   Redirecting stdout / stderr
    Author:    Jean-Michel Fauth, Switzerland
    Copyright: (c) 2005-2007 Jean-Michel Fauth
    Licence:   GPL
    """
    def __init__(self, gmstc, gmgauge, notebook, pageid):
        self.gmstc    = gmstc
        self.gmgauge  = gmgauge
        self.notebook = notebook
        self.pageid   = pageid

        self.type = ''
        self.message = ''
        self.printMessage = False
        
    def write(self, s):
        if self.pageid > -1:
            # swith notebook page to 'command output'
            if self.notebook.GetSelection() != self.pageid: 
                self.notebook.SetSelection(self.pageid)

        s = s.replace('\n', os.linesep)
        # remove/replace escape sequences '\b' or '\r' from stream
        s = s.replace('\b', '').replace('\r', '%s' % os.linesep)
 
        for line in s.split(os.linesep):
            if len(line) == 0:
                continue
            
            if 'GRASS_INFO_PERCENT' in line:
                # 'GRASS_INFO_PERCENT: 10' -> value=10
                value = int(line.rsplit(':', 1)[1].strip())
                if value >= 0 and value < 100:
                    self.gmgauge.SetValue(value)
                else:
                    self.gmgauge.SetValue(0) # reset progress bar on '0%'
            elif 'GRASS_INFO_MESSAGE' in line:
                self.type = 'message'
                if len(self.message) > 0:
                    self.message += os.linesep
                self.message += line.split(':', 1)[1].strip()
            elif 'GRASS_INFO_WARNING' in line:
                self.type = 'warning'
                if len(self.message) > 0:
                    self.message += os.linesep
                self.message += line.split(':', 1)[1].strip()
            elif 'GRASS_INFO_ERROR' in line:
                self.type = 'error'
                if len(self.message) > 0:
                    self.message += os.linesep
                self.message += line.split(':', 1)[1].strip()
            elif 'GRASS_INFO_END' in line:
                self.printMessage = True
            elif not self.type:
                if len(line) > 0:
                    p1 = self.gmstc.GetCurrentPos()
                    self.gmstc.AddTextWrapped(line, wrap=60) # wrap && add os.linesep
                    # self.gmstc.EnsureCaretVisible()
                    p2 = self.gmstc.GetCurrentPos()
                    self.gmstc.StartStyling(p1, 0xff)
                    self.gmstc.SetStyling(p2 - p1 + 1, self.gmstc.StyleUnknown)
            elif len(line) > 0:
                self.message += os.linesep + line.strip()

            if self.printMessage and len(self.message) > 0:
                p1 = self.gmstc.GetCurrentPos()
                if self.type == 'warning':
                    self.message = 'WARNING: ' + self.message
                elif self.type == 'error':
                    self.message = 'ERROR: ' + self.message
                if os.linesep not in self.message:
                    self.gmstc.AddTextWrapped(self.message, wrap=60) #wrap && add os.linesep
                else:
                    self.gmstc.AddText(self.message + os.linesep)
                # self.gmstc.EnsureCaretVisible()
                p2 = self.gmstc.GetCurrentPos()
                self.gmstc.StartStyling(p1, 0xff)
                if self.type == 'error':
                    self.gmstc.SetStyling(p2 - p1 + 1, self.gmstc.StyleError)
                elif self.type == 'warning':
                    self.gmstc.SetStyling(p2 - p1 + 1, self.gmstc.StyleWarning)
                elif self.type == 'self.message':
                    self.gmstc.SetStyling(p2 - p1 + 1, self.gmstc.StyleSelf.Message)

                self.type = ''
                self.message = ''

class GMStc(wx.stc.StyledTextCtrl):
    """Styled GMConsole

    Based on FrameOutErr.py

    Name:      FrameOutErr.py
    Purpose:   Redirecting stdout / stderr
    Author:    Jean-Michel Fauth, Switzerland
    Copyright: (c) 2005-2007 Jean-Michel Fauth
    Licence:   GPL
    """    
    def __init__(self, parent, id, margin=False, wrap=None):
        wx.stc.StyledTextCtrl.__init__(self, parent, id)
        self.parent = parent
        self.wrap = wrap

        #
        # styles
        #
        self.StyleDefault     = 0
        self.StyleDefaultSpec = "face:Courier New,size:10,fore:#000000,back:#FFFFFF"
        self.StyleCommand     = 1
        self.StyleCommandSpec = "face:Courier New,size:10,fore:#000000,back:#bcbcbc"
        self.StyleOutput      = 2
        self.StyleOutputSpec  = "face:Courier New,size:10,fore:#000000,back:#FFFFFF"
        # fatal error
        self.StyleError       = 3
        self.StyleErrorSpec   = "face:Courier New,size:10,fore:#7F0000,back:#FFFFFF"
        # warning
        self.StyleWarning     = 4
        self.StyleWarningSpec = "face:Courier New,size:10,fore:#0000FF,back:#FFFFFF"
        # message
        self.StyleMessage     = 5
        self.StyleMessageSpec = "face:Courier New,size:10,fore:#000000,back:#FFFFFF"
        # unknown
        self.StyleUnknown     = 6
        self.StyleUnknownSpec = "face:Courier New,size:10,fore:#7F0000,back:#FFFFFF"
        
        # default and clear => init
        self.StyleSetSpec(wx.stc.STC_STYLE_DEFAULT, self.StyleDefaultSpec)
        self.StyleClearAll()
        self.StyleSetSpec(self.StyleCommand, self.StyleCommandSpec)
        self.StyleSetSpec(self.StyleOutput,  self.StyleOutputSpec)
        self.StyleSetSpec(self.StyleError,   self.StyleErrorSpec)
        self.StyleSetSpec(self.StyleWarning, self.StyleWarningSpec)
        self.StyleSetSpec(self.StyleMessage, self.StyleMessageSpec)
        self.StyleSetSpec(self.StyleUnknown, self.StyleUnknownSpec)

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
        self.SetTabWidth(4)
        self.SetUseTabs(False)
        self.UsePopUp(True)
        self.SetSelBackground(True, "#FFFF00")
        self.SetUseHorizontalScrollBar(True)

        #
        # bindins
        #
        self.Bind(wx.EVT_WINDOW_DESTROY, self.OnDestroy)

    def OnDestroy(self, evt):
        """The clipboard contents can be preserved after
        the app has exited"""
        
        wx.TheClipboard.Flush()
        evt.Skip()

    def AddTextWrapped(self, str, wrap=None):
        """Add string to text area.

        String is wrapped and linesep is also added to the end
        of the string"""
        if wrap is None and self.wrap:
            wrap = self.wrap

        if wrap is not None:
            str = textwrap.fill(str, wrap) + os.linesep
        else:
            str += os.linesep

        self.AddText(str)

    def SetWrap(self, wrap):
        """Set wrapping value

        @param wrap wrapping value

        @return current wrapping value
        """
        if wrap > 0:
            self.wrap = wrap

        return self.wrap
