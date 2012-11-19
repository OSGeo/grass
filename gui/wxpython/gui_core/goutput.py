"""!
@package gui_core.goutput

@brief Command output widgets

Classes:
 - goutput::CmdThread
 - goutput::GConsole
 - goutput::GStdout
 - goutput::GStderr
 - goutput::GStc

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
from core.settings   import UserSettings, GetDisplayVectSettings
from gui_core.widgets import SearchModuleWidget, EVT_MODULE_SELECTED
from core.modulesdata import ModulesData

wxCmdOutput,   EVT_CMD_OUTPUT   = NewEvent()
wxCmdProgress, EVT_CMD_PROGRESS = NewEvent()
wxCmdRun,      EVT_CMD_RUN      = NewEvent()
wxCmdDone,     EVT_CMD_DONE     = NewEvent()
wxCmdAbort,    EVT_CMD_ABORT    = NewEvent()
wxCmdPrepare,  EVT_CMD_PREPARE  = NewEvent()


GC_EMPTY = 0
GC_SEARCH = 1
GC_PROMPT = 2


def GrassCmd(cmd, env = None, stdout = None, stderr = None):
    """!Return GRASS command thread"""
    return CommandThread(cmd, env = env,
                         stdout = stdout, stderr = stderr)

class CmdThread(threading.Thread):
    """!Thread for GRASS commands"""
    requestId = 0
    def __init__(self, receiver, requestQ = None, resultQ = None, **kwds):
        """!
        @param receiver event receiver (used in PostEvent)
        """
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
        
        self.receiver = receiver
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
            wx.PostEvent(self.receiver, event)
            
            # run command
            event = wxCmdRun(cmd = args[0],
                             pid = requestId)
            
            wx.PostEvent(self.receiver, event)
            
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
            wx.PostEvent(self.receiver, event)
            
    def abort(self, abortall = True):
        """!Abort command(s)"""
        if abortall:
            self._want_abort_all = True
        self.requestCmd.abort()
        if self.requestQ.empty():
            self._want_abort_all = False


# Occurs event when some new text appears.
# Text priority is specified by priority attribute.
# Priority is 1 (lowest), 2, 3 (highest);
# value 0 is currently not used and probably will not be used.
# In theory, it can be used when text is completely uninteresting.
# It is similar to wx.EVT_TEXT.
# However, the new text or the whole text are not event attributes.
gOutputText, EVT_OUTPUT_TEXT = NewEvent()


class GConsole(wx.SplitterWindow):
    """!Create and manage output console for commands run by GUI.
    """
    def __init__(self, parent, id = wx.ID_ANY, margin = False,
                 frame = None,
                 style = wx.TAB_TRAVERSAL | wx.FULL_REPAINT_ON_RESIZE,
                 gcstyle = GC_EMPTY,
                 **kwargs):
        wx.SplitterWindow.__init__(self, parent, id, style = style, *kwargs)
        self.SetName("GConsole")
        
        self.panelOutput = wx.Panel(parent = self, id = wx.ID_ANY)
        self.panelPrompt = wx.Panel(parent = self, id = wx.ID_ANY)

        # initialize variables
        self.parent = parent # GMFrame | CmdPanel | ?
        if frame:
            self.frame = frame
        else:
            self.frame = parent

        self._gcstyle = gcstyle
        self.lineWidth       = 80
        
        # create queues
        self.requestQ = Queue.Queue()
        self.resultQ = Queue.Queue()
        
        # progress bar
        self.progressbar = wx.Gauge(parent = self.panelOutput, id = wx.ID_ANY,
                                    range = 100, pos = (110, 50), size = (-1, 25),
                                    style = wx.GA_HORIZONTAL)
        self.progressbar.Bind(EVT_CMD_PROGRESS, self.OnCmdProgress)
        
        # text control for command output
        self.cmdOutput = GStc(parent = self.panelOutput, id = wx.ID_ANY, margin = margin,
                               wrap = None) 
        self.cmdOutputTimer = wx.Timer(self.cmdOutput, id = wx.ID_ANY)
        self.Bind(EVT_CMD_OUTPUT, self.OnCmdOutput)
        self.Bind(wx.EVT_TIMER, self.OnProcessPendingOutputWindowEvents)
        self.Bind(EVT_CMD_RUN,     self.OnCmdRun)
        self.Bind(EVT_CMD_DONE,    self.OnCmdDone)

        # information about available modules
        modulesData = ModulesData()

        # search & command prompt
        # move to the if below
        # search depends on cmd prompt
        self.cmdPrompt = GPromptSTC(parent = self, modulesData = modulesData)
        if not self._gcstyle & GC_PROMPT:
            self.cmdPrompt.Hide()


        if self._gcstyle & GC_SEARCH:
            self.infoCollapseLabelExp = _("Click here to show search module engine")
            self.infoCollapseLabelCol = _("Click here to hide search module engine")
            self.searchPane = wx.CollapsiblePane(parent = self.panelOutput,
                                                 label = self.infoCollapseLabelExp,
                                                 style = wx.CP_DEFAULT_STYLE |
                                                 wx.CP_NO_TLW_RESIZE | wx.EXPAND)
            self.MakeSearchPaneContent(self.searchPane.GetPane(), modulesData)
            self.searchPane.Collapse(True)
            self.Bind(wx.EVT_COLLAPSIBLEPANE_CHANGED, self.OnSearchPaneChanged, self.searchPane) 
            self.search.Bind(EVT_MODULE_SELECTED,
                             lambda event:
                                 self.cmdPrompt.SetTextAndFocus(event.name + ' '))
        else:
            self.search = None

        # stream redirection
        self.cmdStdOut = GStdout(self)
        self.cmdStdErr = GStderr(self)
        
        # thread
        self.cmdThread = CmdThread(self, self.requestQ, self.resultQ)
        
        self.outputBox = wx.StaticBox(parent = self.panelOutput, id = wx.ID_ANY,
                                      label = " %s " % _("Output window"))

        if self._gcstyle & GC_PROMPT:
            cmdLabel = _("Command prompt")
        else:
            cmdLabel = _("Command")
        self.cmdBox = wx.StaticBox(parent = self.panelOutput, id = wx.ID_ANY,
                                   label = " %s " % cmdLabel)

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
        
        if not self._gcstyle & GC_PROMPT:
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
        
        if self._gcstyle & GC_PROMPT:
            promptSizer = wx.BoxSizer(wx.VERTICAL)
            promptSizer.Add(item = self.cmdPrompt, proportion = 1,
                        flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.TOP, border = 3)
        
        if self._gcstyle & GC_SEARCH:
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
        
        if self._gcstyle & GC_PROMPT:
            proportion = (2, 3)
        else:
            proportion = (1, 1)
        
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
        
        if self._gcstyle & GC_PROMPT:
            promptSizer.Fit(self)
            promptSizer.SetSizeHints(self)
            self.panelPrompt.SetSizer(promptSizer)
        
        # split window
        if self._gcstyle & GC_PROMPT:
            self.SplitHorizontally(self.panelOutput, self.panelPrompt, -50)
        else:
            self.SplitHorizontally(self.panelOutput, self.panelPrompt, -45)
            self.Unsplit()
        self.SetMinimumPaneSize(self.btnCmdClear.GetSize()[1] + 25)
        
        self.SetSashGravity(1.0)
        
        # layout
        self.SetAutoLayout(True)
        self.Layout()

    def MakeSearchPaneContent(self, pane, modulesData):
        """!Create search pane"""
        border = wx.BoxSizer(wx.VERTICAL)
        
        self.search = SearchModuleWidget(parent = pane,
                                         modulesData = modulesData)
        
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
                 switchPage = False, priority = 1):
        """!Generic method for writing log message in 
        given style

        @param line text line
        @param style text style (see GStc)
        @param stdout write to stdout or stderr
        @param switchPage for backward compatibility
        (replace by priority: False=1, True=2)
        @param priority priority of this message
        (0=no priority, 1=normal, 2=medium, 3=high)
        """

        self.cmdOutput.SetStyle()

        # documenting old behavior/implementation:
        # switch notebook if required
        if priority == 1:
            if switchPage:
                priority = 2
        event = gOutputText(priority = priority)
        wx.PostEvent(self, event)
        
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
                   parent = self.frame)
            fileHistory = None
        
        if fileHistory:
            try:
                fileHistory.write(' '.join(command) + os.linesep)
            finally:
                fileHistory.close()
        
        if command[0] in globalvar.grassCmd:
            # send GRASS command without arguments to GUI command interface
            # except display commands (they are handled differently)
            if self.frame.GetName() == "LayerManager" and \
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
                    GMessage(parent = self.frame,
                             message = _("Command '%s' not yet implemented in the WxGUI. "
                                         "Try adding it as a command layer instead.") % command[0])
                    return
                
                if layertype == 'barscale':
                    self.frame.GetLayerTree().GetMapDisplay().OnAddBarscale(None)
                elif layertype == 'rastleg':
                    self.frame.GetLayerTree().GetMapDisplay().OnAddLegend(None)
                elif layertype == 'redraw':
                    self.frame.GetLayerTree().GetMapDisplay().OnRender(None)
                else:
                    # add layer into layer tree
                    lname, found = utils.GetLayerNameFromCmd(command, fullyQualified = True,
                                                             layerType = layertype)
                    if self.frame.GetName() == "LayerManager":
                        self.frame.GetLayerTree().AddLayer(ltype = layertype,
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

                # documenting old behavior/implementation:
                # switch and focus if required
                # TODO: this probably should be run command event
                if switchPage:
                    priority = 3
                    event = gOutputText(priority = priority)
                    wx.PostEvent(self, event)

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
            self.frame.SetStatusText(_("Commands output saved into '%s'") % path)
        
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

    def OnCmdOutput(self, event):
        """!Print command output"""
        message = event.text
        type  = event.type

        self.cmdOutput.AddStyledMessage(message, type)

        # documenting old behavior/implementation:
        # add elipses if not active
        event = gOutputText(priority = 1)
        wx.PostEvent(self, event)

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
            
        self.frame.SetStatusText(_("Commands protocol saved into '%s'") % self.cmdFileProtocol)
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
        self.WriteCmdLog('(%s)\n%s' % (str(time.ctime()), ' '.join(event.cmd)))
        self.btnCmdAbort.Enable()

        event.Skip()

    def OnCmdDone(self, event):
        """!Command done (or aborted)"""

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
        
        if self.frame.GetName() == "LayerManager":
            self.btnCmdAbort.Enable(False)
            if event.cmd[0] not in globalvar.grassCmd or \
                    event.cmd[0] == 'r.mapcalc':
                return
            
            tree = self.frame.GetLayerTree()
            display = None
            if tree:
                display = tree.GetMapDisplay()
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
        elif self.frame.GetName() == 'Modeler':
            pass
        else: # standalone dialogs
            dialog = self.frame
            if hasattr(dialog, "btn_abort"):
                dialog.btn_abort.Enable(False)
            
            if hasattr(dialog, "btn_cancel"):
                dialog.btn_cancel.Enable(True)
            
            if hasattr(dialog, "btn_clipboard"):
                dialog.btn_clipboard.Enable(True)
            
            if hasattr(dialog, "btn_help"):
                dialog.btn_help.Enable(True)
            
            if hasattr(dialog, "btn_run"):
                dialog.btn_run.Enable(True)
            
            if event.returncode == 0 and not event.aborted:
                try:
                    winName = self.frame.parent.GetName()
                except AttributeError:
                    winName = ''
                
                if winName == 'LayerManager':
                    mapTree = self.frame.parent.GetLayerTree()
                elif winName == 'LayerTree':
                    mapTree = self.frame.parent
                elif winName: # GConsole
                    mapTree = self.frame.parent.parent.GetLayerTree()
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
                            elif prompt == '3d-raster':
                                lcmd = ['d.rast3d',
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
        
    def GetPrompt(self):
        """!Get prompt"""
        return self.cmdPrompt
    
class GStdout:
    """!GConsole standard output

    Based on FrameOutErr.py

    Name:      FrameOutErr.py
    Purpose:   Redirecting stdout / stderr
    Author:    Jean-Michel Fauth, Switzerland
    Copyright: (c) 2005-2007 Jean-Michel Fauth
    Licence:   GPL
    """
    def __init__(self, receiver):
        """!
        @param receiver event receiver (used in PostEvent)
        """
        self.receiver = receiver

    def write(self, s):
        if len(s) == 0 or s == '\n':
            return

        for line in s.splitlines():
            if len(line) == 0:
                continue
            
            evt = wxCmdOutput(text = line + '\n',
                              type = '')
            wx.PostEvent(self.receiver, evt)
        
class GStderr:
    """!GConsole standard error output

    Based on FrameOutErr.py

    Name:      FrameOutErr.py
    Purpose:   Redirecting stdout / stderr
    Author:    Jean-Michel Fauth, Switzerland
    Copyright: (c) 2005-2007 Jean-Michel Fauth
    Licence:   GPL
    """
    def __init__(self, receiver):
        """!
        @param receiver event receiver (used in PostEvent)
        """
        self.receiver = receiver
        
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
                wx.PostEvent(self.receiver, evt)
            elif len(line) > 0:
                self.message += line.strip() + '\n'

            if self.printMessage and len(self.message) > 0:
                evt = wxCmdOutput(text = self.message,
                                  type = self.type)
                wx.PostEvent(self.receiver, evt)

                self.type = ''
                self.message = ''
                self.printMessage = False

        # update progress message
        if progressValue > -1:
            # self.gmgauge.SetValue(progressValue)
            evt = wxCmdProgress(value = progressValue)
            wx.PostEvent(self.receiver, evt)
            
class GStc(stc.StyledTextCtrl):
    """!Styled text control for GRASS stdout and stderr.

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

        # remember position of line begining (used for '\r')
        self.linePos         = -1

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
        
        typeface = UserSettings.Get(group = 'appearance', key = 'outputfont', subkey = 'type')   
        if typeface == "":
            typeface = "Courier New"
        
        typesize = UserSettings.Get(group = 'appearance', key = 'outputfont', subkey = 'size')
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
            self.linePos = -1
            for seg in txt.split('\r'):
                if self.linePos > -1:
                    self.SetCurrentPos(self.linePos)
                    self.ReplaceSelection(seg)
                else:
                    self.linePos = self.GetCurrentPos()
                    self.AddText(seg)
        else:
            self.linePos = self.GetCurrentPos()
            
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

    def AddStyledMessage(self, message, style = None):
        """!Add message to text area.

        Handles messages with progress percentages.

        @param message message to be added
        @param style style of message, allowed values: 'message', 'warning', 'error' or None
        """
        # message prefix
        if style == 'warning':
            message = 'WARNING: ' + message
        elif style == 'error':
            message = 'ERROR: ' + message
        
        p1 = self.GetEndStyled()
        self.GotoPos(p1)
        
        # is this still needed?
        if '\b' in message:
            if self.linePos < 0:
                self.linePos = p1
            last_c = ''
            for c in message:
                if c == '\b':
                    self.linePos -= 1
                else:
                    if c == '\r':
                        pos = self.GetCurLine()[1]
                        # self.SetCurrentPos(pos)
                    else:
                        self.SetCurrentPos(self.linePos)
                    self.ReplaceSelection(c)
                    self.linePos = self.GetCurrentPos()
                    if c != ' ':
                        last_c = c
            if last_c not in ('0123456789'):
                self.AddTextWrapped('\n', wrap = None)
                self.linePos = -1
        else:
            self.linePos = -1 # don't force position
            if '\n' not in message:
                self.AddTextWrapped(message, wrap = 60)
            else:
                self.AddTextWrapped(message, wrap = None)
        p2 = self.GetCurrentPos()
        
        if p2 >= p1:
            self.StartStyling(p1, 0xff)
        
            if style == 'error':
                self.SetStyling(p2 - p1, self.StyleError)
            elif style == 'warning':
                self.SetStyling(p2 - p1, self.StyleWarning)
            elif style == 'message':
                self.SetStyling(p2 - p1, self.StyleMessage)
            else: # unknown
                self.SetStyling(p2 - p1, self.StyleUnknown)
        
        self.EnsureCaretVisible()
