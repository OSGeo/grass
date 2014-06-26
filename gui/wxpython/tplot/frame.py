"""
@package frame

@brief Temporal Plot Tool

Classes:
 - frame::DataCursor
 - frame::TplotFrame
 - frame::LookUp

(C) 2012-2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Luca Delucchi
"""
from itertools import cycle
import numpy as np

import wx

try:
    import matplotlib
    # The recommended way to use wx with mpl is with the WXAgg
    # backend.
    matplotlib.use('WXAgg')
    from matplotlib.figure import Figure
    from matplotlib.backends.backend_wxagg import \
        FigureCanvasWxAgg as FigCanvas, \
        NavigationToolbar2WxAgg as NavigationToolbar
    import matplotlib.dates as mdates
    from matplotlib import cbook
except ImportError:
    raise ImportError(_('The Temporal Plot Tool needs the "matplotlib" '
                        '(python-matplotlib) package to be installed.'))

from core.utils import _

import grass.temporal as tgis
from core.gcmd import GError, GException, RunCommand
from gui_core import gselect
from grass.pygrass.vector.geometry import Point
from grass.pygrass.raster import RasterRow
from collections import OrderedDict
ALPHA = 0.5
COLORS = ['b', 'g', 'r', 'c', 'm', 'y', 'k']


def check_version(*version):
    """Checks if given version or newer is installed"""
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


class TplotFrame(wx.Frame):
    """The main frame of the application"""
    def __init__(self, parent):
        wx.Frame.__init__(self, parent, id=wx.ID_ANY,
                          title=_("GRASS GIS Temporal Plot Tool"))

        tgis.init(True)
        self.datasets = []
        self.output = None
        self.timeData = {}
        self._layout()
        self.temporalType = None
        self.unit = None
        # We create a database interface here to speedup the GUI
        self.dbif = tgis.SQLDatabaseInterfaceConnection()
        self.dbif.connect()

    def __del__(self):
        """Close the database interface and stop the messenger and C-interface
           subprocesses.
        """
        if self.dbif.connected is True:
            self.dbif.close()
        tgis.stop_subprocesses()

    def _layout(self):
        """Creates the main panel with all the controls on it:
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
                                            type='strds', multiple=True, size=(150, -1))
        self.drawButton = wx.Button(self.panel, id=wx.ID_ANY, label=_("Draw"))
        self.drawButton.Bind(wx.EVT_BUTTON, self.OnRedraw)
        self.helpButton = wx.Button(self.panel, id=wx.ID_ANY, label=_("Help"))
        self.helpButton.Bind(wx.EVT_BUTTON, self.OnHelp)

        self.xcoor = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                   label=_('Insert longitude (x) coordinate'))

        self.xcoorval = wx.TextCtrl(parent=self.panel, id=wx.ID_ANY,
                                    size=(150, -1))
        self.ycoor = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                   label=_('Insert latitude (y) coordinate'))

        self.ycoorval = wx.TextCtrl(parent=self.panel, id=wx.ID_ANY,
                                    size=(150, -1))

        gridSizer.Add(wx.StaticText(self.panel, id=wx.ID_ANY,
                                    label=_("Select space time dataset(s):")),
                      pos=(0, 0), flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(self.datasetSelect, pos=(1, 0), flag=wx.EXPAND)

        gridSizer.Add(self.xcoor, pos=(2, 0), flag=wx.EXPAND)
        gridSizer.Add(self.ycoor, pos=(2, 1), flag=wx.EXPAND)

        gridSizer.Add(self.xcoorval, pos=(3, 0), flag=wx.EXPAND)
        gridSizer.Add(self.ycoorval, pos=(3, 1), flag=wx.EXPAND)
        gridSizer.Add(self.drawButton, pos=(3, 2), flag=wx.EXPAND)
        gridSizer.Add(self.helpButton, pos=(3, 3), flag=wx.EXPAND)

        self.vbox.Add(gridSizer, proportion=0, flag=wx.EXPAND | wx.ALL,
                      border=10)

        self.panel.SetSizer(self.vbox)
        self.vbox.Fit(self)

    def _getData(self, timeseries):
        """Load data and read properties

        :param list timeseries: a list of timeseries
        """
        self.timeData = OrderedDict()
        mode = None
        unit = None
        columns = ','.join(['name', 'start_time', 'end_time'])
        for series in timeseries:
            name = series[0]
            fullname = name + '@' + series[1]
            etype = series[2]
            sp = tgis.dataset_factory(etype, fullname)
            sp.select(dbif=self.dbif)

            self.timeData[name] = OrderedDict()
            if not sp.is_in_db(dbif=self.dbif):
                GError(self, message=_("Dataset <%s> not found in temporal "
                                       "database") % (fullname))
                return

            self.timeData[name]['temporalDataType'] = etype
            self.timeData[name]['temporalType'] = sp.get_temporal_type()
            self.timeData[name]['granularity'] = sp.get_granularity()
            if mode is None:
                mode = self.timeData[name]['temporalType']
            elif self.timeData[name]['temporalType'] != mode:
                GError(parent=self, message=_("Datasets have different temporal"
                                              " type (absolute x relative), "
                                              "which is not allowed."))
                return

            # check topology
            maps = sp.get_registered_maps_as_objects(dbif=self.dbif)
            self.timeData[name]['validTopology'] = sp.check_temporal_topology(maps=maps, dbif=self.dbif)

            self.timeData[name]['unit'] = None  # only with relative
            if self.timeData[name]['temporalType'] == 'relative':
                start, end, self.timeData[name]['unit'] = sp.get_relative_time()
                if unit is None:
                    unit = self.timeData[name]['unit']
                elif self.timeData[name]['unit'] != unit:
                    GError(self, _("Datasets have different time unit which "
                                   "is not allowed."))
                    return

            rows = sp.get_registered_maps(columns=columns, where=None,
                                          order='start_time', dbif=self.dbif)
            for row in rows:
                self.timeData[name][row[0]] = {}
                self.timeData[name][row[0]]['start_datetime'] = row[1]
                self.timeData[name][row[0]]['end_datetime'] = row[2]
                r = RasterRow(row[0])
                r.open()
                val = r.get_value(self.poi)
                r.close()
                self.timeData[name][row[0]]['value'] = val
        self.unit = unit
        self.temporalType = mode
        return

    def _drawFigure(self):
        """Draws or print 2D plot (temporal extents)"""
        self.axes2d.clear()
        self.axes2d.grid(False)
        if self.temporalType == 'absolute':
            self.axes2d.xaxis_date()
            self.fig.autofmt_xdate()
            self.convert = mdates.date2num
            self.invconvert = mdates.num2date
        else:
            self.convert = lambda x: x
            self.invconvert = self.convert

        colors = cycle(COLORS)

        yticksNames = []
        yticksPos = []
        plots = []
        lookUp = LookUp(self.timeData, self.invconvert)
        for i, name in enumerate(self.datasets):
            name = name[0]
            yticksNames.append(name)  # just name; with mapset it would be long
            yticksPos.append(i)
            xdata = []
            ydata = []
            for keys, values in self.timeData[name].iteritems():
                if keys in ['temporalType', 'granularity', 'validTopology',
                            'unit', 'temporalDataType']:
                    continue
                xdata.append(self.convert(values['start_datetime']))
                ydata.append(values['value'])

            lookUp.AddDataset(yranges=ydata, xranges=xdata, datasetName=name)
            color = colors.next()
            plots.append(self.axes2d.plot(xdata, ydata, marker='o',
                                          color=color, label=name)[0])

        if self.temporalType == 'absolute':
            self.axes2d.set_xlabel(_("Temporal resolution: %s" % self.timeData[name]['granularity']))
        else:
            self.axes2d.set_xlabel(_("Time [%s]") % self.unit)
        self.axes2d.set_ylabel(', '.join(yticksNames))

        #legend
        handles, labels = self.axes2d.get_legend_handles_labels()
        self.axes2d.legend(loc=0)
        if self.output:
            self.canvas.print_figure(filename=self.output, dpi=self.dpi)
        else:
            self.canvas.draw()
            DataCursor(plots, lookUp, InfoFormat, self.convert)

    def OnRedraw(self, event):
        """Required redrawing."""
        datasets = self.datasetSelect.GetValue().strip()
        if not datasets:
            return
        datasets = datasets.split(',')
        try:
            datasets = self._checkDatasets(datasets)
            if not datasets:
                return
        except GException:
            GError(parent=self, message=_("Invalid input data"))
            return

        self.datasets = datasets
        coors = [self.xcoorval.GetValue().strip(),
                 self.ycoorval.GetValue().strip()]
        if coors:
            try:
                self.poi = Point(float(coors[0]), float(coors[1]))
            except GException:
                GError(parent=self, message=_("Invalid input coordinates"))
                return
        self._redraw()

    def _redraw(self):
        """Readraw data.

        Decides if to draw also 3D and adjusts layout if needed.
        """
        self._getData(self.datasets)
        # axes3d are physically removed
        if not self.axes2d:
            self.axes2d = self.fig.add_subplot(1, 1, 1)
        self._drawFigure()

    def _checkDatasets(self, datasets):
        """Checks and validates datasets.

        Reports also type of dataset (e.g. 'strds').

        :param list datasets: list of temporal dataset's name
        :return: (mapName, mapset, type)
        """
        validated = []
        tDict = tgis.tlist_grouped('stds', group_type=True, dbif=self.dbif)
        # nested list with '(map, mapset, etype)' items
        allDatasets = [[[(map, mapset, etype) for map in maps]
                        for etype, maps in etypesDict.iteritems()]
                       for mapset, etypesDict in tDict.iteritems()]
        # flatten this list
        if allDatasets:
            allDatasets = reduce(lambda x, y: x + y, reduce(lambda x, y: x + y,
                                                            allDatasets))
            mapsets = tgis.get_tgis_c_library_interface().available_mapsets()
            allDatasets = [i for i in sorted(allDatasets,
                                             key=lambda l: mapsets.index(l[1]))]

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
                                            message=_("Please specify the "
                                                      "space time dataset "
                                                      "<%s>." % dataset),
                                            caption=_("Ambiguous dataset name"),
                                            choices=[("%(map)s@%(mapset)s:"
                                                      " %(etype)s" % {'map': allDatasets[i][0],
                                                                      'mapset': allDatasets[i][1],
                                                                      'etype': allDatasets[i][2]})
                                                     for i in indices],
                                            style=wx.CHOICEDLG_STYLE | wx.OK)
                if dlg.ShowModal() == wx.ID_OK:
                    index = dlg.GetSelection()
                    validated.append(allDatasets[indices[index]])
                else:
                    continue
            else:
                validated.append(allDatasets[indices[0]])

        return validated

    def OnHelp(self, event):
        """Function to show help"""
        RunCommand('g.manual', quiet=True, entry='g.gui.tplot')

    def SetDatasets(self, datasets, coors, output, dpi):
        """Set the data

        :param list datasets: a list of temporal dataset's name
        :param list coors: a list with x/y coordinates
        :param str output: the name of output png file
        :param int dpi: the dpi value for png file
        """
        if not datasets or not coors:
            return
        try:
            datasets = self._checkDatasets(datasets)
            if not datasets:
                return
        except GException:
            GError(parent=self, message=_("Invalid input temporal dataset"))
            return
        try:
            self.poi = Point(float(coors[0]), float(coors[1]))
        except GException:
            GError(parent=self, message=_("Invalid input coordinates"))
            return
        self.datasets = datasets
        self.output = output
        self.dpi = dpi
        self.datasetSelect.SetValue(','.join(map(lambda x: x[0] + '@' + x[1],
                                                 datasets)))
        self.xcoorval.SetValue(str(coors[0]))
        self.ycoorval.SetValue(str(coors[1]))
        self._redraw()


class LookUp:
    """Helper class for searching info by coordinates"""
    def __init__(self, timeData, convert):
        self.data = {}
        self.timeData = timeData
        self.convert = convert

    def AddDataset(self, yranges, xranges, datasetName):
        if len(yranges) != len(xranges):
            GError(parent=self, message=_("Datasets have different number of"
                                          "values"))
        self.data[datasetName] = {}
        for i in range(len(xranges)):
            self.data[datasetName][xranges[i]] = yranges[i]

    def GetInformation(self, x):
        values = {}
        for key, value in self.data.iteritems():
            if value[x]:
                values[key] = [self.convert(x), value[x]]

        if len(values) == 0:
            return None

        return self.timeData, values


def InfoFormat(timeData, values):
    """Formats information about dataset"""
    text = []
    for key, val in values.iteritems():
        etype = timeData[key]['temporalDataType']
        if etype == 'strds':
            text.append(_("Space time raster dataset: %s") % key)
        elif etype == 'stvds':
            text.append(_("Space time vector dataset: %s") % key)
        elif etype == 'str3ds':
            text.append(_("Space time 3D raster dataset: %s") % key)

        text.append(_("Value for {date} is {val}".format(date=val[0], val=val[1])))
        text.append('\n')
    text.append(_("Press Del to dismiss."))

    return '\n'.join(text)


class DataCursor(object):
    """A simple data cursor widget that displays the x,y location of a
    matplotlib artist when it is selected.


    Source: http://stackoverflow.com/questions/4652439/
            is-there-a-matplotlib-equivalent-of-matlabs-datacursormode/4674445
    """
    def __init__(self, artists, lookUp, formatFunction, convert,
                 tolerance=5, offsets=(-30, 20), display_all=False):
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
        self.convert = convert
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
        """Key pressed - hide annotation if Delete was pressed"""
        if event.key != 'delete':
            return
        for ax in self.axes:
            self.annotations[ax].set_visible(False)
            event.canvas.draw()

    def annotate(self, ax):
        """Draws and hides the annotation box for the given axis "ax"."""
        annotation = ax.annotate(self.formatFunction, xy=(0, 0), ha='center',
                                 xytext=self.offsets, va='bottom',
                                 textcoords='offset points',
                                 bbox=dict(boxstyle='round,pad=0.5',
                                           fc='yellow', alpha=0.7),
                                 arrowprops=dict(arrowstyle='->',
                                                 connectionstyle='arc3,rad=0'),
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
            if 'Line2D' in str(type(event.artist)):
                xData = []
                for a in event.artist.get_xdata():
                    try:
                        d = self.convert(a)
                    except:
                        d = a
                    xData.append(d)
                x = xData[np.argmin(abs(xData - x))]

            info = self.lookUp.GetInformation(x)
            ys = zip(*info[1].values())[1]
            if not info:
                return
            # Update the annotation in the current axis..
            annotation.xy = x, max(ys)
            text = self.formatFunction(*info)
            annotation.set_text(text)
            annotation.set_visible(True)
            event.canvas.draw()


def run(parent=None, datasets=None):
    frame = TplotFrame(parent)
    if datasets:
        frame.SetDatasets(datasets)
    frame.Show()


if __name__ == '__main__':
    run()
