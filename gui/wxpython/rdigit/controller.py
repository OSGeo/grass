# -*- coding: utf-8 -*-
"""
@package rdigit.controller

@brief rdigit controller for drawing and rasterizing

Classes:
 - controller::RDigitController

(C) 2014 by the GRASS Development Team
This program is free software under the GNU General Public
License (>=v2). Read the file COPYING that comes with GRASS
for details.

@author Anna Petrasova <kratochanna gmail.com>
"""
import os
import tempfile
import wx
import uuid
from wx.lib.newevent import NewEvent

from grass.script import core as gcore
from grass.script import raster as grast
from grass.exceptions import CalledModuleError, ScriptError
from grass.pydispatch.signal import Signal

from core.gcmd import GError, GMessage
from core.settings import UserSettings
from core.gthread import gThread
from rdigit.dialogs import NewRasterDialog

updateProgress, EVT_UPDATE_PROGRESS = NewEvent()


class RDigitController(wx.EvtHandler):
    """Controller object for raster digitizer.
    Inherits from EvtHandler to be able to send wx events from thraed.
    """

    def __init__(self, giface, mapWindow):
        """Constructs controller

        :param giface: grass interface object
        :param mapWindow: instance of BufferedMapWindow
        """
        wx.EvtHandler.__init__(self)
        self._giface = giface
        self._mapWindow = mapWindow

        # thread for running rasterization process
        self._thread = gThread()
        # name of raster map which is edited (also new one)
        self._editedRaster = None
        # name of optional background raster
        self._backgroundRaster = None
        # name of temporary raster used to backup original state
        self._backupRasterName = None
        # if we edit an old raster or a new one (important for setting color
        # table)
        self._editOldRaster = False
        # type of output raster map (CELL, FCELL, DCELL)
        self._mapType = None
        # GraphicsSet for drawing areas, lines, points
        self._areas = None
        self._lines = None
        self._points = None
        # list of all GraphicsItems in the order of drawing
        self._all = []
        # if in state of drawing lin or area
        self._drawing = False
        # if running digitizing process in thread (to block drawing)
        self._running = False
        # color used to draw (should be moved to settings)
        self._drawColor = wx.GREEN
        # transparency used to draw (should be moved to settings)
        self._drawTransparency = 100
        # current selected drawing method
        self._graphicsType = 'area'
        # last edited cell value
        self._currentCellValue = None
        # last edited buffer value
        self._currentWidthValue = None
        # digit env
        self._env = os.environ.copy()

        self._oldMouseUse = None
        self._oldCursor = None

        # signal to add new raster to toolbar items
        self.newRasterCreated = Signal('RDigitController:newRasterCreated')
        # signal to add just used cell value in toolbar combo
        self.newFeatureCreated = Signal('RDigitController:newFeatureCreated')
        # signal to upload unique categories of background map into toolbar
        # combo
        self.uploadMapCategories = Signal(
            'RDigitController:uploadMapCategories')
        self.quitDigitizer = Signal('RDigitController:quitDigitizer')
        self.showNotification = Signal('RDigitController:showNotification')

    def _connectAll(self):
        self._mapWindow.mouseLeftDown.connect(self._start)
        self._mapWindow.mouseLeftUp.connect(self._addPoint)
        self._mapWindow.mouseRightUp.connect(self._finish)
        self._mapWindow.Unbind(wx.EVT_CONTEXT_MENU)

    def _disconnectAll(self):
        self._mapWindow.mouseLeftDown.disconnect(self._start)
        self._mapWindow.mouseLeftUp.disconnect(self._addPoint)
        self._mapWindow.mouseRightUp.disconnect(self._finish)
        self._mapWindow.Bind(
            wx.EVT_CONTEXT_MENU,
            self._mapWindow.OnContextMenu)

    def _start(self, x, y):
        """Start digitizing a new object.
        :param x: x coordinate in map units
        :param y: y coordinate in map units
        """
        if self._running:
            return

        if not self._editedRaster:
            GMessage(parent=self._mapWindow, message=_(
                "Please select first the raster map"))
            return
        if not self._drawing:
            if self._graphicsType == 'area':
                item = self._areas.AddItem(coords=[])
                item.SetPropertyVal('penName', 'pen1')
                self._all.append(item)
            elif self._graphicsType == 'line':
                item = self._lines.AddItem(coords=[])
                item.SetPropertyVal('penName', 'pen1')
                self._all.append(item)
            elif self._graphicsType == 'point':
                item = self._points.AddItem(coords=[])
                item.SetPropertyVal('penName', 'pen1')
                self._all.append(item)
            self._drawing = True

    def _addPoint(self, x, y):
        """Add point to an object.
        :param x: x coordinate in map units
        :param y: y coordinate in map units
        """
        if self._running:
            return

        if not self._drawing:
            return

        if self._graphicsType == 'area':
            area = self._areas.GetItem(-1)
            coords = area.GetCoords() + [[x, y]]
            area.SetCoords(coords)
            self.showNotification.emit(text=_("Right click to finish area"))
        elif self._graphicsType == 'line':
            line = self._lines.GetItem(-1)
            coords = line.GetCoords() + [[x, y]]
            line.SetCoords(coords)
            self.showNotification.emit(text=_("Right click to finish line"))
        elif self._graphicsType == 'point':
            point = self._points.GetItem(-1)
            point.SetCoords([x, y])
            self._finish()
        # draw
        self._mapWindow.ClearLines()
        self._lines.Draw()
        self._areas.Draw()
        self._points.Draw()
        self._mapWindow.Refresh()

    def _finish(self):
        """Finish digitizing a new object and redraws.
        Saves current cell value and buffer width for that object.

        :param x: x coordinate in map units
        :param y: y coordinate in map units
        """
        if self._running:
            return

        if self._graphicsType == 'point':
            item = self._points.GetItem(-1)
        elif self._graphicsType == 'area':
            item = self._areas.GetItem(-1)
        elif self._graphicsType == 'line':
            item = self._lines.GetItem(-1)
        else:
            return

        self._drawing = False
        item.SetPropertyVal('brushName', 'done')
        item.AddProperty('cellValue')
        item.AddProperty('widthValue')
        item.SetPropertyVal('cellValue', self._currentCellValue)
        item.SetPropertyVal('widthValue', self._currentWidthValue)
        self.newFeatureCreated.emit()

        self._mapWindow.ClearLines()
        self._points.Draw()
        self._areas.Draw()
        self._lines.Draw()

        self._mapWindow.Refresh()

    def SelectType(self, drawingType):
        """Selects method (area/line/point) for drawing.
        Connects and disconnects signal to allow other tools
        in map toolbar to work.
        """
        if self._graphicsType and drawingType and self._graphicsType != drawingType \
                and self._drawing:
            # if we select different drawing tool, finish the feature
            self._finish()

        if self._graphicsType and not drawingType:
            self._mapWindow.ClearLines(pdc=self._mapWindow.pdcTmp)
            self._mapWindow.mouse['end'] = self._mapWindow.mouse['begin']
            # disconnect mouse events
            self._disconnectAll()
            self._mapWindow.SetNamedCursor(self._oldCursor)
            self._mapWindow.mouse['use'] = self._oldMouseUse
        elif self._graphicsType is None and drawingType:
            self._connectAll()
            # change mouse['box'] and pen to draw line during dragging
            # TODO: better solution for drawing this line
            self._mapWindow.mouse['use'] = None
            self._mapWindow.mouse['box'] = "line"
            self._mapWindow.pen = wx.Pen(
                colour='red', width=2, style=wx.SHORT_DASH)
            # change the cursor
            self._mapWindow.SetNamedCursor('pencil')

        self._graphicsType = drawingType

    def SetCellValue(self, value):
        self._currentCellValue = value

    def SetWidthValue(self, value):
        self._currentWidthValue = value

    def ChangeDrawColor(self, color):
        self._drawColor = color[:3] + (self._drawTransparency,)
        for each in (self._areas, self._lines, self._points):
            each.GetPen('pen1').SetColour(self._drawColor)
            each.GetBrush('done').SetColour(self._drawColor)
        self._mapWindow.UpdateMap(render=False)

    def Start(self):
        """Registers graphics to map window,
        connect required mouse signals.
        """
        self._oldMouseUse = self._mapWindow.mouse['use']
        self._oldCursor = self._mapWindow.GetNamedCursor()

        self._connectAll()

        # change mouse['box'] and pen to draw line during dragging
        # TODO: better solution for drawing this line
        self._mapWindow.mouse['use'] = None
        self._mapWindow.mouse['box'] = "line"
        self._mapWindow.pen = wx.Pen(
            colour='red', width=2, style=wx.SHORT_DASH)

        color = self._drawColor[:3] + (self._drawTransparency,)
        self._areas = self._mapWindow.RegisterGraphicsToDraw(
            graphicsType='polygon', pdc=self._mapWindow.pdcTransparent, mapCoords=True)
        self._areas.AddPen(
            'pen1',
            wx.Pen(
                colour=color,
                width=2,
                style=wx.SOLID))
        self._areas.AddBrush('done', wx.Brush(colour=color, style=wx.SOLID))

        self._lines = self._mapWindow.RegisterGraphicsToDraw(
            graphicsType='line', pdc=self._mapWindow.pdcTransparent, mapCoords=True)
        self._lines.AddPen(
            'pen1',
            wx.Pen(
                colour=color,
                width=2,
                style=wx.SOLID))
        self._lines.AddBrush('done', wx.Brush(colour=color, style=wx.SOLID))

        self._points = self._mapWindow.RegisterGraphicsToDraw(
            graphicsType='point', pdc=self._mapWindow.pdcTransparent, mapCoords=True)
        self._points.AddPen(
            'pen1',
            wx.Pen(
                colour=color,
                width=2,
                style=wx.SOLID))
        self._points.AddBrush('done', wx.Brush(colour=color, style=wx.SOLID))

        # change the cursor
        self._mapWindow.SetNamedCursor('pencil')

    def Stop(self):
        """Before stopping digitizer, asks to save edits"""
        if self._editedRaster:
            dlg = wx.MessageDialog(
                self._mapWindow,
                _("Do you want to save changes?"),
                _("Save raster map changes"),
                wx.YES_NO)
            if dlg.ShowModal() == wx.ID_YES:
                if self._drawing:
                    self._finish()
                self._thread.Run(callable=self._exportRaster,
                                 ondone=lambda event: self._updateAndQuit())
            else:
                self.quitDigitizer.emit()
        else:
            self.quitDigitizer.emit()

    def Save(self):
        """Saves current edits to a raster map"""
        if self._drawing:
            self._finish()

        self._thread.Run(callable=self._exportRaster,
                         ondone=lambda event: self._update())

    def Undo(self):
        """Undo a change, goes object back (finished or not finished)"""
        if len(self._all):
            removed = self._all.pop(-1)
            # try to remove from each, it fails quietly when theitem is not
            # there
            self._areas.DeleteItem(removed)
            self._lines.DeleteItem(removed)
            self._points.DeleteItem(removed)
            self._drawing = False
            self._mapWindow.UpdateMap(render=False)

    def CleanUp(self, restore=True):
        """Cleans up drawing, temporary maps.
        :param restore: if restore previous cursor, mouse['use']
        """
        try:
            if self._backupRasterName:
                gcore.run_command(
                    'g.remove',
                    type='raster',
                    flags='f',
                    name=self._backupRasterName,
                    quiet=True)
        except CalledModuleError:
            pass

        self._mapWindow.ClearLines(pdc=self._mapWindow.pdcTmp)
        self._mapWindow.mouse['end'] = self._mapWindow.mouse['begin']
        # disconnect mouse events
        if self._graphicsType:
            self._disconnectAll()
        # unregister
        self._mapWindow.UnregisterGraphicsToDraw(self._areas)
        self._mapWindow.UnregisterGraphicsToDraw(self._lines)
        self._mapWindow.UnregisterGraphicsToDraw(self._points)
        #self._registeredGraphics = None
        self._mapWindow.UpdateMap(render=False)

        if restore:
            # restore mouse['use'] and cursor to the state before measuring
            # starts
            self._mapWindow.SetNamedCursor(self._oldCursor)
            self._mapWindow.mouse['use'] = self._oldMouseUse

    def _updateAndQuit(self):
        """Called when thread is done. Updates map and calls to quits digitizer."""
        self._running = False
        self._mapWindow.UpdateMap(render=True)
        self.quitDigitizer.emit()

    def _update(self):
        """Called when thread is done. Updates map."""
        self._running = False
        self._mapWindow.UpdateMap(render=True)

    def SelectOldMap(self, name):
        """After selecting old raster, creates a backup copy for editing."""
        try:
            self._backupRaster(name)
        except ScriptError:
            GError(parent=self._mapWindow, message=_(
                "Failed to create backup copy of edited raster map."))
            return False
        self._editedRaster = name
        self._mapType = grast.raster_info(map=name)['datatype']
        self._editOldRaster = True
        return True

    def SelectNewMap(
            self, standalone=False, mapName=None, bgMap=None,
            mapType=None,

    ):
        """After selecting new raster, shows dialog to choose name,
        background map and type of the new map.

        :params standalone, mapName, bgMap, mapType: if digitizer is
        launched as standalone module

        :param bool standalone: if digitizer is launched as standalone
        module
        :param str mapName: edited raster map name
        :param str bgMap: background raster map name
        :param str mapType: raster map type CELL, FCELL, DCELL
        """
        if standalone:
            try:
                self._createNewMap(
                    mapName=mapName, backgroundMap=bgMap,
                    mapType=mapType,
                )
            except ScriptError:
                GError(
                    parent=self._mapWindow, message=_(
                        "Failed to create new raster map."
                    ),
                )
                return False
            return True
        else:
            dlg = NewRasterDialog(parent=self._mapWindow)
            dlg.CenterOnParent()
            if dlg.ShowModal() == wx.ID_OK:
                try:
                    self._createNewMap(
                        mapName=dlg.GetMapName(),
                        backgroundMap=dlg.GetBackgroundMapName(),
                        mapType=dlg.GetMapType(),
                    )
                except ScriptError:
                    GError(
                        parent=self._mapWindow, message=_(
                            "Failed to create new raster map."
                        ),
                    )
                    return False
                finally:
                    dlg.Destroy()
                return True
            else:
                dlg.Destroy()
                return False

    def _createNewMap(self, mapName, backgroundMap, mapType):
        """Creates a new raster map based on specified background and type."""
        name = mapName.split('@')[0]
        background = backgroundMap.split('@')[0]
        types = {'CELL': 'int', 'FCELL': 'float', 'DCELL': 'double'}
        if background:
            back = background
        else:
            back = 'null()'
        try:
            grast.mapcalc(
                exp="{name} = {mtype}({back})".format(
                    name=name,
                    mtype=types[mapType],
                    back=back),
                overwrite=True,
                quiet=True)
            if background:
                self._backgroundRaster = backgroundMap
                gcore.run_command(
                    'r.colors',
                    map=name,
                    raster=self._backgroundRaster,
                    quiet=True)
                if mapType == 'CELL':
                    values = gcore.read_command('r.describe', flags='1n',
                                                map=name, quiet=True).strip()
                    if values:
                        self.uploadMapCategories.emit(
                            values=values.split('\n'))
        except CalledModuleError:
            raise ScriptError
        self._backupRaster(name)

        name = name + '@' + gcore.gisenv()['MAPSET']
        self._editedRaster = name
        self._mapType = mapType
        self.newRasterCreated.emit(name=name)
        gisenv = gcore.gisenv()
        self._giface.grassdbChanged.emit(grassdb=gisenv['GISDBASE'],
                                         location=gisenv['LOCATION_NAME'],
                                         mapset=gisenv['MAPSET'],
                                         action='new',
                                         map=name.split('@')[0],
                                         element='raster')

    def _backupRaster(self, name):
        """Creates a temporary backup raster necessary for undo behavior.

        :param str name: name of raster map for which we create backup
        """
        name = name.split('@')[0]
        backup = name + '_backupcopy_' + str(os.getpid())
        try:
            gcore.run_command('g.copy', raster=[name, backup], quiet=True)
        except CalledModuleError:
            raise ScriptError

        self._backupRasterName = backup

    def _exportRaster(self):
        """Rasterizes digitized features.

        Uses r.in.poly and r.grow for buffering features. Creates separate raster
        maps depending on common cell values and buffering width necessary to
        keep the order of editing. These rasters are then patched together.
        Sets default color table for the newly digitized raster.
        """
        self._setRegion()

        if not self._editedRaster or self._running:
            return
        self._running = True

        if len(self._all) < 1:
            new = self._editedRaster
            if '@' in self._editedRaster:
                new = self._editedRaster.split('@')[0]
            gcore.run_command('g.copy', raster=[self._backupRasterName, new],
                              overwrite=True, quiet=True)
        else:
            tempRaster = 'tmp_rdigit_rast_' + str(os.getpid())
            text = []
            rastersToPatch = []
            i = 0
            lastCellValue = lastWidthValue = None
            evt = updateProgress(
                range=len(self._all),
                value=0, text=_("Rasterizing..."))
            wx.PostEvent(self, evt)
            lastCellValue = self._all[0].GetPropertyVal('cellValue')
            lastWidthValue = self._all[0].GetPropertyVal('widthValue')
            for item in self._all:
                if item.GetPropertyVal('widthValue') and \
                    (lastCellValue != item.GetPropertyVal('cellValue') or
                     lastWidthValue != item.GetPropertyVal('widthValue')):
                    if text:
                        out = self._rasterize(
                            text, lastWidthValue, self._mapType, tempRaster)
                        rastersToPatch.append(out)
                        text = []
                    self._writeItem(item, text)
                    out = self._rasterize(
                        text, item.GetPropertyVal('widthValue'),
                        self._mapType, tempRaster)
                    rastersToPatch.append(out)
                    text = []
                else:
                    self._writeItem(item, text)

                lastCellValue = item.GetPropertyVal('cellValue')
                lastWidthValue = item.GetPropertyVal('widthValue')

                i += 1
                evt = updateProgress(
                    range=len(self._all),
                    value=i, text=_("Rasterizing..."))
                wx.PostEvent(self, evt)
            if text:
                out = self._rasterize(text, item.GetPropertyVal('widthValue'),
                                      self._mapType, tempRaster)
                rastersToPatch.append(out)

            gcore.run_command(
                'r.patch', input=rastersToPatch[:: -1] +
                [self._backupRasterName],
                output=self._editedRaster, overwrite=True, quiet=True,
                env=self._env)
            gcore.run_command(
                'g.remove',
                type='raster',
                flags='f',
                name=rastersToPatch +
                [tempRaster],
                quiet=True)
        try:
            # setting the right color table
            if self._editOldRaster:
                return
            if not self._backgroundRaster:
                table = UserSettings.Get(
                    group='rasterLayer',
                    key='colorTable',
                    subkey='selection')
                if not table:
                    table = 'rainbow'
                gcore.run_command(
                    'r.colors',
                    color=table,
                    map=self._editedRaster,
                    quiet=True)
            else:
                gcore.run_command('r.colors', map=self._editedRaster,
                                  raster=self._backgroundRaster, quiet=True)
        except CalledModuleError:
            self._running = False
            GError(parent=self._mapWindow, message=_(
                "Failed to set default color table for edited raster map"))

    def _writeFeature(self, item, vtype, text):
        """Writes digitized features in r.in.poly format."""
        coords = item.GetCoords()
        if vtype == 'P':
            coords = [coords]
        cellValue = item.GetPropertyVal('cellValue')
        record = '{vtype}\n'.format(vtype=vtype)
        for coord in coords:
            record += ' '.join([str(c) for c in coord])
            record += '\n'
        record += '= {cellValue}\n'.format(cellValue=cellValue)

        text.append(record)

    def _writeItem(self, item, text):
        if item in self._areas.GetAllItems():
            self._writeFeature(item, vtype='A', text=text)
        elif item in self._lines.GetAllItems():
            self._writeFeature(item, vtype='L', text=text)
        elif item in self._points.GetAllItems():
            self._writeFeature(item, vtype='P', text=text)

    def _rasterize(self, text, bufferDist, mapType, tempRaster):
        """Performs the actual rasterization using r.in.poly
        and buffering with r.grow if required.

        :param str text: string in r.in.poly format
        :param float bufferDist: buffer distance in map units
        :param str mapType: CELL, FCELL, DCELL
        :param str tempRaster: name of temporary raster used in computation

        :return: output raster map name as a result of digitization
        """
        output = 'x' + str(uuid.uuid4())[:8]
        asciiFile = tempfile.NamedTemporaryFile(mode='w', delete=False)
        asciiFile.write('\n'.join(text))
        asciiFile.close()

        if bufferDist:
            bufferDist /= 2.
            gcore.run_command(
                'r.in.poly',
                input=asciiFile.name,
                output=tempRaster,
                type_=mapType,
                overwrite=True,
                quiet=True)
            gcore.run_command('r.grow', input=tempRaster, output=output,
                              flags='m', radius=bufferDist, quiet=True,
                              env=self._env)
        else:
            gcore.run_command('r.in.poly', input=asciiFile.name, output=output,
                              type_=mapType, quiet=True, env=self._env)
        os.unlink(asciiFile.name)
        return output

    def _setRegion(self):
        """Set region according input raster map"""
        self._env['GRASS_REGION'] = gcore.region_env(
            raster=self._backupRasterName)
