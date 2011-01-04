"""!
@package goutput

@brief Command output log widget

Classes:
 - GMConsole
 - GMStc
 - GMStdout
 - GMStderr

(C) 2007-2010 by the GRASS Development Team
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

import grass.script as grass

import globalvar
import gcmd
import utils
import preferences
import menuform
import prompt

from debug       import Debug
from preferences import globalSettings as UserSettings
from ghelp       import SearchModuleWindow

wxCmdOutput,   EVT_CMD_OUTPUT   = NewEvent()
wxCmdProgress, EVT_CMD_PROGRESS = NewEvent()
wxCmdRun,      EVT_CMD_RUN      = NewEvent()
wxCmdDone,     EVT_CMD_DONE     = NewEvent()
wxCmdAbort,    EVT_CMD_ABORT    = NewEvent()

def GrassCmd(cmd, stdout, stderr):
    """!Return GRASS command thread"""
    return gcmd.CommandThread(cmd,
                              stdout=stdout, stderr=stderr)

class CmdThread(threading.Thread):
    """!Thread for GRASS commands"""
    requestId = 0
    def __init__(self, parent, requestQ, resultQ, **kwds):
        threading.Thread.__init__(self, **kwds)

        self.setDaemon(True)

        self.parent = parent # GMConsole
        self._want_abort_all = False
        
        self.requestQ = requestQ
        self.resultQ = resultQ

        self.start()

    def RunCmd(self, callable, onDone, *args, **kwds):
        CmdThread.requestId += 1
        
        self.requestCmd = None
        self.requestQ.put((CmdThread.requestId, callable, onDone, args, kwds))
        
        return CmdThread.requestId

    def SetId(self, id):
        """!Set starting id"""
        CmdThread.requestId = id
        
    def run(self):
        os.environ['GRASS_MESSAGE_FORMAT'] = 'gui'
        while True:
            requestId, callable, onDone, args, kwds = self.requestQ.get()
            
            requestTime = time.time()
            event = wxCmdRun(cmd=args[0],
                             pid=requestId)
            
            wx.PostEvent(self.parent, event)
            
            time.sleep(.1)
            
            self.requestCmd = callable(*args, **kwds)
            if self._want_abort_all:
                self.requestCmd.abort()
                if self.requestQ.empty():
                    self._want_abort_all = False
            
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

            # set default color table for raster data
            if UserSettings.Get(group='cmd', key='rasterColorTable', subkey='enabled') and \
                    args[0][0][:2] == 'r.':
                colorTable = UserSettings.Get(group='cmd', key='rasterColorTable', subkey='selection')
                mapName = None
                if args[0][0] == 'r.mapcalc':
                    try:
                        mapName = args[0][1].split('=', 1)[0].strip()
                    except KeyError:
                        pass
                else:
                    moduleInterface = menuform.GUI().ParseCommand(args[0], show = None)
                    outputParam = moduleInterface.get_param(value = 'output', raiseError = False)
                    if outputParam and outputParam['prompt'] == 'raster':
                        mapName = outputParam['value']
                
                if mapName:
                    argsColor = list(args)
                    argsColor[0] = [ 'r.colors',
                                     'map=%s' % mapName,
                                     'color=%s' % colorTable ]
                    self.requestCmdColor = callable(*argsColor, **kwds)
                    self.resultQ.put((requestId, self.requestCmdColor.run()))
            
            event = wxCmdDone(cmd = args[0],
                              aborted = aborted,
                              returncode = returncode,
                              time = requestTime,
                              pid = requestId,
                              onDone = onDone)
            
            # send event
            wx.PostEvent(self.parent, event)
                
    def abort(self, abortall = True):
        """!Abort command(s)"""
        if abortall:
            self._want_abort_all = True
        self.requestCmd.abort()
        if self.requestQ.empty():
            self._want_abort_all = False
        
class GMConsole(wx.SplitterWindow):
    """!Create and manage output console for commands run by GUI.
    """
    def __init__(self, parent, id=wx.ID_ANY, margin=False, pageid=0,
                 notebook = None,
                 style=wx.TAB_TRAVERSAL | wx.FULL_REPAINT_ON_RESIZE,
                 **kwargs):
        wx.SplitterWindow.__init__(self, parent, id, style = style, *kwargs)
        self.SetName("GMConsole")
        
        self.panelOutput = wx.Panel(parent = self, id = wx.ID_ANY)
        self.panelPrompt = wx.Panel(parent = self, id = wx.ID_ANY)
        
        # initialize variables
        self.parent          = parent # GMFrame | CmdPanel | ?
        if notebook:
            self._notebook = notebook
        else:
            self._notebook = self.parent.notebook
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
        self.console_progressbar = wx.Gauge(parent=self.panelOutput, id=wx.ID_ANY,
                                            range=100, pos=(110, 50), size=(-1, 25),
                                            style=wx.GA_HORIZONTAL)
        self.console_progressbar.Bind(EVT_CMD_PROGRESS, self.OnCmdProgress)
        
        #
        # text control for command output
        #
        self.cmd_output = GMStc(parent=self.panelOutput, id=wx.ID_ANY, margin=margin,
                                wrap=None) 
        self.cmd_output_timer = wx.Timer(self.cmd_output, id=wx.ID_ANY)
        self.cmd_output.Bind(EVT_CMD_OUTPUT, self.OnCmdOutput)
        self.cmd_output.Bind(wx.EVT_TIMER, self.OnProcessPendingOutputWindowEvents)
        self.Bind(EVT_CMD_RUN, self.OnCmdRun)
        self.Bind(EVT_CMD_DONE, self.OnCmdDone)
        
        # search & command prompt
        self.cmd_prompt = prompt.GPromptSTC(parent = self)
        
        if self.parent.GetName() != 'LayerManager':
            self.search = None
            self.cmd_prompt.Hide()
        else:
            self.infoCollapseLabelExp = _("Click here to show search module engine")
            self.infoCollapseLabelCol = _("Click here to hide search module engine")
            self.searchPane = wx.CollapsiblePane(parent = self.panelOutput,
                                                 label = self.infoCollapseLabelExp,
                                                 style = wx.CP_DEFAULT_STYLE |
                                                 wx.CP_NO_TLW_RESIZE | wx.EXPAND)
            self.MakeSearchPaneContent(self.searchPane.GetPane())
            self.searchPane.Collapse(True)
            self.Bind(wx.EVT_COLLAPSIBLEPANE_CHANGED, self.OnSearchPaneChanged, self.searchPane) 
            self.search.Bind(wx.EVT_TEXT,             self.OnUpdateStatusBar)
        
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
        self.btn_console_clear = wx.Button(parent = self.panelPrompt, id = wx.ID_ANY,
                                           label = _("C&lear output"), size=(125,-1))
        self.btn_cmd_clear = wx.Button(parent = self.panelPrompt, id = wx.ID_ANY,
                                       label = _("&Clear command"), size=(125,-1))
        if self.parent.GetName() != 'LayerManager':
            self.btn_cmd_clear.Hide()
        self.btn_console_save  = wx.Button(parent = self.panelPrompt, id = wx.ID_ANY,
                                           label = _("&Save output"), size=(125,-1))
        # abort
        self.btn_abort = wx.Button(parent = self.panelPrompt, id = wx.ID_ANY, label = _("&Abort command"),
                                   size=(125,-1))
        self.btn_abort.SetToolTipString(_("Abort the running command"))
        self.btn_abort.Enable(False)
        
        self.btn_cmd_clear.Bind(wx.EVT_BUTTON,     self.cmd_prompt.OnCmdErase)
        self.btn_console_clear.Bind(wx.EVT_BUTTON, self.ClearHistory)
        self.btn_console_save.Bind(wx.EVT_BUTTON,  self.SaveHistory)
        self.btn_abort.Bind(wx.EVT_BUTTON,         self.OnCmdAbort)
        self.btn_abort.Bind(EVT_CMD_ABORT,         self.OnCmdAbort)
        
        self.__layout()
        
    def __layout(self):
        """!Do layout"""
        OutputSizer = wx.BoxSizer(wx.VERTICAL)
        PromptSizer = wx.BoxSizer(wx.VERTICAL)
        ButtonSizer = wx.BoxSizer(wx.HORIZONTAL)

        if self.search and self.search.IsShown():
            OutputSizer.Add(item=self.searchPane, proportion=0,
                            flag=wx.EXPAND | wx.ALL, border=3)
        OutputSizer.Add(item=self.cmd_output, proportion=1,
                        flag=wx.EXPAND | wx.ALL, border=3)
        OutputSizer.Add(item=self.console_progressbar, proportion=0,
                        flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=3)
        
        PromptSizer.Add(item=self.cmd_prompt, proportion=1,
                        flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, border=3)
        
        ButtonSizer.Add(item=self.btn_console_clear, proportion=0,
                        flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE | wx.ALL, border=5)
        ButtonSizer.Add(item=self.btn_console_save, proportion=0,
                        flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE | wx.ALL, border=5)
        ButtonSizer.Add(item=self.btn_cmd_clear, proportion=0,
                        flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE | wx.ALL, border=5)
        ButtonSizer.Add(item=self.btn_abort, proportion=0,
                        flag=wx.ALIGN_CENTER | wx.FIXED_MINSIZE | wx.ALL, border=5)
        PromptSizer.Add(item=ButtonSizer, proportion=0,
                        flag=wx.ALIGN_CENTER)
        
        OutputSizer.Fit(self)
        OutputSizer.SetSizeHints(self)
        
        PromptSizer.Fit(self)
        PromptSizer.SetSizeHints(self)
        
        self.panelOutput.SetSizer(OutputSizer)
        self.panelPrompt.SetSizer(PromptSizer)
        
        # split window
        if self.parent.GetName() == 'LayerManager':
            self.SplitHorizontally(self.panelOutput, self.panelPrompt, -50)
            self.SetMinimumPaneSize(self.btn_cmd_clear.GetSize()[1] + 50)
        else:
            self.SplitHorizontally(self.panelOutput, self.panelPrompt, -45)
            self.SetMinimumPaneSize(self.btn_cmd_clear.GetSize()[1] + 10)
        
        self.SetSashGravity(1.0)
        
        # layout
        self.SetAutoLayout(True)
        self.Layout()

    def MakeSearchPaneContent(self, pane):
        """!Create search pane"""
        border = wx.BoxSizer(wx.VERTICAL)
        
        self.search = SearchModuleWindow(parent = pane, cmdPrompt = self.cmd_prompt)
        
        border.Add(item = self.search, proportion = 0,
                   flag = wx.EXPAND | wx.ALL, border = 1)
        
        pane.SetSizer(border)
        border.Fit(pane)
        
    def OnSearchPaneChanged(self, event):
        """!Collapse search module box"""
        if self.searchPane.IsExpanded():
            self.searchPane.SetLabel(self.infoCollapseLabelCol)
        else:
            self.searchPane.SetLabel(self.infoCollapseLabelExp)
        
        self.panelOutput.Layout()
        self.panelOutput.SendSizeEvent()
        
    def GetPanel(self, prompt = True):
        """!Get panel

        @param prompt get prompt / output panel

        @return wx.Panel reference
        """
        if prompt:
            return self.panelPrompt

        return self.panelOutput
    
    def Redirect(self):
        """!Redirect stdout/stderr
        
        @return True redirected
        @return False failed
        """
        if Debug.GetLevel() == 0 and int(grass.gisenv().get('DEBUG', 0)) == 0:
            # don't redirect when debugging is enabled
            sys.stdout = self.cmd_stdout
            sys.stderr = self.cmd_stderr
            return True
        else:
            sys.stdout = sys.__stdout__
            sys.stderr = sys.__stderr__
        return False

    def WriteLog(self, text, style = None, wrap = None,
                 switchPage = False):
        """!Generic method for writing log message in 
        given style

        @param line text line
        @param style text style (see GMStc)
        @param stdout write to stdout or stderr
        """

        self.cmd_output.SetStyle()

        if switchPage and \
                self._notebook.GetSelection() != self.parent.goutput.pageid:
            self._notebook.SetSelection(self.parent.goutput.pageid)
        
        if not style:
            style = self.cmd_output.StyleDefault
        
        # p1 = self.cmd_output.GetCurrentPos()
        p1 = self.cmd_output.GetEndStyled()
#        self.cmd_output.GotoPos(p1)
        self.cmd_output.DocumentEnd()
        
        for line in text.splitlines():
            # fill space
            if len(line) < self.lineWidth:
                diff = self.lineWidth - len(line) 
                line += diff * ' '
            
            self.cmd_output.AddTextWrapped(line, wrap=wrap) # adds '\n'
            
            p2 = self.cmd_output.GetCurrentPos()
            
            self.cmd_output.StartStyling(p1, 0xff)
            self.cmd_output.SetStyling(p2 - p1, style)
        
        self.cmd_output.EnsureCaretVisible()
        
    def WriteCmdLog(self, line, pid=None):
        """!Write message in selected style"""
        if pid:
            line = '(' + str(pid) + ') ' + line
        self.WriteLog(line, style=self.cmd_output.StyleCommand, switchPage = True)

    def WriteWarning(self, line):
        """!Write message in warning style"""
        self.WriteLog(line, style=self.cmd_output.StyleWarning, switchPage = True)

    def WriteError(self, line):
        """!Write message in error style"""
        self.WriteLog(line, style=self.cmd_output.StyleError, switchPage = True)

    def RunCmd(self, command, compReg = True, switchPage = False,
               onDone = None):
        """!Run command typed into console command prompt (GPrompt).
        
        @todo Display commands (*.d) are captured and processed
        separately by mapdisp.py. Display commands are rendered in map
        display widget that currently has the focus (as indicted by
        mdidx).
        
        @param command command given as a list (produced e.g. by shlex.split())
        @param compReg True use computation region
        @param switchPage switch to output page
        @param onDone function to be called when command is finished
        """
        if len(command) == 0:
            Debug.msg(2, "GPrompt:RunCmd(): empty command")
            return
        
        # update history file
        env = grass.gisenv()
        try:
            fileHistory = open(os.path.join(env['GISDBASE'], env['LOCATION_NAME'], env['MAPSET'],
                                            '.bash_history'), 'a')
        except IOError, e:
            self.WriteError(str(e))
            fileHistory = None
        
        if fileHistory:
            try:
                fileHistory.write(' '.join(command) + os.linesep)
            finally:
                fileHistory.close()
        
        # update history items
        if self.parent.GetName() == 'LayerManager':
            try:
                self.parent.cmdinput.SetHistoryItems()
            except AttributeError:
                pass
        
        if command[0] in globalvar.grassCmd['all']:
            # send GRASS command without arguments to GUI command interface
            # except display commands (they are handled differently)
            if self.parent.GetName() == "LayerManager" and \
                    command[0][0:2] == "d.":
                # display GRASS commands
                try:
                    layertype = {'d.rast'         : 'raster',
                                 'd.rast3d'       : '3d-raster',
                                 'd.rgb'          : 'rgb',
                                 'd.his'          : 'his',
                                 'd.shaded'       : 'shaded',
                                 'd.legend'       : 'rastleg',
                                 'd.rast.arrow'   : 'rastarrow',
                                 'd.rast.num'     : 'rastnum',
                                 'd.vect'         : 'vector',
                                 'd.thematic.area': 'thememap',
                                 'd.vect.chart'   : 'themechart',
                                 'd.grid'         : 'grid',
                                 'd.geodesic'     : 'geodesic',
                                 'd.rhumbline'    : 'rhumb',
                                 'd.labels'       : 'labels'}[command[0]]
                except KeyError:
                    gcmd.GMessage(parent = self.parent,
                                  message = _("Command '%s' not yet implemented in the WxGUI. "
                                              "Try adding it as a command layer instead.") % command[0])
                    return None
                
                # add layer into layer tree
                if command[0] == 'd.rast':
                    lname = utils.GetLayerNameFromCmd(command, fullyQualified = True,
                                                      layerType = 'raster')
                elif command[0] == 'd.vect':
                    lname = utils.GetLayerNameFromCmd(command, fullyQualified = True,
                                                      layerType = 'vector')
                else:
                    lname = None
                
                if self.parent.GetName() == "LayerManager":
                    self.parent.curr_page.maptree.AddLayer(ltype=layertype,
                                                           lname=lname,
                                                           lcmd=command)
            
            else:
                # other GRASS commands (r|v|g|...)
                # switch to 'Command output' if required
                if switchPage:
                    if self._notebook.GetSelection() != self.parent.goutput.pageid:
                        self._notebook.SetSelection(self.parent.goutput.pageid)
                    
                    self.parent.SetFocus()
                    self.parent.Raise()
                
                # activate computational region (set with g.region)
                # for all non-display commands.
                if compReg:
                    tmpreg = os.getenv("GRASS_REGION")
                    if os.environ.has_key("GRASS_REGION"):
                        del os.environ["GRASS_REGION"]
                
                if len(command) == 1:
                    import menuform
                    task = menuform.GUI().ParseInterface(command)
                    # if not task.has_required():
                    # task = None # run command
                else:
                    task = None
                
                if task and command[0] not in ('v.krige'):
                    # process GRASS command without argument
                    menuform.GUI().ParseCommand(command, parentframe = self)
                else:
                    # process GRASS command with argument
                    self.cmdThread.RunCmd(GrassCmd,
                                          onDone,
                                          command,
                                          self.cmd_stdout, self.cmd_stderr)                                          
                    self.cmd_output_timer.Start(50)
                    
                    return None
                
                # deactivate computational region and return to display settings
                if compReg and tmpreg:
                    os.environ["GRASS_REGION"] = tmpreg
        else:
            # Send any other command to the shell. Send output to
            # console output window
            if len(command) == 1:
                import menuform
                try:
                    task = menuform.GUI().ParseInterface(command)
                except:
                    task = None
            else:
                task = None
                
            if task:
                # process GRASS command without argument
                menuform.GUI().ParseCommand(command, parentframe = self)
            else:
                self.cmdThread.RunCmd(GrassCmd,
                                      onDone,
                                      command,
                                      self.cmd_stdout, self.cmd_stderr)                                         
            self.cmd_output_timer.Start(50)
        
        return None

    def ClearHistory(self, event):
        """!Clear history of commands"""
        self.cmd_output.SetReadOnly(False)
        self.cmd_output.ClearAll()
        self.cmd_output.SetReadOnly(True)
        self.console_progressbar.SetValue(0)

    def SaveHistory(self, event):
        """!Save history of commands"""
        self.history = self.cmd_output.GetSelectedText()
        if self.history == '':
            self.history = self.cmd_output.GetText()

        # add newline if needed
        if len(self.history) > 0 and self.history[-1] != '\n':
            self.history += '\n'

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
        """!Get running command or None"""
        return self.requestQ.get()
    
    def OnUpdateStatusBar(self, event):
        """!Update statusbar text"""
        if event.GetString():
            nItems = len(self.cmd_prompt.GetCommandItems())
            self.parent.SetStatusText(_('%d modules match') % nItems, 0)
        else:
            self.parent.SetStatusText('', 0)
        
        event.Skip()

    def OnCmdOutput(self, event):
        """!Print command output"""
        message = event.text
        type  = event.type
        if self._notebook.GetSelection() != self.parent.goutput.pageid:
            textP = self._notebook.GetPageText(self.parent.goutput.pageid)
            if textP[-1] != ')':
                textP += ' (...)'
            self._notebook.SetPageText(self.parent.goutput.pageid,
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
            if '\n' not in message:
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
        """!Update progress message info"""
        self.console_progressbar.SetValue(event.value)

    def OnCmdAbort(self, event):
        """!Abort running command"""
        self.cmdThread.abort()
        
    def OnCmdRun(self, event):
        """!Run command"""
        if self.parent.GetName() == 'Modeler':
            self.parent.OnCmdRun(event)
        
        self.WriteCmdLog('(%s)\n%s' % (str(time.ctime()), ' '.join(event.cmd)))
        self.btn_abort.Enable()

    def OnCmdDone(self, event):
        """!Command done (or aborted)"""
        if self.parent.GetName() == 'Modeler':
            self.parent.OnCmdDone(event)
        
        if event.aborted:
            # Thread aborted (using our convention of None return)
            self.WriteLog(_('Please note that the data are left in inconsistent state '
                            'and may be corrupted'), self.cmd_output.StyleWarning)
            self.WriteCmdLog('(%s) %s (%d sec)' % (str(time.ctime()),
                                                   _('Command aborted'),
                                                   (time.time() - event.time)))
            # pid=self.cmdThread.requestId)
            self.btn_abort.Enable(False)
        else:
            try:
                # Process results here
                self.WriteCmdLog('(%s) %s (%d sec)' % (str(time.ctime()),
                                                       _('Command finished'),
                                                       (time.time() - event.time)))
            except KeyError:
                # stopped deamon
                pass

            self.btn_abort.Enable(False)
        
        if event.onDone:
            event.onDone(cmd = event.cmd, returncode = event.returncode)
        
        self.console_progressbar.SetValue(0) # reset progress bar on '0%'

        self.cmd_output_timer.Stop()

        if event.cmd[0] == 'g.gisenv':
            Debug.SetLevel()
            self.Redirect()
        
        if self.parent.GetName() == "LayerManager":
            self.btn_abort.Enable(False)
            if event.cmd[0] not in globalvar.grassCmd['all'] or \
                    event.cmd[0] == 'r.mapcalc':
                return
            
            display = self.parent.GetLayerTree().GetMapDisplay()
            if not display or not display.IsAutoRendered():
                return
            mapLayers = map(lambda x: x.GetName(),
                            display.GetRender().GetListOfLayers(l_type = 'raster') +
                            display.GetRender().GetListOfLayers(l_type = 'vector'))
            
            try:
                task = menuform.GUI().ParseCommand(event.cmd, show = None)
            except gcmd.GException:
                task = None
                return
            
            for p in task.get_options()['params']:
                if p.get('prompt', '') not in ('raster', 'vector'):
                    continue
                mapName = p.get('value', '')
                if '@' not in mapName:
                    mapName = mapName + '@' + grass.gisenv()['MAPSET']
                if mapName in mapLayers:
                    display.GetWindow().UpdateMap(render = True)
                    return
        elif self.parent.GetName() == 'Modeler':
            pass
        else: # standalone dialogs
            dialog = self.parent.parent
            if hasattr(self.parent.parent, "btn_abort"):
                dialog.btn_abort.Enable(False)
            
            if hasattr(self.parent.parent, "btn_cancel"):
                dialog.btn_cancel.Enable(True)
            
            if hasattr(self.parent.parent, "btn_clipboard"):
                dialog.btn_clipboard.Enable(True)
            
            if hasattr(self.parent.parent, "btn_help"):
                dialog.btn_help.Enable(True)
            
            if hasattr(self.parent.parent, "btn_run"):
                dialog.btn_run.Enable(True)
            
            if event.returncode == 0 and not event.aborted:
                try:
                    winName = self.parent.parent.parent.GetName()
                except AttributeError:
                    winName = ''
                
                if winName == 'LayerManager':
                    mapTree = self.parent.parent.parent.GetLayerTree()
                elif winName == 'LayerTree':
                    mapTree = self.parent.parent.parent
                elif winName: # GMConsole
                    mapTree = self.parent.parent.parent.parent.GetLayerTree()
                else:
                    mapTree = None
                
                cmd = dialog.notebookpanel.createCmd(ignoreErrors = True)
                if hasattr(dialog, "addbox") and dialog.addbox.IsChecked():
                    # add created maps into layer tree
                    for p in dialog.task.get_options()['params']:
                        prompt = p.get('prompt', '')
                        if prompt in ('raster', 'vector', '3d-raster') and \
                                p.get('age', 'old') == 'new' and \
                                p.get('value', None):
                            name = utils.GetLayerNameFromCmd(cmd, fullyQualified=True, param = p.get('name', ''))
                            
                            if mapTree.GetMap().GetListOfLayers(l_name = name):
                                continue
                            
                            if prompt == 'raster':
                                lcmd = ['d.rast',
                                        'map=%s' % name]
                            else:
                                lcmd = ['d.vect',
                                        'map=%s' % name]
                            mapTree.AddLayer(ltype = prompt,
                                             lcmd = lcmd,
                                             lname = name)
            
            if hasattr(dialog, "get_dcmd") and \
                    dialog.get_dcmd is None and \
                    hasattr(dialog, "closebox") and \
                    dialog.closebox.IsChecked() and \
                    (event.returncode == 0 or event.aborted):
                self.cmd_output.Update()
                time.sleep(2)
                dialog.Close()
        
    def OnProcessPendingOutputWindowEvents(self, event):
        self.ProcessPendingEvents()

class GMStdout:
    """!GMConsole standard output

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

        for line in s.splitlines():
            if len(line) == 0:
                continue
            
            evt = wxCmdOutput(text=line + '\n',
                              type='')
            wx.PostEvent(self.parent.cmd_output, evt)
        
class GMStderr:
    """!GMConsole standard error output

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
        
    def flush(self):
        pass
    
    def write(self, s):
        if "GtkPizza" in s:
            return
        
        # remove/replace escape sequences '\b' or '\r' from stream
        progressValue = -1
        
        for line in s.splitlines():
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
                self.message += line.split(':', 1)[1].strip() + '\n'
            elif 'GRASS_INFO_WARNING' in line:
                self.type = 'warning'
                self.message += line.split(':', 1)[1].strip() + '\n'
            elif 'GRASS_INFO_ERROR' in line:
                self.type = 'error'
                self.message += line.split(':', 1)[1].strip() + '\n'
            elif 'GRASS_INFO_END' in line:
                self.printMessage = True
            elif self.type == '':
                if len(line) == 0:
                    continue
                evt = wxCmdOutput(text=line,
                                  type='')
                wx.PostEvent(self.parent.cmd_output, evt)
            elif len(line) > 0:
                self.message += line.strip() + '\n'

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
    """!Styled GMConsole

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
        self.SetUndoCollection(True)
        self.SetReadOnly(True)

        #
        # styles
        #                
        self.SetStyle()
        
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
        # bindings
        #
        self.Bind(wx.EVT_WINDOW_DESTROY, self.OnDestroy)
        
    def SetStyle(self):
        """!Set styles for styled text output windows with type face 
        and point size selected by user (Courier New 10 is default)"""

        settings = preferences.Settings()
        
        typeface = settings.Get(group='display', key='outputfont', subkey='type')   
        if typeface == "": typeface = "Courier New"
                           
        typesize = settings.Get(group='display', key='outputfont', subkey='size')
        if typesize == None or typesize <= 0: typesize = 10
        typesize = float(typesize)

        self.StyleDefault     = 0
        self.StyleDefaultSpec = "face:%s,size:%d,fore:#000000,back:#FFFFFF" % (typeface, typesize)
        self.StyleCommand     = 1
        self.StyleCommandSpec = "face:%s,size:%d,,fore:#000000,back:#bcbcbc" % (typeface, typesize)
        self.StyleOutput      = 2
        self.StyleOutputSpec  = "face:%s,size:%d,,fore:#000000,back:#FFFFFF" % (typeface, typesize)
        # fatal error
        self.StyleError       = 3
        self.StyleErrorSpec   = "face:%s,size:%d,,fore:#7F0000,back:#FFFFFF" % (typeface, typesize)
        # warning
        self.StyleWarning     = 4
        self.StyleWarningSpec = "face:%s,size:%d,,fore:#0000FF,back:#FFFFFF" % (typeface, typesize)
        # message
        self.StyleMessage     = 5
        self.StyleMessageSpec = "face:%s,size:%d,,fore:#000000,back:#FFFFFF" % (typeface, typesize)
        # unknown
        self.StyleUnknown     = 6
        self.StyleUnknownSpec = "face:%s,size:%d,,fore:#000000,back:#FFFFFF" % (typeface, typesize)
        
        # default and clear => init
        self.StyleSetSpec(wx.stc.STC_STYLE_DEFAULT, self.StyleDefaultSpec)
        self.StyleClearAll()
        self.StyleSetSpec(self.StyleCommand, self.StyleCommandSpec)
        self.StyleSetSpec(self.StyleOutput,  self.StyleOutputSpec)
        self.StyleSetSpec(self.StyleError,   self.StyleErrorSpec)
        self.StyleSetSpec(self.StyleWarning, self.StyleWarningSpec)
        self.StyleSetSpec(self.StyleMessage, self.StyleMessageSpec)
        self.StyleSetSpec(self.StyleUnknown, self.StyleUnknownSpec)        

    def OnDestroy(self, evt):
        """!The clipboard contents can be preserved after
        the app has exited"""
        
        wx.TheClipboard.Flush()
        evt.Skip()

    def AddTextWrapped(self, txt, wrap=None):
        """!Add string to text area.

        String is wrapped and linesep is also added to the end
        of the string"""
        # allow writing to output window
        self.SetReadOnly(False)
        
        if wrap:
            txt = textwrap.fill(txt, wrap) + '\n'
        else:
            if txt[-1] != '\n':
                txt += '\n'
        
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
                    txt = _('Unable to encode text. Please set encoding in GUI preferences.') + '\n'
                    
                self.AddText(txt)
        
        # reset output window to read only
        self.SetReadOnly(True)
    
