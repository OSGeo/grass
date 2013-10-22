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
from core.utils import _

import grass.script as grass

from grass.pydispatch.signal import Signal

# to disable Abstract class not referenced
#pylint: disable=R0921


class Notification:
    """!Enum class for notifications suggestions.

    Can be used for log messages, commands, warnings, errors.
    The value is the suggestion how user should be notified
    about the new message.
    """
    NO_NOTIFICATION = 0
    HIGHLIGHT = 1
    MAKE_VISIBLE = 2
    RAISE_WINDOW = 3


class Layer(object):
    """!Layer is generaly usable layer object.

    @note Currently without specifying the interface.
    Current implementations only provides all attributes of existing layer
    as used in lmgr.
    """
    pass


class LayerList(object):
    def GetSelectedLayers(self, checkedOnly=True):
        """!Returns list of selected layers.

        @note Usage of checked and selected is still subject to change.
        Checked layers should be showed. Selected are for analyses.
        However, this may be the same for some implementations
        (e.g. it d.mon has all layers checked and selected).
        """
        raise NotImplementedError()

    def GetSelectedLayer(self, checkedOnly=False):
        """!Returns selected layer or None when there is no selected layer.

        @note Parameter checkedOnly is here False by default. This might change
        if we find the right way of handling unchecked layers.
        """
        raise NotImplementedError()

    def AddLayer(self, ltype, name=None, checked=None,
                 opacity=1.0, cmd=None):
        """!Adds a new layer to the layer list.

        Launches property dialog if needed (raster, vector, etc.)

        @param ltype layer type (raster, vector, 3d-raster, ...)
        @param name layer name
        @param checked if True layer is checked
        @param opacity layer opacity level
        @param cmd command (given as a list)
        """
        raise NotImplementedError()

    def GetLayersByName(self, name):
        """!Returns list of layers with a given name.

        @param name fully qualified map name

        @todo if common usage is just to check the presence of layer,
        intoroduce a new method ContainsLayerByName(name)
        """
        raise NotImplementedError()

    def GetLayerByData(self, key, value):
        """!Returns layer with specified.

        @note Returns only one layer. This might change.

        @warning Avoid using this method, it might be removed in the future.
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

    def WriteLog(self, text, wrap=None, notification=Notification.HIGHLIGHT):
        """!Writes log message.
        """
        raise NotImplementedError()

    def WriteCmdLog(self, text, pid=None, notification=Notification.MAKE_VISIBLE):
        """!Writes message related to start or end of the command.
        """
        raise NotImplementedError()

    def WriteWarning(self, text):
        """!Writes warning message for the user.
        """
        raise NotImplementedError()

    def WriteError(self, text):
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

        # workaround, standalone grass interface should be moved to sep. file
        from core.gconsole import GConsole, \
            EVT_CMD_OUTPUT, EVT_CMD_PROGRESS

        self._gconsole = GConsole()
        self._gconsole.Bind(EVT_CMD_PROGRESS, self._onCmdProgress)
        self._gconsole.Bind(EVT_CMD_OUTPUT, self._onCmdOutput)
        self._gconsole.writeLog.connect(self.WriteLog)
        self._gconsole.writeCmdLog.connect(self.WriteCmdLog)
        self._gconsole.writeWarning.connect(self.WriteWarning)
        self._gconsole.writeError.connect(self.WriteError)

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

    def RunCmd(self, command, compReg=True, skipInterface=False,
               onDone=None, onPrepare=None, userData=None, notification=Notification.MAKE_VISIBLE):
        self._gconsole.RunCmd(command=command, compReg=compReg,
                              skipInterface=skipInterface, onDone=onDone,
                              onPrepare=onPrepare, userData=userData, notification=notification)

    def Help(self, entry):
        self._gconsole.RunCmd(['g.manual', 'entry=%s' % entry])

    def WriteLog(self, text, wrap=None,
                 notification=Notification.HIGHLIGHT):
        self._write(grass.message, text)

    def WriteCmdLog(self, text, pid=None, notification=Notification.MAKE_VISIBLE):
        if pid:
            text = '(' + str(pid) + ') ' + text
        self._write(grass.message, text)

    def WriteWarning(self, text):
        self._write(grass.warning, text)

    def WriteError(self, text):
        self._write(grass.error, text)

    def _write(self, function, text):
        orig = os.getenv("GRASS_MESSAGE_FORMAT")
        os.environ["GRASS_MESSAGE_FORMAT"] = 'standard'
        function(text)
        os.environ["GRASS_MESSAGE_FORMAT"] = orig

    def GetLayerList(self):
        raise NotImplementedError()

    def GetMapDisplay(self):
        """!Get current map display.
        """
        return None

    def GetAllMapDisplays(self):
        """!Get list of all map displays.
        """
        return []

    def GetMapWindow(self):
        raise NotImplementedError()

    def GetProgress(self):
        # TODO: implement some progress with same inface as gui one
        # (probably using g.message or similarly to Write... functions)
        raise NotImplementedError()
