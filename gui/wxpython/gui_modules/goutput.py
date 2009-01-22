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
@author Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import textwrap
import time
import threading
import Queue

import wx
import wx.stc
from wx.lib.newevent import NewEvent

import globalvar
import gcmd
import utils
from debug import Debug as Debug
from preferences import globalSettings as UserSettings

wxCmdOutput,   EVT_CMD_OUTPUT   = NewEvent()
wxCmdProgress, EVT_CMD_PROGRESS = NewEvent()
wxCmdRun,      EVT_CMD_RUN      = NewEvent()
wxCmdDone,     EVT_CMD_DONE     = NewEvent()
wxCmdAbort,    EVT_CMD_ABORT    = NewEvent()

def GrassCmd(cmd, stdout, stderr):
    """Return GRASS command thread"""
    return gcmd.CommandThread(cmd,
                              stdout=stdout, stderr=stderr)

class CmdThread(threading.Thread):
    """Thread for GRASS commands"""
    requestId = 0
    def __init__(self, parent, requestQ, resultQ, **kwds):
        threading.Thread.__init__(self, **kwds)

        self.setDaemon(True)

        self.parent = parent # GMConsole
        
        self.requestQ = requestQ
        self.resultQ = resultQ

        self.start()

    def RunCmd(self, callable, *args, **kwds):
        CmdThread.requestId += 1

        self.requestCmd = None
        self.requestQ.put((CmdThread.requestId, callable, args, kwds))
        
        return CmdThread.requestId

    def run(self):
        while True:
            requestId, callable, args, kwds = self.requestQ.get()
            
            requestTime = time.time()
            event = wxCmdRun(cmd=args[0],
                             pid=requestId)
            wx.PostEvent(self.parent, event)

            time.sleep(.1)
            
            self.requestCmd = callable(*args, **kwds)

            self.resultQ.put((requestId, self.requestCmd.run()))

            try:
                returncode = self.requestCmd.module.returncode
            except AttributeError:
                returncode = 0 # being optimistic
            
            try:
                aborted = self.requestCmd.aborted
            except AttributeError:
                aborted = False
            
            time.sleep(.1)
            
            event = wxCmdDone(aborted=aborted,
                              returncode=returncode,
                              time=requestTime,
                              pid=requestId)
            
            wx.PostEvent(self.parent, event)

    def abort(self):
        self.requestCmd.abort()
    
class GMConsole(wx.Panel):
    """
    Create and manage output console for commands entered on the
    GIS Manager command line.
    """
    def __init__(self, parent, id=wx.ID_ANY, margin=False, pageid=0,
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.TAB_TRAVERSAL | wx.FULL_REPAINT_ON_RESIZE):
        wx.Panel.__init__(self, parent, id, pos, size, style)
        self.SetName("GMConsole")
        
        # initialize variables
        self.Map             = None
        self.parent          = parent # GMFrame | CmdPanel
        self.lineWidth       = 80
        self.pageid          = pageid
        # remember position of line begining (used for '\r')
        self.linePos         = -1
        
        #
        # create queues
        #
        self.requestQ = Queue.Queue()
        self.resultQ = Queue.Queue()

        #
        # progress bar
        #
        self.console_progressbar = wx.Gauge(parent=self, id=wx.ID_ANY,
                                            range=100, pos=(110, 50), size=(-1, 25),
                                            style=wx.GA_HORIZONTAL)
        self.console_progressbar.Bind(EVT_CMD_PROGRESS, self.OnCmdProgress)
        
        #
        # text control for command output
        #
        self.cmd_output = GMStc(parent=self, id=wx.ID_ANY, margin=margin,
                                wrap=None) 
        self.cmd_output_timer = wx.Timer(self.cmd_output, id=wx.ID_ANY)
        self.cmd_output.Bind(EVT_CMD_OUTPUT, self.OnCmdOutput)
        self.cmd_output.Bind(wx.EVT_TIMER, self.OnProcessPendingOutputWindowEvents)
        self.Bind(EVT_CMD_RUN, self.OnCmdRun)
        self.Bind(EVT_CMD_DONE, self.OnCmdDone)
        
        #
        # stream redirection
        #
        self.cmd_stdout = GMStdout(self)
        self.cmd_stderr = GMStderr(self)

        #
        # thread
        #
        self.cmdThread = CmdThread(self, self.requestQ, self.resultQ)

        #
        # buttons
        #
        self.console_clear = wx.Button(parent=self, id=wx.ID_CLEAR)
        self.console_save  = wx.Button(parent=self, id=wx.ID_SAVE)
        self.Bind(wx.EVT_BUTTON, self.ClearHistory, self.console_clear)
        self.Bind(wx.EVT_BUTTON, self.SaveHistory,  self.console_save)

        self.Bind(EVT_CMD_ABORT, self.OnCmdAbort)
        
        self.__layout()

    def __layout(self):
        """Do layout"""
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
            sys.stdout = self.cmd_stdout
            sys.stderr = self.cmd_stderr
            
            return True

        return False

    def WriteLog(self, text, style=None, wrap=None):
        """Generic method for writing log message in 
        given style

        @param line text line
        @param style text style (see GMStc)
        @param stdout write to stdout or stderr
        """
        if not style:
            style = self.cmd_output.StyleDefault
        
        # p1 = self.cmd_output.GetCurrentPos()
        p1 = self.cmd_output.GetEndStyled()
        self.cmd_output.GotoPos(p1)
        
        for line in text.splitlines():
            # fill space
            if len(line) < self.lineWidth:
                diff = self.lineWidth - len(line) 
                line += diff * ' '
            
            self.cmd_output.AddTextWrapped(line, wrap=wrap) # adds os.linesep
            
            p2 = self.cmd_output.GetCurrentPos()
            
            self.cmd_output.StartStyling(p1, 0xff)
            self.cmd_output.SetStyling(p2 - p1, style)
        
        self.cmd_output.EnsureCaretVisible()
        
    def WriteCmdLog(self, line, pid=None):
        """Write out line in selected style"""
        if pid:
            line = '(' + str(pid) + ') ' + line
        self.WriteLog(line, style=self.cmd_output.StyleCommand)

    def WriteWarning(self, line):
        """Write out line in warning style"""
        self.WriteLog(line, style=self.cmd_output.StyleWarning)

    def RunCmd(self, command, compReg=True, switchPage=False):
        """
        Run in GUI GRASS (or other) commands typed into
        console command text widget, and send stdout output to output
        text widget.

        Command is transformed into a list for processing.

        TODO: Display commands (*.d) are captured and
        processed separately by mapdisp.py. Display commands are
        rendered in map display widget that currently has
        the focus (as indicted by mdidx).

        @param command command (list)
        @param compReg if true use computation region
        @param switchPage switch to output page
        """
        
        # map display window available ?
        try:
            curr_disp = self.parent.curr_page.maptree.mapdisplay
            self.Map = curr_disp.GetRender()
        except:
            curr_disp = None

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
                
                # switch to 'Command output'
                if switchPage and \
                        self.parent.notebook.GetSelection() != self.parent.goutput.pageid:
                    self.parent.notebook.SetSelection(self.parent.goutput.pageid)

                # activate computational region (set with g.region)
                # for all non-display commands.
                if compReg:
                    tmpreg = os.getenv("GRASS_REGION")
                    if os.environ.has_key("GRASS_REGION"):
                        del os.environ["GRASS_REGION"]
                    
                if len(cmdlist) == 1:
                    import menuform
                    # process GRASS command without argument
                    menuform.GUI().ParseCommand(cmdlist, parentframe=self)
                else:
                    # process GRASS command with argument
                    self.cmdThread.RunCmd(GrassCmd,
                                          cmdlist,
                                          self.cmd_stdout, self.cmd_stderr)
                    
                    self.cmd_output_timer.Start(50)

                    return None
                
                # deactivate computational region and return to display settings
                if compReg and tmpreg:
                    os.environ["GRASS_REGION"] = tmpreg
        else:
            # Send any other command to the shell. Send output to
            # console output window

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

    def GetCmd(self):
        """Get running command or None"""
        return self.requestQ.get()
        
    def OnCmdOutput(self, event):
        """Print command output"""
        message = event.text
        type  = event.type
        
        # switch to 'Command output'
        ### if self.parent.notebook.GetSelection() != self.parent.goutput.pageid:
        ### self.parent.notebook.SetSelection(self.parent.goutput.pageid)
        if self.parent.notebook.GetSelection() != self.parent.goutput.pageid:
            textP = self.parent.notebook.GetPageText(self.parent.goutput.pageid)
            if textP[-1] != ')':
                textP += ' (...)'
            self.parent.notebook.SetPageText(self.parent.goutput.pageid,
                                             textP)
        
        # message prefix
        if type == 'warning':
            messege = 'WARNING: ' + message
        elif type == 'error':
            message = 'ERROR: ' + message
        
        p1 = self.cmd_output.GetEndStyled()
        self.cmd_output.GotoPos(p1)
        
        if '\b' in message:
            if self.linepos < 0:
                self.linepos = p1
            last_c = ''
            for c in message:
                if c == '\b':
                   self.linepos -= 1
                else:
                    if c == '\r':
                        pos = self.cmd_output.GetCurLine()[1]
                        # self.cmd_output.SetCurrentPos(pos)
                    else:
                        self.cmd_output.SetCurrentPos(self.linepos)
                    self.cmd_output.ReplaceSelection(c)
                    self.linepos = self.cmd_output.GetCurrentPos()
                    if c != ' ':
                        last_c = c
            if last_c not in ('0123456789'):
                self.cmd_output.AddTextWrapped('\n', wrap=None)
                self.linepos = -1
        else:
            self.linepos = -1 # don't force position
            if os.linesep not in message:
                self.cmd_output.AddTextWrapped(message, wrap=60)
            else:
                self.cmd_output.AddTextWrapped(message, wrap=None)
        
        p2 = self.cmd_output.GetCurrentPos()
        
        if p2 >= p1:
            self.cmd_output.StartStyling(p1, 0xff)
        
            if type == 'error':
                self.cmd_output.SetStyling(p2 - p1, self.cmd_output.StyleError)
            elif type == 'warning':
                self.cmd_output.SetStyling(p2 - p1, self.cmd_output.StyleWarning)
            elif type == 'message':
                self.cmd_output.SetStyling(p2 - p1, self.cmd_output.StyleMessage)
            else: # unknown
                self.cmd_output.SetStyling(p2 - p1, self.cmd_output.StyleUnknown)
        
        self.cmd_output.EnsureCaretVisible()
        
    def OnCmdProgress(self, event):
        """Update progress message info"""
        self.console_progressbar.SetValue(event.value)

    def OnCmdAbort(self, event):
        """Abort running command"""
        self.cmdThread.abort()

    def OnCmdRun(self, event):
        """Run command"""
        self.WriteCmdLog('(%s)\n%s' % (str(time.ctime()), ' '.join(event.cmd)))

    def OnCmdDone(self, event):
        """Command done (or aborted)"""
        if event.aborted:
            # Thread aborted (using our convention of None return)
            self.WriteLog(_('Please note that the data are left in incosistent stage '
                            'and can be corrupted'), self.cmd_output.StyleWarning)
            self.WriteCmdLog('(%s) %s (%d sec)' % (str(time.ctime()),
                                                   _('Command aborted'),
                                                   (time.time() - event.time)))
            # pid=self.cmdThread.requestId)
        else:
            try:
                # Process results here
                self.WriteCmdLog('(%s) %s (%d sec)' % (str(time.ctime()),
                                                       _('Command finished'),
                                                       (time.time() - event.time)))
                # pid=event.pid)
            except KeyError:
                # stopped deamon
                pass
        
        self.console_progressbar.SetValue(0) # reset progress bar on '0%'

        self.cmd_output_timer.Stop()

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

            if event.returncode == 0 and \
                    not event.aborted and hasattr(dialog, "addbox") and \
                    dialog.addbox.IsChecked():
                # add new map into layer tree
                if dialog.outputType in ('raster', 'vector'):
                    # add layer into layer tree
                    cmd = dialog.notebookpanel.createCmd(ignoreErrors = True)
                    name = utils.GetLayerNameFromCmd(cmd, fullyQualified=True, param='output')
                    winName = self.parent.parent.parent.GetName()
                    if winName == 'LayerManager':
                        mapTree = self.parent.parent.parent.curr_page.maptree
                    else: # GMConsole
                        mapTree = self.parent.parent.parent.parent.curr_page.maptree
                    
                    if dialog.outputType == 'raster':
                        lcmd = ['d.rast',
                                'map=%s' % name]
                    else:
                        lcmd = ['d.vect',
                                'map=%s' % name]
                    mapTree.AddLayer(ltype=dialog.outputType,
                                     lcmd=lcmd,
                                     lname=name)
            
            if dialog.get_dcmd is None and \
                   dialog.closebox.IsChecked():
                time.sleep(1)
                dialog.Close()

        event.Skip()
        
    def OnProcessPendingOutputWindowEvents(self, event):
        self.ProcessPendingEvents()

class GMStdout:
    """GMConsole standard output

    Based on FrameOutErr.py

    Name:      FrameOutErr.py
    Purpose:   Redirecting stdout / stderr
    Author:    Jean-Michel Fauth, Switzerland
    Copyright: (c) 2005-2007 Jean-Michel Fauth
    Licence:   GPL
    """
    def __init__(self, parent):
        self.parent = parent # GMConsole

    def write(self, s):
        if len(s) == 0 or s == '\n':
            return
        
        s = s.replace('\n', os.linesep)
        
        for line in s.split(os.linesep):
            if len(line) == 0:
                continue
            
            evt = wxCmdOutput(text=line + os.linesep,
                              type='')
            wx.PostEvent(self.parent.cmd_output, evt)
        
class GMStderr:
    """GMConsole standard error output

    Based on FrameOutErr.py

    Name:      FrameOutErr.py
    Purpose:   Redirecting stdout / stderr
    Author:    Jean-Michel Fauth, Switzerland
    Copyright: (c) 2005-2007 Jean-Michel Fauth
    Licence:   GPL
    """
    def __init__(self, parent):
        self.parent = parent # GMConsole
 
        self.type = ''
        self.message = ''
        self.printMessage = False
        
    def write(self, s):
        if "GtkPizza" in s:
            return
        
        s = s.replace('\n', os.linesep)
        # remove/replace escape sequences '\b' or '\r' from stream
        progressValue = -1
        
        for line in s.split(os.linesep):
            if len(line) == 0:
                continue

            if 'GRASS_INFO_PERCENT' in line:
                value = int(line.rsplit(':', 1)[1].strip())
                if value >= 0 and value < 100:
                    progressValue = value
                else:
                    progressValue = 0
            elif 'GRASS_INFO_MESSAGE' in line:
                self.type = 'message'
                self.message = line.split(':', 1)[1].strip()
            elif 'GRASS_INFO_WARNING' in line:
                self.type = 'warning'
                self.message = line.split(':', 1)[1].strip()
            elif 'GRASS_INFO_ERROR' in line:
                self.type = 'error'
                self.message = line.split(':', 1)[1].strip()
            elif 'GRASS_INFO_END' in line:
                self.printMessage = True
            elif self.type == '':
                if len(line) == 0:
                    continue
                evt = wxCmdOutput(text=line,
                                  type='')
                wx.PostEvent(self.parent.cmd_output, evt)
            elif len(line) > 0:
                self.message += line.strip() + os.linesep

            if self.printMessage and len(self.message) > 0:
                evt = wxCmdOutput(text=self.message,
                                  type=self.type)
                wx.PostEvent(self.parent.cmd_output, evt)

                self.type = ''
                self.message = ''
                self.printMessage = False

        # update progress message
        if progressValue > -1:
            # self.gmgauge.SetValue(progressValue)
            evt = wxCmdProgress(value=progressValue)
            wx.PostEvent(self.parent.console_progressbar, evt)
            
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
        self.StyleUnknownSpec = "face:Courier New,size:10,fore:#000000,back:#FFFFFF"
        
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

    def AddTextWrapped(self, txt, wrap=None):
        """Add string to text area.

        String is wrapped and linesep is also added to the end
        of the string"""
        if wrap:
            txt = textwrap.fill(txt, wrap) + os.linesep
        else:
            if txt[-1] != os.linesep:
                txt += os.linesep
        
        if '\r' in txt:
            self.parent.linePos = -1
            for seg in txt.split('\r'):
                if self.parent.linePos > -1:
                    self.SetCurrentPos(self.parent.linePos)
                    self.ReplaceSelection(seg)
                else:
                    self.parent.linePos = self.GetCurrentPos()
                    self.AddText(seg)
        else:
            self.parent.linePos = self.GetCurrentPos()
            try:
                self.AddText(txt)
            except UnicodeDecodeError:
                enc = UserSettings.Get(group='atm', key='encoding', subkey='value')
                if enc:
                    txt = unicode(txt, enc)
                elif os.environ.has_key('GRASS_DB_ENCODING'):
                    txt = unicode(txt, os.environ['GRASS_DB_ENCODING'])
                else:
                    txt = _('Unable to encode text. Please set encoding in GUI preferences.') + os.linesep
                    
                self.AddText(txt) 
    
