#!/usr/bin/env python3

"""
@package frame

@brief Temporal Plot Tool

Classes:
 - frame::DataCursor
 - frame::TplotFrame
 - frame::LookUp

(C) 2012-2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Luca Delucchi
@author start stvds support Matej Krejci
"""
import os
import six
from itertools import cycle
import numpy as np

import wx
from grass.pygrass.modules import Module

import grass.script as grass
from functools import reduce

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
except ImportError as e:
    raise ImportError(_('The Temporal Plot Tool needs the "matplotlib" '
                        '(python-matplotlib) package to be installed. {0}').format(e))


import grass.temporal as tgis
from core.gcmd import GMessage, GError, GException, RunCommand
from gui_core.widgets import CoordinatesValidator
from gui_core import gselect
from core import globalvar
from grass.pygrass.vector.geometry import Point
from grass.pygrass.raster import RasterRow
from grass.pygrass.gis.region import Region
from collections import OrderedDict
from subprocess import PIPE
try:
    import wx.lib.agw.flatnotebook as FN
except ImportError:
    import wx.lib.flatnotebook as FN
import wx.lib.filebrowsebutton as filebrowse

from gui_core.widgets import GNotebook
from gui_core.wrap import TextCtrl, Button, StaticText

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


def findBetween(s, first, last):
    try:
        start = s.rindex(first) + len(first)
        end = s.rindex(last, start)
        return s[start:end]
    except ValueError:
        return ""


class TplotFrame(wx.Frame):
    """The main frame of the application"""

    def __init__(self, parent, giface):
        wx.Frame.__init__(self, parent, id=wx.ID_ANY,
                          title=_("GRASS GIS Temporal Plot Tool"))

        tgis.init(True)
        self._giface = giface
        self.datasetsV = None
        self.datasetsR = None
        self.overwrite = False
        # self.vectorDraw=False
        # self.rasterDraw=False
        self.init()
        self._layout()

        # We create a database interface here to speedup the GUI
        self.dbif = tgis.SQLDatabaseInterfaceConnection()
        self.dbif.connect()
        self.Bind(wx.EVT_CLOSE, self.onClose)
        self.region = Region()

    def init(self):
        self.timeDataR = OrderedDict()
        self.timeDataV = OrderedDict()
        self.temporalType = None
        self.unit = None
        self.listWhereConditions = []
        self.plotNameListR = []
        self.plotNameListV = []
        self.poi = None
        self.csvpath = None

    def __del__(self):
        """Close the database interface and stop the messenger and C-interface
           subprocesses.
        """
        if self.dbif.connected is True:
            self.dbif.close()
        tgis.stop_subprocesses()

    def onClose(self, evt):
        if self._giface.GetMapDisplay():
            self.coorval.OnClose()
            self.cats.OnClose()
        self.__del__()
        self.Destroy()

    def _layout(self):
        """Creates the main panel with all the controls on it:
             * mpl canvas
             * mpl navigation toolbar
             * Control panel for interaction
        """
        self.mainPanel = wx.Panel(self)
        # Create the mpl Figure and FigCanvas objects.
        # 5x4 inches, 100 dots-per-inch
        #
        # color =  wx.SystemSettings.GetColour(wx.SYS_COLOUR_BACKGROUND)
        # ------------CANVAS AND TOOLBAR------------
        self.fig = Figure((5.0, 4.0), facecolor=(1, 1, 1))
        self.canvas = FigCanvas(self.mainPanel, wx.ID_ANY, self.fig)
        # axes are initialized later
        self.axes2d = None

        # Create the navigation toolbar, tied to the canvas
        #
        self.toolbar = NavigationToolbar(self.canvas)

        #
        # Layout
        #
        # ------------MAIN VERTICAL SIZER------------
        self.vbox = wx.BoxSizer(wx.VERTICAL)
        self.vbox.Add(self.canvas, 1, wx.LEFT | wx.TOP | wx.EXPAND)
        self.vbox.Add(self.toolbar, 0, wx.EXPAND)
        # self.vbox.AddSpacer(10)

        # ------------ADD NOTEBOOK------------
        self.ntb = GNotebook(parent=self.mainPanel, style=FN.FNB_FANCY_TABS)

        # ------------ITEMS IN NOTEBOOK PAGE (RASTER)------------------------

        self.controlPanelRaster = wx.Panel(parent=self.ntb, id=wx.ID_ANY)
        self.datasetSelectLabelR = StaticText(
            parent=self.controlPanelRaster,
            id=wx.ID_ANY,
            label=_(
                'Raster temporal '
                'dataset (strds)\n'
                'Press ENTER after'
                ' typing the name or select'
                ' with the combobox'))
        self.datasetSelectR = gselect.Select(
            parent=self.controlPanelRaster, id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE, type='strds', multiple=True)
        self.coor = StaticText(parent=self.controlPanelRaster, id=wx.ID_ANY,
                               label=_('X and Y coordinates separated by '
                                       'comma:'))
        try:
            self._giface.GetMapWindow()
            self.coorval = gselect.CoordinatesSelect(
                parent=self.controlPanelRaster, giface=self._giface)
        except:
            self.coorval = TextCtrl(parent=self.controlPanelRaster,
                                       id=wx.ID_ANY,
                                       size=globalvar.DIALOG_TEXTCTRL_SIZE,
                                       validator=CoordinatesValidator())

        self.coorval.SetToolTip(_("Coordinates can be obtained for example"
                                  " by right-clicking on Map Display."))
        self.controlPanelSizerRaster = wx.BoxSizer(wx.VERTICAL)
        # self.controlPanelSizer.Add(wx.StaticText(self.panel, id=wx.ID_ANY,
        # label=_("Select space time raster dataset(s):")),
        # pos=(0, 0), flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
        self.controlPanelSizerRaster.Add(self.datasetSelectLabelR,
                                         flag=wx.EXPAND)
        self.controlPanelSizerRaster.Add(self.datasetSelectR, flag=wx.EXPAND)

        self.controlPanelSizerRaster.Add(self.coor, flag=wx.EXPAND)
        self.controlPanelSizerRaster.Add(self.coorval, flag=wx.EXPAND)

        self.controlPanelRaster.SetSizer(self.controlPanelSizerRaster)
        self.controlPanelSizerRaster.Fit(self)
        self.ntb.AddPage(page=self.controlPanelRaster, text=_('STRDS'),
                         name='STRDS')

        # ------------ITEMS IN NOTEBOOK PAGE (VECTOR)------------------------
        self.controlPanelVector = wx.Panel(parent=self.ntb, id=wx.ID_ANY)
        self.datasetSelectLabelV = StaticText(
            parent=self.controlPanelVector, id=wx.ID_ANY,
            label=_(
                'Vector temporal '
                'dataset (stvds)\n'
                'Press ENTER after'
                ' typing the name or select'
                ' with the combobox'))
        self.datasetSelectV = gselect.Select(
            parent=self.controlPanelVector, id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE, type='stvds', multiple=True)
        self.datasetSelectV.Bind(wx.EVT_TEXT,
                                 self.OnVectorSelected)

        self.attribute = gselect.ColumnSelect(parent=self.controlPanelVector)
        self.attributeLabel = StaticText(parent=self.controlPanelVector,
                                         id=wx.ID_ANY,
                                         label=_('Select attribute column'))
        # TODO fix the category selection as done for coordinates
        try:
            self._giface.GetMapWindow()
            self.cats = gselect.VectorCategorySelect(
                parent=self.controlPanelVector, giface=self._giface)
        except:
            self.cats = TextCtrl(
                parent=self.controlPanelVector,
                id=wx.ID_ANY,
                size=globalvar.DIALOG_TEXTCTRL_SIZE)
        self.catsLabel = StaticText(parent=self.controlPanelVector,
                                    id=wx.ID_ANY,
                                    label=_('Select category of vector(s)'))

        self.controlPanelSizerVector = wx.BoxSizer(wx.VERTICAL)
        # self.controlPanelSizer.Add(wx.StaticText(self.panel, id=wx.ID_ANY,
        # label=_("Select space time raster dataset(s):")),
        # pos=(0, 0), flag=wx.EXPAND | wx.ALIGN_CENTER_VERTICAL)
        self.controlPanelSizerVector.Add(self.datasetSelectLabelV,
                                         flag=wx.EXPAND)
        self.controlPanelSizerVector.Add(self.datasetSelectV, flag=wx.EXPAND)

        self.controlPanelSizerVector.Add(self.attributeLabel, flag=wx.EXPAND)
        self.controlPanelSizerVector.Add(self.attribute, flag=wx.EXPAND)

        self.controlPanelSizerVector.Add(self.catsLabel, flag=wx.EXPAND)
        self.controlPanelSizerVector.Add(self.cats, flag=wx.EXPAND)

        self.controlPanelVector.SetSizer(self.controlPanelSizerVector)
        self.controlPanelSizerVector.Fit(self)
        self.ntb.AddPage(page=self.controlPanelVector, text=_('STVDS'),
                         name='STVDS')
        
        # ------------ITEMS IN NOTEBOOK PAGE (LABELS)------------------------
        self.controlPanelLabels = wx.Panel(parent=self.ntb, id=wx.ID_ANY)
        self.titleLabel = StaticText(parent=self.controlPanelLabels,
                                     id=wx.ID_ANY,
                                     label=_('Set title for the plot'))
        self.title = TextCtrl(parent=self.controlPanelLabels, id=wx.ID_ANY,
                              size=globalvar.DIALOG_TEXTCTRL_SIZE)
        self.xLabel = StaticText(parent=self.controlPanelLabels,
                                 id=wx.ID_ANY,
                                 label=_('Set label for X axis'))
        self.x = TextCtrl(parent=self.controlPanelLabels, id=wx.ID_ANY,
                          size=globalvar.DIALOG_TEXTCTRL_SIZE)
        self.yLabel = StaticText(parent=self.controlPanelLabels,
                                 id=wx.ID_ANY,
                                 label=_('Set label for Y axis'))
        self.y = TextCtrl(parent=self.controlPanelLabels, id=wx.ID_ANY,
                          size=globalvar.DIALOG_TEXTCTRL_SIZE)
        self.controlPanelSizerLabels = wx.BoxSizer(wx.VERTICAL)
        self.controlPanelSizerLabels.Add(self.titleLabel, flag=wx.EXPAND)
        self.controlPanelSizerLabels.Add(self.title, flag=wx.EXPAND)
        self.controlPanelSizerLabels.Add(self.xLabel, flag=wx.EXPAND)
        self.controlPanelSizerLabels.Add(self.x, flag=wx.EXPAND)
        self.controlPanelSizerLabels.Add(self.yLabel, flag=wx.EXPAND)
        self.controlPanelSizerLabels.Add(self.y, flag=wx.EXPAND)
        self.controlPanelLabels.SetSizer(self.controlPanelSizerLabels)
        self.controlPanelSizerLabels.Fit(self)
        self.ntb.AddPage(page=self.controlPanelLabels, text=_('Labels'),
                         name='Labels')

        # ------------ITEMS IN NOTEBOOK PAGE (EXPORT)------------------------
        self.controlPanelExport = wx.Panel(parent=self.ntb, id=wx.ID_ANY)
        self.csvLabel = StaticText(parent=self.controlPanelExport,
                                   id=wx.ID_ANY,
                                   label=_('Path for output CSV file '
                                           'with plotted data'))
        self.csvButton = filebrowse.FileBrowseButton(parent=self.controlPanelExport,
                                                     id=wx.ID_ANY,
                                                     size=globalvar.DIALOG_GSELECT_SIZE,
                                                     labelText='',
                                                     dialogTitle=_('CVS path'),
                                                     buttonText=_('Browse'),
                                                     startDirectory=os.getcwd(),
                                                     fileMode=wx.FD_SAVE)
        self.headerLabel = StaticText(parent=self.controlPanelExport,
                                      id=wx.ID_ANY,
                                      label=_('Do you want the CSV header?'))
        self.headerCheck = wx.CheckBox(parent=self.controlPanelExport,
                                         id=wx.ID_ANY)
        self.controlPanelSizerCheck = wx.BoxSizer(wx.HORIZONTAL)
        self.controlPanelSizerCheck.Add(self.headerCheck)
        self.controlPanelSizerCheck.Add(self.headerLabel)
        self.controlPanelSizerExport = wx.BoxSizer(wx.VERTICAL)
        self.controlPanelSizerExport.Add(self.csvLabel)
        self.controlPanelSizerExport.Add(self.csvButton)
        self.controlPanelSizerExport.Add(self.controlPanelSizerCheck)
        self.controlPanelExport.SetSizer(self.controlPanelSizerExport)
        self.controlPanelSizerCheck.Fit(self)
        self.controlPanelSizerExport.Fit(self)
        self.ntb.AddPage(page=self.controlPanelExport, text=_('Export'),
                         name='Export')

        # ------------Buttons on the bottom(draw,help)------------
        self.vButtPanel = wx.Panel(self.mainPanel, id=wx.ID_ANY)
        self.vButtSizer = wx.BoxSizer(wx.HORIZONTAL)

        self.drawButton = Button(self.vButtPanel, id=wx.ID_ANY,
                                 label=_("Draw"))
        self.drawButton.Bind(wx.EVT_BUTTON, self.OnRedraw)
        self.helpButton = Button(self.vButtPanel, id=wx.ID_ANY,
                                 label=_("Help"))
        self.helpButton.Bind(wx.EVT_BUTTON, self.OnHelp)
        self.vButtSizer.Add(self.drawButton)
        self.vButtSizer.Add(self.helpButton)
        self.vButtPanel.SetSizer(self.vButtSizer)

        self.mainPanel.SetSizer(self.vbox)
        self.vbox.Add(self.ntb, flag=wx.EXPAND)
        self.vbox.Add(self.vButtPanel, flag=wx.EXPAND)
        self.vbox.Fit(self)
        self.mainPanel.Fit()

    def _getSTRDdata(self, timeseries):
        """Load data and read properties
        :param list timeseries: a list of timeseries
        """
        if not self.poi:
            GError(parent=self, message=_("Invalid input coordinates"),
                   showTraceback=False)
            return
        mode = None
        unit = None
        columns = ','.join(['name', 'start_time', 'end_time'])
        for series in timeseries:
            name = series[0]
            fullname = name + '@' + series[1]
            etype = series[2]
            sp = tgis.dataset_factory(etype, fullname)
            if not sp.is_in_db(dbif=self.dbif):
                GError(message=_("Dataset <%s> not found in temporal "
                                 "database") % (fullname), parent=self)
                return
            sp.select(dbif=self.dbif)

            minmin = sp.metadata.get_min_min()
            self.plotNameListR.append(name)
            self.timeDataR[name] = OrderedDict()

            self.timeDataR[name]['temporalDataType'] = etype
            self.timeDataR[name]['temporalType'] = sp.get_temporal_type()
            self.timeDataR[name]['granularity'] = sp.get_granularity()

            if mode is None:
                mode = self.timeDataR[name]['temporalType']
            elif self.timeDataR[name]['temporalType'] != mode:
                GError(
                    parent=self, message=_(
                        "Datasets have different temporal"
                        " type (absolute x relative), "
                        "which is not allowed."))
                return

            # check topology
            maps = sp.get_registered_maps_as_objects(dbif=self.dbif)
            self.timeDataR[name]['validTopology'] = sp.check_temporal_topology(
                maps=maps, dbif=self.dbif)

            self.timeDataR[name]['unit'] = None  # only with relative
            if self.timeDataR[name]['temporalType'] == 'relative':
                start, end, self.timeDataR[name][
                    'unit'] = sp.get_relative_time()
                if unit is None:
                    unit = self.timeDataR[name]['unit']
                elif self.timeDataR[name]['unit'] != unit:
                    GError(parent=self, message=_("Datasets have different "
                                                  "time unit which is not "
                                                  "allowed."))
                    return

            rows = sp.get_registered_maps(columns=columns, where=None,
                                          order='start_time', dbif=self.dbif)
            for row in rows:
                self.timeDataR[name][row[0]] = {}
                self.timeDataR[name][row[0]]['start_datetime'] = row[1]
                self.timeDataR[name][row[0]]['end_datetime'] = row[2]
                r = RasterRow(row[0])
                r.open()
                val = r.get_value(self.poi)
                r.close()
                if val == -2147483648 and val < minmin:
                    self.timeDataR[name][row[0]]['value'] = None
                else:
                    self.timeDataR[name][row[0]]['value'] = val

        self.unit = unit
        self.temporalType = mode
        return

    def _parseVDbConn(self, mapp, layerInp):
        '''find attribute key according to layer of input map'''
        vdb = Module('v.db.connect', map=mapp, flags='g', stdout_=PIPE)

        vdb = vdb.outputs.stdout
        for line in vdb.splitlines():
            lsplit = line.split('|')
            layer = lsplit[0].split('/')[0]
            if str(layer) == str(layerInp):
                return lsplit[2]
        return None

    def _getExistingCategories(self, mapp, cats):
        """Get a list of categories for a vector map"""
        vdb = grass.read_command('v.category', input=mapp, option='print')
        categories = vdb.splitlines()
        if not cats:
            return categories
        for cat in cats:
            if str(cat) not in categories:
                GMessage(message=_("Category {ca} is not on vector map"
                                   " {ma} and it will be not used").format(ma=mapp,
                                                                           ca=cat),
                         parent=self)
                cats.remove(cat)
        return cats

    def _getSTVDData(self, timeseries):
        """Load data and read properties
        :param list timeseries: a list of timeseries
        """

        mode = None
        unit = None
        cats = None
        attribute = self.attribute.GetValue()
        if self.cats.GetValue() != '':
            cats = self.cats.GetValue().split(',')
        if cats and self.poi:
            GMessage(message=_("Both coordinates and categories are set, "
                               "coordinates will be used. The use categories "
                               "remove text from coordinate form"))
        if not attribute or attribute == '':
            GError(parent=self, showTraceback=False,
                   message=_("With Vector temporal dataset you have to select"
                             " an attribute column"))
            return
        columns = ','.join(['name', 'start_time', 'end_time', 'id', 'layer'])
        for series in timeseries:
            name = series[0]
            fullname = name + '@' + series[1]
            etype = series[2]
            sp = tgis.dataset_factory(etype, fullname)
            if not sp.is_in_db(dbif=self.dbif):
                GError(message=_("Dataset <%s> not found in temporal "
                                 "database") % (fullname), parent=self,
                       showTraceback=False)
                return
            sp.select(dbif=self.dbif)

            rows = sp.get_registered_maps(dbif=self.dbif, order="start_time",
                                          columns=columns, where=None)

            self.timeDataV[name] = OrderedDict()
            self.timeDataV[name]['temporalDataType'] = etype
            self.timeDataV[name]['temporalType'] = sp.get_temporal_type()
            self.timeDataV[name]['granularity'] = sp.get_granularity()

            if mode is None:
                mode = self.timeDataV[name]['temporalType']
            elif self.timeDataV[name]['temporalType'] != mode:
                GError(
                    parent=self, showTraceback=False, message=_(
                        "Datasets have different temporal type ("
                        "absolute x relative), which is not allowed."))
                return
            self.timeDataV[name]['unit'] = None  # only with relative
            if self.timeDataV[name]['temporalType'] == 'relative':
                start, end, self.timeDataV[name][
                    'unit'] = sp.get_relative_time()
                if unit is None:
                    unit = self.timeDataV[name]['unit']
                elif self.timeDataV[name]['unit'] != unit:
                    GError(message=_("Datasets have different time unit which"
                                     " is not allowed."), parent=self,
                           showTraceback=False)
                    return
            if self.poi:
                self.plotNameListV.append(name)
                # TODO set an appropriate distance, right now a big one is set
                # to return the closer point to the selected one
                out = grass.vector_what(map='pois_srvds',
                                        coord=self.poi.coords(),
                                        distance=10000000000000000)
                if len(out) != len(rows):
                    GError(parent=self, showTraceback=False,
                           message=_("Difference number of vector layers and "
                                     "maps in the vector temporal dataset"))
                    return
                for i in range(len(rows)):
                    row = rows[i]
                    values = out[i]
                    if str(row['layer']) == str(values['Layer']):
                        lay = "{map}_{layer}".format(map=row['name'],
                                                     layer=values['Layer'])
                        self.timeDataV[name][lay] = {}
                        self.timeDataV[name][lay][
                            'start_datetime'] = row['start_time']
                        self.timeDataV[name][lay][
                            'end_datetime'] = row['start_time']
                        self.timeDataV[name][lay]['value'] = values[
                            'Attributes'][attribute]
            else:
                wherequery = ''
                cats = self._getExistingCategories(rows[0]['name'], cats)
                totcat = len(cats)
                ncat = 1
                for cat in cats:
                    if ncat == 1 and totcat != 1:
                        wherequery += '{k}={c} or'.format(c=cat, k="{key}")
                    elif ncat == 1 and totcat == 1:
                        wherequery += '{k}={c}'.format(c=cat, k="{key}")
                    elif ncat == totcat:
                        wherequery += ' {k}={c}'.format(c=cat, k="{key}")
                    else:
                        wherequery += ' {k}={c} or'.format(c=cat, k="{key}")

                    catn = "cat{num}".format(num=cat)
                    self.plotNameListV.append("{na}+{cat}".format(na=name,
                                                                  cat=catn))
                    self.timeDataV[name][catn] = OrderedDict()
                    ncat += 1
                for row in rows:
                    lay = int(row['layer'])
                    catkey = self._parseVDbConn(row['name'], lay)
                    if not catkey:
                        GError(
                            parent=self, showTraceback=False, message=_(
                                "No connection between vector map {vmap} "
                                "and layer {la}".format(
                                    vmap=row['name'], la=lay)))
                        return
                    vals = grass.vector_db_select(
                        map=row['name'], layer=lay, where=wherequery.format(
                            key=catkey), columns=attribute)
                    layn = "lay{num}".format(num=lay)
                    for cat in cats:
                        catn = "cat{num}".format(num=cat)
                        if layn not in self.timeDataV[name][catn].keys():
                            self.timeDataV[name][catn][layn] = {}
                        self.timeDataV[name][catn][layn][
                            'start_datetime'] = row['start_time']
                        self.timeDataV[name][catn][layn][
                            'end_datetime'] = row['end_time']
                        self.timeDataV[name][catn][layn]['value'] = vals['values'][int(cat)][
                            0]
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

        self.colors = cycle(COLORS)

        self.yticksNames = []
        self.yticksPos = []
        self.plots = []
        self.drawTitle = self.title.GetValue()
        self.drawX = self.x.GetValue()
        self.drawY = self.y.GetValue()

        if self.datasetsR:
            self.lookUp = LookUp(self.timeDataR, self.invconvert)
        else:
            self.lookUp = LookUp(self.timeDataV, self.invconvert)

        if self.datasetsR:
            self.drawR()
        if self.datasetsV:
            if self.poi:
                self.drawV()
            elif self.cats:
                self.drawVCats()

        self.canvas.draw()
        DataCursor(self.plots, self.lookUp, InfoFormat, self.convert)

    def _setLabels(self, x):
        """Function to set the right labels"""
        if self.drawX != '':
            self.axes2d.set_xlabel(self.drawX)
        else:
            if self.temporalType == 'absolute':
                self.axes2d.set_xlabel(_("Temporal resolution: %s" %  x ))
            else:
                self.axes2d.set_xlabel(_("Time [%s]") % self.unit)
        if self.drawY != '':
            self.axes2d.set_ylabel(self.drawY)
        else:
            self.axes2d.set_ylabel(', '.join(self.yticksNames))
        if self.drawTitle != '':
            self.axes2d.set_title(self.drawTitle)

    def _writeCSV(self, x, y):
        """Used to write CSV file of plotted data"""
        import csv
        if isinstance(y[0], list):
            zipped = list(zip(x, *y))
        else:
            zipped = list(zip(x, y))
        with open(self.csvpath, "wb") as fi:
            writer = csv.writer(fi)
            if self.header:
                head = ["Time"]
                head.extend(self.yticksNames)
                writer.writerow(head)
            writer.writerows(zipped)
        
    def drawR(self):
        ycsv = []
        xcsv = []
        for i, name in enumerate(self.datasetsR):
            name = name[0]
            # just name; with mapset it would be long
            self.yticksNames.append(name)
            self.yticksPos.append(1)  # TODO
            xdata = []
            ydata = []
            for keys, values in six.iteritems(self.timeDataR[name]):
                if keys in ['temporalType', 'granularity', 'validTopology',
                            'unit', 'temporalDataType']:
                    continue
                xdata.append(self.convert(values['start_datetime']))
                ydata.append(values['value'])
                xcsv.append(values['start_datetime'])

            if len(ydata) == ydata.count(None):
                GError(parent=self, showTraceback=False,
                       message=_("Problem getting data from raster temporal"
                                 " dataset. Empty list of values."))
                return
            self.lookUp.AddDataset(yranges=ydata, xranges=xdata,
                                   datasetName=name)
            color = next(self.colors)
            self.plots.append(self.axes2d.plot(xdata, ydata, marker='o',
                                               color=color,
                                               label=self.plotNameListR[i])[0])
            if self.csvpath:
                ycsv.append(ydata)

        if self.csvpath:
            self._writeCSV(xcsv, ycsv)
        self._setLabels(self.timeDataR[name]['granularity'])
        # legend
        handles, labels = self.axes2d.get_legend_handles_labels()
        self.axes2d.legend(loc=0)

    def drawVCats(self):
        ycsv = []
        for i, name in enumerate(self.plotNameListV):
            # just name; with mapset it would be long
            labelname = name.replace('+', ' ')
            self.yticksNames.append(labelname)
            name_cat = name.split('+')
            name = name_cat[0]
            self.yticksPos.append(1)  # TODO
            xdata = []
            ydata = []
            xcsv = []
            for keys, values in six.iteritems(self.timeDataV[name_cat[0]]
                                                            [name_cat[1]]):
                if keys in ['temporalType', 'granularity', 'validTopology',
                            'unit', 'temporalDataType']:
                    continue
                xdata.append(self.convert(values['start_datetime']))
                if values['value'] == '':
                    ydata.append(None)
                else:
                    ydata.append(values['value'])
                xcsv.append(values['start_datetime'])

            if len(ydata) == ydata.count(None):
                GError(parent=self, showTraceback=False,
                       message=_("Problem getting data from vector temporal"
                                 " dataset. Empty list of values for cat "
                                 "{ca}.".format(ca=name_cat[1].replace('cat',
                                                                       ''))))
                continue
            self.lookUp.AddDataset(yranges=ydata, xranges=xdata,
                                   datasetName=name)
            color = next(self.colors)

            self.plots.append(
                self.axes2d.plot(
                    xdata,
                    ydata,
                    marker='o',
                    color=color,
                    label=labelname)[0])
            if self.csvpath:
                ycsv.append(ydata)

        if self.csvpath:
            self._writeCSV(xcsv, ycsv)
        self._setLabels(self.timeDataV[name]['granularity'])

        # legend
        handles, labels = self.axes2d.get_legend_handles_labels()
        self.axes2d.legend(loc=0)
        self.listWhereConditions = []

    def drawV(self):
        ycsv = []
        for i, name in enumerate(self.plotNameListV):
            # just name; with mapset it would be long
            self.yticksNames.append(self.attribute.GetValue())
            self.yticksPos.append(0)  # TODO
            xdata = []
            ydata = []
            xcsv = []
            for keys, values in six.iteritems(self.timeDataV[name]):
                if keys in ['temporalType', 'granularity', 'validTopology',
                            'unit', 'temporalDataType']:
                    continue
                xdata.append(self.convert(values['start_datetime']))
                ydata.append(values['value'])
                xcsv.append(values['start_datetime'])

            if len(ydata) == ydata.count(None):
                GError(parent=self, showTraceback=False,
                       message=_("Problem getting data from vector temporal"
                                 " dataset. Empty list of values."))
                return
            self.lookUp.AddDataset(yranges=ydata, xranges=xdata,
                                   datasetName=name)
            color = next(self.colors)

            self.plots.append(self.axes2d.plot(xdata, ydata, marker='o',
                                               color=color, label=name)[0])
            if self.csvpath:
                ycsv.append(ydata)

        if self.csvpath:
            self._writeCSV(xcsv, ycsv)
        self._setLabels(self.timeDataV[name]['granularity'])

        # legend
        handles, labels = self.axes2d.get_legend_handles_labels()
        self.axes2d.legend(loc=0)
        self.listWhereConditions = []

    def OnRedraw(self, event=None):
        """Required redrawing."""
        self.csvpath = self.csvButton.GetValue()
        self.header = self.headerCheck.IsChecked()
        if (os.path.exists(self.csvpath) and not self.overwrite):
            dlg = wx.MessageDialog(self, _("{pa} already exists, do you want "
                                   "to overwrite?".format(pa=self.csvpath)),
                                   _("File exists"), 
                                   wx.OK | wx.CANCEL | wx.ICON_QUESTION)
            if dlg.ShowModal() != wx.ID_OK:
                dlg.Destroy()
                GError(parent=self, showTraceback=False,
                       message=_("Please change name of output CSV file or "))
                return
            dlg.Destroy()
        self.init()
        datasetsR = self.datasetSelectR.GetValue().strip()
        datasetsV = self.datasetSelectV.GetValue().strip()

        if not datasetsR and not datasetsV:
            return

        try:
            getcoors = self.coorval.coordsField.GetValue()
        except:
            try:
                getcoors = self.coorval.GetValue()
            except:
                getcoors = None
        if getcoors and getcoors != '':
            try:
                coordx, coordy = getcoors.split(',')
                coordx, coordy = float(coordx), float(coordy)
            except (ValueError, AttributeError):
                try:
                    coordx, coordy = self.coorval.GetValue().split(',')
                    coordx, coordy = float(coordx), float(coordy)
                except (ValueError, AttributeError):
                    GMessage(message=_("Incorrect coordinates format, should "
                                       "be: x,y"), parent=self)
            coors = [coordx, coordy]
            if coors:
                try:
                    self.poi = Point(float(coors[0]), float(coors[1]))
                except GException:
                    GError(parent=self, message=_("Invalid input coordinates"),
                           showTraceback=False)
                    return
                if not self.poi:
                    GError(parent=self, message=_("Invalid input coordinates"),
                           showTraceback=False)
                    return
                bbox = self.region.get_bbox()
                if not bbox.contains(self.poi):
                    GError(parent=self, message=_("Seed point outside the "
                                                  "current region"),
                           showTraceback=False)
                    return
        # check raster dataset
        if datasetsR:
            datasetsR = datasetsR.split(',')
            try:
                datasetsR = self._checkDatasets(datasetsR, 'strds')
                if not datasetsR:
                    return
            except GException:
                GError(parent=self, message=_("Invalid input raster dataset"),
                       showTraceback=False)
                return
            if not self.poi:
                GError(parent=self, message=_("Invalid input coordinates"),
                       showTraceback=False)
                return
            self.datasetsR = datasetsR

        # check vector dataset
        if datasetsV:
            datasetsV = datasetsV.split(',')
            try:
                datasetsV = self._checkDatasets(datasetsV, 'stvds')
                if not datasetsV:
                    return
            except GException:
                GError(parent=self, message=_("Invalid input vector dataset"),
                       showTraceback=False)
                return
            self.datasetsV = datasetsV
        self._redraw()

    def _redraw(self):
        """Readraw data.

        Decides if to draw also 3D and adjusts layout if needed.
        """
        if self.datasetsR:
            self._getSTRDdata(self.datasetsR)

        if self.datasetsV:
            self._getSTVDData(self.datasetsV)

        # axes3d are physically removed
        if not self.axes2d:
            self.axes2d = self.fig.add_subplot(1, 1, 1)
        self._drawFigure()

    def _checkDatasets(self, datasets, typ):
        """Checks and validates datasets.

        Reports also type of dataset (e.g. 'strds').

        :param list datasets: list of temporal dataset's name
        :return: (mapName, mapset, type)
        """
        validated = []
        tDict = tgis.tlist_grouped(type=typ, group_type=True, dbif=self.dbif)
        # nested list with '(map, mapset, etype)' items
        allDatasets = [[[(map, mapset, etype) for map in maps]
                        for etype, maps in six.iteritems(etypesDict)]
                       for mapset, etypesDict in six.iteritems(tDict)]
        # flatten this list
        if allDatasets:
            allDatasets = reduce(lambda x, y: x + y, reduce(lambda x, y: x + y,
                                                            allDatasets))
            mapsets = tgis.get_tgis_c_library_interface().available_mapsets()
            allDatasets = [
                i
                for i in sorted(
                    allDatasets, key=lambda l: mapsets.index(l[1]))]

        for dataset in datasets:
            errorMsg = _("Space time dataset <%s> not found.") % dataset
            if dataset.find("@") >= 0:
                nameShort, mapset = dataset.split('@', 1)
                indices = [n for n, (mapName, mapsetName, etype) in enumerate(
                    allDatasets) if nameShort == mapName and mapsetName == mapset]
            else:
                indices = [n for n, (mapName, mapset, etype) in enumerate(
                    allDatasets) if dataset == mapName]

            if len(indices) == 0:
                raise GException(errorMsg)
            elif len(indices) >= 2:
                dlg = wx.SingleChoiceDialog(
                    self,
                    message=_(
                        "Please specify the "
                        "space time dataset "
                        "<%s>." % dataset),
                    caption=_("Ambiguous dataset name"),
                    choices=[
                        ("%(map)s@%(mapset)s:"
                         " %(etype)s" % {
                             'map': allDatasets[i][0],
                             'mapset': allDatasets[i][1],
                             'etype': allDatasets[i][2]}) for i in indices],
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
        RunCommand(prog='g.manual', quiet=True, entry='g.gui.tplot')

    def SetDatasets(self, rasters, vectors, coors, cats, attr, title, xlabel,
                    ylabel, csvfile, head, overwrite):
        """Set the data
        :param list rasters: a list of temporal raster dataset's name
        :param list vectors: a list of temporal vector dataset's name
        :param list coors: a list with x/y coordinates
        :param list cats: a list with incld. categories of vector
        :param str attr:  name of atribute of vectror data
        """
        if not (rasters or vectors) or not (coors or cats):
            return
        try:
            if rasters:
                self.datasetsR = self._checkDatasets(rasters, 'strds')
            if vectors:
                self.datasetsV = self._checkDatasets(vectors, 'stvds')
            if not (self.datasetsR or self.datasetsV):
                return
        except GException:
            GError(parent=self, message=_("Invalid input temporal dataset"),
                   showTraceback=False)
            return
        if coors:
            try:
                self.poi = Point(float(coors[0]), float(coors[1]))
            except GException:
                GError(parent=self, message=_("Invalid input coordinates"),
                       showTraceback=False)
                return
            try:
                self.coorval.coordsField.SetValue(','.join(coors))
            except:
                self.coorval.SetValue(','.join(coors))
        if self.datasetsV:
            vdatas = ','.join(map(lambda x: x[0] + '@' + x[1], self.datasetsV))
            self.datasetSelectV.SetValue(vdatas)
            if attr:
                self.attribute.SetValue(attr)
            if cats:
                self.cats.SetValue(cats)
        if self.datasetsR:
            self.datasetSelectR.SetValue(
                ','.join(map(lambda x: x[0] + '@' + x[1], self.datasetsR)))
        if title:
            self.title.SetValue(title)
        if xlabel:
            self.x.SetValue(xlabel)
        if ylabel:
            self.y.SetValue(ylabel)
        if csvfile:
            self.csvpath = csvfile
        self.header = head
        self.overwrite = overwrite
        self._redraw()

    def OnVectorSelected(self, event):
        """Update the controlbox related to stvds"""
        dataset = self.datasetSelectV.GetValue().strip()
        name = dataset.split('@')[0]
        mapset = dataset.split('@')[1] if len(dataset.split('@')) > 1 else ''
        found = False
        for each in tgis.tlist(type='stvds', dbif=self.dbif):
            each_name, each_mapset = each.split('@')
            if name == each_name:
                if mapset and mapset != each_mapset:
                    continue
                dataset = name + '@' + each_mapset
                found = True
                break
        if found:
            try:
                vect_list = grass.read_command('t.vect.list', flags='u',
                                               input=dataset, column='name')
            except Exception:
                self.attribute.Clear()
                GError(
                    parent=self,
                    message=_("Invalid input temporal dataset"),
                    showTraceback=False)
                return
            vect_list = list(set(sorted(vect_list.split())))
            for vec in vect_list:
                self.attribute.InsertColumns(vec, 1)
        else:
            self.attribute.Clear()


class LookUp:
    """Helper class for searching info by coordinates"""

    def __init__(self, timeData, convert):
        self.data = {}
        self.timeData = timeData
        self.convert = convert

    def AddDataset(self, yranges, xranges, datasetName):
        if len(yranges) != len(xranges):
            GError(parent=self, showTraceback=False,
                   message=_("Datasets have different number of values"))
            return
        self.data[datasetName] = {}
        for i in range(len(xranges)):
            self.data[datasetName][xranges[i]] = yranges[i]

    def GetInformation(self, x):
        values = {}
        for key, value in six.iteritems(self.data):
            if value[x]:
                values[key] = [self.convert(x), value[x]]

        if len(values) == 0:
            return None

        return self.timeData, values


def InfoFormat(timeData, values):
    """Formats information about dataset"""
    text = []
    for key, val in six.iteritems(values):
        etype = timeData[key]['temporalDataType']
        if etype == 'strds':
            text.append(_("Space time raster dataset: %s") % key)
        elif etype == 'stvds':
            text.append(_("Space time vector dataset: %s") % key)
        elif etype == 'str3ds':
            text.append(_("Space time 3D raster dataset: %s") % key)

        text.append(_("Value for {date} is {val}".format(date=val[0],
                                                         val=val[1])))
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
            ys = list(zip(*info[1].values()))[1]
            if not info:
                return
            # Update the annotation in the current axis..
            annotation.xy = x, max(ys)
            text = self.formatFunction(*info)
            annotation.set_text(text)
            annotation.set_visible(True)
            event.canvas.draw()
