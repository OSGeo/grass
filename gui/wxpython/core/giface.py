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
from core.gcmd import RunCommand
from core.utils import CmdToTuple

import grass.script as grass

class StandaloneGrassInterface():
    def RunCmd(self, command,
               onDone = None, onPrepare = None, userData = None, **kwargs):
        if onPrepare:
            onPrepare(userData)

        cmdTuple = CmdToTuple(command)
        returncode = RunCommand(cmdTuple[0], **cmdTuple[1])

        if onDone:
            onDone(cmd = command, returncode = returncode)

    def Help(self, entry):
        RunCommand('g.manual', quiet = True, entry = entry)

    def WriteLog(self, text, wrap = None,
                 switchPage = False, priority = 1):
        grass.message(text)

    def WriteCmdLog(self, line, pid = None, switchPage = True):
        grass.message(text)

    def WriteWarning(self, line):
        grass.warning(line)

    def WriteError(self, line):
        grass.error(line)

    def GetLayerTree(self):
        return None

    def GetMapDisplay(self):
        """!Get current map display.
        """
        return None

    def GetAllMapDisplays(self):
        """!Get list of all map displays.
        """
        return []