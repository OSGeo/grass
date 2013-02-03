"""!
@package core.giface

@brief GRASS interface for standalone application (without layer manager)

Classes:
 - giface::StandaloneGrassInterface

(C) 2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
@author Vaclav Petras <wenzeslaus gmail.com>
"""

import os
from core.gconsole import GConsole, \
    EVT_CMD_OUTPUT, EVT_CMD_PROGRESS, EVT_CMD_RUN, EVT_CMD_DONE, \
    EVT_WRITE_LOG, EVT_WRITE_CMD_LOG, EVT_WRITE_WARNING, EVT_WRITE_ERROR

import grass.script as grass

class StandaloneGrassInterface():
    def __init__(self):
        self._gconsole = GConsole()
        self._gconsole.Bind(EVT_CMD_PROGRESS, self._onCmdProgress)
        self._gconsole.Bind(EVT_CMD_OUTPUT, self._onCmdOutput)
        self._gconsole.Bind(EVT_WRITE_LOG,
                            lambda event:
                                self.WriteLog(text = event.text))
        self._gconsole.Bind(EVT_WRITE_CMD_LOG,
                            lambda event:
                                self.WriteCmdLog(line = event.line))
        self._gconsole.Bind(EVT_WRITE_WARNING,
                            lambda event:
                                self.WriteWarning(line = event.line))
        self._gconsole.Bind(EVT_WRITE_ERROR,
                            lambda event:
                                self.WriteError(line = event.line))

    def _onCmdOutput(self, event):
        """!Print command output"""
        message = event.text
        style  = event.type

        if style == 'warning':
            self.WriteWarning(message)
        elif style == 'error':
            self.WriteError(message)
        else:
            self.WriteLog(message)
        event.Skip()

    def _onCmdProgress(self, event):
        """!Update progress message info"""
        grass.percent(event.value, 100, 1)
        event.Skip()

    def RunCmd(self, command, compReg=True, switchPage=False, skipInterface=False,
               onDone=None, onPrepare=None, userData=None, priority=1):
        self._gconsole.RunCmd(command=command, compReg=compReg, switchPage=switchPage,
                              skipInterface=skipInterface, onDone=onDone,
                              onPrepare=onPrepare, userData=userData, priority=priority)

    def Help(self, entry):
        self._gconsole.RunCmd(['g.manual', 'entry=%s' % entry])

    def WriteLog(self, text, wrap = None,
                 switchPage = False, priority = 1):
        self._write(grass.message, text)

    def WriteCmdLog(self, line, pid = None, switchPage = True):
        if pid:
            line = '(' + str(pid) + ') ' + line
        self._write(grass.message, line)

    def WriteWarning(self, line):
        self._write(grass.warning, line)

    def WriteError(self, line):
        self._write(grass.error, line)

    def _write(self, function, text):
        orig = os.getenv("GRASS_MESSAGE_FORMAT")
        os.environ["GRASS_MESSAGE_FORMAT"] = 'standard'
        function(text)
        os.environ["GRASS_MESSAGE_FORMAT"] = orig

    def GetLayerList(self):
        return None
        
    def GetMapDisplay(self):
        """!Get current map display.
        """
        return None

    def GetAllMapDisplays(self):
        """!Get list of all map displays.
        """
        return []

    def GetMapWindow(self):
        return None

    def GetProgress(self):
        raise NotImplementedError
