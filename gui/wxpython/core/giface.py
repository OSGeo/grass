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

from grass.pydispatch.signal import Signal

# to disable Abstract class not referenced
#pylint: disable=R0921


class Layer(object):
    """!Layer is generaly usable layer object.

    @note Currently without specifying the interface.
    Current implementations only provides all attributes of existing layer
    as used in lmgr.
    """


class LayerList(object):
    def GetSelectedLayers(self, checkedOnly=True):
        """!Returns list of selected layers.

        @note Usage of checked and selected is still subject to change.
        Checked layers should be showed. Selected are for analyses.
        However, this may be the same for some implementations
        (e.g. it d.mon has all layers checked and selected).
        """
        raise NotImplementedError()


class GrassInterface:
    """!GrassInterface provides the functionality which should be available
    to every GUI component.

    @note The GrassInterface process is not finished.
    """
    def RunCmd(self, *args, **kwargs):
        """!Executes a command.
        """
        raise NotImplementedError()

    def Help(self, entry):
        """Shows a manual page for a given entry.
        """
        raise NotImplementedError()

    def WriteLog(self, text, wrap=None, switchPage=False, priority=1):
        """!Writes log message.

        @note It is not clear how the switchPage and priority should work.
        @note Use priority rather than switchPage, switchPage will be removed.
        """
        raise NotImplementedError()

    def WriteCmdLog(self, line, pid=None, switchPage=True):
        """!Writes message related to start or end of the command.
        """
        raise NotImplementedError()

    def WriteWarning(self, line):
        """!Writes warning message for the user.

        Currently used also for important messages for user.
        Overlaps with log message with high priority.
        """
        raise NotImplementedError()

    def WriteError(self, line):
        """!Writes error message for the user."""
        raise NotImplementedError()

    def GetLayerTree(self):
        """!Returns LayerManager's tree GUI object.
        @note Will be removed from the interface.
        """
        raise NotImplementedError()

    def GetLayerList(self):
        """!Returns a layer management object.
        """
        raise NotImplementedError()

    def GetMapDisplay(self):
        """!Returns current map display.

        @note For layer related tasks use GetLayerList().

        @return MapFrame instance
        @return None when no mapdisplay open
        """
        raise NotImplementedError()

    def GetAllMapDisplays(self):
        """!Get list of all map displays.

        @note Might be removed from the interface.

        @return list of MapFrame instances
        """
        raise NotImplementedError()

    def GetMapWindow(self):
        """!Returns current map window.
        @note For layer related tasks use GetLayerList().
        """
        raise NotImplementedError()

    def GetProgress(self):
        """!Returns object which shows the progress.

        @note Some implementations may not implement this method.
        """
        raise NotImplementedError()


class StandaloneGrassInterface():
    """!@implements GrassInterface"""
    def __init__(self):

        # Signal when some map is created or updated by a module.
        # attributes: name: map name, ltype: map type,
        # add: if map should be added to layer tree (questionable attribute)
        self.mapCreated = Signal('StandaloneGrassInterface.mapCreated')

        # Signal emitted to request updating of map
        self.updateMap = Signal('StandaloneGrassInterface.updateMap')

        self._gconsole = GConsole()
        self._gconsole.Bind(EVT_CMD_PROGRESS, self._onCmdProgress)
        self._gconsole.Bind(EVT_CMD_OUTPUT, self._onCmdOutput)
        self._gconsole.Bind(EVT_WRITE_LOG,
                            lambda event: self.WriteLog(text=event.text))
        self._gconsole.Bind(EVT_WRITE_CMD_LOG,
                            lambda event: self.WriteCmdLog(line=event.line))
        self._gconsole.Bind(EVT_WRITE_WARNING,
                            lambda event: self.WriteWarning(line=event.line))
        self._gconsole.Bind(EVT_WRITE_ERROR,
                            lambda event: self.WriteError(line=event.line))

    def _onCmdOutput(self, event):
        """!Print command output"""
        message = event.text
        style = event.type

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

    def WriteLog(self, text, wrap=None,
                 switchPage=False, priority=1):
        self._write(grass.message, text)

    def WriteCmdLog(self, line, pid=None, switchPage=True):
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
        raise NotImplementedError()
