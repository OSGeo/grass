"""
@package core.gconsole

@brief Command output widgets

Classes:
 - goutput::CmdThread
 - goutput::GStdout
 - goutput::GStderr
 - goutput::GConsole

(C) 2007-2015 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton (Arizona State University)
@author Martin Landa <landa.martin gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com> (refactoring)
@author Anna Kratochvilova <kratochanna gmail.com> (refactoring)
"""

from __future__ import print_function

import os
import sys
import re
import time
import threading

if sys.version_info.major == 2:
    import Queue
else:
    import queue as Queue

import codecs
import locale

import wx
from wx.lib.newevent import NewEvent

import grass.script as grass
from grass.script import task as gtask

from grass.pydispatch.signal import Signal

from core import globalvar
from core.gcmd import CommandThread, GError, GException
from gui_core.forms import GUI
from core.debug import Debug
from core.settings import UserSettings
from core.giface import Notification
from gui_core.widgets import FormNotebook

wxCmdOutput, EVT_CMD_OUTPUT = NewEvent()
wxCmdProgress, EVT_CMD_PROGRESS = NewEvent()
wxCmdRun, EVT_CMD_RUN = NewEvent()
wxCmdDone, EVT_CMD_DONE = NewEvent()
wxCmdAbort, EVT_CMD_ABORT = NewEvent()
wxCmdPrepare, EVT_CMD_PREPARE = NewEvent()


def GrassCmd(cmd, env=None, stdout=None, stderr=None):
    """Return GRASS command thread"""
    return CommandThread(cmd, env=env, stdout=stdout, stderr=stderr)


class CmdThread(threading.Thread):
    """Thread for GRASS commands"""

    requestId = 0

    def __init__(self, receiver, requestQ=None, resultQ=None, **kwds):
        """
        :param receiver: event receiver (used in PostEvent)
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

        self.requestCmd = None

        self.receiver = receiver
        self._want_abort_all = False

        self.start()

    def RunCmd(self, *args, **kwds):
        """Run command in queue

        :param args: unnamed command arguments
        :param kwds: named command arguments

        :return: request id in queue
        """
        CmdThread.requestId += 1

        self.requestCmd = None
        self.requestQ.put((CmdThread.requestId, args, kwds))

        return CmdThread.requestId

    def GetId(self):
        """Get id for next command"""
        return CmdThread.requestId + 1

    def SetId(self, id):
        """Set starting id"""
        CmdThread.requestId = id

    def run(self):
        os.environ["GRASS_MESSAGE_FORMAT"] = "gui"
        while True:
            requestId, args, kwds = self.requestQ.get()
            for key in (
                "callable",
                "onDone",
                "onPrepare",
                "userData",
                "addLayer",
                "notification",
            ):
                if key in kwds:
                    vars()[key] = kwds[key]
                    del kwds[key]
                else:
                    vars()[key] = None

            if not vars()["callable"]:
                vars()["callable"] = GrassCmd

            requestTime = time.time()

            # prepare
            if self.receiver:
                event = wxCmdPrepare(
                    cmd=args[0],
                    time=requestTime,
                    pid=requestId,
                    onPrepare=vars()["onPrepare"],
                    userData=vars()["userData"],
                )

                wx.PostEvent(self.receiver, event)

                # run command
                event = wxCmdRun(
                    cmd=args[0], pid=requestId, notification=vars()["notification"]
                )

                wx.PostEvent(self.receiver, event)

            time.sleep(0.1)
            self.requestCmd = vars()["callable"](*args, **kwds)
            if self._want_abort_all and self.requestCmd is not None:
                self.requestCmd.abort()
                if self.requestQ.empty():
                    self._want_abort_all = False

            self.resultQ.put((requestId, self.requestCmd.run()))

            try:
                returncode = self.requestCmd.module.returncode
            except AttributeError:
                returncode = 0  # being optimistic

            try:
                aborted = self.requestCmd.aborted
            except AttributeError:
                aborted = False

            time.sleep(0.1)

            # set default color table for raster data
            if (
                UserSettings.Get(
                    group="rasterLayer", key="colorTable", subkey="enabled"
                )
                and args[0][0][:2] == "r."
            ):
                colorTable = UserSettings.Get(
                    group="rasterLayer", key="colorTable", subkey="selection"
                )
                mapName = None
                if args[0][0] == "r.mapcalc":
                    try:
                        mapName = args[0][1].split("=", 1)[0].strip()
                    except KeyError:
                        pass
                else:
                    moduleInterface = GUI(show=None).ParseCommand(args[0])
                    outputParam = moduleInterface.get_param(
                        value="output", raiseError=False
                    )
                    if outputParam and outputParam["prompt"] == "raster":
                        mapName = outputParam["value"]

                if mapName:
                    argsColor = list(args)
                    argsColor[0] = [
                        "r.colors",
                        "map=%s" % mapName,
                        "color=%s" % colorTable,
                    ]
                    self.requestCmdColor = vars()["callable"](*argsColor, **kwds)
                    self.resultQ.put((requestId, self.requestCmdColor.run()))

            if self.receiver:
                event = wxCmdDone(
                    cmd=args[0],
                    aborted=aborted,
                    returncode=returncode,
                    time=requestTime,
                    pid=requestId,
                    onDone=vars()["onDone"],
                    userData=vars()["userData"],
                    addLayer=vars()["addLayer"],
                    notification=vars()["notification"],
                )

                # send event
                wx.PostEvent(self.receiver, event)

    def abort(self, abortall=True):
        """Abort command(s)"""
        if abortall:
            self._want_abort_all = True
        if self.requestCmd is not None:
            self.requestCmd.abort()
        if self.requestQ.empty():
            self._want_abort_all = False


class GStdout:
    """GConsole standard output

    Based on FrameOutErr.py

    Name:      FrameOutErr.py
    Purpose:   Redirecting stdout / stderr
    Author:    Jean-Michel Fauth, Switzerland
    Copyright: (c) 2005-2007 Jean-Michel Fauth
    Licence:   GPL
    """

    def __init__(self, receiver):
        """
        :param receiver: event receiver (used in PostEvent)
        """
        self.receiver = receiver

    def flush(self):
        pass

    def write(self, s):
        if len(s) == 0 or s == "\n":
            return

        for line in s.splitlines():
            if len(line) == 0:
                continue

            evt = wxCmdOutput(text=line + "\n", type="")
            wx.PostEvent(self.receiver, evt)


class GStderr:
    """GConsole standard error output

    Based on FrameOutErr.py

    Name:      FrameOutErr.py
    Purpose:   Redirecting stdout / stderr
    Author:    Jean-Michel Fauth, Switzerland
    Copyright: (c) 2005-2007 Jean-Michel Fauth
    Licence:   GPL
    """

    def __init__(self, receiver):
        """
        :param receiver: event receiver (used in PostEvent)
        """
        self.receiver = receiver
        self.type = ""
        self.message = ""
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

            if "GRASS_INFO_PERCENT" in line:
                value = int(line.rsplit(":", 1)[1].strip())
                if value >= 0 and value < 100:
                    progressValue = value
                else:
                    progressValue = 0
            elif "GRASS_INFO_MESSAGE" in line:
                self.type = "message"
                self.message += line.split(":", 1)[1].strip() + "\n"
            elif "GRASS_INFO_WARNING" in line:
                self.type = "warning"
                self.message += line.split(":", 1)[1].strip() + "\n"
            elif "GRASS_INFO_ERROR" in line:
                self.type = "error"
                self.message += line.split(":", 1)[1].strip() + "\n"
            elif "GRASS_INFO_END" in line:
                self.printMessage = True
            elif self.type == "":
                if len(line) == 0:
                    continue
                evt = wxCmdOutput(text=line, type="")
                wx.PostEvent(self.receiver, evt)
            elif len(line) > 0:
                self.message += line.strip() + "\n"

            if self.printMessage and len(self.message) > 0:
                evt = wxCmdOutput(text=self.message, type=self.type)
                wx.PostEvent(self.receiver, evt)

                self.type = ""
                self.message = ""
                self.printMessage = False

        # update progress message
        if progressValue > -1:
            # self.gmgauge.SetValue(progressValue)
            evt = wxCmdProgress(value=progressValue)
            wx.PostEvent(self.receiver, evt)


# Occurs when an ignored command is called.
# Attribute cmd contains command (as a list).
gIgnoredCmdRun, EVT_IGNORED_CMD_RUN = NewEvent()


class GConsole(wx.EvtHandler):
    """Backend for command execution, esp. interactive command execution"""

    def __init__(self, guiparent=None, giface=None, ignoredCmdPattern=None):
        """
        :param guiparent: parent window for created GUI objects
        :param lmgr: layer manager window (TODO: replace by giface)
        :param ignoredCmdPattern: regular expression specifying commads
                                  to be ignored (e.g. @c '^d\..*' for
                                  display commands)
        """
        wx.EvtHandler.__init__(self)

        # Signal when some map is created or updated by a module.
        # attributes: name: map name, ltype: map type,
        self.mapCreated = Signal("GConsole.mapCreated")
        # emitted when map display should be re-render
        self.updateMap = Signal("GConsole.updateMap")
        # emitted when log message should be written
        self.writeLog = Signal("GConsole.writeLog")
        # emitted when command log message should be written
        self.writeCmdLog = Signal("GConsole.writeCmdLog")
        # emitted when warning message should be written
        self.writeWarning = Signal("GConsole.writeWarning")
        # emitted when error message should be written
        self.writeError = Signal("GConsole.writeError")

        self._guiparent = guiparent
        self._giface = giface
        self._ignoredCmdPattern = ignoredCmdPattern

        # create queues
        self.requestQ = Queue.Queue()
        self.resultQ = Queue.Queue()

        self.cmdOutputTimer = wx.Timer(self)
        self.Bind(wx.EVT_TIMER, self.OnProcessPendingOutputWindowEvents)
        self.Bind(EVT_CMD_RUN, self.OnCmdRun)
        self.Bind(EVT_CMD_DONE, self.OnCmdDone)
        self.Bind(EVT_CMD_ABORT, self.OnCmdAbort)

        # stream redirection
        self.cmdStdOut = GStdout(receiver=self)
        self.cmdStdErr = GStderr(receiver=self)

        # thread
        self.cmdThread = CmdThread(self, self.requestQ, self.resultQ)

    def Redirect(self):
        """Redirect stdout/stderr"""
        if Debug.GetLevel() == 0 and grass.debug_level(force=True) == 0:
            # don't redirect when debugging is enabled
            sys.stdout = self.cmdStdOut
            sys.stderr = self.cmdStdErr
        else:
            enc = locale.getdefaultlocale()[1]
            if enc:
                if sys.version_info.major == 2:
                    sys.stdout = codecs.getwriter(enc)(sys.__stdout__)
                    sys.stderr = codecs.getwriter(enc)(sys.__stderr__)
                else:
                    # https://stackoverflow.com/questions/4374455/how-to-set-sys-stdout-encoding-in-python-3
                    sys.stdout = codecs.getwriter(enc)(sys.__stdout__.detach())
                    sys.stderr = codecs.getwriter(enc)(sys.__stderr__.detach())
            else:
                sys.stdout = sys.__stdout__
                sys.stderr = sys.__stderr__

    def WriteLog(
        self, text, style=None, wrap=None, notification=Notification.HIGHLIGHT
    ):
        """Generic method for writing log message in
        given style

        :param text: text line
        :param notification: form of notification
        """
        self.writeLog.emit(text=text, wrap=wrap, notification=notification)

    def WriteCmdLog(self, text, pid=None, notification=Notification.MAKE_VISIBLE):
        """Write message in selected style

        :param text: message to be printed
        :param pid: process pid or None
        :param notification: form of notification
        """
        self.writeCmdLog.emit(text=text, pid=pid, notification=notification)

    def WriteWarning(self, text):
        """Write message in warning style"""
        self.writeWarning.emit(text=text)

    def WriteError(self, text):
        """Write message in error style"""
        self.writeError.emit(text=text)

    def RunCmd(
        self,
        command,
        compReg=True,
        env=None,
        skipInterface=False,
        onDone=None,
        onPrepare=None,
        userData=None,
        addLayer=None,
        notification=Notification.MAKE_VISIBLE,
    ):
        """Run command typed into console command prompt (GPrompt).

        .. todo::
            Document the other event.
        .. todo::
            Solve problem with the other event (now uses gOutputText
            event but there is no text, use onPrepare handler instead?)
        .. todo::
            Skip interface is ignored and determined always automatically.

        Posts event EVT_IGNORED_CMD_RUN when command which should be ignored
        (according to ignoredCmdPattern) is run.
        For example, see layer manager which handles d.* on its own.

        :param command: command given as a list (produced e.g. by utils.split())
        :param compReg: True use computation region
        :param notification: form of notification
        :param bool skipInterface: True to do not launch GRASS interface
                                   parser when command has no arguments
                                   given
        :param onDone: function to be called when command is finished
        :param onPrepare: function to be called before command is launched
        :param addLayer: to be passed in the mapCreated signal
        :param userData: data defined for the command
        """
        if len(command) == 0:
            Debug.msg(2, "GPrompt:RunCmd(): empty command")
            return

        # update history file
        self.UpdateHistoryFile(" ".join(command))

        if command[0] in globalvar.grassCmd:
            # send GRASS command without arguments to GUI command interface
            # except ignored commands (event is emitted)
            if (
                self._ignoredCmdPattern
                and re.compile(self._ignoredCmdPattern).search(" ".join(command))
                and "--help" not in command
                and "--ui" not in command
            ):
                event = gIgnoredCmdRun(cmd=command)
                wx.PostEvent(self, event)
                return

            else:
                # other GRASS commands (r|v|g|...)
                try:
                    task = GUI(show=None).ParseCommand(command)
                except GException as e:
                    GError(parent=self._guiparent, message=str(e), showTraceback=False)
                    return

                hasParams = False
                if task:
                    options = task.get_options()
                    hasParams = options["params"] and options["flags"]
                    # check for <input>=-
                    for p in options["params"]:
                        if (
                            p.get("prompt", "") == "input"
                            and p.get("element", "") == "file"
                            and p.get("age", "new") == "old"
                            and p.get("value", "") == "-"
                        ):
                            GError(
                                parent=self._guiparent,
                                message=_(
                                    "Unable to run command:\n%(cmd)s\n\n"
                                    "Option <%(opt)s>: read from standard input is not "
                                    "supported by wxGUI"
                                )
                                % {"cmd": " ".join(command), "opt": p.get("name", "")},
                            )
                            return

                if len(command) == 1:
                    if command[0].startswith("g.gui."):
                        import imp
                        import inspect

                        pyFile = command[0]
                        if sys.platform == "win32":
                            pyFile += ".py"
                        pyPath = os.path.join(os.environ["GISBASE"], "scripts", pyFile)
                        if not os.path.exists(pyPath):
                            pyPath = os.path.join(
                                os.environ["GRASS_ADDON_BASE"], "scripts", pyFile
                            )
                        if not os.path.exists(pyPath):
                            GError(
                                parent=self._guiparent,
                                message=_("Module <%s> not found.") % command[0],
                            )
                        pymodule = imp.load_source(command[0].replace(".", "_"), pyPath)
                        try:  # PY3
                            pymain = inspect.getfullargspec(pymodule.main)
                        except AttributeError:
                            pymain = inspect.getargspec(pymodule.main)
                        if pymain and "giface" in pymain.args:
                            pymodule.main(self._giface)
                            return

                    # no arguments given
                    if hasParams and not isinstance(self._guiparent, FormNotebook):
                        # also parent must be checked, see #3135 for details
                        try:
                            GUI(
                                parent=self._guiparent, giface=self._giface
                            ).ParseCommand(command)
                        except GException as e:
                            print(e, file=sys.stderr)

                        return

                if env:
                    env = env.copy()
                else:
                    env = os.environ.copy()
                # activate computational region (set with g.region)
                # for all non-display commands.
                if compReg and "GRASS_REGION" in env:
                    del env["GRASS_REGION"]

                # process GRASS command with argument
                self.cmdThread.RunCmd(
                    command,
                    stdout=self.cmdStdOut,
                    stderr=self.cmdStdErr,
                    onDone=onDone,
                    onPrepare=onPrepare,
                    userData=userData,
                    addLayer=addLayer,
                    env=env,
                    notification=notification,
                )
                self.cmdOutputTimer.Start(50)

                # we don't need to change computational region settings
                # because we work on a copy
        else:
            # Send any other command to the shell. Send output to
            # console output window
            #
            # Check if the script has an interface (avoid double-launching
            # of the script)

            # check if we ignore the command (similar to grass commands part)
            if self._ignoredCmdPattern and re.compile(self._ignoredCmdPattern).search(
                " ".join(command)
            ):
                event = gIgnoredCmdRun(cmd=command)
                wx.PostEvent(self, event)
                return

            skipInterface = True
            if os.path.splitext(command[0])[1] in (".py", ".sh"):
                try:
                    sfile = open(command[0], "r")
                    for line in sfile.readlines():
                        if len(line) < 2:
                            continue
                        if line[0] == "#" and line[1] == "%":
                            skipInterface = False
                            break
                    sfile.close()
                except IOError:
                    pass

            if len(command) == 1 and not skipInterface:
                try:
                    task = gtask.parse_interface(command[0])
                except:
                    task = None
            else:
                task = None

            if task:
                # process GRASS command without argument
                GUI(parent=self._guiparent, giface=self._giface).ParseCommand(command)
            else:
                self.cmdThread.RunCmd(
                    command,
                    stdout=self.cmdStdOut,
                    stderr=self.cmdStdErr,
                    onDone=onDone,
                    onPrepare=onPrepare,
                    userData=userData,
                    addLayer=addLayer,
                    env=env,
                    notification=notification,
                )
            self.cmdOutputTimer.Start(50)

    def GetLog(self, err=False):
        """Get widget used for logging

        .. todo::
           what's this?

        :param bool err: True to get stderr widget
        """
        if err:
            return self.cmdStdErr

        return self.cmdStdOut

    def GetCmd(self):
        """Get running command or None"""
        return self.requestQ.get()

    def OnCmdAbort(self, event):
        """Abort running command"""
        self.cmdThread.abort()
        event.Skip()

    def OnCmdRun(self, event):
        """Run command"""
        self.WriteCmdLog(
            "(%s)\n%s" % (str(time.ctime()), " ".join(event.cmd)),
            notification=event.notification,
        )
        event.Skip()

    def OnCmdDone(self, event):
        """Command done (or aborted)

        Sends signal mapCreated if map is recognized in output
        parameters or for specific modules (as r.colors).
        """
        # Process results here
        try:
            ctime = time.time() - event.time
            if ctime < 60:
                stime = _("%d sec") % int(ctime)
            else:
                mtime = int(ctime / 60)
                stime = _("%(min)d min %(sec)d sec") % {
                    "min": mtime,
                    "sec": int(ctime - (mtime * 60)),
                }
        except KeyError:
            # stopped deamon
            stime = _("unknown")

        if event.aborted:
            # Thread aborted (using our convention of None return)
            self.WriteWarning(
                _(
                    "Please note that the data are left in"
                    " inconsistent state and may be corrupted"
                )
            )
            msg = _("Command aborted")
        else:
            msg = _("Command finished")

        self.WriteCmdLog(
            "(%s) %s (%s)" % (str(time.ctime()), msg, stime),
            notification=event.notification,
        )

        if event.onDone:
            event.onDone(event)

        self.cmdOutputTimer.Stop()

        if event.cmd[0] == "g.gisenv":
            Debug.SetLevel()
            self.Redirect()

        # do nothing when no map added
        if event.returncode != 0 or event.aborted:
            event.Skip()
            return

        if event.cmd[0] not in globalvar.grassCmd:
            return

        # find which maps were created
        try:
            task = GUI(show=None).ParseCommand(event.cmd)
        except GException as e:
            print(e, file=sys.stderr)
            task = None
            return

        name = task.get_name()
        for p in task.get_options()["params"]:
            prompt = p.get("prompt", "")
            if prompt in ("raster", "vector", "raster_3d") and p.get("value", None):
                if p.get("age", "old") == "new" or name in (
                    "r.colors",
                    "r3.colors",
                    "v.colors",
                    "v.proj",
                    "r.proj",
                ):
                    # if multiple maps (e.g. r.series.interp), we need add each
                    if p.get("multiple", False):
                        lnames = p.get("value").split(",")
                        # in case multiple input (old) maps in r.colors
                        # we don't want to rerender it multiple times! just
                        # once
                        if p.get("age", "old") == "old":
                            lnames = lnames[0:1]
                    else:
                        lnames = [p.get("value")]
                    for lname in lnames:
                        if "@" not in lname:
                            lname += "@" + grass.gisenv()["MAPSET"]
                        if grass.find_file(lname, element=p.get("element"))["fullname"]:
                            self.mapCreated.emit(
                                name=lname, ltype=prompt, add=event.addLayer
                            )
                            gisenv = grass.gisenv()
                            self._giface.grassdbChanged.emit(
                                grassdb=gisenv["GISDBASE"],
                                location=gisenv["LOCATION_NAME"],
                                mapset=gisenv["MAPSET"],
                                action="new",
                                map=lname.split("@")[0],
                                element=prompt,
                            )
        if name == "r.mask":
            self.updateMap.emit()

        event.Skip()

    def OnProcessPendingOutputWindowEvents(self, event):
        wx.GetApp().ProcessPendingEvents()

    def UpdateHistoryFile(self, command):
        """Update history file

        :param command: the command given as a string
        """
        env = grass.gisenv()
        try:
            filePath = os.path.join(
                env["GISDBASE"], env["LOCATION_NAME"], env["MAPSET"], ".bash_history"
            )
            fileHistory = codecs.open(filePath, encoding="utf-8", mode="a")
        except IOError as e:
            GError(
                _("Unable to write file '%(filePath)s'.\n\nDetails: %(error)s")
                % {"filePath": filePath, "error": e},
                parent=self._guiparent,
            )
            return

        try:
            fileHistory.write(command + os.linesep)
        finally:
            fileHistory.close()

        # update wxGUI prompt
        if self._giface:
            self._giface.UpdateCmdHistory(command)
