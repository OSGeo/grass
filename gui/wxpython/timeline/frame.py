"""!
@package frame

@brief Timeline Tool

Classes:
 - frame::DataCursor
 - frame::TimelineFrame
 - frame::LookUp

(C) 2012-13 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com>
"""
import os
import sys
import wx
from math import ceil
from itertools import cycle
import numpy as np

try:
    import matplotlib
    # The recommended way to use wx with mpl is with the WXAgg
    # backend.
    matplotlib.use('WXAgg')
    from matplotlib.figure import Figure
    import matplotlib.pyplot as plt
    from matplotlib.backends.backend_wxagg import \
        FigureCanvasWxAgg as FigCanvas, \
        NavigationToolbar2WxAgg as NavigationToolbar
    import matplotlib.dates as mdates
    from matplotlib import cbook
except ImportError:
    raise ImportError(_('The Timeline Tool needs "Matplotlib" package to be installed.'))

if __name__ == '__main__':
    sys.path.append(os.path.join(os.environ['GISBASE'], "etc", "gui", "wxpython"))

import grass.script as grass
from core.utils import _

import grass.temporal as tgis
from core.gcmd import GError, GException, RunCommand
from gui_core import gselect
from core import globalvar

ALPHA = 0.5
COLORS = ['b', 'g', 'r', 'c', 'm', 'y', 'k']


def check_version(*version):
    """!Checks if given version or newer is installed"""
    versionInstalled = []
    for i in matplotlib.__version__.split('.'):
        try:
            v = int(i)
            versionInstalled.append(v)
        except ValueError:
            versionInstalled.append(0)
    if versionInstalled < list(version):
        return False
    else:
        return True


class TimelineFrame(wx.Frame):
    """!The main frame of the application"""
    def __init__(self, parent):
        wx.Frame.__init__(self, parent, id=wx.ID_ANY, title=_("Timeline Tool"))

        tgis.init()
        self.datasets = []
        self.timeData = {}
        self._layout()
        self.temporalType = None
        self.unit = None

    def _layout(self):
        """!Creates the main panel with all the controls on it:
             * mpl canvas
             * mpl navigation toolbar
             * Control panel for interaction
        """
        self.panel = wx.Panel(self)

        # Create the mpl Figure and FigCanvas objects.
        # 5x4 inches, 100 dots-per-inch
        #
        # color =  wx.SystemSettings.GetColour(wx.SYS_COLOUR_BACKGROUND)
        self.fig = Figure((5.0, 4.0), facecolor=(1, 1, 1))
        self.canvas = FigCanvas(self.panel, wx.ID_ANY, self.fig)
        # axes are initialized later
        self.axes2d = None
        self.axes3d = None

        # Create the navigation toolbar, tied to the canvas
        #
        self.toolbar = NavigationToolbar(self.canvas)

        #
        # Layout
        #

        self.vbox = wx.BoxSizer(wx.VERTICAL)
        self.vbox.Add(self.canvas, 1, wx.LEFT | wx.TOP | wx.EXPAND)
        self.vbox.Add(self.toolbar, 0, wx.EXPAND)
        self.vbox.AddSpacer(10)

        gridSizer = wx.GridBagSizer(hgap=5, vgap=5)

        self.datasetSelect = gselect.Select(parent=self.panel, id=wx.ID_ANY,
                                            size=globalvar.DIALOG_GSELECT_SIZE,
                                            type='stds', multiple=True)
        self.drawButton = wx.Button(self.panel, id=wx.ID_ANY, label=_("Draw"))
        self.drawButton.Bind(wx.EVT_BUTTON, self.OnRedraw)
        self.helpButton = wx.Button(self.panel, id=wx.ID_ANY, label=_("Help"))
        self.helpButton.Bind(wx.EVT_BUTTON, self.OnHelp)
        self.view3dCheck = wx.CheckBox(self.panel, id=wx.ID_ANY,
                                       label=_("3D plot of spatio-temporal extents"))
        self.view3dCheck.Bind(wx.EVT_CHECKBOX, self.OnRedraw)
        if not check_version(1, 0, 0):
            self.view3dCheck.SetLabel(_("3D plot of spatio-temporal extents "
                                        "(matplotlib >= 1.0.0)"))
            self.view3dCheck.Disable()

        gridSizer.Add(wx.StaticText(self.panel, id=wx.ID_ANY,
                                    label=_("Select space time dataset(s):")),
                      pos=(0, 0), flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(self.datasetSelect, pos=(1, 0), flag=wx.EXPAND)
        gridSizer.Add(self.drawButton, pos=(1, 1), flag=wx.EXPAND)
        gridSizer.Add(self.helpButton, pos=(1, 2), flag=wx.EXPAND)
        gridSizer.Add(self.view3dCheck, pos=(2, 0), flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)

        self.vbox.Add(gridSizer, proportion=0, flag=wx.EXPAND | wx.ALL, border=10)

        self.panel.SetSizer(self.vbox)
        self.vbox.Fit(self)

    def _getData(self, timeseries):
        """!Load data and read properties"""
        self.timeData = {}
        mode = None
        unit = None
        for series in timeseries:
            name = series[0] + '@' + series[1]
            etype = series[2]
            sp = tgis.dataset_factory(etype, name)
            if not sp.is_in_db():
                GError(self, message=_("Dataset <%s> not found in temporal database") % (name))
                return

            sp.select()

            self.timeData[name] = {}
            self.timeData[name]['elementType'] = series[2]
            self.timeData[name]['temporalType'] = sp.get_temporal_type()  # abs/rel

            if mode is None:
                mode = self.timeData[name]['temporalType']
            elif self.timeData[name]['temporalType'] != mode:
                GError(parent=self, message=_("Datasets have different temporal type "
                                              "(absolute x relative), which is not allowed."))
                return

            # check topology
            maps = sp.get_registered_maps_as_objects()
            self.timeData[name]['validTopology'] = sp.check_temporal_topology(maps)

            self.timeData[name]['temporalMapType'] = sp.get_map_time()  # point/interval
            self.timeData[name]['unit'] = None  # only with relative
            if self.timeData[name]['temporalType'] == 'relative':
                start, end, self.timeData[name]['unit'] = sp.get_relative_time()
                if unit is None:
                    unit = self.timeData[name]['unit']
                elif self.timeData[name]['unit'] != unit:
                    GError(self, _("Datasets have different time unit which is not allowed."))
                    return

            self.timeData[name]['start_datetime'] = []
            # self.timeData[name]['start_plot'] = []
            self.timeData[name]['end_datetime'] = []
            # self.timeData[name]['end_plot'] = []
            self.timeData[name]['names'] = []
            self.timeData[name]['north'] = []
            self.timeData[name]['south'] = []
            self.timeData[name]['west'] = []
            self.timeData[name]['east'] = []

            columns = ','.join(['name', 'start_time', 'end_time',
                                'north', 'south', 'west', 'east'])

            rows = sp.get_registered_maps(columns=columns, where=None,
                                          order='start_time', dbif=None)
            if rows is None:
                rows = []
            for row in rows:
                mapName, start, end, north, south, west, east = row
                self.timeData[name]['start_datetime'].append(start)
                self.timeData[name]['end_datetime'].append(end)
                self.timeData[name]['names'].append(mapName)
                self.timeData[name]['north'].append(north)
                self.timeData[name]['south'].append(south)
                self.timeData[name]['west'].append(west)
                self.timeData[name]['east'].append(east)

        self.temporalType = mode
        self.unit = unit

    def _draw3dFigure(self):
        """!Draws 3d view (spatio-temporal extents).


        Only for matplotlib versions >= 1.0.0.
        Earlier versions cannot draw time ticks and alpha
        and it has a slightly different API.
        """
        self.axes3d.clear()
        self.axes3d.grid(False)
        # self.axes3d.grid(True)
        if self.temporalType == 'absolute':
            if check_version(1, 1, 0):
                self.axes3d.zaxis_date()
            convert = mdates.date2num
        else:
            convert = lambda x: x

        colors = cycle(COLORS)
        plots = []
        for name in self.datasets:
            name = name[0] + '@' + name[1]
            startZ = convert(self.timeData[name]['start_datetime'])
            mapType = self.timeData[name]['temporalMapType']
            if mapType == 'interval':
                dZ = convert(self.timeData[name]['end_datetime']) - startZ

            else:
                dZ = [0] * len(startZ)

            startX = self.timeData[name]['west']
            dX = self.timeData[name]['east'] - np.array(startX)
            startY = self.timeData[name]['south']
            dY = self.timeData[name]['north'] - np.array(startY)

            color = colors.next()
            plots.append(self.axes3d.bar3d(startX, startY, startZ, dX, dY, dZ,
                                           color=color, alpha=ALPHA))

        params = grass.read_command('g.proj', flags='g')
        params = grass.parse_key_val(params)
        if 'unit' in params:
            self.axes3d.set_xlabel(_("X [%s]") % params['unit'])
            self.axes3d.set_ylabel(_("Y [%s]") % params['unit'])
        else:
            self.axes3d.set_xlabel(_("X"))
            self.axes3d.set_ylabel(_("Y"))

        self.axes3d.set_zlabel(_('Time'))
        self.axes3d.mouse_init()
        self.canvas.draw()

    def _draw2dFigure(self):
        """!Draws 2D plot (temporal extents)"""
        self.axes2d.clear()
        self.axes2d.grid(True)
        if self.temporalType == 'absolute':
            self.axes2d.xaxis_date()
            self.fig.autofmt_xdate()
            convert = mdates.date2num
        else:
            convert = lambda x: x

        colors = cycle(COLORS)

        yticksNames = []
        yticksPos = []
        plots = []
        lookUp = LookUp(self.timeData)
        for i, name in enumerate(self.datasets):
            name = name[0] + '@' + name[1]
            yticksNames.append(name)
            yticksPos.append(i)
            barData = []
            pointData = []
            mapType = self.timeData[name]['temporalMapType']

            start = convert(self.timeData[name]['start_datetime'])
            # TODO: mixed
            if mapType == 'interval':
                end = convert(self.timeData[name]['end_datetime'])
                lookUpData = zip(start, end)
                duration = end - np.array(start)
                barData = zip(start, duration)
                lookUp.AddDataset(type_='bar', yrange=(i - 0.1, i + 0.1),
                                  xranges=lookUpData, datasetName=name)

            else:
                # self.timeData[name]['end_plot'] = None
                pointData = start
                lookUp.AddDataset(type_='point', yrange=i, xranges=pointData, datasetName=name)
            color = colors.next()
            if mapType == 'interval':
                plots.append(self.axes2d.broken_barh(xranges=barData, yrange=(i - 0.1, 0.2),
                                                     facecolors=color, alpha=ALPHA))
            else:
                plots.append(self.axes2d.plot(pointData, [i] * len(pointData),
                                              marker='o', linestyle='None', color=color)[0])

        if self.temporalType == 'absolute':
            pass
            # self.axes2d.set_xlabel(_("Time"))
        else:
            self.axes2d.set_xlabel(_("Time [%s]") % self.unit)

        self.axes2d.set_yticks(yticksPos)
        self.axes2d.set_yticklabels(yticksNames)
        self.axes2d.set_ylim(min(yticksPos) - 1, max(yticksPos) + 1)

        # adjust xlim
        xlim = self.axes2d.get_xlim()
        padding = ceil((xlim[1] - xlim[0]) / 20.)
        self.axes2d.set_xlim(xlim[0] - padding, xlim[1] + padding)

        self.canvas.draw()
        DataCursor(plots, lookUp, InfoFormat)

    def OnRedraw(self, event):
        """!Required redrawing."""
        datasets = self.datasetSelect.GetValue().strip().split(',')
        if not datasets:
            return
        try:
            datasets = self._checkDatasets(datasets)
        except GException:
            GError(parent=self, message=_("Invalid input data"))
            return

        self.datasets = datasets
        self._redraw()

    def _redraw(self):
        """!Readraw data.

        Decides if to draw also 3D and adjusts layout if needed.
        """
        self._getData(self.datasets)

        # axes3d are physically removed
        if not self.axes2d:
            self.axes2d = self.fig.add_subplot(1, 1, 1)
        self._draw2dFigure()
        if check_version(1, 0, 0):
            if self.view3dCheck.IsChecked():
                self.axes2d.change_geometry(2, 1, 1)
                if not self.axes3d:
                    # do not remove this import - unused but it is required for 3D
#                    from mpl_toolkits.mplot3d import Axes3D  # pylint: disable=W0611
                    self.axes3d = self.fig.add_subplot(2, 1, 2, projection='3d')

                self.axes3d.set_visible(True)
                self._draw3dFigure()
            else:
                if self.axes3d:
                    self.fig.delaxes(self.axes3d)
                    self.axes3d = None
                self.axes2d.change_geometry(1, 1, 1)
                self.canvas.draw()

        if check_version(1, 1, 0):
            # not working, maybe someone is more lucky
            try:
                plt.tight_layout()
            except:
                pass

    def _checkDatasets(self, datasets):
        """!Checks and validates datasets.

        Reports also type of dataset (e.g. 'strds').

        @return (mapName, mapset, type)
        """
        validated = []
        tDict = tgis.tlist_grouped('stds', group_type=True)
        # nested list with '(map, mapset, etype)' items
        allDatasets = [[[(map, mapset, etype) for map in maps]
                     for etype, maps in etypesDict.iteritems()]
                    for mapset, etypesDict in tDict.iteritems()]
        # flatten this list
        allDatasets = reduce(lambda x, y: x + y, reduce(lambda x, y: x + y, allDatasets))

        for dataset in datasets:
            errorMsg = _("Space time dataset <%s> not found.") % dataset
            if dataset.find("@") >= 0:
                nameShort, mapset = dataset.split('@', 1)
                indices = [n for n, (mapName, mapsetName, etype) in enumerate(allDatasets)
                           if nameShort == mapName and mapsetName == mapset]
            else:
                indices = [n for n, (mapName, mapset, etype) in enumerate(allDatasets)
                           if dataset == mapName]

            if len(indices) == 0:
                raise GException(errorMsg)
            elif len(indices) >= 2:
                dlg = wx.SingleChoiceDialog(self,
                         message=_("Please specify the space time dataset <%s>." % dataset),
                         caption=_("Ambiguous dataset name"),
                         choices=[("%(map)s@%(mapset)s: %(etype)s" % {'map': allDatasets[i][0],
                                                                      'mapset': allDatasets[i][1],
                                                                      'etype': allDatasets[i][2]})
                                                                               for i in indices],
                         style=wx.CHOICEDLG_STYLE | wx.OK)
                if dlg.ShowModal() == wx.ID_OK:
                    index = dlg.GetSelection()
                    validated.append(allDatasets[indices[index]])
            else:
                validated.append(allDatasets[indices[0]])

        return validated

    def OnHelp(self, event):
        RunCommand('g.manual', quiet=True, entry='g.gui.timeline')

#  interface

    def SetDatasets(self, datasets):
        """!Set data"""
        if not datasets:
            return
        try:
            datasets = self._checkDatasets(datasets)
        except GException:
            GError(parent=self, message=_("Invalid input data"))
            return
        self.datasets = datasets
        self.datasetSelect.SetValue(','.join(map(lambda x: x[0] + '@' + x[1], datasets)))
        self._redraw()

    def Show3D(self, show):
        """!Show also 3D if possible"""
        if check_version(1, 0, 0):
            self.view3dCheck.SetValue(show)


class LookUp:
    """!Helper class for searching info by coordinates"""
    def __init__(self, timeData):
        self.data = {}
        self.timeData = timeData

    def AddDataset(self, type_, yrange, xranges, datasetName):
        if type_ == 'bar':
            self.data[yrange] = {'name': datasetName}
            for i, (start, end) in enumerate(xranges):
                self.data[yrange][(start, end)] = i
        elif type_ == 'point':
            self.data[(yrange, yrange)] = {'name': datasetName}
            for i, start in enumerate(xranges):
                self.data[(yrange, yrange)][(start, start)] = i

    def GetInformation(self, x, y):
        keys = None
        for keyY in self.data.keys():
            if keyY[0] <= y <= keyY[1]:
                for keyX in self.data[keyY].keys():
                    if keyX != 'name' and  keyX[0] <= x <= keyX[1]:
                        keys = keyY, keyX
                        break
                if keys:
                    break
        if not keys:
            return None

        datasetName = self.data[keys[0]]['name']
        mapIndex = self.data[keys[0]][keys[1]]
        return  self.timeData, datasetName, mapIndex


def InfoFormat(timeData, datasetName, mapIndex):
    """!Formats information about dataset"""
    text = []
    etype = timeData[datasetName]['elementType']
    if etype == 'strds':
        text.append(_("Space time raster dataset: %s") % datasetName)
    elif etype == 'stvds':
        text.append(_("Space time vector dataset: %s") % datasetName)
    elif etype == 'str3ds':
        text.append(_("Space time 3D raster dataset: %s") % datasetName)

    text.append(_("Map name: %s") % timeData[datasetName]['names'][mapIndex])
    text.append(_("Start time: %s") % timeData[datasetName]['start_datetime'][mapIndex])
    text.append(_("End time: %s") % timeData[datasetName]['end_datetime'][mapIndex])

    if not timeData[datasetName]['validTopology']:
        text.append(_("WARNING: invalid topology"))

    text.append(_("\nPress Del to dismiss."))

    return '\n'.join(text)


class DataCursor(object):
    """A simple data cursor widget that displays the x,y location of a
    matplotlib artist when it is selected.


    Source: http://stackoverflow.com/questions/4652439/
            is-there-a-matplotlib-equivalent-of-matlabs-datacursormode/4674445
    """
    def __init__(self, artists, lookUp, formatFunction, tolerance=5, offsets=(-30, 30),
                 display_all=False):
        """Create the data cursor and connect it to the relevant figure.
        "artists" is the matplotlib artist or sequence of artists that will be
            selected.
        "tolerance" is the radius (in points) that the mouse click must be
            within to select the artist.
        "offsets" is a tuple of (x,y) offsets in points from the selected
            point to the displayed annotation box
        "display_all" controls whether more than one annotation box will
            be shown if there are multiple axes.  Only one will be shown
            per-axis, regardless.
        """
        self.lookUp = lookUp
        self.formatFunction = formatFunction
        self.offsets = offsets
        self.display_all = display_all
        if not cbook.iterable(artists):
            artists = [artists]
        self.artists = artists

        self.axes = tuple(set(art.axes for art in self.artists))
        self.figures = tuple(set(ax.figure for ax in self.axes))

        self.annotations = {}
        for ax in self.axes:
            self.annotations[ax] = self.annotate(ax)
        for artist in self.artists:
            artist.set_picker(tolerance)
        for fig in self.figures:
            fig.canvas.mpl_connect('pick_event', self)
            fig.canvas.mpl_connect('key_press_event', self.keyPressed)

    def keyPressed(self, event):
        """!Key pressed - hide annotation if Delete was pressed"""
        if event.key != 'delete':
            return
        for ax in self.axes:
            self.annotations[ax].set_visible(False)
            event.canvas.draw()

    def annotate(self, ax):
        """Draws and hides the annotation box for the given axis "ax"."""
        annotation = ax.annotate(self.formatFunction, xy=(0, 0), ha='center',
                xytext=self.offsets, textcoords='offset points', va='bottom',
                bbox=dict(boxstyle='round,pad=0.5', fc='yellow', alpha=0.7),
                arrowprops=dict(arrowstyle='->', connectionstyle='arc3,rad=0'),
                annotation_clip=False, multialignment='left')
        annotation.set_visible(False)

        return annotation

    def __call__(self, event):
        """Intended to be called through "mpl_connect"."""
        # Rather than trying to interpolate, just display the clicked coords
        # This will only be called if it's within "tolerance", anyway.
        x, y = event.mouseevent.xdata, event.mouseevent.ydata
        annotation = self.annotations[event.artist.axes]
        if x is not None:
            if not self.display_all:
                # Hide any other annotation boxes...
                for ann in self.annotations.values():
                    ann.set_visible(False)
            # Update the annotation in the current axis..
            annotation.xy = x, y

            if 'Line2D' in str(type(event.artist)):
                y = event.artist.get_ydata()[0]
                xData = event.artist.get_xdata()
                x = xData[np.argmin(abs(xData - x))]

            info = self.lookUp.GetInformation(x, y)
            if not info:
                return
            text = self.formatFunction(*info)
            annotation.set_text(text)
            annotation.set_visible(True)
            event.canvas.draw()


def run(parent=None, datasets=None):
    frame = TimelineFrame(parent)
    if datasets:
        frame.SetDatasets(datasets)
    frame.Show()


if __name__ == '__main__':
    run()
