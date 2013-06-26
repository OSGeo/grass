"""
@package rlisetup.py

@brief   GUI per r.li.setup module

Classes:
 - RLiSetupFrame (first frame to show existing conf file and choose some
                 operation)
 - RLIWizard (the main wizard)
 - FirstPage (first page of wizard, choose name of conf file, raster, vector,
              sampling region)
 - Keybord (page to insert region areas from keybord)
 - SamplingAreas (define sampling area)
 - SummaryPage (show choosen options)

(C) 2011 by the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Luca Delucchi <lucadeluge gmail com>
"""

import sys
import os

wxbase = os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython')
if wxbase not in sys.path:
    sys.path.append(wxbase)

import wx
import wx.wizard as wiz
import wx.lib.scrolledpanel as scrolled

from gui_core import gselect
from core import gcmd
from location_wizard.wizard import TitledPage as TitledPage
from rlisetup.functions import checkValue, retRLiPath
from grass.script import core as grass
from grass.script import raster as grast


#@NOTE: r.li.setup writes in the settings file with
## r.li.windows.tcl:
#exec echo "SAMPLINGFRAME $per_x|$per_y|$per_rl|$per_cl" >> $env(TMP).set


class RLIWizard(object):
    """
    Start wizard here and finish wizard here
    """

    def __init__(self, parent):
        self.parent = parent
        self.wizard = wiz.Wizard(parent=parent, id=wx.ID_ANY,
                                 title=_("Create new configuration file for " \
                                 "r.li modules"))
        self.rlipath = retRLiPath()
        #pages
        self.startpage = FirstPage(self.wizard, self)
        self.keyboardpage = KeybordPage(self.wizard, self)
        self.samplingareapage = SamplingAreasPage(self.wizard, self)
        self.summarypage = SummaryPage(self.wizard, self)
        self.units = SampleUnitsKeyPage(self.wizard, self)
        self.moving = MovingWindows(self.wizard, self)

        #order of pages
        self.startpage.SetNext(self.samplingareapage)
        self.keyboardpage.SetPrev(self.startpage)
        self.keyboardpage.SetNext(self.samplingareapage)
        self.samplingareapage.SetPrev(self.startpage)
        self.samplingareapage.SetNext(self.summarypage)
        self.units.SetPrev(self.samplingareapage)
        self.units.SetNext(self.summarypage)
        self.moving.SetPrev(self.samplingareapage)
        self.moving.SetNext(self.summarypage)
        self.summarypage.SetPrev(self.samplingareapage)

        #layout
        self.startpage.DoLayout()
        self.keyboardpage.DoLayout()
        self.samplingareapage.DoLayout()
        self.summarypage.DoLayout()
        self.units.DoLayout()
        self.moving.DoLayout()

        self.wizard.FitToPage(self.startpage)
        #run_wizard
        if self.wizard.RunWizard(self.startpage):
            dlg = wx.MessageDialog(parent=self.parent,
                                message=_("Do you want to create r.li " \
                                          "configuration file <%s>?") % self.startpage.conf_name,
                                caption=_("Create new r.li configuration file?"),
                                style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)

            if dlg.ShowModal() == wx.ID_NO:
                self._cleanup()
                dlg.Destroy()
            else:
                self._write_confile()
                self._cleanup()
                dlg.Destroy()
        else:
            self.wizard.Destroy()
            gcmd.GMessage(parent=self.parent,
                     message=_("r.li.setup wizard canceled. "
                                "Configuration file not created."))
            self._cleanup()

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
        grass.run_command('g.region', rast=self.startpage.rast)
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
            self.SF_S = self.gregion['n'] - (self.SF_NSRES * self.SF_Y + self.SF_RL)
            self.SF_W = self.gregion['w'] + (self.SF_EWRES * self.SF_X)
            self.SF_E = self.gregion['w'] + (self.SF_EWRES * self.SF_X + self.SF_CL)
            self.per_x = float(self.SF_X) / float(self.rasterinfo['cols'])
            self.per_y = float(self.SF_Y) / float(self.rasterinfo['rows'])
            self.per_rl = float(self.SF_RL) / float(self.rasterinfo['rows'])
            self.per_cl = float(self.SF_CL) / float(self.rasterinfo['cols'])
            #import pdb; pdb.set_trace()
            fil.write("SAMPLINGFRAME %r|%r|%r|%r\n" % (self.per_x, self.per_y,
                                                       self.per_rl, self.per_cl))

    def _circle(self, radius, mask):
        """create a circle mask"""
        self.CIR_RL = round((2 * float(radius)) / float(self.rasterinfo['ewres']))
        self.CIR_CL = round((2 * float(radius)) / float(self.rasterinfo['nsres']))
        if not self.CIR_RL % 2:
            self.CIR_RL += 1
        if not self.CIR_CL % 2:
            self.CIR_CL += 1
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
        grass.run_command('g.region', rast=self.startpage.rast)

    def _write_area(self, fil):
        """Write the region"""
        if self.samplingareapage.samplingtype == 'whole':
            cl = float(self.SF_CL) / float(self.rasterinfo['cols'])
            rl = float(self.SF_RL) / float(self.rasterinfo['rows'])
            #this two variable are unused, problably to remove
            x = float(self.SF_X) / float(self.rasterinfo['cols'])            
            y = float(self.SF_Y) / float(self.rasterinfo['rows'])            
            if self.startpage.region == 'whole':
                fil.write("SAMPLEAREA %r|%r|%r|%r\n" % (self.per_x, self.per_y,
                                                        rl, cl))
            elif self.startpage.region == 'key':
                fil.write("SAMPLEAREA %r|%r|%r|%r\n" % (self.per_x, self.per_y,
                                                        rl, cl))
        elif self.samplingareapage.samplingtype == 'moving':
            if self.moving.boxtype == 'circle':
                self._circle(self.moving.width, self.moving.height)
                cl = float(self.CIR_CL) / float(self.rasterinfo['cols'])
                rl = float(self.CIR_RL) / float(self.rasterinfo['rows'])
            else:
                cl = float(self.moving.width) / float(self.rasterinfo['cols'])
                rl = float(self.moving.height) / float(self.rasterinfo['rows'])
            fil.write("SAMPLEAREA -1|-1|%r|%r" % (rl, cl))
            if self.moving.boxtype == 'circle':
                fil.write("|%s" % self.moving.height)
            fil.write("\nMOVINGWINDOW\n")
        elif self.samplingareapage.samplingtype == 'units':
            if self.units.boxtype == 'circle':
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
        self.moving.width = ''
        self.moving.height = ''
        self.moving.boxtype = ''


class FirstPage(TitledPage):
    """
    Set name of configuration file, choose raster and optionally vector/sites
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Select maps and define name"))

        self.region = 'whole'
        self.rast = ''
        self.conf_name = ''
        self.vect = ''

        self.parent = parent

        #name of output configuration file
        self.newconflabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                        label=_('Name for new configuration file to create'))

        self.newconftxt = wx.TextCtrl(parent=self, id=wx.ID_ANY,
                                      size=(250, -1))
        wx.CallAfter(self.newconftxt.SetFocus)

        self.sizer.Add(item=self.newconflabel, border=5, pos=(0, 0),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.newconftxt, border=5, pos=(0, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        #raster
        self.mapsellabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                        label=_('Raster map to use to select areas'))
        self.mapselect = gselect.Select(parent=self, id=wx.ID_ANY,
                                        size=(250, -1), type='cell',
                                        multiple=False)
        self.sizer.Add(item=self.mapsellabel, border=5, pos=(1, 0),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.mapselect, border=5, pos=(1, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        #vector
        self.vectsellabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                        label=_('Vector map to use to select areas'))
        self.vectselect = gselect.Select(parent=self, id=wx.ID_ANY,
                                         size=(250, -1), type='vector',
                                         multiple=False)
        self.vectsellabel.Enable(False)
        self.vectselect.Enable(False)
        self.sizer.Add(item=self.vectsellabel, border=5, pos=(2, 0),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.vectselect, border=5, pos=(2, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        #define sampling region
        self.sampling_reg = wx.RadioBox(parent=self, id=wx.ID_ANY,
                                      label=" %s " % _("Define sampling " \
                                      " region (region for analysis)"),
                                      choices=[_('Whole map layer'),
                                               _('Keyboard setting'),
                                               _('Draw the sampling frame')],
                                      majorDimension=1,
                                      style=wx.RA_SPECIFY_ROWS)

        self.sampling_reg.EnableItem(2, False)  # disable 'draw' for now
        self.sampling_reg.SetItemToolTip(2, _("This option is not supported yet"))
        self.sizer.Add(item=self.sampling_reg,
                        flag=wx.ALIGN_CENTER | wx.ALL | wx.EXPAND, border=5,
                        pos=(4, 0), span=(1, 2))
        #bindings
        self.sampling_reg.Bind(wx.EVT_RADIOBOX, self.OnSampling)
        self.newconftxt.Bind(wx.EVT_KILL_FOCUS, self.OnName)
        self.newconftxt.Bind(wx.EVT_TEXT, self.OnNameChanged)
        self.vectselect.Bind(wx.EVT_TEXT, self.OnVector)
        self.mapselect.Bind(wx.EVT_TEXT, self.OnRast)
        #self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnExitPage)

        wx.CallAfter(wx.FindWindowById(wx.ID_FORWARD).Enable, False)
        # wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        # self.OnName(None)

    def OnSampling(self, event):
        """!Change map type"""
        if event.GetInt() == 0:
            self.region = 'whole'
            self.SetNext(self.parent.samplingareapage)
        elif event.GetInt() == 1:
            self.region = 'key'
            self.SetNext(self.parent.keyboardpage)
        elif event.GetInt() == 2: # currently disabled
            self.region = 'draw'

    def OnName(self, event):
        """!Sets the name of configuration file"""
        if self.conf_name in self.parent.parent.listfiles:
            gcmd.GMessage(parent=self,
                          message=_("The configuration file %s "
                                    "already exists, please change name") % self.conf_name)
            self.newconftxt.SetValue('')
            self.conf_name = ''

    def OnNameChanged(self, event):
        """!Name of configuration file has changed"""
        self.conf_name = self.newconftxt.GetValue()
        next = wx.FindWindowById(wx.ID_FORWARD)
        next.Enable(self.CheckInput())

    def OnRast(self, event):
        """!Sets raster map"""
        self.rast = event.GetString()
        next = wx.FindWindowById(wx.ID_FORWARD)
        next.Enable(self.CheckInput())

    def OnVector(self, event):
        """!Sets vector map"""
        self.vect = event.GetString()
        next = wx.FindWindowById(wx.ID_FORWARD)
        next.Enable(self.CheckInput())

    def CheckInput(self):
        """!Check input fields.

        @return True if configuration file is given and raster xor vector map,
        False otherwise
        """
        return bool(self.conf_name and (bool(self.rast) != bool(self.vect)))

    def OnExitPage(self, event=None):
        """!Function during exiting"""
        if self.region == 'draw' or self.conf_name == '' or self.rast == '':
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
        if event.GetDirection():
            if self.region == 'key':
                self.SetNext(self.parent.keyboardpage)
                self.parent.samplingareapage.SetPrev(self.parent.keyboardpage)
            elif self.region == 'whole':
                self.SetNext(self.parent.samplingareapage)
                self.parent.samplingareapage.SetPrev(self)
            elif self.region == 'draw':
                gcmd.GMessage(parent=self,
                              message=_("Function not supported yet"))
                event.Veto()
                return


class KeybordPage(TitledPage):
    """
    Set name of configuration file, choose raster and optionally vector/sites
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Insert sampling frame parameter"))

        self.parent = parent
        self.sizer.AddGrowableCol(2)
        self.col_len = ''
        self.row_len = ''
        self.col_up = '0'
        self.row_up = '0'

        #column up/left
        self.ColUpLeftlabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                            label=_("Column of upper left " \
                                            "corner"))

        self.ColUpLefttxt = wx.TextCtrl(parent=self, id=wx.ID_ANY,
                                        size=(250, -1))
        wx.CallAfter(self.ColUpLeftlabel.SetFocus)

        self.sizer.Add(item=self.ColUpLeftlabel, border=5, pos=(1, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.ColUpLefttxt, border=5, pos=(1, 2),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        #row up/left
        self.RowUpLeftlabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                            label=_('Row of upper left corner'))

        self.RowUpLefttxt = wx.TextCtrl(parent=self, id=wx.ID_ANY,
                                        size=(250, -1))
        wx.CallAfter(self.RowUpLeftlabel.SetFocus)

        self.sizer.Add(item=self.RowUpLeftlabel, border=5, pos=(2, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.RowUpLefttxt, border=5, pos=(2, 2),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        #row lenght
        self.RowLenlabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                         label=_('Row lenght of sampling frame'))

        self.RowLentxt = wx.TextCtrl(parent=self, id=wx.ID_ANY, size=(250, -1))
        wx.CallAfter(self.RowLenlabel.SetFocus)

        self.sizer.Add(item=self.RowLenlabel, border=5, pos=(3, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.RowLentxt, border=5, pos=(3, 2),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        #column lenght
        self.ColLenlabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                         label=_('Row lenght of sampling frame'))

        self.ColLentxt = wx.TextCtrl(parent=self, id=wx.ID_ANY, size=(250, -1))
        wx.CallAfter(self.ColLenlabel.SetFocus)

        self.sizer.Add(item=self.ColLenlabel, border=5, pos=(4, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.ColLentxt, border=5, pos=(4, 2),
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
        """!Sets the name of configuration file"""
        self.col_up = self.ColUpLefttxt.GetValue()
        checkValue(self.col_up)

    def OnRowLeft(self, event):
        """!Sets the name of configuration file"""
        self.row_up = self.RowUpLefttxt.GetValue()
        checkValue(self.row_up)

    def OnColLen(self, event):
        """!Sets the name of configuration file"""
        self.col_len = self.ColLentxt.GetValue()
        checkValue(self.col_len)

    def OnRowLen(self, event):
        """!Sets the name of configuration file"""
        self.row_len = self.RowLentxt.GetValue()
        checkValue(self.row_len)

    def OnEnterPage(self, event):
        """!Sets the default values, for the entire map
        """
        if self.col_len == '' and self.row_len == '':
            rastinfo = grast.raster_info(self.parent.startpage.rast)
            self.col_len = rastinfo['cols']
            self.row_len = rastinfo['rows']

        self.ColLentxt.SetValue(self.col_len)
        self.RowLentxt.SetValue(self.row_len)

    def OnExitPage(self, event=None):
        """!Function during exiting"""
        if self.row_len == '' or self.col_len == '' or self.row_up == '' or self.col_up == '':
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)


class SamplingAreasPage(TitledPage):
    """
    Set name of configuration file, choose raster and optionally vector/sites
    """

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Insert sampling areas"))
        self.samplingtype = 'whole'
        self.parent = parent
        # toggles
        self.radioBox = wx.RadioBox(parent=self, id=wx.ID_ANY,
                                      label="",
                                      choices=[_("Whole map layer"),
                                               _("Regions"),
                                               _("Sample units"),
                                               _("Moving window"),
                                               _("Select areas from the\n"
                                                 "overlayed vector map")],
                                      majorDimension=1,
                                      style=wx.RA_SPECIFY_COLS | wx.NO_BORDER)
        self.radioBox.EnableItem(1, False)
        self.radioBox.SetItemToolTip(1, _("This option is not supported yet"))

        # layout
        self.sizer.SetVGap(10)
        self.sizer.Add(item=self.radioBox, flag=wx.ALIGN_LEFT, pos=(0, 0))

        self.regionBox = wx.RadioBox(parent=self, id=wx.ID_ANY,
                        label=_("Choose a method"),
                        choices=[_('Use keyboard to enter sampling area'),
                        _('Use mouse to draw sampling area')],
                        majorDimension=1,
                        style=wx.RA_SPECIFY_ROWS)
        self.regionBox.EnableItem(1, False)
        self.regionBox.SetItemToolTip(1, _("This option is not supported yet"))
        self.sizer.Add(self.regionBox, flag=wx.ALIGN_CENTER, pos=(1, 0))

        # bindings
        self.radioBox.Bind(wx.EVT_RADIOBOX, self.SetVal)
        self.regionBox.Bind(wx.EVT_RADIOBOX, self.OnRegionDraw)
        self.regionbox = 'keybord'

        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

    def OnEnterPage(self, event):
        """!Insert values into text controls for summary of location
        creation options
        """
        self.SetVal(None)
        if self.parent.startpage.vect:
            self.radioBox.ShowItem(4, True)
        else:
            self.radioBox.ShowItem(4, False)
        self.sizer.Layout()

        wx.FindWindowById(wx.ID_FORWARD).Enable(True)

    def SetVal(self, event):
        """!Choose method"""
        radio = self.radioBox.GetSelection()
        if radio == 0:
            self.samplingtype = 'whole'
            self.DrawNothing()
        # elif event.GetInt() == 1: # disabled
        # self.samplingtype = 'regions'
        # self.Region()
        elif radio == 2:
            self.samplingtype = 'units'
            self.SetNext(self.parent.units)
            self.KeyDraw()
        elif radio == 3:
            self.samplingtype = 'moving'
            self.SetNext(self.parent.moving)
            self.KeyDraw()
        elif radio == 4:
            self.samplingtype = 'vector'
            self.DrawNothing()

    def Region(self):
        """show this only if radio2 it is selected"""
        try:
            self.sizer.Hide(self.regionBox)
            self.sizer.Remove(self.regionBox)
            self.sizer.Layout()
        except:
            pass
        gcmd.GMessage(parent=self, message=_("Function not supported yet"))
        return

    def KeyDraw(self):
        """Show this only if radio3 and radio4 it is selected"""
        self.sizer.Show(self.regionBox)
        self.sizer.Layout()

    def OnRegionDraw(self, event):
        """Function called by KeyDraw to find what method is choosed"""
        if event.GetInt() == 0:
            self.regionbox = 'keybord'
            self.SetNext(self.parent.units)

        elif event.GetInt() == 1:
            self.regionbox = 'drawmouse'
            return
            #self.SetNext(self.parent.draw)

    def DrawNothing(self):
        """"""
        self.sizer.Hide(self.regionBox)
        self.sizer.Layout()

        self.SetNext(self.parent.summarypage)


class SampleUnitsKeyPage(TitledPage):
    """!Set values from keyboard for sample units"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Units"))

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
                                   choices=[_('Rectangle'), _('Circle')])

        self.panelSizer.Add(self.typeBox, flag=wx.ALIGN_LEFT, pos=(0, 0),
                            span=(1, 2))
        self.widthLabel = wx.StaticText(parent=self.scrollPanel, id=wx.ID_ANY)
        self.widthTxt = wx.TextCtrl(parent=self.scrollPanel, id=wx.ID_ANY,
                                    size=(250, -1))

        self.panelSizer.Add(item=self.widthLabel, pos=(1, 0),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.panelSizer.Add(item=self.widthTxt, pos=(1, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        self.heightLabel = wx.StaticText(parent=self.scrollPanel, id=wx.ID_ANY)
        self.heightTxt = wx.TextCtrl(parent=self.scrollPanel, id=wx.ID_ANY,
                                     size=(250, -1))

        self.panelSizer.Add(item=self.heightLabel, pos=(2, 0),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.panelSizer.Add(item=self.heightTxt, pos=(2, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.widthLabels = [_('Width size (in cells)? '),
                            _('What radius size (in meters)? ')]
        self.heightLabels = [_('Height size (in cells)? '),
                             _('Name of the circle mask')]

        self.distributionBox = wx.RadioBox(parent=self.scrollPanel,
                                           id=wx.ID_ANY,
                                           majorDimension=1,
                                           style=wx.RA_SPECIFY_COLS,
                                           label= " %s " % _("Select method of sampling unit distribution"),
                                           choices=[_('Random non overlapping'),
                                                    _('Systematic contiguos'),
                                                    _('Stratified random'),
                                                    _('Systematic non contiguos'),
                                                    _('Centered over sites')]
                                            )
        self.distributionBox.EnableItem(5, False)  # disable 'draw' for now
        self.panelSizer.Add(item=self.distributionBox, pos=(3, 0), span=(1, 2),
                    flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        self.distr1Label = wx.StaticText(parent=self.scrollPanel, id=wx.ID_ANY,
                                         label=_("What number of Sampling " \
                                         "Units to use?"))
        self.distr1Txt = wx.TextCtrl(parent=self.scrollPanel, id=wx.ID_ANY,
                                     size=(250, -1))
        self.panelSizer.Add(item=self.distr1Label, pos=(4, 0),
                    flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.panelSizer.Add(item=self.distr1Txt, pos=(4, 1),
                    flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.distr2Label = wx.StaticText(parent=self.scrollPanel, id=wx.ID_ANY,
                                         label="")
        self.distr2Txt = wx.TextCtrl(parent=self.scrollPanel, id=wx.ID_ANY,
                                     size=(250, -1))
        self.panelSizer.Add(item=self.distr2Label, pos=(5, 0),
                    flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.panelSizer.Add(item=self.distr2Txt, pos=(5, 1),
                    flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.panelSizer.Hide(self.distr2Txt)

        self.typeBox.Bind(wx.EVT_RADIOBOX, self.OnType)
        self.distributionBox.Bind(wx.EVT_RADIOBOX, self.OnDistr)
        self.widthTxt.Bind(wx.EVT_TEXT, self.OnWidth)
        self.heightTxt.Bind(wx.EVT_TEXT, self.OnHeight)
        self.distr1Txt.Bind(wx.EVT_TEXT, self.OnDistr1)
        self.distr2Txt.Bind(wx.EVT_TEXT, self.OnDistr2)
        #self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.sizer.Add(item=self.scrollPanel, pos=(0, 0), flag=wx.EXPAND)
        self.sizer.AddGrowableCol(0)
        self.sizer.AddGrowableRow(0)
        self.scrollPanel.SetSizer(self.panelSizer)
        self.OnType(None)

    def OnType(self, event):
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
            self.distr1Label.SetLabel(_("What number of Sampling Units to use?"))
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
        self.width = event.GetString()

    def OnHeight(self, event):
        self.height = event.GetString()

    def OnDistr1(self, event):
        self.distr1 = event.GetString()

    def OnDistr2(self, event):
        self.distr2 = event.GetString()


class MovingWindows(TitledPage):
    """!Set values from keyboard for sample units"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Units"))

        self.parent = parent
        self.sizer.AddGrowableCol(2)
        self.width = ''
        self.height = ''
        self.boxtype = ''
        # type of shape
        self.typeBox = wx.RadioBox(parent=self, id=wx.ID_ANY,
                                      label=" %s " % _("Select type of shape"),
                                      choices=[_('Rectangle'), _('Circle')],
                                      majorDimension=1,
                                      style=wx.RA_SPECIFY_COLS)

        self.sizer.Add(self.typeBox, flag=wx.ALIGN_LEFT, pos=(1, 1))

        self.typeBox.Bind(wx.EVT_RADIOBOX, self.OnType)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)

        self.widthLabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                        label=_('Width size (in cells) ?'))
        self.widthTxt = wx.TextCtrl(parent=self, id=wx.ID_ANY, size=(250, -1))
        self.sizer.Add(item=self.widthLabel, border=5, pos=(2, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.widthTxt, border=5, pos=(2, 2),
                    flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.heightLabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                         label=_('Height size (in cells) ?'))
        self.heightTxt = wx.TextCtrl(parent=self, id=wx.ID_ANY, size=(250, -1))
        self.sizer.Add(item=self.heightLabel, border=5, pos=(3, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.heightTxt, border=5, pos=(3, 2),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        self.widthLabels = [_('Width size (in cells)? '),
                            _('What radius size (in meters)? ')]
        self.heightLabels = [_('Height size (in cells)? '),
                             _('Name of the circle mask')]

        self.widthTxt.Bind(wx.EVT_TEXT, self.OnWidth)
        self.heightTxt.Bind(wx.EVT_TEXT, self.OnHeight)

    def OnEnterPage(self, event):
        self.OnType(None)

    def OnType(self, event):
        chosen = self.typeBox.GetSelection()
        self.widthLabel.SetLabel(self.widthLabels[chosen])
        self.heightLabel.SetLabel(self.heightLabels[chosen])
        self.sizer.Layout()
        if chosen == 0:
            self.boxtype = 'rectangle'
        else:
            self.boxtype = 'circle'

    def OnWidth(self, event):
        self.width = event.GetString()

    def OnHeight(self, event):
        self.height = event.GetString()


class SummaryPage(TitledPage):
    """!Shows summary result of choosing data"""

    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Summary"))
        global rlisettings

        self.parent = parent
        self.sizer.AddGrowableCol(2)

        #configuration file name
        self.conflabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                       label=_('Configuration file name:'))
        self.conftxt = wx.StaticText(parent=self, id=wx.ID_ANY,
                                     label="")
        self.sizer.Add(item=self.conflabel, border=5, pos=(0, 0),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.conftxt, border=5, pos=(0, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        #raster name
        self.rastlabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                       label=_('Raster name:'))
        self.rasttxt = wx.StaticText(parent=self, id=wx.ID_ANY,
                                     label="")
        self.sizer.Add(item=self.rastlabel, border=5, pos=(1, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.rasttxt, border=5, pos=(1, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        #vector name
        self.vectlabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                        label=_('Vector name:'))
        self.vecttxt = wx.StaticText(parent=self, id=wx.ID_ANY,
                        label="")
        self.sizer.Add(item=self.vectlabel, border=5, pos=(2, 0),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.vecttxt, border=5, pos=(2, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        #region type name
        self.regionlabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                        label=_('Region type:'))
        self.regiontxt = wx.StaticText(parent=self, id=wx.ID_ANY,
                        label="")
        self.sizer.Add(item=self.regionlabel, border=5, pos=(3, 0),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.regiontxt, border=5, pos=(3, 1),
                        flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

        #region keyboard
        self.regionkeylabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                            label="")
        self.regionkeytxt = wx.StaticText(parent=self, id=wx.ID_ANY,
                                          label="")
        self.sizer.Add(item=self.regionkeylabel, border=5, pos=(4, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.regionkeytxt, border=5, pos=(4, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        #sampling area
        self.samplinglabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                           label=_('Sampling area type:'))
        self.samplingtxt = wx.StaticText(parent=self, id=wx.ID_ANY,
                                         label="")
        self.sizer.Add(item=self.samplinglabel, border=5, pos=(5, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.samplingtxt, border=5, pos=(5, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        #shapetype
        self.shapelabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                        label="")
        self.shapetxt = wx.StaticText(parent=self, id=wx.ID_ANY,
                                      label="")
        self.sizer.Add(item=self.shapelabel, border=5, pos=(6, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.shapetxt, border=5, pos=(6, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        #shapedim
        self.shapewidthlabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                        label="")
        self.shapewidthtxt = wx.StaticText(parent=self, id=wx.ID_ANY,
                                      label="")
        self.sizer.Add(item=self.shapewidthlabel, border=5, pos=(7, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.shapewidthtxt, border=5, pos=(7, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.shapeheightlabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                        label="")
        self.shapeheighttxt = wx.StaticText(parent=self, id=wx.ID_ANY,
                                      label="")
        self.sizer.Add(item=self.shapeheightlabel, border=5, pos=(8, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.shapeheighttxt, border=5, pos=(8, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        #units type
        self.unitslabel = wx.StaticText(parent=self, id=wx.ID_ANY, label="")
        self.unitstxt = wx.StaticText(parent=self, id=wx.ID_ANY, label="")
        self.sizer.Add(item=self.unitslabel, border=5, pos=(9, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.unitstxt, border=5, pos=(9, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.unitsmorelabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                                            label="")
        self.unitsmoretxt = wx.StaticText(parent=self, id=wx.ID_ANY, label="")
        self.sizer.Add(item=self.unitsmorelabel, border=5, pos=(10, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.unitsmoretxt, border=5, pos=(10, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.unitsmorelabel2 = wx.StaticText(parent=self, id=wx.ID_ANY,
                                            label="")
        self.unitsmoretxt2 = wx.StaticText(parent=self, id=wx.ID_ANY, label="")
        self.sizer.Add(item=self.unitsmorelabel2, border=5, pos=(11, 0),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)
        self.sizer.Add(item=self.unitsmoretxt2, border=5, pos=(11, 1),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL)

    def OnEnterPage(self, event):
        """!Insert values into text controls for summary of location
        creation options
        """
        self.conftxt.SetLabel(self.parent.startpage.conf_name)
        self.rasttxt.SetLabel(self.parent.startpage.rast)
        self.samplingtxt.SetLabel(self.parent.samplingareapage.samplingtype)
        self.regiontxt.SetLabel(self.parent.startpage.region)
        self.vecttxt.SetLabel(self.parent.startpage.vect)
        if self.parent.startpage.region == 'key':
            #region keybord values
            self.regionkeylabel.SetLabel(_('Region keybord values:'))
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

        if self.parent.samplingareapage.samplingtype == 'units':
            self.shapelabel.SetLabel(_('Type of shape:'))
            self.shapetxt.SetLabel(self.parent.units.boxtype)
            if self.parent.units.boxtype == 'circle':
                self.shapewidthlabel.SetLabel(_("Radius size:"))
                self.shapeheightlabel.SetLabel(_("Name circle mask:"))
            else:
                self.shapewidthlabel.SetLabel(_("Width size:"))
                self.shapeheightlabel.SetLabel(_("Heigth size:"))
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
        elif self.parent.samplingareapage.samplingtype == 'moving':
            self.shapelabel.SetLabel(_('Type of shape:'))
            self.shapetxt.SetLabel(self.parent.moving.boxtype)
            if self.parent.moving.boxtype == 'circle':
                self.shapewidthlabel.SetLabel(_("Radius size:"))
                self.shapeheightlabel.SetLabel(_("Name circle mask:"))
            else:
                self.shapewidthlabel.SetLabel(_("Width size:"))
                self.shapeheightlabel.SetLabel(_("Heigth size:"))
            self.shapewidthtxt.SetLabel(self.parent.moving.width)
            self.shapeheighttxt.SetLabel(self.parent.moving.height)
        else:
            self.shapelabel.SetLabel("")
            self.shapetxt.SetLabel("")
            self.shapewidthlabel.SetLabel("")
            self.shapewidthtxt.SetLabel("")
            self.shapeheightlabel.SetLabel("")
            self.shapeheighttxt.SetLabel("")
