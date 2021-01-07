"""
@package rlisetup.py

@brief   GUI per r.li.setup module

Classes:
 - RLiSetupFrame (first frame to show existing conf file and choose some
                 operation)
 - RLIWizard (the main wizard)
 - FirstPage (first page of wizard, choose name of conf file, raster, vector,
              sampling region)
 - Keyboard (page to insert region areas from keyboard)
 - SamplingAreas (define sampling area)
 - SummaryPage (show choosen options)

(C) 2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Luca Delucchi <lucadeluge gmail com>
"""

import os

import wx
from core.globalvar import wxPythonPhoenix
if wxPythonPhoenix:
    from wx import adv as wiz
    from wx.adv import Wizard
else:
    from wx import wizard as wiz
    from wx.wizard import Wizard
import wx.lib.scrolledpanel as scrolled

from gui_core import gselect
from gui_core.wrap import Button, StaticText, TextCtrl
from location_wizard.wizard import GridBagSizerTitledPage as TitledPage
from rlisetup.functions import checkValue, retRLiPath
from rlisetup.sampling_frame import RLiSetupMapPanel
from grass.script import core as grass
from grass.script import raster as grast
from grass.script import vector as gvect
from grass.exceptions import CalledModuleError

from .functions import (
    SamplingType, convertFeature, obtainAreaVector, obtainCategories,
    sampleAreaVector,
)
from core.gcmd import GError, GMessage, RunCommand


class RLIWizard(object):
    """
    !Start wizard here and finish wizard here
    """

    def __init__(self, parent):
        self.parent = parent
        self.wizard = Wizard(parent=parent, id=wx.ID_ANY,
                                 title=_("Create new configuration file for "
                                         "r.li modules"))
        self.rlipath = retRLiPath()

        self.msAreaList = []
        # pages
        self.startpage = FirstPage(self.wizard, self)
        self.drawsampleframepage = DrawSampleFramePage(self.wizard, self)
        self.keyboardpage = KeyboardPage(self.wizard, self)
        self.samplingareapage = SamplingAreasPage(self.wizard, self)
        self.summarypage = SummaryPage(self.wizard, self)
        self.units = SampleUnitsKeyPage(self.wizard, self)
        self.drawunits = UnitsMousePage(self.wizard, self)
        self.drawsampleunitspage = DrawSampleUnitsPage(self.wizard, self)
        self.vectorareas = VectorAreasPage(self.wizard, self)
        self.moving = MovingKeyPage(self.wizard, self)
        self.regions = DrawRegionsPage(self.wizard, self)

        # order of pages
        self.startpage.SetNext(self.samplingareapage)
        self.keyboardpage.SetPrev(self.startpage)
        self.keyboardpage.SetNext(self.samplingareapage)
        self.drawsampleframepage.SetNext(self.samplingareapage)
        self.drawsampleframepage.SetPrev(self.startpage)
        self.samplingareapage.SetPrev(self.startpage)
        self.samplingareapage.SetNext(self.summarypage)

        self.regions.SetPrev(self.samplingareapage)
        self.regions.SetNext(self.summarypage)

        self.units.SetPrev(self.samplingareapage)
        self.units.SetNext(self.summarypage)

        self.drawunits.SetPrev(self.samplingareapage)
        self.drawunits.SetNext(self.drawsampleunitspage)

        self.drawsampleunitspage.SetPrev(self.drawunits)
        self.drawsampleunitspage.SetNext(self.summarypage)

        self.moving.SetPrev(self.samplingareapage)
        self.moving.SetNext(self.summarypage)

        self.vectorareas.SetPrev(self.samplingareapage)
        self.vectorareas.SetNext(self.summarypage)

        self.summarypage.SetPrev(self.samplingareapage)

        # layout
        self.startpage.DoLayout()
        self.drawsampleframepage.DoLayout()
        self.keyboardpage.DoLayout()
        self.samplingareapage.DoLayout()
        self.summarypage.DoLayout()
        self.units.DoLayout()
        self.drawunits.DoLayout()
        self.drawsampleunitspage.DoLayout()
        self.regions.DoLayout()
        self.moving.DoLayout()
        self.vectorareas.DoLayout()

        self.wizard.FitToPage(self.startpage)
        # run_wizard
        if self.wizard.RunWizard(self.startpage):
            dlg = wx.MessageDialog(
                parent=self.parent,
                message=_(
                    "Do you want to create r.li "
                    "configuration file <%s>?") %
                self.startpage.conf_name,
                caption=_("Create new r.li configuration file?"),
                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)

            if dlg.ShowModal() == wx.ID_NO:
                self._cleanup()
            else:
                self._write_confile()
                self._cleanup()
            dlg.Destroy()
        else:
            self._cleanup()
            self.wizard.Destroy()
            GMessage(parent=self.parent,
                     message=_("r.li.setup wizard canceled. "
                               "Configuration file not created."))

    def _write_confile(self):
        """Write the configuration file"""
        f = open(os.path.join(self.rlipath, self.startpage.conf_name), 'w')
        self.rasterinfo = grast.raster_info(self.startpage.rast)
        self._write_region(f)
        self._write_area(f)
        f.close()

    def _temp_region(self):
        # save current settings:
        grass.use_temp_region()
        # Temporarily aligning region resolution to $RASTER resolution
        # keep boundary settings
        grass.run_command('g.region', raster=self.startpage.rast)
        self.gregion = grass.region()
        self.SF_NSRES = self.gregion['nsres']
        self.SF_EWRES = self.gregion['ewres']

    def _write_region(self, fil):
        """Write the region"""
        if self.startpage.region == 'whole':
            fil.write("SAMPLINGFRAME 0|0|1|1\n")
            self._temp_region()
            self.SF_X = 0.0
            self.SF_Y = 0.0
            self.SF_RL = abs(int(float(self.gregion['s'] - self.gregion['n'])
                                 / float(self.gregion['nsres'])))
            self.SF_CL = abs(int(float(self.gregion['e'] - self.gregion['w'])
                                 / float(self.gregion['ewres'])))
            self.SF_N = self.gregion['n']
            self.SF_S = self.gregion['s']
            self.SF_E = self.gregion['e']
            self.SF_W = self.gregion['w']
            self.per_x = float(self.SF_X) / float(self.rasterinfo['cols'])
            self.per_y = float(self.SF_Y) / float(self.rasterinfo['rows'])
            self.per_rl = float(self.SF_RL) / float(self.rasterinfo['rows'])
            self.per_cl = float(self.SF_CL) / float(self.rasterinfo['cols'])
        elif self.startpage.region == 'key':
            self._temp_region()
            self.SF_X = float(self.keyboardpage.col_up)
            self.SF_Y = float(self.keyboardpage.row_up)
            self.SF_RL = float(self.keyboardpage.row_len)
            self.SF_CL = float(self.keyboardpage.col_len)
            self.SF_N = self.gregion['n'] - (self.SF_NSRES * self.SF_Y)
            self.SF_S = self.gregion[
                'n'] - (self.SF_NSRES * self.SF_Y + self.SF_RL)
            self.SF_W = self.gregion['w'] + (self.SF_EWRES * self.SF_X)
            self.SF_E = self.gregion[
                'w'] + (self.SF_EWRES * self.SF_X + self.SF_CL)
            self.per_x = float(self.SF_X) / float(self.rasterinfo['cols'])
            self.per_y = float(self.SF_Y) / float(self.rasterinfo['rows'])
            self.per_rl = float(self.SF_RL) / float(self.rasterinfo['rows'])
            self.per_cl = float(self.SF_CL) / float(self.rasterinfo['cols'])
            fil.write("SAMPLINGFRAME %r|%r|%r|%r\n" % (self.per_x, self.per_y,
                                                       self.per_rl,
                                                       self.per_cl))
        elif self.startpage.region == 'draw':
            self._temp_region()
            tregion = self.drawsampleframepage.tregion
            # should we call this? with align param?
            RunCommand('g.region', align=self.startpage.rast, n=tregion['n'],
                       s=tregion['s'], w=tregion['w'], e=tregion['e'])
            newreg = grass.region()
            self.SF_N = newreg['n']  # set env(SF_N) $n
            self.SF_S = newreg['s']  # set env(SF_S) $s
            self.SF_E = newreg['e']  # set env(SF_E) $e
            self.SF_W = newreg['w']  # set env(SF_W) $w

            self.SF_Y = abs(
                round(
                    self.gregion['n'] -
                    newreg['n']) /
                newreg['nsres'])
#         set env(SF_Y) [expr abs(round(($s_n - $n) / $nres)) ]
            self.SF_X = abs(
                round(
                    self.gregion['w'] -
                    newreg['w']) /
                newreg['ewres'])
#         set env(SF_X) [expr abs(round(($s_w - $w) / $sres)) ]
            self.SF_RL = abs(
                round(
                    newreg['n'] -
                    newreg['s']) /
                newreg['nsres'])
#         set env(SF_RL) [expr abs(round(($n - $s) / $nres)) ]
            self.SF_CL = abs(
                round(
                    newreg['e'] -
                    newreg['w']) /
                newreg['ewres'])
#         set env(SF_CL) [expr abs(round(($e - $w) / $sres)) ]
            self.per_x = float(self.SF_X) / float(self.rasterinfo['cols'])
#         double($env(SF_X)) / double($cols)
            self.per_y = float(self.SF_Y) / float(self.rasterinfo['rows'])
#           double($env(SF_Y)) / double($rows)
            self.per_rl = float(self.SF_RL) / float(self.rasterinfo['rows'])
#         double($env(SF_RL)) / double($rows)
            self.per_cl = float(self.SF_CL) / float(self.rasterinfo['cols'])
#         double($env(SF_CL)) / double($cols)
            fil.write("SAMPLINGFRAME %r|%r|%r|%r\n" %
                      (self.per_x, self.per_y, self.per_rl, self.per_cl))

    def _value_for_circle(self, radius):
        self.CIR_RL = round((2 * float(radius)) /
                            float(self.rasterinfo['ewres']))
        self.CIR_CL = round((2 * float(radius)) /
                            float(self.rasterinfo['nsres']))
        if not self.CIR_RL % 2:
            self.CIR_RL += 1
        if not self.CIR_CL % 2:
            self.CIR_CL += 1
        return

    def _circle(self, radius, mask):
        """Create a circle mask"""
        self._value_for_circle(radius)
        eastEdge = float(self.SF_W + (self.CIR_RL * self.SF_EWRES))
        southEdge = float(self.SF_N - (self.CIR_CL * self.SF_NSRES))
        grass.del_temp_region()
        grass.use_temp_region()
        grass.run_command('g.region', n=self.SF_N, s=southEdge, e=eastEdge,
                          w=self.SF_W)
        xcenter = grass.region(complete=True)['center_easting']
        ycenter = grass.region(complete=True)['center_northing']
        grass.run_command('r.circle', flags='b', out=mask, max=radius,
                          coordinate=[xcenter, ycenter], quiet=True)
        grass.del_temp_region()
        grass.use_temp_region()
        grass.run_command('g.region', raster=self.startpage.rast)

    def getSamplingType(self):
        """Obtain the sampling type"""
        devicetype = self.samplingareapage.regionbox
        samplingtype = self.samplingareapage.samplingtype
        shapetype = self.units.boxtype
        if samplingtype == SamplingType.UNITS:
            if devicetype == 'mouse':
                if shapetype == 'circle':
                    samtype = SamplingType.MUNITSC
                else:
                    samtype = SamplingType.MUNITSR
            elif devicetype == 'keyboard':
                if shapetype == 'circle':
                    samtype = SamplingType.KUNITSC
                else:
                    samtype = SamplingType.KUNITSR

        elif samplingtype == SamplingType.MVWIN:
            if devicetype == 'mouse':
                if shapetype == 'circle':
                    samtype = SamplingType.MMVWINC
                else:
                    samtype = SamplingType.MMVWINR
            elif devicetype == 'keyboard':
                if shapetype == 'circle':
                    samtype = SamplingType.KMVWINC
                else:
                    samtype = SamplingType.KMVWINR
        elif samplingtype == SamplingType.WHOLE:
            samtype = SamplingType.WHOLE
        elif samplingtype == SamplingType.REGIONS:
            samtype = SamplingType.REGIONS
        elif samplingtype == SamplingType.VECT:
            samtype = SamplingType.VECT
        else:
            samtype = samplingtype
        return samtype

    def _write_area(self, fil):
        """Write the area according the type"""
        samtype = self.getSamplingType()

        # sampling type is whole
        if samtype == SamplingType.WHOLE:
            cl = float(self.SF_CL) / float(self.rasterinfo['cols'])
            rl = float(self.SF_RL) / float(self.rasterinfo['rows'])
            # this two variable are unused, problably to remove
            x = float(self.SF_X) / float(self.rasterinfo['cols'])
            y = float(self.SF_Y) / float(self.rasterinfo['rows'])
            fil.write("SAMPLEAREA %r|%r|%r|%r\n" % (self.per_x, self.per_y,
                                                    rl, cl))
        ##KMWINC = samplingtype=moving, regionbox=keyboard, shape=circle
        elif samtype == SamplingType.KMVWINC:
            self._circle(self.moving.width, self.moving.height)
            cl = float(self.CIR_CL) / float(self.rasterinfo['cols'])
            rl = float(self.CIR_RL) / float(self.rasterinfo['rows'])
            fil.write("MASKEDSAMPLEAREA -1|-1|%r|%r" % (rl, cl))
            fil.write("|%s" % self.moving.height)
            fil.write("\nMOVINGWINDOW\n")
        # KMWINR = samplingtype moving, regionbox=keyboard, shape=rectangle
        elif samtype == SamplingType.KMVWINR:
            cl = float(self.moving.width) / float(self.rasterinfo['cols'])
            rl = float(self.moving.height) / float(self.rasterinfo['rows'])
            fil.write("SAMPLEAREA -1|-1|%r|%r" % (rl, cl))
            fil.write("\nMOVINGWINDOW\n")
        # MMVWINR = samplingtype moving, regionbox=mouse, shape=rectangle
        elif samtype == SamplingType.MMVWINR:
            cl = float(self.msAreaList[0]['cols']
                       ) / float(self.rasterinfo['cols'])
            rl = float(self.msAreaList[0]['rows']
                       ) / float(self.rasterinfo['rows'])
            fil.write("SAMPLEAREA -1|-1|%r|%r" % (rl, cl))
            fil.write("\nMOVINGWINDOW\n")
        # MMVWINR = samplingtype moving, regionbox=mouse, shape=circle
        elif samtype == SamplingType.MMVWINC:
            self._value_for_circle(self.msAreaList[0].radius)
            cl = float(self.CIR_CL) / float(self.rasterinfo['cols'])
            rl = float(self.CIR_RL) / float(self.rasterinfo['rows'])
            fil.write("SAMPLEAREA -1|-1|%r|%r" % (rl, cl))
            fil.write("|%s" % self.msAreaList[0].raster)
            fil.write("\nMOVINGWINDOW\n")
        ##KUNITSC = samplingtype=units, regionbox=keyboard, shape=cirlce
        ##KUNITSR = samplingtype=units, regionbox=keyboard, shape=rectangle
        elif samtype == SamplingType.KUNITSC or samtype == SamplingType.KUNITSR:
            if samtype == SamplingType.KUNITSC:
                self._circle(self.units.width, self.units.height)
                cl = float(self.CIR_CL) / float(self.rasterinfo['cols'])
                rl = float(self.CIR_RL) / float(self.rasterinfo['rows'])
            else:
                cl = float(self.units.width) / float(self.rasterinfo['cols'])
                rl = float(self.units.height) / float(self.rasterinfo['rows'])

            fil.write("SAMPLEAREA -1|-1|%r|%r\n" % (rl, cl))
            if self.units.distrtype == 'non_overlapping':
                fil.write("RANDOMNONOVERLAPPING %s\n" % self.units.distr1)
            elif self.units.distrtype == 'systematic_contiguos':
                fil.write("SYSTEMATICCONTIGUOUS\n")
            elif self.units.distrtype == 'stratified_random':
                fil.write("STRATIFIEDRANDOM %s|%s\n" % (self.units.distr1,
                                                        self.units.distr2))
            elif self.units.distrtype == 'systematic_noncontiguos':
                fil.write("SYSTEMATICNONCONTIGUOUS %s\n" % self.units.distr1)
            elif self.units.distrtype == 'centered_oversites':
                fil.write("")

        # elif self.samplingareapage.samplingtype == SamplingType.UNITS and
        # self.samplingareapage.regionbox=='mouse':

        ##MUNITSC = samplingtype=units, regionbox=mouse, shape=cirlce
        ##MUNITSR = samplingtype=units, regionbox=mouse, shape=rectangle
        elif self.samplingareapage.samplingtype in [SamplingType.MUNITSR,
                                                    SamplingType.MUNITSC]:
            # get the raster region into rastregion
            grass.use_temp_region()
            grass.run_command('g.region', raster=self.startpage.rast)
            rastregion = grass.region()
            s_n = rastregion['n']
            s_w = rastregion['w']
            rows = float(self.rasterinfo['rows'])
            cols = float(self.rasterinfo['cols'])
            for tregion in self.msAreaList:
                if self.samplingareapage.samplingtype == SamplingType.MUNITSC:
                    tregion = tregion.region
                abs_y = abs(
                    round(
                        (float(s_n) - tregion['n']) / tregion
                        ['nsres']))
                abs_x = abs(
                    round(
                        (float(s_w) - tregion['w']) / tregion
                        ['ewres']))
                abs_rl = abs(
                    round(
                        (tregion['n'] - tregion['s']) /
                        tregion['nsres']))
                abs_cl = abs(
                    round(
                        (tregion['e'] - tregion['w']) /
                        tregion['ewres']))

                x = float(abs_x) / float(cols)
                y = float(abs_y) / float(rows)
                rl = float(abs_rl) / float(rows)
                cl = float(abs_cl) / float(cols)
                sarea = str(x) + "|" + str(y) + "|" + str(rl) + "|" + str(cl)
                if self.samplingareapage.samplingtype == SamplingType.MUNITSR:
                    fil.write("SQUAREAREA %s\n" % sarea)
                elif self.samplingareapage.samplingtype == SamplingType.MUNITSC:
                    fil.write("MASKEDSAMPLEAREA %s" % sarea)
                    fil.write("|%s\n" % self.msAreaList[0].raster)

        elif self.samplingareapage.samplingtype == SamplingType.REGIONS:
            rows = float(self.rasterinfo['rows'])
            cols = float(self.rasterinfo['cols'])
            for marea in self.msAreaList:
                gregion = marea.region
                abs_y = self.SF_Y + abs(
                    round((self.SF_N - gregion['n']) / self.SF_NSRES))
                abs_x = self.SF_X + abs(
                    round((self.SF_W - gregion['w']) / self.SF_EWRES))
                abs_rl = abs(
                    round(
                        gregion['n'] -
                        gregion['s']) /
                    self.SF_NSRES)
                abs_cl = abs(
                    round(
                        gregion['e'] -
                        gregion['w']) /
                    self.SF_EWRES)

                x = float(abs_x) / float(cols)
                y = float(abs_y) / float(rows)
                rl = float(abs_rl) / float(rows)
                cl = float(abs_cl) / float(cols)

                maskArea = str(
                    x) + "|" + str(y) + "|" + str(rl) + "|" + str(cl) + "|" + marea.raster
                fil.write("SQUAREAREA %s\n" % maskArea)
        elif self.samplingareapage.samplingtype == SamplingType.VECT:
            for marea in self.msAreaList:
                fil.write(marea)
            fil.write("RASTERMAP {name}\n".format(name=self.startpage.rast))
            fil.write("VECTORMAP {name}\n".format(name=self.startpage.vect))

    def _cleanup(self):
        """Clean all the variables to save into configuration file"""
        self.startpage.conf_name = ''
        self.startpage.rast = ''
        self.startpage.vect = ''
        self.startpage.region = 'whole'
        self.keyboardpage.col_len = ''
        self.keyboardpage.col_up = ''
        self.keyboardpage.row_len = ''
        self.keyboardpage.row_up = ''
        self.samplingareapage.samplingtype = 'whole'
        self.units.width = ''
        self.units.height = ''
        self.units.boxtype = ''
        self.regions.numregions = 0
        self.moving.width = ''
        self.moving.height = ''
        self.moving.boxtype = ''
        for page in (self.drawsampleframepage,
                     self.regions,
                     self.drawsampleunitspage,
                     self.vectorareas):
            if page.mapPanel:
                page.mapPanel._mgr.UnInit()


class FirstPage(TitledPage):
    """
    !Set name of configuration file, choose raster and optionally vector/sites
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Select maps and define name"))

        self.region = 'whole'
        self.rast = ''
        self.conf_name = ''
        self.vect = ''
        self.VectorEnabled = True

        self.parent = parent

        # name of output configuration file
        self.newconflabel = StaticText(
            parent=self, id=wx.ID_ANY,
            label=_('Name for new configuration file to create'))

        self.newconftxt = TextCtrl(parent=self, id=wx.ID_ANY,
                                   size=(250, -1))
        wx.CallAfter(self.newconftxt.SetFocus)

        self.sizer.Add(self.newconflabel, border=5, pos=(0, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.newconftxt, border=5, pos=(0, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        # raster
        self.mapsellabel = StaticText(
            parent=self, id=wx.ID_ANY,
            label=_('Raster map to use to select areas'))
        self.mapselect = gselect.Select(parent=self, id=wx.ID_ANY,
                                        size=(250, -1), type='cell',
                                        multiple=False)
        self.sizer.Add(self.mapsellabel, border=5, pos=(1, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.mapselect, border=5, pos=(1, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        # vector
        self.vectsellabel = StaticText(
            parent=self, id=wx.ID_ANY,
            label=_('Vector map to use to select areas'))
        self.vectselect = gselect.Select(parent=self, id=wx.ID_ANY,
                                         size=(250, -1), type='vector',
                                         multiple=False)
        self.sizer.Add(self.vectsellabel, border=5, pos=(2, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.vectselect, border=5, pos=(2, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        # vector layer
        self.vectlaylabel = StaticText(
            parent=self, id=wx.ID_ANY,
            label=_('Vector map layer to use to select areas'))
        self.vectlayer = wx.ComboBox(parent=self, id=wx.ID_ANY,
                                     size=(250, -1))
        self.sizer.Add(self.vectlaylabel, border=5, pos=(3, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.vectlayer, border=5, pos=(3, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        # define sampling region
        self.sampling_reg = wx.RadioBox(
            parent=self,
            id=wx.ID_ANY,
            label=" %s " % _(
                "Define sampling "
                "region (region for analysis)"),
            choices=[
                _('Whole map layer'),
                _('Keyboard setting'),
                _('Draw the sampling frame')],
            majorDimension=1,
            style=wx.RA_SPECIFY_ROWS)

        self.sizer.Add(self.sampling_reg,
                       flag=wx.ALIGN_CENTER | wx.ALL | wx.EXPAND, border=5,
                       pos=(5, 0), span=(1, 2))
        self.infoError = StaticText(self, label='')
        self.infoError.SetForegroundColour(wx.RED)
        self.sizer.Add(self.infoError,
                       flag=wx.ALIGN_CENTER | wx.ALL | wx.EXPAND, border=5,
                       pos=(6, 0), span=(1, 2))

        # bindings
        self.sampling_reg.Bind(wx.EVT_RADIOBOX, self.OnSampling)
        self.newconftxt.Bind(wx.EVT_KILL_FOCUS, self.OnName)
        self.newconftxt.Bind(wx.EVT_TEXT, self.OnNameChanged)
        self.vectselect.Bind(wx.EVT_TEXT, self.OnVector)
        self.mapselect.Bind(wx.EVT_TEXT, self.OnRast)
        self.vectlayer.Bind(wx.EVT_TEXT, self.OnLayer)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnExitPage)

        wx.CallAfter(wx.FindWindowById(wx.ID_FORWARD).Enable, False)

    def OnSampling(self, event):
        """Change region type"""
        if event.GetInt() == 0:
            self.region = 'whole'
            self.SetNext(self.parent.samplingareapage)
        elif event.GetInt() == 1:
            self.region = 'key'
            self.SetNext(self.parent.keyboardpage)
        elif event.GetInt() == 2:  # currently disabled
            self.region = 'draw'
            self.SetNext(self.parent.drawsampleframepage)

    def OnName(self, event):
        """Sets the name of configuration file"""
        if self.conf_name in self.parent.parent.listfiles:
            GMessage(
                parent=self, message=_(
                    "The configuration file %s "
                    "already exists, please change name") %
                self.conf_name)
            self.newconftxt.SetValue('')
            self.conf_name = ''

    def OnNameChanged(self, event):
        """Name of configuration file has changed"""
        self.conf_name = self.newconftxt.GetValue()
        next = wx.FindWindowById(wx.ID_FORWARD)
        next.Enable(self.CheckInput())

    def OnRast(self, event):
        """Sets raster map"""
        self.rast = self.mapselect.GetValue()
        next = wx.FindWindowById(wx.ID_FORWARD)
        next.Enable(self.CheckInput())

    def OnVector(self, event):
        """Sets vector map"""
        self.vect = self.vectselect.GetValue()
        if self.vect:
            self.VectorEnabled, layers = self.CheckVector(self.vect)
            if self.VectorEnabled:
                self.vectlayer.SetItems(layers)
                self.vectlayer.SetSelection(0)
                self.vectorlayer = self.vectlayer.GetValue()
                self.infoError.SetLabel('')
            else:
                self.vectlayer.Clear()
                self.vectlayer.SetValue('')
                self.vect = ''
        else:
            self.infoError.SetLabel('')
            self.vectlayer.Clear()
            self.vectlayer.SetValue('')

        next = wx.FindWindowById(wx.ID_FORWARD)
        next.Enable(self.CheckInput())

    def OnLayer(self, event):
        try:
            self.vectorlayer = self.vectlayer.GetValue()
        except:
            self.vectorlayer = None
        next = wx.FindWindowById(wx.ID_FORWARD)
        next.Enable(self.CheckInput())

    def OnEnterPage(self, event):
        """Sets the default values, for the entire map
        """
        next = wx.FindWindowById(wx.ID_FORWARD)
        next.Enable(self.CheckInput())
        wx.CallAfter(wx.FindWindowById(wx.ID_FORWARD).Enable,
                     self.CheckInput())

    def CheckVector(self, vector):
        """Check if the type of vector is area and return the number of
        vector's layer"""
        try:
            areas = gvect.vector_info_topo(vector)['areas']
        except CalledModuleError:
            self.infoError.SetLabel(_("Vector %s was not found, please "
                                      "select another vector") % vector)
            return False, []
        if areas == 0:
            self.infoError.SetLabel(_("Vector %s has no areas, please "
                                      "select another vector") % vector)
            return False, []
        links = gvect.vector_info(vector)['num_dblinks']
        if links == 0:
            self.infoError.SetLabel(_("Vector %s has no table connected, "
                                      "please select another vector") % vector)
            return False, []
        elif links > 0:
            layers = []
            for i in range(1, links + 1):
                layers.append(str(i))
            return True, layers
        else:
            return False, []

    def CheckInput(self):
        """Check input fields.

        :return: True if configuration file is given and raster xor vector map,
                 False otherwise
        """
        return bool(self.conf_name and bool(self.rast and
                                            bool(self.VectorEnabled)))

    def OnExitPage(self, event=None):
        """Function during exiting"""
        next = wx.FindWindowById(wx.ID_FORWARD)
        next.Enable(self.CheckInput())
        if event.GetDirection():
            if self.region == 'key':
                self.SetNext(self.parent.keyboardpage)
                self.parent.samplingareapage.SetPrev(self.parent.keyboardpage)
            elif self.region == 'whole':
                self.SetNext(self.parent.samplingareapage)
                self.parent.samplingareapage.SetPrev(self)
            elif self.region == 'draw':
                self.SetNext(self.parent.drawsampleframepage)
                self.parent.samplingareapage.SetPrev(
                    self.parent.drawsampleframepage)
                return


class KeyboardPage(TitledPage):
    """
    !Choose the region setting the values of border using the keyboard
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Insert sampling frame parameter"))

        self.parent = parent
        self.col_len = ''
        self.row_len = ''
        self.col_up = '0'
        self.row_up = '0'

        # column up/left
        self.ColUpLeftlabel = StaticText(parent=self, id=wx.ID_ANY,
                                            label=_("Column of upper left "
                                                    "corner"))

        self.ColUpLefttxt = TextCtrl(parent=self, id=wx.ID_ANY,
                                     size=(250, -1))
        wx.CallAfter(self.ColUpLeftlabel.SetFocus)

        self.sizer.Add(self.ColUpLeftlabel, border=5, pos=(1, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.ColUpLefttxt, border=5, pos=(1, 2),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.AddGrowableCol(2)
        # row up/left
        self.RowUpLeftlabel = StaticText(
            parent=self, id=wx.ID_ANY, label=_('Row of upper left corner'))

        self.RowUpLefttxt = TextCtrl(parent=self, id=wx.ID_ANY,
                                     size=(250, -1))
        wx.CallAfter(self.RowUpLeftlabel.SetFocus)

        self.sizer.Add(self.RowUpLeftlabel, border=5, pos=(2, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.RowUpLefttxt, border=5, pos=(2, 2),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        # row length
        self.RowLenlabel = StaticText(
            parent=self,
            id=wx.ID_ANY,
            label=_('Row length of sampling frame'))

        self.RowLentxt = TextCtrl(parent=self, id=wx.ID_ANY, size=(250, -1))
        wx.CallAfter(self.RowLenlabel.SetFocus)

        self.sizer.Add(self.RowLenlabel, border=5, pos=(3, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.RowLentxt, border=5, pos=(3, 2),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        # column length
        self.ColLenlabel = StaticText(
            parent=self,
            id=wx.ID_ANY,
            label=_('Row length of sampling frame'))

        self.ColLentxt = TextCtrl(parent=self, id=wx.ID_ANY, size=(250, -1))
        wx.CallAfter(self.ColLenlabel.SetFocus)

        self.sizer.Add(self.ColLenlabel, border=5, pos=(4, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.ColLentxt, border=5, pos=(4, 2),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        self.ColUpLefttxt.SetValue(self.col_up)
        self.RowUpLefttxt.SetValue(self.row_up)

        self.ColUpLefttxt.Bind(wx.EVT_KILL_FOCUS, self.OnColLeft)
        self.RowUpLefttxt.Bind(wx.EVT_KILL_FOCUS, self.OnRowLeft)
        self.ColLentxt.Bind(wx.EVT_KILL_FOCUS, self.OnColLen)
        self.RowLentxt.Bind(wx.EVT_KILL_FOCUS, self.OnRowLen)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnExitPage)

    def OnColLeft(self, event):
        """Sets the name of configuration file"""
        self.col_up = self.ColUpLefttxt.GetValue()
        checkValue(self.col_up)

    def OnRowLeft(self, event):
        """Sets the name of configuration file"""
        self.row_up = self.RowUpLefttxt.GetValue()
        checkValue(self.row_up)

    def OnColLen(self, event):
        """Sets the name of configuration file"""
        self.col_len = self.ColLentxt.GetValue()
        checkValue(self.col_len)

    def OnRowLen(self, event):
        """Sets the name of configuration file"""
        self.row_len = self.RowLentxt.GetValue()
        checkValue(self.row_len)

    def OnEnterPage(self, event):
        """Sets the default values, for the entire map
        """
        # R# check if raster exists before anything
        if self.col_len == '' and self.row_len == '':
            rastinfo = grast.raster_info(self.parent.startpage.rast)
            self.col_len = rastinfo['cols']
            self.row_len = rastinfo['rows']

        self.ColLentxt.SetValue(self.col_len)
        self.RowLentxt.SetValue(self.row_len)

    def OnExitPage(self, event=None):
        """Function during exiting"""
        if self.row_len == '' or self.col_len == '' or self.row_up == '' or self.col_up == '':
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)


class DrawSampleFramePage(TitledPage):
    """Choose the region setting the values drawing a box"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Draw sampling frame"))
        self.parent = parent
        self.mapPanel = None
        self.tregion = None
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnExitPage)

    def SampleFrameChanged(self, region):
        """"Enables the next dialog when region is set"""
        if region:
            self.tregion = region
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)

    def OnEnterPage(self, event):
        """Function during entering"""
        if self.mapPanel is None:
            self.mapPanel = RLiSetupMapPanel(self, samplingType='drawFrame')
            self.mapPanel.sampleFrameChanged.connect(self.SampleFrameChanged)
            self.sizer.Add(self.mapPanel, flag=wx.EXPAND, pos=(0, 0))
            self.sizer.AddGrowableCol(0)
            self.sizer.AddGrowableRow(0)
            self._raster = None
        else:
            self.mapPanel._region = {}

        self.SampleFrameChanged(region=None)
        rast = self.parent.startpage.rast
        if self._raster != rast:
            map_ = self.mapPanel.GetMap()
            map_.DeleteAllLayers()
            cmdlist = ['d.rast', 'map=%s' % rast]
            map_.AddLayer(ltype='raster', command=cmdlist, active=True,
                          name=rast, hidden=False, opacity=1.0, render=True)

    def OnExitPage(self, event=None):
        """Function during exiting"""
        if event.GetDirection():
            self.SetNext(self.parent.samplingareapage)
            self.parent.samplingareapage.SetPrev(self)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)


class SamplingAreasPage(TitledPage):
    """
    Set name of configuration file, choose raster and optionally vector/sites.
    This is coming after choose the region
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Insert sampling areas"))
        self.samplingtype = 'whole'
        self.parent = parent
        self.overwriteTemp = False
        # toggles
        self.radioBox = wx.RadioBox(parent=self, id=wx.ID_ANY,
                                    label="",
                                    choices=[_("Whole map layer"),
                                             _("Regions"),
                                             _("Sample units"),
                                             _("Moving window"),
                                             _("Select areas from the\n"
                                               "overlaid vector map")],
                                    majorDimension=1,
                                    style=wx.RA_SPECIFY_COLS | wx.NO_BORDER)
        # layout
        self.sizer.SetVGap(10)
        self.sizer.Add(self.radioBox, flag=wx.ALIGN_LEFT, pos=(0, 0))

        self.regionBox = wx.RadioBox(
            parent=self,
            id=wx.ID_ANY,
            label=_("Choose a method"),
            choices=[
                _('Use keyboard to enter sampling area'),
                _('Use mouse to draw sampling area')],
            majorDimension=1,
            style=wx.RA_SPECIFY_ROWS)
        #self.regionBox.EnableItem(1, False)
        self.regionBox.SetItemToolTip(1, _("This option is not supported yet"))
        self.sizer.Add(self.regionBox, flag=wx.ALIGN_CENTER, pos=(1, 0))

        # bindings
        self.radioBox.Bind(wx.EVT_RADIOBOX, self.SetVal)
        self.regionBox.Bind(wx.EVT_RADIOBOX, self.OnRegionDraw)
        self.regionbox = 'keyboard'

        self.regionPanelSizer = wx.GridBagSizer(1, 2)
        self.regionNumPanel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.regionNumLabel = StaticText(
            parent=self.regionNumPanel, id=wx.ID_ANY,
            label=_('Number of regions to draw:'))
        self.regionNumTxt = TextCtrl(parent=self.regionNumPanel,
                                        id=wx.ID_ANY, size=(50, -1))
        self.regionPanelSizer.Add(self.regionNumLabel, flag=wx.ALIGN_CENTER,
                                  pos=(0, 0))
        self.regionPanelSizer.Add(self.regionNumTxt, flag=wx.ALIGN_CENTER,
                                  pos=(0, 1))

        self.regionNumPanel.SetSizer(self.regionPanelSizer)
        self.sizer.Add(self.regionNumPanel, flag=wx.ALIGN_CENTER, pos=(2, 0))

        self.areaPanelSizer = wx.GridBagSizer(2, 3)
        self.areaPanel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.overwriteText = StaticText(
            parent=self.areaPanel, id=wx.ID_ANY, label=_(
                'Do you want to overwrite existing'
                ' temporal maps if they exist?'))
        self.overwriteCheck = wx.CheckBox(parent=self.areaPanel, id=wx.ID_ANY)
        self.areaText = StaticText(
            parent=self.areaPanel, id=wx.ID_ANY,
            label=_('Do you want to check vector areas?'))
        self.areaOK = Button(self.areaPanel, wx.ID_ANY, 'Yes', (50, 80))
        self.areaOK.SetToolTip(_("Select if use area by area"))
        self.areaNO = Button(self.areaPanel, wx.ID_ANY, 'No', (50, 80))
        self.areaNO.SetToolTip(_("All the features will be used"))
        self.areaOK.Bind(wx.EVT_BUTTON, self.OnVectYes)
        self.areaNO.Bind(wx.EVT_BUTTON, self.OnVectNo)
        self.overwriteCheck.Bind(wx.EVT_CHECKBOX, self.OnOverwrite)
        self.areaPanelSizer.Add(self.overwriteText, flag=wx.ALIGN_CENTER,
                                pos=(0, 0))
        self.areaPanelSizer.Add(self.overwriteCheck, flag=wx.ALIGN_CENTER,
                                pos=(0, 1))
        self.areaPanelSizer.Add(self.areaText, flag=wx.ALIGN_CENTER,
                                pos=(1, 0))
        self.areaPanelSizer.Add(self.areaOK, flag=wx.ALIGN_CENTER, pos=(1, 1))
        self.areaPanelSizer.Add(self.areaNO, flag=wx.ALIGN_CENTER, pos=(1, 2))
        self.areaPanel.SetSizer(self.areaPanelSizer)
        self.sizer.Add(self.areaPanel, flag=wx.ALIGN_CENTER, pos=(3, 0))

        self.calculatingAreas = StaticText(
            parent=self, id=wx.ID_ANY,
            label=_('Analysing all vector features...'))
        self.sizer.Add(self.calculatingAreas, flag=wx.ALIGN_CENTER, pos=(4, 0))
        self.numregions = ''
        self.regionNumTxt.Bind(wx.EVT_TEXT, self.OnNumRegions)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnNumRegions(self, event):
        """Obtain the number of regions"""
        if self.regionNumTxt.GetValue():
            self.SetNext(self.parent.regions)
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
            self.numregions = self.regionNumTxt.GetValue()
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)

    def OnEnterPage(self, event):
        """Insert values into text controls for summary of location
        creation options
        """
        self.SetVal(None)
        if self.parent.startpage.vect:
            self.radioBox.ShowItem(4, True)
            self.vect_data = []
        else:
            self.radioBox.ShowItem(4, False)
        self.sizer.Layout()
        wx.FindWindowById(wx.ID_FORWARD).Enable(True)

    def SetVal(self, event):
        """Choose method"""
        radio = self.radioBox.GetSelection()
        wx.FindWindowById(wx.ID_FORWARD).Enable(True)
        if radio == 0:
            self.samplingtype = SamplingType.WHOLE
            self.DrawNothing()
        elif radio == 1:
            self.samplingtype = SamplingType.REGIONS
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        elif radio == 2:
            self.samplingtype = SamplingType.UNITS
            regtype = self.regionBox.GetSelection()
            self.RegionDraw(regtype)
        elif radio == 3:
            self.samplingtype = SamplingType.MVWIN
            regtype = self.regionBox.GetSelection()
            self.RegionDraw(regtype)
        elif radio == 4:
            self.samplingtype = SamplingType.VECT
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        self.ShowExtraOptions(self.samplingtype)

    def ShowExtraOptions(self, samtype):
        """Show some extra options for some sampling type"""
        if samtype == SamplingType.REGIONS:
            self.sizer.Hide(self.regionBox)
            self.sizer.Hide(self.areaPanel)
            self.sizer.Hide(self.calculatingAreas)
            self.sizer.Show(self.regionNumPanel)
        elif samtype == SamplingType.UNITS or samtype == SamplingType.MVWIN:
            self.sizer.Hide(self.regionNumPanel)
            self.sizer.Hide(self.areaPanel)
            self.sizer.Hide(self.calculatingAreas)
            self.sizer.Show(self.regionBox)
        elif samtype == SamplingType.VECT:
            self.sizer.Hide(self.regionBox)
            self.sizer.Hide(self.regionNumPanel)
            self.sizer.Hide(self.calculatingAreas)
            self.sizer.Show(self.areaPanel)
        self.sizer.Layout()

    def OnRegionDraw(self, event):
        self.RegionDraw(event.GetInt())
        return

    def OnOverwrite(self, event):
        self.overwriteTemp = self.overwriteCheck.GetValue()
        return

    def OnVectYes(self, event):
        """The user choose to select the vector areas, this function set the
        next page to VectorAreasPage"""
        self.SetNext(self.parent.vectorareas)
        wx.FindWindowById(wx.ID_FORWARD).Enable(True)
        self.parent.wizard.ShowPage(self.parent.vectorareas)

    def OnVectNo(self, event):
        """The user choose to use all the vector areas, this function analyses
        all the vector areas and fill the msAreaList variable"""
        self.sizer.Show(self.calculatingAreas)
        self.sizer.Hide(self.regionNumPanel)
        self.sizer.Hide(self.regionBox)
        self.sizer.Hide(self.areaPanel)
        self.SetNext(self.parent.summarypage)

        vect_cats = obtainCategories(self.parent.startpage.vect,
                                     self.parent.startpage.vectorlayer)

        self._progressDlg = wx.ProgressDialog(
            title=_("Analysing vector"),
            message="Analysing vector",
            maximum=len(vect_cats),
            parent=self,
            style=wx.PD_CAN_ABORT | wx.PD_APP_MODAL | wx.PD_AUTO_HIDE | wx.PD_SMOOTH)
        self._progressDlgMax = len(vect_cats)
        grass.use_temp_region()
        self.parent.msAreaList = sampleAreaVector(
            self.parent.startpage.vect,
            self.parent.startpage.rast,
            vect_cats,
            self.parent.startpage.vectorlayer,
            self.overwriteTemp,
            self._progressDlg)
        grass.del_temp_region()
        if self.parent.msAreaList:
            self.calculatingAreas.SetLabel(_("All feature are been analyzed."))
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
            self.parent.wizard.ShowPage(self.parent.summarypage)
        else:
            self.calculatingAreas.SetLabel(_("An error occurred"))
        self._progressDlg.Destroy()
        self._progressDlg = None

    def RegionDraw(self, regtype):
        """Set the next page to units or drawunits"""
        if regtype == 0:
            self.regionbox = 'keyboard'
            if self.samplingtype == SamplingType.UNITS:
                self.SetNext(self.parent.units)
            elif self.samplingtype == SamplingType.MVWIN:
                self.SetNext(self.parent.moving)
        elif regtype == 1:
            self.regionbox = 'mouse'
            self.SetNext(self.parent.drawunits)

    def DrawNothing(self):
        """Remove all the optional choices. Used also when the wizard enter in
           SamplingAreasPage, all the optional choices should be hide here"""
        self.sizer.Hide(self.regionNumPanel)
        self.sizer.Hide(self.regionBox)
        self.sizer.Hide(self.areaPanel)
        self.sizer.Hide(self.calculatingAreas)
        self.sizer.Layout()

        self.SetNext(self.parent.summarypage)


class DrawRegionsPage(TitledPage):

    def __init__(self, wizard, parent):
        self.parent = parent
        TitledPage.__init__(self, wizard, _("Draw sampling regions"))
        self.regioncount = 0
        self.mapPanel = None
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        #self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnExitPage)

    def afterRegionDrawn(self, marea):
        if marea:
            self.parent.msAreaList.append(marea)

        self.regioncount = self.regioncount + 1
        numregions = int(self.parent.samplingareapage.numregions)

        if self.regioncount > numregions:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
            self.parent.wizard.ShowPage(self.parent.summarypage)
        else:
            self.title.SetLabel(_('Draw sample region ' +
                                  str(self.regioncount) + ' of ' +
                                  str(numregions)))
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)

    def OnEnterPage(self, event):
        """Function during entering"""
        if self.parent.samplingareapage.samplingtype == SamplingType.WHOLE:
            self.title.SetLabel(_("Draw moving windows region"))
        elif self.parent.samplingareapage.samplingtype == SamplingType.UNITS:
            self.title.SetLabel(_("Draw sampling region"))
        if self.mapPanel is None:
            self.mapPanel = RLiSetupMapPanel(
                self, samplingType=self.parent.samplingareapage.samplingtype, )
            self.mapPanel.afterRegionDrawn.connect(self.afterRegionDrawn)

            self.sizer.Add(self.mapPanel, flag=wx.EXPAND, pos=(1, 0))
            self.sizer.AddGrowableCol(0)
            self.sizer.AddGrowableRow(1)
            self._raster = None

        rast = self.parent.startpage.rast
        self.afterRegionDrawn(marea=None)

        if self._raster != rast:
            map_ = self.mapPanel.GetMap()
            map_.DeleteAllLayers()
            cmdlist = ['d.rast', 'map=%s' % rast]
            map_.AddLayer(ltype='raster', command=cmdlist, active=True,
                          name=rast, hidden=False, opacity=1.0, render=True)

    # def OnExitPage(self, event=None):
        #!Function during exiting
        # print event.GetDirection()
        # if event.GetDirection():
        #    self.SetNext(self.parent.samplingareapage)
        #    self.parent.samplingareapage.SetPrev(self)


class SampleUnitsKeyPage(TitledPage):
    """Set values from keyboard for sample units
       It is used if you choose keyboard from Sampling Units or Moving windows
       in sampling areas page
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _(
            "Select sample units from keyboard"))

        self.parent = parent
        self.scrollPanel = scrolled.ScrolledPanel(parent=self, id=wx.ID_ANY)
        self.scrollPanel.SetupScrolling(scroll_x=False)

        self.panelSizer = wx.GridBagSizer(5, 5)
        self.width = ''
        self.height = ''
        self.boxtype = 'rectangle'
        self.distrtype = 'non_overlapping'
        # type of shape
        self.typeBox = wx.RadioBox(parent=self.scrollPanel, id=wx.ID_ANY,
                                   majorDimension=1, style=wx.RA_SPECIFY_COLS,
                                   label=" %s " % _("Select type of shape"),
                                   choices=[_('Rectangle'), _('Circle'),
                                            ('None')])

        self.panelSizer.Add(self.typeBox, flag=wx.ALIGN_LEFT, pos=(0, 0),
                            span=(1, 2))
        self.widthLabel = StaticText(parent=self.scrollPanel, id=wx.ID_ANY)
        self.widthTxt = TextCtrl(parent=self.scrollPanel, id=wx.ID_ANY,
                                 size=(250, -1))

        self.panelSizer.Add(
            self.widthLabel, pos=(1, 0),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.panelSizer.Add(
            self.widthTxt, pos=(1, 1),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        self.heightLabel = StaticText(parent=self.scrollPanel, id=wx.ID_ANY)
        self.heightTxt = TextCtrl(parent=self.scrollPanel, id=wx.ID_ANY,
                                  size=(250, -1))

        self.panelSizer.Add(
            self.heightLabel, pos=(2, 0),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.panelSizer.Add(
            self.heightTxt, pos=(2, 1),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.widthLabels = [_('Width size (in cells)?'),
                            _('What radius size (in meters)?')]
        self.heightLabels = [_('Height size (in cells)?'),
                             _('Name of the circle mask')]

        self.distributionBox = wx.RadioBox(
            parent=self.scrollPanel,
            id=wx.ID_ANY,
            majorDimension=1,
            style=wx.RA_SPECIFY_COLS,
            label=" %s " %
            _("Select method of sampling unit distribution"),
            choices=[
                _('Random non overlapping'),
                _('Systematic contiguos'),
                _('Stratified random'),
                _('Systematic non contiguos'),
                _('Centered over sites')])
        self.panelSizer.Add(self.distributionBox, pos=(3, 0), span=(
            1, 2), flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        self.distr1Label = StaticText(parent=self.scrollPanel, id=wx.ID_ANY,
                                         label=_("What number of Sampling "
                                                 "Units to use?"))
        self.distr1Txt = TextCtrl(parent=self.scrollPanel, id=wx.ID_ANY,
                                  size=(250, -1))
        self.panelSizer.Add(
            self.distr1Label, pos=(4, 0),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.panelSizer.Add(
            self.distr1Txt, pos=(4, 1),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.distr2Label = StaticText(parent=self.scrollPanel, id=wx.ID_ANY,
                                         label="")
        self.distr2Txt = TextCtrl(parent=self.scrollPanel, id=wx.ID_ANY,
                                  size=(250, -1))
        self.panelSizer.Add(
            self.distr2Label, pos=(5, 0),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.panelSizer.Add(
            self.distr2Txt, pos=(5, 1),
            flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.panelSizer.Hide(self.distr2Txt)

        self.typeBox.Bind(wx.EVT_RADIOBOX, self.OnType)
        self.distributionBox.Bind(wx.EVT_RADIOBOX, self.OnDistr)
        self.widthTxt.Bind(wx.EVT_TEXT, self.OnWidth)
        self.heightTxt.Bind(wx.EVT_TEXT, self.OnHeight)
        self.distr1Txt.Bind(wx.EVT_TEXT, self.OnDistr1)
        self.distr2Txt.Bind(wx.EVT_TEXT, self.OnDistr2)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.sizer.Add(self.scrollPanel, pos=(0, 0), flag=wx.EXPAND)
        self.sizer.AddGrowableCol(0)
        self.sizer.AddGrowableRow(0)
        self.scrollPanel.SetSizer(self.panelSizer)
        #self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnExitPage)
        self.OnType(None)

    def OnEnterPage(self, event=None):
        """Function during entering"""
        # This is an hack to force the user to choose Rectangle or Circle
        self.typeBox.SetSelection(2),
        self.typeBox.ShowItem(2, False)
        self.panelSizer.Layout()

    def OnExitPage(self, event=None):
        """Function during exiting"""
        if event.GetDirection():
            self.SetNext(self.parent.summarypage)
            self.SetPrev(self.parent.samplingareapage)

    def OnType(self, event):
        """Set if rectangle or circle will be used"""
        chosen = self.typeBox.GetSelection()
        self.widthLabel.SetLabel(self.widthLabels[chosen])
        self.heightLabel.SetLabel(self.heightLabels[chosen])
        self.panelSizer.Layout()
        if chosen == 0:
            self.boxtype = 'rectangle'
        else:
            self.boxtype = 'circle'

    def OnDistr(self, event):
        chosen = self.distributionBox.GetSelection()
        if chosen == 0:
            self.distrtype = 'non_overlapping'
            self.distr1Label.SetLabel(
                _("What number of Sampling Units to use?"))
            self.panelSizer.Show(self.distr1Txt)
            self.distr2Label.SetLabel("")
            self.panelSizer.Hide(self.distr2Txt)
            self.panelSizer.Layout()
        elif chosen == 1:
            self.distrtype = 'systematic_contiguos'
            self.distr1Label.SetLabel("")
            self.distr2Label.SetLabel("")
            self.panelSizer.Hide(self.distr1Txt)
            self.panelSizer.Hide(self.distr2Txt)
            self.panelSizer.Layout()
        elif chosen == 2:
            self.distrtype = 'stratified_random'
            self.distr1Label.SetLabel(_("Insert number of row strates"))
            self.distr2Label.SetLabel(_("Insert number of column strates"))
            self.panelSizer.Show(self.distr1Txt)
            self.panelSizer.Show(self.distr2Txt)
            self.panelSizer.Layout()
        elif chosen == 3:
            self.distrtype = 'systematic_noncontiguos'
            self.distr1Label.SetLabel(_("Insert distance between units"))
            self.panelSizer.Show(self.distr1Txt)
            self.distr2Label.SetLabel("")
            self.panelSizer.Hide(self.distr2Txt)
            self.panelSizer.Layout()
        elif chosen == 4:
            self.distrtype = 'centered_oversites'
            self.distr1Label.SetLabel("")
            self.distr2Label.SetLabel("")
            self.panelSizer.Hide(self.distr2Txt)
            self.panelSizer.Hide(self.distr1Txt)
            self.panelSizer.Layout()

    def OnWidth(self, event):
        self.width = self.widthTxt.GetValue()

    def OnHeight(self, event):
        self.height = self.heightTxt.GetValue()

    def OnDistr1(self, event):
        self.distr1 = self.distr1Txt.GetValue()

    def OnDistr2(self, event):
        self.distr2 = self.distr2Txt.GetValue()


class MovingKeyPage(TitledPage):
    """Set values from keyboard for sample units"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Set sample units"))

        self.parent = parent
        self.width = ''
        self.height = ''
        self.boxtype = 'rectangle'
        # type of shape
        self.typeBox = wx.RadioBox(parent=self, id=wx.ID_ANY,
                                   label=" %s " % _("Select type of shape"),
                                   choices=[_('Rectangle'), _('Circle')],
                                   #                                               ('None')],
                                   majorDimension=1,
                                   style=wx.RA_SPECIFY_COLS)

        self.sizer.Add(self.typeBox, flag=wx.ALIGN_LEFT, pos=(1, 1))

        self.typeBox.Bind(wx.EVT_RADIOBOX, self.OnType)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        self.widthLabel = StaticText(parent=self, id=wx.ID_ANY,
                                        label=_('Width size (in cells)?'))
        self.widthTxt = TextCtrl(parent=self, id=wx.ID_ANY, size=(250, -1))
        self.sizer.Add(self.widthLabel, border=5, pos=(2, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.widthTxt, border=5, pos=(2, 2),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.heightLabel = StaticText(parent=self, id=wx.ID_ANY,
                                         label=_('Height size (in cells)?'))
        self.heightTxt = TextCtrl(parent=self, id=wx.ID_ANY, size=(250, -1))
        self.sizer.Add(self.heightLabel, border=5, pos=(3, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.heightTxt, border=5, pos=(3, 2),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        self.sizer.AddGrowableCol(2)
        self.widthLabels = [_('Width size (in cells)?'),
                            _('What radius size (in meters)?')]
        self.heightLabels = [_('Height size (in cells)?'),
                             _('Name of the circle mask')]

        self.widthTxt.Bind(wx.EVT_TEXT, self.OnWidth)
        self.heightTxt.Bind(wx.EVT_TEXT, self.OnHeight)
        wx.FindWindowById(wx.ID_FORWARD).Enable(False)

    def OnEnterPage(self, event):
        # This is an hack to force the user to choose Rectangle or Circle
        #        self.typeBox.SetSelection(2),
        #        self.typeBox.ShowItem(2, False)
        if self.parent.samplingareapage.samplingtype == SamplingType.MVWIN:
            self.title.SetLabel(_("Set moving windows"))
        self.OnType(None)

    def OnType(self, event):
        chosen = self.typeBox.GetSelection()
        self.widthLabel.SetLabel(self.widthLabels[chosen])
        self.heightLabel.SetLabel(self.heightLabels[chosen])
        self.sizer.Layout()
        if chosen == 0:
            self.parent.samplingareapage.samplingtype = SamplingType.KMVWINR
            self.boxtype = 'rectangle'
        elif chosen == 1:
            self.parent.samplingareapage.samplingtype = SamplingType.KMVWINC
            self.boxtype = 'circle'

    def CheckInput(self):
        """Check input fields.

        :return: True if configuration file is given and raster xor
                 vector map, False otherwise
        """
        return bool(self.width and bool(self.height))

    def OnWidth(self, event):
        self.width = self.widthTxt.GetValue()
        next = wx.FindWindowById(wx.ID_FORWARD)
        next.Enable(self.CheckInput())

    def OnHeight(self, event):
        self.height = self.heightTxt.GetValue()
        next = wx.FindWindowById(wx.ID_FORWARD)
        next.Enable(self.CheckInput())


class UnitsMousePage(TitledPage):
    """Choose the sampling area setting the values using the mouse drawing the
       areas with rectangle or circle. It is used both from 'Moving windows'
       and 'Sample units'.
    """

    def __init__(self, wizard, parent):
        self.parent = parent
        self.wizard = wizard
        TitledPage.__init__(self, self.wizard, _("Draw sampling units"))
        self.numregions = ''
        self.mapPanel = None
        # type of shape
        self.typeBox = wx.RadioBox(parent=self, id=wx.ID_ANY,
                                   majorDimension=1, style=wx.RA_SPECIFY_COLS,
                                   label=" %s " % _("Select type of shape"),
                                   choices=[_('Rectangle'), _('Circle'), ('')])
        # This is an hack to force the user to choose Rectangle or Circle
        self.typeBox.SetSelection(2),
        self.typeBox.ShowItem(2, False)
        self.sizer.Add(self.typeBox, flag=wx.ALIGN_LEFT, pos=(0, 0),
                       span=(1, 2))

        self.regionPanelSizer = wx.GridBagSizer(1, 2)
        self.regionNumPanel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.regionNumLabel = StaticText(
            parent=self.regionNumPanel, id=wx.ID_ANY,
            label=_('Number of sampling area to draw:'))
        self.regionNumTxt = TextCtrl(parent=self.regionNumPanel,
                                     id=wx.ID_ANY, size=(50, -1))
        self.regionPanelSizer.Add(self.regionNumLabel, flag=wx.ALIGN_CENTER,
                                  pos=(0, 0))
        self.regionPanelSizer.Add(self.regionNumTxt, flag=wx.ALIGN_CENTER,
                                  pos=(0, 1))

        self.regionNumPanel.SetSizer(self.regionPanelSizer)

        self.sizer.Add(self.regionNumPanel, flag=wx.ALIGN_LEFT, pos=(1, 0),
                       span=(1, 2))

        self.typeBox.Bind(wx.EVT_RADIOBOX, self.OnType)
        self.regionNumTxt.Bind(wx.EVT_TEXT, self.OnNumRegions)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.sizer.AddGrowableCol(0)
        self.OnType(None)
        self.regionNumTxt.SetValue('')

    def OnEnterPage(self, event):
        """Function during entering"""
        if self.numregions:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        if self.parent.samplingareapage.samplingtype in [SamplingType.MVWIN,
                                                         SamplingType.MMVWINR,
                                                         SamplingType.MMVWINC]:
            self.title.SetLabel(_("Draw moving windows region"))
            self.sizer.Hide(self.regionNumPanel)
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
        elif self.parent.samplingareapage.samplingtype in [SamplingType.UNITS,
                                                           SamplingType.MUNITSR,
                                                           SamplingType.MUNITSC]:
            self.title.SetLabel(_("Draw sampling region"))
            self.sizer.Show(self.regionNumPanel)
        if self.typeBox.GetSelection() == 2:
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        self.sizer.Layout()

    def OnType(self, event):
        chosen = self.typeBox.GetSelection()
        if chosen == 0:
            if self.parent.samplingareapage.samplingtype in [
                    SamplingType.MVWIN, SamplingType.MMVWINR, SamplingType.MMVWINC]:
                self.parent.samplingareapage.samplingtype = SamplingType.MMVWINR
                wx.FindWindowById(wx.ID_FORWARD).Enable(True)
            else:
                self.parent.samplingareapage.samplingtype = SamplingType.MUNITSR
            self.drawtype = 'rectangle'
        else:
            if self.parent.samplingareapage.samplingtype in [
                    SamplingType.MVWIN, SamplingType.MMVWINR, SamplingType.MMVWINC]:
                self.parent.samplingareapage.samplingtype = SamplingType.MMVWINC
                wx.FindWindowById(wx.ID_FORWARD).Enable(True)
            else:
                self.parent.samplingareapage.samplingtype = SamplingType.MUNITSC
            self.drawtype = 'circle'

    def OnNumRegions(self, event):
        """Set the number of region"""
        if self.regionNumTxt.GetValue():
            self.SetNext(self.parent.drawsampleunitspage)
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
            self.numregions = self.regionNumTxt.GetValue()
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)

    def OnExitPage(self, event=None):
        """Function during exiting"""
        if event.GetDirection():
            self.SetNext(self.drawsampleunitspage)
            self.SetPrev(self.samplingareapage)


class DrawSampleUnitsPage(TitledPage):
    """Choose the sampling area drawing them"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Draw sampling units"))
        self.parent = parent
        self.mapPanel = None
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        #self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnExitPage)

    def SampleFrameChanged(self, region):
        #region = self.GetSampleUnitRegion()
        if region:
            self.parent.msAreaList.append(region)
        self.regioncount = self.regioncount + 1

        drawtype = self.parent.drawunits.drawtype
        if self.regioncount > self.numregions:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
            self.parent.wizard.ShowPage(self.parent.summarypage)
        else:
            self.title.SetLabel(_('Draw Sampling ' + drawtype + ' '
                                  + str(self.regioncount) + ' of '
                                  + str(self.numregions)))
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)

    def OnEnterPage(self, event):
        """Function during entering"""

        if self.parent.samplingareapage.samplingtype in [SamplingType.MVWIN,
                                                         SamplingType.MMVWINC,
                                                         SamplingType.MMVWINR]:
            self.numregions = 1
        else:
            self.numregions = int(self.parent.drawunits.numregions)
        self.regioncount = 0
        if self.mapPanel:
            self.sizer.Remove(self.mapPanel)

        gtype = self.parent.drawunits.drawtype
        self.mapPanel = RLiSetupMapPanel(
            self, samplingType=self.parent.samplingareapage.samplingtype, )
        if gtype == 'circle':
            self.mapPanel.afterCircleDrawn.connect(self.SampleFrameChanged)
        else:
            self.mapPanel.sampleFrameChanged.connect(self.SampleFrameChanged)

        self.sizer.Add(self.mapPanel, flag=wx.EXPAND, pos=(1, 0))
        self.sizer.AddGrowableCol(0)
        self.sizer.AddGrowableRow(1)
        self._raster = None

        self.SampleFrameChanged(region=None)
        rast = self.parent.startpage.rast

        if self._raster != rast:
            map_ = self.mapPanel.GetMap()
            map_.DeleteAllLayers()
            cmdlist = ['d.rast', 'map=%s' % rast]
            map_.AddLayer(ltype='raster', command=cmdlist, active=True,
                          name=rast, hidden=False, opacity=1.0, render=True)

    def OnExitPage(self, event=None):
        #!Function during exiting
        pass

        # if event.GetDirection():
        #    self.SetNext(self.parent.samplingareapage)
        #    self.parent.samplingareapage.SetPrev(self)


class VectorAreasPage(TitledPage):
    """Choose the sampling area using the vector features"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Select sampling areas"))
        self.parent = parent
        self.areascount = 0
        self.mapPanel = None
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnExitPage)
        self.areaPanelSizer = wx.GridBagSizer(1, 3)
        self.areaPanel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.areaText = StaticText(parent=self.areaPanel, id=wx.ID_ANY,
                                      label=_('Is this area ok?'))
        self.areaOK = Button(self.areaPanel, wx.ID_ANY, 'Yes', (50, 80))
        self.areaNO = Button(self.areaPanel, wx.ID_ANY, 'No', (50, 80))
        self.areaOK.Bind(wx.EVT_BUTTON, self.OnYes)
        self.areaNO.Bind(wx.EVT_BUTTON, self.OnNo)
        self.areaPanelSizer.Add(self.areaText, flag=wx.ALIGN_CENTER,
                                pos=(0, 0))
        self.areaPanelSizer.Add(self.areaOK, flag=wx.ALIGN_CENTER,
                                pos=(0, 1))
        self.areaPanelSizer.Add(self.areaNO, flag=wx.ALIGN_CENTER,
                                pos=(0, 2))
        self.areaPanel.SetSizer(self.areaPanelSizer)
        self.sizer.Add(self.areaPanel, flag=wx.ALIGN_CENTER, pos=(2, 0))

    def afterRegionDrawn(self):
        """Function to update the title and the number of selected area"""
        self.areascount = self.areascount + 1
        if self.areascount == self.areanum:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
            self.areaOK.Enable(False)
            self.areaNO.Enable(False)
            return True
        else:
            self.title.SetLabel(_('Select sample area ' + str(self.areascount + 1)
                                  + ' of ' + str(self.areanum)))
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
            return False

    def OnYes(self, event):
        """Function to create the string for the conf file if the area
        is selected"""
        self.parent.msAreaList.append(obtainAreaVector(self.outname))
        if not self.afterRegionDrawn():
            self.newCat()

    def OnNo(self, event):
        """Function to pass to the next feature if it is not selected"""
        if not self.afterRegionDrawn():
            self.newCat()

    def newCat(self):
        """Convert to raster and draw the new feature"""
        cat = self.vect_cats[self.areascount]
        self.outpref = "{rast}_{vect}_".format(vect=self.vect.split('@')[0],
                                               rast=self.rast.split('@')[0])
        self.outname = "{pref}{cat}".format(pref=self.outpref, cat=cat)
        # check if raster already axist

        if len(grass.list_strings('raster', pattern=self.outname, mapset='.')
               ) == 1 and not self.parent.samplingareapage.overwriteTemp:
            GError(parent=self, message=_("The raster map <%s> already exists."
                                          " Please remove or rename the maps "
                                          "with the prefix '%s' or select the "
                                          "option to overwrite existing maps"
                                          % (self.outname, self.outpref)))
            self.parent.wizard.ShowPage(self.parent.samplingareapage)
            return
        convertFeature(self.vect, self.outname, cat, self.rast,
                       self.parent.startpage.vectorlayer,
                       self.parent.samplingareapage.overwriteTemp)
        cmdlistcat = ['d.rast', 'map=%s' % self.outname]
        self.map_.AddLayer(ltype='raster', command=cmdlistcat, active=True,
                           name=self.outname, hidden=False, opacity=1.0,
                           render=True)
        for l in self.map_.GetListOfLayers():
            if l.name == self.outname:
                self.mapPanel.mapWindow.ZoomToMap(layers=[l], render=True,
                                                  ignoreNulls=True)
            elif l.name != self.rast:
                self.map_.DeleteLayer(l)
        self.areaText.SetLabel("Is this area (cat={n}) ok?".format(n=cat))

    def OnEnterPage(self, event):
        """Function during entering: draw the raster map and the first vector
        feature"""
        if self.mapPanel is None:
            self.mapPanel = RLiSetupMapPanel(
                self, samplingType=self.parent.samplingareapage.samplingtype)
            self.sizer.Add(self.mapPanel, flag=wx.EXPAND, pos=(1, 0))
            self.sizer.AddGrowableCol(0)
            self.sizer.AddGrowableRow(1)
            self._raster = None

        self.rast = self.parent.startpage.rast
        self.vect = self.parent.startpage.vect
        self.vect_cats = obtainCategories(
            self.vect, layer=self.parent.startpage.vectorlayer)

        self.areanum = len(self.vect_cats)
        if self.areanum == 0:
            GError(parent=self, message=_("The polygon seems to have 0 areas"))
            self.parent.wizard.ShowPage(self.parent.samplingareapage)
            return
        self.title.SetLabel(_('Select sample area 1 of ' + str(self.areanum)))
        grass.use_temp_region()
        if self._raster != self.rast:
            self.map_ = self.mapPanel.GetMap()
            self.map_.DeleteAllLayers()
            cmdlist = ['d.rast', 'map=%s' % self.rast]
            self.map_.AddLayer(ltype='raster', command=cmdlist, active=True,
                               name=self.rast, hidden=False, opacity=1.0,
                               render=True)
        self.newCat()

    def OnExitPage(self, event=None):
        """Function during exiting"""
        grass.del_temp_region()
#        if event.GetDirection():
#            self.SetNext(self.parent.samplingareapage)
#            self.parent.samplingareapage.SetPrev(self)


class SummaryPage(TitledPage):
    """Shows summary result of choosing data"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Summary"))
        global rlisettings

        self.parent = parent

        # configuration file name
        self.conflabel = StaticText(parent=self, id=wx.ID_ANY,
                                    label=_('Configuration file name:'))
        self.conftxt = StaticText(parent=self, id=wx.ID_ANY,
                                  label="")
        self.sizer.Add(self.conflabel, border=5, pos=(0, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.conftxt, border=5, pos=(0, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        # raster name
        self.rastlabel = StaticText(parent=self, id=wx.ID_ANY,
                                    label=_('Raster name:'))
        self.rasttxt = StaticText(parent=self, id=wx.ID_ANY,
                                  label="")
        self.sizer.Add(self.rastlabel, border=5, pos=(1, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.rasttxt, border=5, pos=(1, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        # vector name
        self.vectlabel = StaticText(parent=self, id=wx.ID_ANY,
                                    label=_('Vector name:'))
        self.vecttxt = StaticText(parent=self, id=wx.ID_ANY, label="")
        self.sizer.Add(self.vectlabel, border=5, pos=(2, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.vecttxt, border=5, pos=(2, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        # region type name
        self.regionlabel = StaticText(parent=self, id=wx.ID_ANY,
                                      label=_('Region type:'))
        self.regiontxt = StaticText(parent=self, id=wx.ID_ANY, label="")
        self.sizer.Add(self.regionlabel, border=5, pos=(3, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.regiontxt, border=5, pos=(3, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        # region keyboard
        self.regionkeylabel = StaticText(parent=self, id=wx.ID_ANY,
                                         label="")
        self.regionkeytxt = StaticText(parent=self, id=wx.ID_ANY, label="")
        self.sizer.Add(self.regionkeylabel, border=5, pos=(4, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.regionkeytxt, border=5, pos=(4, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        # sampling area
        self.samplinglabel = StaticText(parent=self, id=wx.ID_ANY,
                                        label=_('Sampling area type:'))
        self.samplingtxt = StaticText(parent=self, id=wx.ID_ANY, label="")
        self.sizer.Add(self.samplinglabel, border=5, pos=(5, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.samplingtxt, border=5, pos=(5, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        # shapetype
        self.shapelabel = StaticText(parent=self, id=wx.ID_ANY, label="")
        self.shapetxt = StaticText(parent=self, id=wx.ID_ANY, label="")
        self.sizer.Add(self.shapelabel, border=5, pos=(6, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.shapetxt, border=5, pos=(6, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        # shapedim
        self.shapewidthlabel = StaticText(parent=self, id=wx.ID_ANY,
                                          label="")
        self.shapewidthtxt = StaticText(parent=self, id=wx.ID_ANY,
                                        label="")
        self.sizer.Add(self.shapewidthlabel, border=5, pos=(7, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.shapewidthtxt, border=5, pos=(7, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.shapeheightlabel = StaticText(parent=self, id=wx.ID_ANY,
                                           label="")
        self.shapeheighttxt = StaticText(parent=self, id=wx.ID_ANY,
                                         label="")
        self.sizer.Add(self.shapeheightlabel, border=5, pos=(8, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.shapeheighttxt, border=5, pos=(8, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        # units type
        self.unitslabel = StaticText(parent=self, id=wx.ID_ANY, label="")
        self.unitstxt = StaticText(parent=self, id=wx.ID_ANY, label="")
        self.sizer.Add(self.unitslabel, border=5, pos=(9, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.unitstxt, border=5, pos=(9, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.unitsmorelabel = StaticText(parent=self, id=wx.ID_ANY,
                                         label="")
        self.unitsmoretxt = StaticText(parent=self, id=wx.ID_ANY, label="")
        self.sizer.Add(self.unitsmorelabel, border=5, pos=(10, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.unitsmoretxt, border=5, pos=(10, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.unitsmorelabel2 = StaticText(parent=self, id=wx.ID_ANY,
                                          label="")
        self.unitsmoretxt2 = StaticText(parent=self, id=wx.ID_ANY, label="")
        self.sizer.Add(self.unitsmorelabel2, border=5, pos=(11, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(self.unitsmoretxt2, border=5, pos=(11, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

    def OnEnterPage(self, event):
        """Insert values into text controls for summary of location
        creation options
        """
        self.conftxt.SetLabel(self.parent.startpage.conf_name)
        self.rasttxt.SetLabel(self.parent.startpage.rast)
        self.samplingtxt.SetLabel(self.parent.samplingareapage.samplingtype)
        self.regiontxt.SetLabel(self.parent.startpage.region)
        self.vecttxt.SetLabel(self.parent.startpage.vect)
        if self.parent.startpage.region == 'key':
            # region keyboard values
            self.regionkeylabel.SetLabel(_('Region keyboard values:'))
            regKeyVals = "Column left up: %s - Row left up: %s\n" \
                         "Column length: %s - Row length: %s\n" % (
                             self.parent.keyboardpage.col_up,
                             self.parent.keyboardpage.row_up,
                             self.parent.keyboardpage.col_len,
                             self.parent.keyboardpage.row_len)
            self.regionkeytxt.SetLabel(regKeyVals)
        else:
            self.regionkeylabel.SetLabel("")
            self.regionkeytxt.SetLabel("")

        if self.parent.samplingareapage.samplingtype == SamplingType.UNITS \
                and self.parent.samplingareapage.regionbox == 'keyboard':
            self.shapelabel.SetLabel(_('Type of shape:'))
            self.shapetxt.SetLabel(self.parent.units.boxtype)
            if self.parent.units.boxtype == 'circle':
                self.shapewidthlabel.SetLabel(_("Radius size:"))
                self.shapeheightlabel.SetLabel(_("Name circle mask:"))
            else:
                self.shapewidthlabel.SetLabel(_("Width size:"))
                self.shapeheightlabel.SetLabel(_("Height size:"))
            self.shapewidthtxt.SetLabel(self.parent.units.width)
            self.shapeheighttxt.SetLabel(self.parent.units.height)
            self.unitslabel.SetLabel(_("Method of distribution:"))
            self.unitstxt.SetLabel(self.parent.units.distrtype)
            if self.parent.units.distrtype == 'non_overlapping':
                self.unitsmorelabel.SetLabel(_("Number sampling units:"))
                self.unitsmoretxt.SetLabel(self.parent.units.distr1)
            elif self.parent.units.distrtype == 'systematic_noncontiguos':
                self.unitsmorelabel.SetLabel(_("Distance between units:"))
                self.unitsmoretxt.SetLabel(self.parent.units.distr1)
            elif self.parent.units.distrtype == 'stratified_random':
                self.unitsmorelabel.SetLabel(_("Number row strates:"))
                self.unitsmoretxt.SetLabel(self.parent.units.distr1)
                self.unitsmorelabel2.SetLabel(_("Number column strates:"))
                self.unitsmoretxt2.SetLabel(self.parent.units.distr2)
        elif self.parent.samplingareapage.samplingtype == SamplingType.UNITS \
                and self.parent.samplingareapage.regionbox == 'mouse':
            self.shapelabel.SetLabel(_('Type of shape:'))
            self.shapetxt.SetLabel(self.parent.units.boxtype)
            self.unitstxt.SetLabel('by mouse')
        elif self.parent.samplingareapage.samplingtype == 'moving':
            self.shapelabel.SetLabel(_('Type of shape:'))
            self.shapetxt.SetLabel(self.parent.moving.boxtype)
            if self.parent.moving.boxtype == 'circle':
                self.shapewidthlabel.SetLabel(_("Radius size:"))
                self.shapeheightlabel.SetLabel(_("Name circle mask:"))
            else:
                self.shapewidthlabel.SetLabel(_("Width size:"))
                self.shapeheightlabel.SetLabel(_("Height size:"))
            self.shapewidthtxt.SetLabel(self.parent.moving.width)
            self.shapeheighttxt.SetLabel(self.parent.moving.height)
        else:
            self.shapelabel.SetLabel("")
            self.shapetxt.SetLabel("")
            self.shapewidthlabel.SetLabel("")
            self.shapewidthtxt.SetLabel("")
            self.shapeheightlabel.SetLabel("")
            self.shapeheighttxt.SetLabel("")
