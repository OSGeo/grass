"""!
@package gui_core.goutput

@brief Command output widgets

Classes:
 - goutput::CmdThread
 - goutput::GMConsole
 - goutput::GMStdout
 - goutput::GMStderr
 - goutput::GMStc
 - goutput::PyStc

(C) 2007-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton (Arizona State University)
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (copy&paste customization)
"""

import os
import sys
import textwrap
import time
import threading
import Queue
import codecs
import locale
import keyword

import wx
from   wx import stc
from wx.lib.newevent import NewEvent

import grass.script as grass
from   grass.script import task as gtask

from core            import globalvar
from core            import utils
from core.gcmd       import CommandThread, GMessage, GError, GException, EncodeString
from gui_core.forms  import GUI
from gui_core.prompt import GPromptSTC
from core.debug      import Debug
from core.settings   import UserSettings, Settings, GetDisplayVectSettings
from gui_core.ghelp  import SearchModuleWindow

wxCmdOutput,   EVT_CMD_OUTPUT   = NewEvent()
wxCmdProgress, EVT_CMD_PROGRESS = NewEvent()
wxCmdRun,      EVT_CMD_RUN      = NewEvent()
wxCmdDone,     EVT_CMD_DONE     = NewEvent()
wxCmdAbort,    EVT_CMD_ABORT    = NewEvent()
wxCmdPrepare,  EVT_CMD_PREPARE  = NewEvent()

def GrassCmd(cmd, env = None, stdout = None, stderr = None):
    """!Return GRASS command thread"""
    return CommandThread(cmd, env = env,
                         stdout = stdout, stderr = stderr)

class CmdThread(threading.Thread):
    """!Thread for GRASS commands"""
    requestId = 0
    def __init__(self, parent, requestQ = None, resultQ = None, **kwds):
        threading.Thread.__init__(self, **kwds)
        
        if requestQ is None:
            self.requestQ = Queue.Queue()
        else:
            self.requestQ = requestQ
        
        if resultQ is None:
            self.resultQ = Queue.Queue()
        else:
            self.resultQ = resultQ
        
        self.setDaemon(True)
        
        self.parent = parent # GMConsole
        self._want_abort_all = False
        
        self.start()

    def RunCmd(self, *args, **kwds):
        """!Run command in queue

        @param args unnamed command arguments
        @param kwds named command arguments

        @return request id in queue
        """
        CmdThread.requestId += 1
        
        self.requestCmd = None
        self.requestQ.put((CmdThread.requestId, args, kwds))
        
        return CmdThread.requestId

    def SetId(self, id):
        """!Set starting id"""
        CmdThread.requestId = id
        
    def run(self):
        os.environ['GRASS_MESSAGE_FORMAT'] = 'gui'
        while True:
            requestId, args, kwds = self.requestQ.get()
            for key in ('callable', 'onDone', 'onPrepare', 'userData'):
                if key in kwds:
                    vars()[key] = kwds[key]
                    del kwds[key]
                else:
                    vars()[key] = None
            
            if not vars()['callable']:
                vars()['callable'] = GrassCmd
            
            requestTime = time.time()
            
            # prepare
            event = wxCmdPrepare(cmd = args[0],
                                 time = requestTime,
                                 pid = requestId,
                                 onPrepare = vars()['onPrepare'],
                                 userData = vars()['userData'])
            wx.PostEvent(self.parent, event)
            
            # run command
            event = wxCmdRun(cmd = args[0],
                             pid = requestId)
            
            wx.PostEvent(self.parent, event)
            
            time.sleep(.1)
            self.requestCmd = vars()['callable'](*args, **kwds)
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
            if UserSettings.Get(group = 'rasterLayer', key = 'colorTable', subkey = 'enabled') and \
                    args[0][0][:2] == 'r.':
                colorTable = UserSettings.Get(group = 'rasterLayer', key = 'colorTable', subkey = 'selection')
                mapName = None
                if args[0][0] == 'r.mapcalc':
                    try:
                        mapName = args[0][1].split('=', 1)[0].strip()
                    except KeyError:
                        pass
                else:
                    moduleInterface = GUI(show = None).ParseCommand(args[0])
                    outputParam = moduleInterface.get_param(value = 'output', raiseError = False)
                    if outputParam and outputParam['prompt'] == 'raster':
                        mapName = outputParam['value']
                
                if mapName:
                    argsColor = list(args)
                    argsColor[0] = [ 'r.colors',
                                     'map=%s' % mapName,
                                     'color=%s' % colorTable ]
                    self.requestCmdColor = vars()['callable'](*argsColor, **kwds)
                    self.resultQ.put((requestId, self.requestCmdColor.run()))
            
            event = wxCmdDone(cmd = args[0],
                              aborted = aborted,
                              returncode = returncode,
                              time = requestTime,
                              pid = requestId,
                              onDone = vars()['onDone'],
                              userData = vars()['userData'])
            
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
    def __init__(self, parent, id = wx.ID_ANY, margin = False,
                 notebook = None,
                 style = wx.TAB_TRAVERSAL | wx.FULL_REPAINT_ON_RESIZE,
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
        
        # remember position of line begining (used for '\r')
        self.linePos         = -1
        
        # create queues
        self.requestQ = Queue.Queue()
        self.resultQ = Queue.Queue()
        
        # progress bar
        self.progressbar = wx.Gauge(parent = self.panelOutput, id = wx.ID_ANY,
                                    range = 100, pos = (110, 50), size = (-1, 25),
                                    style = wx.GA_HORIZONTAL)
        self.progressbar.Bind(EVT_CMD_PROGRESS, self.OnCmdProgress)
        
        # text control for command output
        self.cmdOutput = GMStc(parent = self.panelOutput, id = wx.ID_ANY, margin = margin,
                               wrap = None) 
        self.cmdOutputTimer = wx.Timer(self.cmdOutput, id = wx.ID_ANY)
        self.cmdOutput.Bind(EVT_CMD_OUTPUT, self.OnCmdOutput)
        self.cmdOutput.Bind(wx.EVT_TIMER, self.OnProcessPendingOutputWindowEvents)
        self.Bind(EVT_CMD_RUN,     self.OnCmdRun)
        self.Bind(EVT_CMD_DONE,    self.OnCmdDone)
        self.Bind(EVT_CMD_PREPARE, self.OnCmdPrepare)
        
        # search & command prompt
        self.cmdPrompt = GPromptSTC(parent = self)
        
        if self.parent.GetName() != 'LayerManager':
            self.search = None
            self.cmdPrompt.Hide()
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
        
        # stream redirection
        self.cmdStdOut = GMStdout(self)
        self.cmdStdErr = GMStderr(self)
        
        # thread
        self.cmdThread = CmdThread(self, self.requestQ, self.resultQ)
        
        self.outputBox = wx.StaticBox(parent = self.panelOutput, id = wx.ID_ANY,
                                      label = " %s " % _("Output window"))
        self.cmdBox = wx.StaticBox(parent = self.panelOutput, id = wx.ID_ANY,
                                   label = " %s " % _("Command prompt"))
        
        # buttons
        self.btnOutputClear = wx.Button(parent = self.panelOutput, id = wx.ID_CLEAR)
        self.btnOutputClear.SetToolTipString(_("Clear output window content"))
        self.btnCmdClear = wx.Button(parent = self.panelOutput, id = wx.ID_CLEAR)
        self.btnCmdClear.SetToolTipString(_("Clear command prompt content"))
        self.btnOutputSave  = wx.Button(parent = self.panelOutput, id = wx.ID_SAVE)
        self.btnOutputSave.SetToolTipString(_("Save output window content to the file"))
        self.btnCmdAbort = wx.Button(parent = self.panelOutput, id = wx.ID_STOP)
        self.btnCmdAbort.SetToolTipString(_("Abort running command"))
        self.btnCmdAbort.Enable(False)
        self.btnCmdProtocol = wx.ToggleButton(parent = self.panelOutput, id = wx.ID_ANY,
                                              label = _("&Protocol"),
                                              size = self.btnCmdClear.GetSize())
        self.btnCmdProtocol.SetToolTipString(_("Toggle to save list of executed commands into file; "
                                               "content saved when switching off."))
        
        if self.parent.GetName() != 'LayerManager':
            self.btnCmdClear.Hide()
            self.btnCmdProtocol.Hide()
        
        self.btnCmdClear.Bind(wx.EVT_BUTTON,     self.cmdPrompt.OnCmdErase)
        self.btnOutputClear.Bind(wx.EVT_BUTTON,  self.OnOutputClear)
        self.btnOutputSave.Bind(wx.EVT_BUTTON,   self.OnOutputSave)
        self.btnCmdAbort.Bind(wx.EVT_BUTTON,     self.OnCmdAbort)
        self.Bind(EVT_CMD_ABORT,                 self.OnCmdAbort)
        self.btnCmdProtocol.Bind(wx.EVT_TOGGLEBUTTON, self.OnCmdProtocol)
        
        self._layout()
        
    def _layout(self):
        """!Do layout"""
        outputSizer = wx.BoxSizer(wx.VERTICAL)
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        outBtnSizer = wx.StaticBoxSizer(self.outputBox, wx.HORIZONTAL)
        cmdBtnSizer = wx.StaticBoxSizer(self.cmdBox, wx.HORIZONTAL)
        
        if self.cmdPrompt.IsShown():
            promptSizer = wx.BoxSizer(wx.VERTICAL)
            promptSizer.Add(item = self.cmdPrompt, proportion = 1,
                        flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, border = 3)
        
        if self.search and self.search.IsShown():
            outputSizer.Add(item = self.searchPane, proportion = 0,
                            flag = wx.EXPAND | wx.ALL, border = 3)
        outputSizer.Add(item = self.cmdOutput, proportion = 1,
                        flag = wx.EXPAND | wx.ALL, border = 3)
        outputSizer.Add(item = self.progressbar, proportion = 0,
                        flag = wx.EXPAND | wx.LEFT | wx.RIGHT, border = 3)
        outBtnSizer.Add(item = self.btnOutputClear, proportion = 1,
                        flag = wx.ALIGN_LEFT | wx.LEFT | wx.RIGHT, border = 5)
        outBtnSizer.Add(item = self.btnOutputSave, proportion = 1,
                        flag = wx.ALIGN_RIGHT | wx.RIGHT, border = 5)
        
        cmdBtnSizer.Add(item = self.btnCmdProtocol, proportion = 1,
                        flag = wx.ALIGN_CENTER | wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT, border = 5)
        cmdBtnSizer.Add(item = self.btnCmdClear, proportion = 1,
                        flag = wx.ALIGN_CENTER | wx.RIGHT, border = 5)
        cmdBtnSizer.Add(item = self.btnCmdAbort, proportion = 1,
                        flag = wx.ALIGN_CENTER | wx.RIGHT, border = 5)
        
        if self.parent.GetName() != 'LayerManager':
            proportion = (1, 1)
        else:
            proportion = (2, 3)
        
        btnSizer.Add(item = outBtnSizer, proportion = proportion[0],
                     flag = wx.ALL | wx.ALIGN_CENTER, border = 5)
        btnSizer.Add(item = cmdBtnSizer, proportion = proportion[1],
                     flag = wx.ALIGN_CENTER | wx.TOP | wx.BOTTOM | wx.RIGHT, border = 5)
        outputSizer.Add(item = btnSizer, proportion = 0,
                        flag = wx.EXPAND)
        
        outputSizer.Fit(self)
        outputSizer.SetSizeHints(self)
        self.panelOutput.SetSizer(outputSizer)
        # eliminate gtk_widget_size_allocate() warnings
        outputSizer.SetVirtualSizeHints(self.panelOutput)
        
        if self.cmdPrompt.IsShown():
            promptSizer.Fit(self)
            promptSizer.SetSizeHints(self)
            self.panelPrompt.SetSizer(promptSizer)
        
        # split window
        if self.cmdPrompt.IsShown():
            self.SplitHorizontally(self.panelOutput, self.panelPrompt, -50)
        else:
            self.SplitHorizontally(self.panelOutput, self.panelPrompt, -45)
            self.Unsplit()
        self.SetMinimumPaneSize(self.btnCmdClear.GetSize()[1] + 25)
        
        self.SetSashGravity(1.0)
        
        # layout
        self.SetAutoLayout(True)
        self.Layout()

    def MakeSearchPaneContent(self, pane):
        """!Create search pane"""
        border = wx.BoxSizer(wx.VERTICAL)
        
        self.search = SearchModuleWindow(parent = pane, cmdPrompt = self.cmdPrompt)
        
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
        """
        if Debug.GetLevel() == 0 and int(grass.gisenv().get('DEBUG', 0)) == 0:
            # don't redirect when debugging is enabled
            sys.stdout = self.cmdStdOut
            sys.stderr = self.cmdStdErr
        else:
            enc = locale.getdefaultlocale()[1]
            if enc:
                sys.stdout = codecs.getwriter(enc)(sys.__stdout__)
                sys.stderr = codecs.getwriter(enc)(sys.__stderr__)
            else:
                sys.stdout = sys.__stdout__
                sys.stderr = sys.__stderr__
        
    def WriteLog(self, text, style = None, wrap = None,
                 switchPage = False):
        """!Generic method for writing log message in 
        given style

        @param line text line
        @param style text style (see GMStc)
        @param stdout write to stdout or stderr
        """

        self.cmdOutput.SetStyle()

        if switchPage:
            self._notebook.SetSelectionByName('output')
        
        if not style:
            style = self.cmdOutput.StyleDefault
        
        # p1 = self.cmdOutput.GetCurrentPos()
        p1 = self.cmdOutput.GetEndStyled()
        # self.cmdOutput.GotoPos(p1)
        self.cmdOutput.DocumentEnd()
        
        for line in text.splitlines():
            # fill space
            if len(line) < self.lineWidth:
                diff = self.lineWidth - len(line) 
                line += diff * ' '
            
            self.cmdOutput.AddTextWrapped(line, wrap = wrap) # adds '\n'
            
            p2 = self.cmdOutput.GetCurrentPos()
            
            self.cmdOutput.StartStyling(p1, 0xff)
            self.cmdOutput.SetStyling(p2 - p1, style)
        
        self.cmdOutput.EnsureCaretVisible()
        
    def WriteCmdLog(self, line, pid = None, switchPage = True):
        """!Write message in selected style
        
        @param line message to be printed
        @param pid process pid or None
        @param switchPage True to switch page
        """
        if pid:
            line = '(' + str(pid) + ') ' + line
        self.WriteLog(line, style = self.cmdOutput.StyleCommand, switchPage = switchPage)

    def WriteWarning(self, line):
        """!Write message in warning style"""
        self.WriteLog(line, style = self.cmdOutput.StyleWarning, switchPage = True)

    def WriteError(self, line):
        """!Write message in error style"""
        self.WriteLog(line, style = self.cmdOutput.StyleError, switchPage = True)

    def RunCmd(self, command, compReg = True, switchPage = False, skipInterface = False,
               onDone = None, onPrepare = None, userData = None):
        """!Run command typed into console command prompt (GPrompt).
        
        @todo Display commands (*.d) are captured and processed
        separately by mapdisp.py. Display commands are rendered in map
        display widget that currently has the focus (as indicted by
        mdidx).
        
        @param command command given as a list (produced e.g. by utils.split())
        @param compReg True use computation region
        @param switchPage switch to output page
        @param skipInterface True to do not launch GRASS interface
        parser when command has no arguments given
        @param onDone function to be called when command is finished
        @param onPrepare function to be called before command is launched
        @param userData data defined for the command
        """
        if len(command) == 0:
            Debug.msg(2, "GPrompt:RunCmd(): empty command")
            return
        
        # update history file
        env = grass.gisenv()
        try:
            filePath = os.path.join(env['GISDBASE'],
                                    env['LOCATION_NAME'],
                                    env['MAPSET'],
                                    '.bash_history')
            fileHistory = codecs.open(filePath, encoding = 'utf-8', mode = 'a')
        except IOError, e:
            GError(_("Unable to write file '%(filePath)s'.\n\nDetails: %(error)s") % 
                    {'filePath': filePath, 'error' : e },
                   parent = self.parent)
            fileHistory = None
        
        if fileHistory:
            try:
                fileHistory.write(' '.join(command) + os.linesep)
            finally:
                fileHistory.close()
        
        if command[0] in globalvar.grassCmd:
            # send GRASS command without arguments to GUI command interface
            # except display commands (they are handled differently)
            if self.parent.GetName() == "LayerManager" and \
                    command[0][0:2] == "d." and \
                    'help' not in ' '.join(command[1:]):
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
                                 'd.rast.leg'     : 'maplegend',
                                 'd.vect'         : 'vector',
                                 'd.thematic.area': 'thememap',
                                 'd.vect.chart'   : 'themechart',
                                 'd.grid'         : 'grid',
                                 'd.geodesic'     : 'geodesic',
                                 'd.rhumbline'    : 'rhumb',
                                 'd.labels'       : 'labels',
                                 'd.barscale'     : 'barscale',
                                 'd.redraw'       : 'redraw'}[command[0]]
                except KeyError:
                    GMessage(parent = self.parent,
                             message = _("Command '%s' not yet implemented in the WxGUI. "
                                         "Try adding it as a command layer instead.") % command[0])
                    return
                
                if layertype == 'barscale':
                    self.parent.curr_page.maptree.GetMapDisplay().OnAddBarscale(None)
                elif layertype == 'rastleg':
                    self.parent.curr_page.maptree.GetMapDisplay().OnAddLegend(None)
                elif layertype == 'redraw':
                    self.parent.curr_page.maptree.GetMapDisplay().OnRender(None)
                else:
                    # add layer into layer tree
                    lname, found = utils.GetLayerNameFromCmd(command, fullyQualified = True,
                                                             layerType = layertype)
                    if self.parent.GetName() == "LayerManager":
                        self.parent.curr_page.maptree.AddLayer(ltype = layertype,
                                                               lname = lname,
                                                               lcmd = command)
            
            else:
                # other GRASS commands (r|v|g|...)
                try:
                    task = GUI(show = None).ParseCommand(command)
                except GException, e:
                    GError(parent = self,
                           message = unicode(e),
                           showTraceback = False)
                    return
                
                hasParams = False
                if task:
                    options = task.get_options()
                    hasParams = options['params'] and options['flags']
                    # check for <input>=-
                    for p in options['params']:
                        if p.get('prompt', '') == 'input' and \
                                p.get('element', '') == 'file' and \
                                p.get('age', 'new') == 'old' and \
                                p.get('value', '') == '-':
                            GError(parent = self,
                                   message = _("Unable to run command:\n%(cmd)s\n\n"
                                               "Option <%(opt)s>: read from standard input is not "
                                               "supported by wxGUI") % { 'cmd': ' '.join(command),
                                                                         'opt': p.get('name', '') })
                            return
                
                if len(command) == 1 and hasParams and \
                        command[0] != 'v.krige':
                    # no arguments given
                    try:
                        GUI(parent = self, lmgr = self.parent).ParseCommand(command)
                    except GException, e:
                        print >> sys.stderr, e
                    return
                
                # switch to 'Command output' if required
                if switchPage:
                    self._notebook.SetSelectionByName('output')
                    
                    self.parent.SetFocus()
                    self.parent.Raise()
                
                # activate computational region (set with g.region)
                # for all non-display commands.
                if compReg:
                    tmpreg = os.getenv("GRASS_REGION")
                    if "GRASS_REGION" in os.environ:
                        del os.environ["GRASS_REGION"]
                
                # process GRASS command with argument
                self.cmdThread.RunCmd(command, stdout = self.cmdStdOut, stderr = self.cmdStdErr,
                                      onDone = onDone, onPrepare = onPrepare, userData = userData,
                                      env = os.environ.copy())
                self.cmdOutputTimer.Start(50)
                
                # deactivate computational region and return to display settings
                if compReg and tmpreg:
                    os.environ["GRASS_REGION"] = tmpreg
        else:
            # Send any other command to the shell. Send output to
            # console output window
            if len(command) == 1 and not skipInterface:
                try:
                    task = gtask.parse_interface(command[0])
                except:
                    task = None
            else:
                task = None
                
            if task:
                # process GRASS command without argument
                GUI(parent = self, lmgr = self.parent).ParseCommand(command)
            else:
                self.cmdThread.RunCmd(command, stdout = self.cmdStdOut, stderr = self.cmdStdErr,
                                      onDone = onDone, onPrepare = onPrepare, userData = userData)
            self.cmdOutputTimer.Start(50)
        
    def OnOutputClear(self, event):
        """!Clear content of output window"""
        self.cmdOutput.SetReadOnly(False)
        self.cmdOutput.ClearAll()
        self.cmdOutput.SetReadOnly(True)
        self.progressbar.SetValue(0)

    def GetProgressBar(self):
        """!Return progress bar widget"""
        return self.progressbar
    
    def GetLog(self, err = False):
        """!Get widget used for logging

        @param err True to get stderr widget
        """
        if err:
            return self.cmdStdErr
        
        return self.cmdStdOut
    
    def OnOutputSave(self, event):
        """!Save (selected) text from output window to the file"""
        text = self.cmdOutput.GetSelectedText()
        if not text:
            text = self.cmdOutput.GetText()
        
        # add newline if needed
        if len(text) > 0 and text[-1] != '\n':
            text += '\n'
        
        dlg = wx.FileDialog(self, message = _("Save file as..."),
                            defaultFile = "grass_cmd_output.txt",
                            wildcard = _("%(txt)s (*.txt)|*.txt|%(files)s (*)|*") % 
                            {'txt': _("Text files"), 'files': _("Files")},
                            style = wx.SAVE | wx.FD_OVERWRITE_PROMPT)
        
        # Show the dialog and retrieve the user response. If it is the OK response,
        # process the data.
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            
            try:
                output = open(path, "w")
                output.write(text)
            except IOError, e:
                GError(_("Unable to write file '%(path)s'.\n\nDetails: %(error)s") % {'path': path, 'error': e})
            finally:
                output.close()
            self.parent.SetStatusText(_("Commands output saved into '%s'") % path)
        
        dlg.Destroy()

    def GetCmd(self):
        """!Get running command or None"""
        return self.requestQ.get()
    
    def SetCopyingOfSelectedText(self, copy):
        """!Enable or disable copying of selected text in to clipboard.
        Effects prompt and output.
        
        @param copy True for enable, False for disable
        """
        if copy:
            self.cmdPrompt.Bind(stc.EVT_STC_PAINTED, self.cmdPrompt.OnTextSelectionChanged)
            self.cmdOutput.Bind(stc.EVT_STC_PAINTED, self.cmdOutput.OnTextSelectionChanged)
        else:
            self.cmdPrompt.Unbind(stc.EVT_STC_PAINTED)
            self.cmdOutput.Unbind(stc.EVT_STC_PAINTED)
        
    def OnUpdateStatusBar(self, event):
        """!Update statusbar text"""
        if event.GetString():
            nItems = len(self.cmdPrompt.GetCommandItems())
            self.parent.SetStatusText(_('%d modules match') % nItems, 0)
        else:
            self.parent.SetStatusText('', 0)
        
        event.Skip()

    def OnCmdOutput(self, event):
        """!Print command output"""
        message = event.text
        type  = event.type
        if self._notebook.GetSelection() != self._notebook.GetPageIndexByName('output'):
            page = self._notebook.GetPageIndexByName('output')
            textP = self._notebook.GetPageText(page)
            if textP[-1] != ')':
                textP += ' (...)'
            self._notebook.SetPageText(page, textP)
        
        # message prefix
        if type == 'warning':
            messege = 'WARNING: ' + message
        elif type == 'error':
            message = 'ERROR: ' + message
        
        p1 = self.cmdOutput.GetEndStyled()
        self.cmdOutput.GotoPos(p1)
        
        if '\b' in message:
            if self.linepos < 0:
                self.linepos = p1
            last_c = ''
            for c in message:
                if c == '\b':
                    self.linepos -= 1
                else:
                    if c == '\r':
                        pos = self.cmdOutput.GetCurLine()[1]
                        # self.cmdOutput.SetCurrentPos(pos)
                    else:
                        self.cmdOutput.SetCurrentPos(self.linepos)
                    self.cmdOutput.ReplaceSelection(c)
                    self.linepos = self.cmdOutput.GetCurrentPos()
                    if c != ' ':
                        last_c = c
            if last_c not in ('0123456789'):
                self.cmdOutput.AddTextWrapped('\n', wrap = None)
                self.linepos = -1
        else:
            self.linepos = -1 # don't force position
            if '\n' not in message:
                self.cmdOutput.AddTextWrapped(message, wrap = 60)
            else:
                self.cmdOutput.AddTextWrapped(message, wrap = None)

        p2 = self.cmdOutput.GetCurrentPos()
        
        if p2 >= p1:
            self.cmdOutput.StartStyling(p1, 0xff)
        
            if type == 'error':
                self.cmdOutput.SetStyling(p2 - p1, self.cmdOutput.StyleError)
            elif type == 'warning':
                self.cmdOutput.SetStyling(p2 - p1, self.cmdOutput.StyleWarning)
            elif type == 'message':
                self.cmdOutput.SetStyling(p2 - p1, self.cmdOutput.StyleMessage)
            else: # unknown
                self.cmdOutput.SetStyling(p2 - p1, self.cmdOutput.StyleUnknown)
        
        self.cmdOutput.EnsureCaretVisible()
        
    def OnCmdProgress(self, event):
        """!Update progress message info"""
        self.progressbar.SetValue(event.value)

    def CmdProtocolSave(self):
        """Save commands protocol into the file"""
        if not hasattr(self, 'cmdFileProtocol'):
            return # it should not happen
        
        try:
            output = open(self.cmdFileProtocol, "w")
            cmds = self.cmdPrompt.GetCommands()
            output.write('\n'.join(cmds))
            if len(cmds) > 0:
                output.write('\n')
        except IOError, e:
            GError(_("Unable to write file '%(filePath)s'.\n\nDetails: %(error)s") % 
                    {'filePath': self.cmdFileProtocol, 'error': e})
        finally:
            output.close()
            
        self.parent.SetStatusText(_("Commands protocol saved into '%s'") % self.cmdFileProtocol)
        del self.cmdFileProtocol
        
    def OnCmdProtocol(self, event = None):
        """!Save commands into file"""
        if not event.IsChecked():
            # stop capturing commands, save list of commands to the
            # protocol file
            self.CmdProtocolSave()
        else:
            # start capturing commands
            self.cmdPrompt.ClearCommands()
            # ask for the file
            dlg = wx.FileDialog(self, message = _("Save file as..."),
                                defaultFile = "grass_cmd_protocol.txt",
                                wildcard = _("%(txt)s (*.txt)|*.txt|%(files)s (*)|*") % 
                                            {'txt': _("Text files"), 'files': _("Files")},
                                style = wx.SAVE | wx.FD_OVERWRITE_PROMPT)
            if dlg.ShowModal() == wx.ID_OK:
                self.cmdFileProtocol = dlg.GetPath()
            else:
                wx.CallAfter(self.btnCmdProtocol.SetValue, False)
            
            dlg.Destroy()
            
        event.Skip()
        
    def OnCmdAbort(self, event):
        """!Abort running command"""
        self.cmdThread.abort()
        
    def OnCmdRun(self, event):
        """!Run command"""
        if self.parent.GetName() == 'Modeler':
            self.parent.OnCmdRun(event)
        
        self.WriteCmdLog('(%s)\n%s' % (str(time.ctime()), ' '.join(event.cmd)))
        self.btnCmdAbort.Enable()

    def OnCmdPrepare(self, event):
        """!Prepare for running command"""
        if self.parent.GetName() == 'Modeler':
            self.parent.OnCmdPrepare(event)
        
        event.Skip()
        
    def OnCmdDone(self, event):
        """!Command done (or aborted)"""
        if self.parent.GetName() == 'Modeler':
            self.parent.OnCmdDone(event)
            
        # Process results here
        try:
            ctime = time.time() - event.time
            if ctime < 60:
                stime = _("%d sec") % int(ctime)
            else:
                mtime = int(ctime / 60)
                stime = _("%(min)d min %(sec)d sec") %  { 'min' : mtime, 
                                                          'sec' : int(ctime - (mtime * 60)) }
        except KeyError:
            # stopped deamon
            stime = _("unknown")
        
        if event.aborted:
            # Thread aborted (using our convention of None return)
            self.WriteLog(_('Please note that the data are left in inconsistent state '
                            'and may be corrupted'), self.cmdOutput.StyleWarning)
            msg = _('Command aborted')
        else:
            msg = _('Command finished')
            
        self.WriteCmdLog('(%s) %s (%s)' % (str(time.ctime()), msg, stime))
        self.btnCmdAbort.Enable(False)
        
        if event.onDone:
            event.onDone(cmd = event.cmd, returncode = event.returncode)
        
        self.progressbar.SetValue(0) # reset progress bar on '0%'

        self.cmdOutputTimer.Stop()

        if event.cmd[0] == 'g.gisenv':
            Debug.SetLevel()
            self.Redirect()
        
        if self.parent.GetName() == "LayerManager":
            self.btnCmdAbort.Enable(False)
            if event.cmd[0] not in globalvar.grassCmd or \
                    event.cmd[0] == 'r.mapcalc':
                return
            
            display = self.parent.GetLayerTree().GetMapDisplay()
            if not display or not display.IsAutoRendered():
                return
            mapLayers = map(lambda x: x.GetName(),
                            display.GetMap().GetListOfLayers(l_type = 'raster') +
                            display.GetMap().GetListOfLayers(l_type = 'vector'))
            
            try:
                task = GUI(show = None).ParseCommand(event.cmd)
            except GException, e:
                print >> sys.stderr, e
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
                            name, found = utils.GetLayerNameFromCmd(cmd, fullyQualified = True,
                                                                    param = p.get('name', ''))
                            
                            if mapTree.GetMap().GetListOfLayers(l_name = name):
                                display = mapTree.GetMapDisplay()
                                if display and display.IsAutoRendered():
                                    display.GetWindow().UpdateMap(render = True)
                                continue
                            
                            if prompt == 'raster':
                                lcmd = ['d.rast',
                                        'map=%s' % name]
                            else:
                                defaultParams = GetDisplayVectSettings()
                                lcmd = ['d.vect',
                                        'map=%s' % name] + defaultParams
                            mapTree.AddLayer(ltype = prompt,
                                             lcmd = lcmd,
                                             lname = name)
            
            if hasattr(dialog, "get_dcmd") and \
                    dialog.get_dcmd is None and \
                    hasattr(dialog, "closebox") and \
                    dialog.closebox.IsChecked() and \
                    (event.returncode == 0 or event.aborted):
                self.cmdOutput.Update()
                time.sleep(2)
                dialog.Close()
        
    def OnProcessPendingOutputWindowEvents(self, event):
        wx.GetApp().ProcessPendingEvents()

    def ResetFocus(self):
        """!Reset focus"""
        self.cmdPrompt.SetFocus()
        
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
            
            evt = wxCmdOutput(text = line + '\n',
                              type = '')
            wx.PostEvent(self.parent.cmdOutput, evt)
        
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
                evt = wxCmdOutput(text = line,
                                  type = '')
                wx.PostEvent(self.parent.cmdOutput, evt)
            elif len(line) > 0:
                self.message += line.strip() + '\n'

            if self.printMessage and len(self.message) > 0:
                evt = wxCmdOutput(text = self.message,
                                  type = self.type)
                wx.PostEvent(self.parent.cmdOutput, evt)

                self.type = ''
                self.message = ''
                self.printMessage = False

        # update progress message
        if progressValue > -1:
            # self.gmgauge.SetValue(progressValue)
            evt = wxCmdProgress(value = progressValue)
            wx.PostEvent(self.parent.progressbar, evt)
            
class GMStc(stc.StyledTextCtrl):
    """!Styled GMConsole

    Based on FrameOutErr.py

    Name:      FrameOutErr.py
    Purpose:   Redirecting stdout / stderr
    Author:    Jean-Michel Fauth, Switzerland
    Copyright: (c) 2005-2007 Jean-Michel Fauth
    Licence:   GPL
    """    
    def __init__(self, parent, id, margin = False, wrap = None):
        stc.StyledTextCtrl.__init__(self, parent, id)
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
            self.SetMarginType(0, stc.STC_MARGIN_NUMBER)
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
        
    def OnTextSelectionChanged(self, event):
        """!Copy selected text to clipboard and skip event.
        The same function is in TextCtrlAutoComplete class (prompt.py).
        """
        wx.CallAfter(self.Copy)
        event.Skip()
        
    def SetStyle(self):
        """!Set styles for styled text output windows with type face 
        and point size selected by user (Courier New 10 is default)"""

        settings = Settings()
        
        typeface = settings.Get(group = 'appearance', key = 'outputfont', subkey = 'type')   
        if typeface == "":
            typeface = "Courier New"
        
        typesize = settings.Get(group = 'appearance', key = 'outputfont', subkey = 'size')
        if typesize == None or typesize <= 0:
            typesize = 10
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
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT, self.StyleDefaultSpec)
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

    def AddTextWrapped(self, txt, wrap = None):
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
                enc = UserSettings.Get(group = 'atm', key = 'encoding', subkey = 'value')
                if enc:
                    txt = unicode(txt, enc)
                elif 'GRASS_DB_ENCODING' in os.environ:
                    txt = unicode(txt, os.environ['GRASS_DB_ENCODING'])
                else:
                    txt = EncodeString(txt)
                
                self.AddText(txt)
        
        # reset output window to read only
        self.SetReadOnly(True)
    
class PyStc(stc.StyledTextCtrl):
    """!Styled Python output (see gmodeler::frame::PythonPanel for
    usage)

    Based on StyledTextCtrl_2 from wxPython demo
    """    
    def __init__(self, parent, id = wx.ID_ANY, statusbar = None):
        stc.StyledTextCtrl.__init__(self, parent, id)
        
        self.parent = parent
        self.statusbar = statusbar
        
        self.modified = False # content modified ?
        
        self.faces = { 'times': 'Times New Roman',
                       'mono' : 'Courier New',
                       'helv' : 'Arial',
                       'other': 'Comic Sans MS',
                       'size' : 10,
                       'size2': 8,
                       }
        
        self.CmdKeyAssign(ord('B'), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMIN)
        self.CmdKeyAssign(ord('N'), stc.STC_SCMOD_CTRL, stc.STC_CMD_ZOOMOUT)
        
        self.SetLexer(stc.STC_LEX_PYTHON)
        self.SetKeyWords(0, " ".join(keyword.kwlist))
        
        self.SetProperty("fold", "1")
        self.SetProperty("tab.timmy.whinge.level", "1")
        self.SetMargins(0, 0)
        self.SetTabWidth(4)
        self.SetUseTabs(False)
        
        self.SetEdgeMode(stc.STC_EDGE_BACKGROUND)
        self.SetEdgeColumn(78)
        
        # setup a margin to hold fold markers
        self.SetMarginType(2, stc.STC_MARGIN_SYMBOL)
        self.SetMarginMask(2, stc.STC_MASK_FOLDERS)
        self.SetMarginSensitive(2, True)
        self.SetMarginWidth(2, 12)
        
        # like a flattened tree control using square headers
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPEN,    stc.STC_MARK_BOXMINUS,          "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDER,        stc.STC_MARK_BOXPLUS,           "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERSUB,     stc.STC_MARK_VLINE,             "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERTAIL,    stc.STC_MARK_LCORNER,           "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEREND,     stc.STC_MARK_BOXPLUSCONNECTED,  "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDEROPENMID, stc.STC_MARK_BOXMINUSCONNECTED, "white", "#808080")
        self.MarkerDefine(stc.STC_MARKNUM_FOLDERMIDTAIL, stc.STC_MARK_TCORNER,           "white", "#808080")
        
        self.Bind(stc.EVT_STC_UPDATEUI, self.OnUpdateUI)
        self.Bind(stc.EVT_STC_MARGINCLICK, self.OnMarginClick)
        self.Bind(wx.EVT_KEY_DOWN, self.OnKeyPressed)
        
        # Make some styles, the lexer defines what each style is used
        # for, we just have to define what each style looks like.
        # This set is adapted from Scintilla sample property files.
        
        # global default styles for all languages
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "face:%(helv)s,size:%(size)d" % self.faces)
        self.StyleClearAll()  # reset all to be like the default
        
        # global default styles for all languages
        self.StyleSetSpec(stc.STC_STYLE_DEFAULT,     "face:%(helv)s,size:%(size)d" % self.faces)
        self.StyleSetSpec(stc.STC_STYLE_LINENUMBER,  "back:#C0C0C0,face:%(helv)s,size:%(size2)d" % self.faces)
        self.StyleSetSpec(stc.STC_STYLE_CONTROLCHAR, "face:%(other)s" % self.faces)
        self.StyleSetSpec(stc.STC_STYLE_BRACELIGHT,  "fore:#FFFFFF,back:#0000FF,bold")
        self.StyleSetSpec(stc.STC_STYLE_BRACEBAD,    "fore:#000000,back:#FF0000,bold")
        
        # Python styles
        # Default 
        self.StyleSetSpec(stc.STC_P_DEFAULT, "fore:#000000,face:%(helv)s,size:%(size)d" % self.faces)
        # Comments
        self.StyleSetSpec(stc.STC_P_COMMENTLINE, "fore:#007F00,face:%(other)s,size:%(size)d" % self.faces)
        # Number
        self.StyleSetSpec(stc.STC_P_NUMBER, "fore:#007F7F,size:%(size)d" % self.faces)
        # String
        self.StyleSetSpec(stc.STC_P_STRING, "fore:#7F007F,face:%(helv)s,size:%(size)d" % self.faces)
        # Single quoted string
        self.StyleSetSpec(stc.STC_P_CHARACTER, "fore:#7F007F,face:%(helv)s,size:%(size)d" % self.faces)
        # Keyword
        self.StyleSetSpec(stc.STC_P_WORD, "fore:#00007F,bold,size:%(size)d" % self.faces)
        # Triple quotes
        self.StyleSetSpec(stc.STC_P_TRIPLE, "fore:#7F0000,size:%(size)d" % self.faces)
        # Triple double quotes
        self.StyleSetSpec(stc.STC_P_TRIPLEDOUBLE, "fore:#7F0000,size:%(size)d" % self.faces)
        # Class name definition
        self.StyleSetSpec(stc.STC_P_CLASSNAME, "fore:#0000FF,bold,underline,size:%(size)d" % self.faces)
        # Function or method name definition
        self.StyleSetSpec(stc.STC_P_DEFNAME, "fore:#007F7F,bold,size:%(size)d" % self.faces)
        # Operators
        self.StyleSetSpec(stc.STC_P_OPERATOR, "bold,size:%(size)d" % self.faces)
        # Identifiers
        self.StyleSetSpec(stc.STC_P_IDENTIFIER, "fore:#000000,face:%(helv)s,size:%(size)d" % self.faces)
        # Comment-blocks
        self.StyleSetSpec(stc.STC_P_COMMENTBLOCK, "fore:#7F7F7F,size:%(size)d" % self.faces)
        # End of line where string is not closed
        self.StyleSetSpec(stc.STC_P_STRINGEOL, "fore:#000000,face:%(mono)s,back:#E0C0E0,eol,size:%(size)d" % self.faces)
        
        self.SetCaretForeground("BLUE")
        
    def OnKeyPressed(self, event):
        """!Key pressed
        
        @todo implement code completion (see wxPython demo)
        """
        if not self.modified:
            self.modified = True
            if self.statusbar:
                self.statusbar.SetStatusText(_('Python script contains local modifications'), 0)
        
        event.Skip()
        
    def OnUpdateUI(self, evt):
        # check for matching braces
        braceAtCaret = -1
        braceOpposite = -1
        charBefore = None
        caretPos = self.GetCurrentPos()
        
        if caretPos > 0:
            charBefore  = self.GetCharAt(caretPos - 1)
            styleBefore = self.GetStyleAt(caretPos - 1)
        
        # check before
        if charBefore and chr(charBefore) in "[]{}()" and styleBefore == stc.STC_P_OPERATOR:
            braceAtCaret = caretPos - 1
        
        # check after
        if braceAtCaret < 0:
            charAfter = self.GetCharAt(caretPos)
            styleAfter = self.GetStyleAt(caretPos)
            
            if charAfter and chr(charAfter) in "[]{}()" and styleAfter == stc.STC_P_OPERATOR:
                braceAtCaret = caretPos
        
        if braceAtCaret >= 0:
            braceOpposite = self.BraceMatch(braceAtCaret)
        
        if braceAtCaret != -1  and braceOpposite == -1:
            self.BraceBadLight(braceAtCaret)
        else:
            self.BraceHighlight(braceAtCaret, braceOpposite)
        
    def OnMarginClick(self, evt):
        # fold and unfold as needed
        if evt.GetMargin() == 2:
            if evt.GetShift() and evt.GetControl():
                self.FoldAll()
            else:
                lineClicked = self.LineFromPosition(evt.GetPosition())
                
                if self.GetFoldLevel(lineClicked) & stc.STC_FOLDLEVELHEADERFLAG:
                    if evt.GetShift():
                        self.SetFoldExpanded(lineClicked, True)
                        self.Expand(lineClicked, True, True, 1)
                    elif evt.GetControl():
                        if self.GetFoldExpanded(lineClicked):
                            self.SetFoldExpanded(lineClicked, False)
                            self.Expand(lineClicked, False, True, 0)
                        else:
                            self.SetFoldExpanded(lineClicked, True)
                            self.Expand(lineClicked, True, True, 100)
                    else:
                        self.ToggleFold(lineClicked)
        
    def FoldAll(self):
        lineCount = self.GetLineCount()
        expanding = True
        
        # find out if we are folding or unfolding
        for lineNum in range(lineCount):
            if self.GetFoldLevel(lineNum) & stc.STC_FOLDLEVELHEADERFLAG:
                expanding = not self.GetFoldExpanded(lineNum)
                break
        
        lineNum = 0
        while lineNum < lineCount:
            level = self.GetFoldLevel(lineNum)
            if level & stc.STC_FOLDLEVELHEADERFLAG and \
               (level & stc.STC_FOLDLEVELNUMBERMASK) == stc.STC_FOLDLEVELBASE:
                
                if expanding:
                    self.SetFoldExpanded(lineNum, True)
                    lineNum = self.Expand(lineNum, True)
                    lineNum = lineNum - 1
                else:
                    lastChild = self.GetLastChild(lineNum, -1)
                    self.SetFoldExpanded(lineNum, False)
                    
                    if lastChild > lineNum:
                        self.HideLines(lineNum+1, lastChild)
            
            lineNum = lineNum + 1
        
    def Expand(self, line, doExpand, force=False, visLevels=0, level=-1):
        lastChild = self.GetLastChild(line, level)
        line = line + 1
        
        while line <= lastChild:
            if force:
                if visLevels > 0:
                    self.ShowLines(line, line)
                else:
                    self.HideLines(line, line)
            else:
                if doExpand:
                    self.ShowLines(line, line)
            
            if level == -1:
                level = self.GetFoldLevel(line)
            
            if level & stc.STC_FOLDLEVELHEADERFLAG:
                if force:
                    if visLevels > 1:
                        self.SetFoldExpanded(line, True)
                    else:
                        self.SetFoldExpanded(line, False)
                    
                    line = self.Expand(line, doExpand, force, visLevels-1)
                else:
                    if doExpand and self.GetFoldExpanded(line):
                        line = self.Expand(line, True, force, visLevels-1)
                    else:
                        line = self.Expand(line, False, force, visLevels-1)
            else:
                line = line + 1
        
        return line
