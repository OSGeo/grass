"""
@package wxplot.dialogs

@brief Dialogs for different plotting routines

Classes:
 - dialogs::ProfileRasterDialog
 - dialogs::ScatterRasterDialog
 - dialogs::PlotStatsFrame
 - dialogs::HistRasterDialog
 - dialogs::TextDialog
 - dialogs::OptDialog

(C) 2011-2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton, Arizona State University
"""

import os

import wx

from core import globalvar
from core.settings import UserSettings
from core.globalvar import ICONDIR
from gui_core.gselect import Select
from gui_core.wrap import (
    ColourSelect, ComboBox, Button, CheckBox, Choice,
    ScrolledPanel, SpinCtrl, StaticBox, StaticText, TextCtrl,
)

from grass.script import core as grass


class ProfileRasterDialog(wx.Dialog):

    def __init__(self, parent, id=wx.ID_ANY,
                 title=_("Select raster maps to profile"),
                 style=wx.DEFAULT_DIALOG_STYLE, **kwargs):
        """Dialog to select raster maps to profile.
        """

        wx.Dialog.__init__(self, parent, id, title, style=style, **kwargs)

        self.parent = parent
        self.colorList = [
            "blue",
            "red",
            "green",
            "yellow",
            "magenta",
            "cyan",
            "aqua",
            "black",
            "grey",
            "orange",
            "brown",
            "purple",
            "violet",
            "indigo"]

        self.rasterList = self.parent.rasterList

        self._do_layout()

    def _do_layout(self):

        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.GridBagSizer(hgap=3, vgap=3)

        rastText = ''
        for r in self.rasterList:
            rastText += '%s,' % r

        rastText = rastText.rstrip(',')

        txt = _("Select raster map(s) to profile:")
        label = StaticText(parent=self, id=wx.ID_ANY, label=txt)
        box.Add(label,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 0))

        selection = Select(self, id=wx.ID_ANY,
                           size=globalvar.DIALOG_GSELECT_SIZE,
                           type='cell', multiple=True)
        selection.SetValue(rastText)
        selection.Bind(wx.EVT_TEXT, self.OnSelection)

        box.Add(selection, pos=(0, 1))

        sizer.Add(box, proportion=0,
                  flag=wx.ALL, border=10)

        line = wx.StaticLine(
            parent=self, id=wx.ID_ANY, size=(
                20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(line, proportion=0, flag=wx.GROW |
                  wx.LEFT | wx.RIGHT, border=5)

        btnsizer = wx.StdDialogButtonSizer()

        btn = Button(self, wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = Button(self, wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        sizer.Add(
            btnsizer,
            proportion=0,
            flag=wx.ALIGN_RIGHT | wx.ALL,
            border=5)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def OnSelection(self, event):
        """Choose maps to profile. Convert these into a list
        """
        self.rasterList = self.FindWindowById(
            event.GetId()).GetValue().split(',')


class ScatterRasterDialog(wx.Dialog):

    def __init__(self, parent, id=wx.ID_ANY,
                 title=_("Select pairs of raster maps for scatterplots"),
                 style=wx.DEFAULT_DIALOG_STYLE, **kwargs):
        """Dialog to select raster maps to profile.
        """

        wx.Dialog.__init__(self, parent, id, title, style=style, **kwargs)

        self.parent = parent
        self.rasterList = self.parent.rasterList
        self.bins = self.parent.bins
        self.scattertype = self.parent.scattertype
        self.maptype = self.parent.maptype
        self.spinbins = ''
        self.colorList = [
            "blue",
            "red",
            "green",
            "yellow",
            "magenta",
            "cyan",
            "aqua",
            "black",
            "grey",
            "orange",
            "brown",
            "purple",
            "violet",
            "indigo"]

        self._do_layout()

    def _do_layout(self):

        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.GridBagSizer(hgap=3, vgap=3)

        # parse raster pair tuples
        rastText = ''
        if len(self.rasterList) > 0:
            for r in self.rasterList:
                if isinstance(r, tuple):
                    rastText += '%s,%s,' % r
                else:
                    rastText += '%s,' % r
            rastText = rastText.rstrip(',')

        # select rasters
        txt = _("Select pairs of raster maps for bivariate scatterplots:")
        label = StaticText(parent=self, id=wx.ID_ANY, label=txt)
        box.Add(label,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 0))

        selection = Select(self, id=wx.ID_ANY,
                           size=globalvar.DIALOG_GSELECT_SIZE,
                           type='cell', multiple=True)
        selection.SetValue(rastText)
        selection.Bind(wx.EVT_TEXT, self.OnSelection)

        box.Add(selection, pos=(0, 1))

        # Nsteps for FP maps
        label = StaticText(parent=self, id=wx.ID_ANY,
                           label=_("Number of bins (for FP maps)"))
        box.Add(label,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(1, 0))
        self.spinbins = SpinCtrl(
            parent=self, id=wx.ID_ANY, value="", pos=(
                30, 50), size=(
                100, -1), style=wx.SP_ARROW_KEYS)
        self.spinbins.SetRange(1, 1000)
        self.spinbins.SetValue(self.bins)
        box.Add(self.spinbins,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(1, 1))

# TODO possibly make bubble plots with marker size proportional to cell counts
#        # scatterplot type
#        label = wx.StaticText(parent = self, id = wx.ID_ANY,
#                              label = _("Scatterplot type"))
#        box.Add(item = label,
#                flag = wx.ALIGN_CENTER_VERTICAL, pos = (2, 0))
#        types = ['normal', 'bubble']
#        scattertype = ComboBox(parent = self, id = wx.ID_ANY, size = (250, -1),
#                                choices = types, style = wx.CB_DROPDOWN)
#        scattertype.SetStringSelection(self.scattertype)
#        box.Add(item = scattertype,
#                flag = wx.ALIGN_CENTER_VERTICAL, pos = (2, 1))

        sizer.Add(box, proportion=0,
                  flag=wx.ALL, border=10)

        line = wx.StaticLine(
            parent=self, id=wx.ID_ANY, size=(
                20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(line, proportion=0, flag=wx.GROW |
                  wx.LEFT | wx.RIGHT, border=5)

        btnsizer = wx.StdDialogButtonSizer()

        btn = Button(self, wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = Button(self, wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        sizer.Add(
            btnsizer,
            proportion=0,
            flag=wx.ALIGN_RIGHT | wx.ALL,
            border=5)

        self.spinbins.Bind(wx.EVT_TEXT, self.OnSetBins)
        self.spinbins.Bind(wx.EVT_SPINCTRL, self.OnSetBins)
#        scattertype.Bind(wx.EVT_TEXT, self.OnSetScattertypes)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def OnSelection(self, event):
        """Select raster maps for scatterplot. Must select maps in pairs.
        """
        self.rasterList = self.FindWindowById(
            event.GetId()).GetValue().split(',', 1)

    def OnSetBins(self, event):
        """Bins for histogramming FP maps (=nsteps in r.stats)
        """
        self.bins = self.spinbins.GetValue()

    def OnSetScattertypes(self, event):
        self.scattertype = event.GetString()

    def GetRasterPairs(self):
        """Get raster pairs"""
        pairsList = list()
        pair = list()
        for r in self.rasterList:
            pair.append(r)
            if len(pair) == 2:
                pairsList.append(tuple(pair))
                pair = list()

        return list(pairsList)

    def GetSettings(self):
        """Get type and bins"""
        return self.scattertype, self.bins


class PlotStatsFrame(wx.Frame):

    def __init__(self, parent, id, message='', title='',
                 style=wx.DEFAULT_FRAME_STYLE, **kwargs):
        """Dialog to display and save statistics for plots
        """
        wx.Frame.__init__(self, parent, id, style=style, **kwargs)
        self.SetLabel(_("Statistics"))
        self.SetIcon(
            wx.Icon(
                os.path.join(
                    ICONDIR,
                    'grass.ico'),
                wx.BITMAP_TYPE_ICO))
        self.panel = wx.Panel(self)

        sp = ScrolledPanel(
            self.panel, -1, size=(400, 400),
            style=wx.TAB_TRAVERSAL | wx.SUNKEN_BORDER, name="Statistics")

        #
        # initialize variables
        #
        self.parent = parent
        self.message = message
        self.title = title
        self.CenterOnParent()

        #
        # Display statistics
        #
        sizer = wx.BoxSizer(wx.VERTICAL)
        txtSizer = wx.BoxSizer(wx.VERTICAL)

        statstitle = StaticText(
            parent=self.panel,
            id=wx.ID_ANY,
            label=self.title)
        sizer.Add(statstitle, proportion=0,
                  flag=wx.GROW | wx.ALL, border=3)
        line = wx.StaticLine(
            parent=self.panel, id=wx.ID_ANY, size=(
                20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(line, proportion=0,
                  flag=wx.GROW | wx.ALL, border=3)
        for stats in self.message:
            statstxt = StaticText(parent=sp, id=wx.ID_ANY, label=stats)
            statstxt.SetBackgroundColour("WHITE")
            txtSizer.Add(
                statstxt,
                proportion=1,
                flag=wx.EXPAND | wx.LEFT | wx.RIGHT,
                border=3)
            line = wx.StaticLine(
                parent=sp, id=wx.ID_ANY, size=(
                    20, -1), style=wx.LI_HORIZONTAL)
            txtSizer.Add(line, proportion=0, flag=wx.GROW |
                         wx.ALL, border=3)

        sp.SetSizer(txtSizer)
        sp.SetAutoLayout(1)
        sp.SetupScrolling()

        sizer.Add(sp, proportion=1,
                  flag=wx.GROW | wx.LEFT | wx.RIGHT | wx.BOTTOM, border=3)

        line = wx.StaticLine(
            parent=self.panel, id=wx.ID_ANY, size=(
                20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(line, proportion=0, flag=wx.GROW |
                  wx.LEFT | wx.RIGHT, border=3)

        #
        # buttons
        #
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)

        btn_clipboard = Button(self.panel, id=wx.ID_COPY, label=_('C&opy'))
        btn_clipboard.SetToolTip(
            _("Copy regression statistics the clipboard (Ctrl+C)"))
        btnSizer.Add(
            btn_clipboard,
            proportion=0,
            flag=wx.ALL,
            border=5)

        btnCancel = Button(self.panel, wx.ID_CLOSE)
        btnCancel.SetDefault()
        btnSizer.Add(
            btnCancel,
            proportion=0,
            flag=wx.ALL,
            border=5)

        sizer.Add(
            btnSizer,
            proportion=0,
            flag=wx.ALL,
            border=5)

        # bindings
        btnCancel.Bind(wx.EVT_BUTTON, self.OnClose)
        btn_clipboard.Bind(wx.EVT_BUTTON, self.OnCopy)

        self.panel.SetSizer(sizer)
        sizer.Fit(self)

    def OnCopy(self, event):
        """Copy the regression stats to the clipboard
        """
        str = self.title + '\n'
        for item in self.message:
            str += item

        rdata = wx.TextDataObject()
        rdata.SetText(str)

        if wx.TheClipboard.Open():
            wx.TheClipboard.SetData(rdata)
            wx.TheClipboard.Close()
            wx.MessageBox(_("Regression statistics copied to clipboard"))

    def OnClose(self, event):
        """Button 'Close' pressed
        """
        self.Close(True)


class HistRasterDialog(wx.Dialog):

    def __init__(self, parent, id=wx.ID_ANY,
                 title=_("Select raster map or imagery group to histogram"),
                 style=wx.DEFAULT_DIALOG_STYLE, **kwargs):
        """Dialog to select raster maps to histogram.
        """

        wx.Dialog.__init__(self, parent, id, title, style=style, **kwargs)

        self.parent = parent
        self.rasterList = self.parent.rasterList
        self.group = self.parent.group
        self.bins = self.parent.bins
        self.histtype = self.parent.histtype
        self.maptype = self.parent.maptype
        self.spinbins = ''

        self._do_layout()

    def _do_layout(self):

        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.GridBagSizer(hgap=3, vgap=3)

        #
        # select single raster or image group to histogram radio buttons
        #
        self.rasterRadio = wx.RadioButton(
            self, id=wx.ID_ANY, label=" %s " %
            _("Histogram single raster"), style=wx.RB_GROUP)
        self.groupRadio = wx.RadioButton(
            self, id=wx.ID_ANY, label=" %s " %
            _("Histogram imagery group"))
        if self.maptype == 'raster':
            self.rasterRadio.SetValue(True)
        elif self.maptype == 'group':
            self.groupRadio.SetValue(True)
        box.Add(
            self.rasterRadio,
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(
                0,
                0))
        box.Add(
            self.groupRadio,
            flag=wx.ALIGN_CENTER_VERTICAL,
            pos=(
                0,
                1))

        #
        # Select a raster to histogram
        #
        label = StaticText(parent=self, id=wx.ID_ANY,
                           label=_("Select raster map:"))
        box.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(1, 0))
        self.rselection = Select(self, id=wx.ID_ANY,
                                 size=globalvar.DIALOG_GSELECT_SIZE,
                                 type='cell')
        if self.groupRadio.GetValue():
            self.rselection.Disable()
        else:
            rastText = ''
            for r in self.rasterList:
                rastText += '%s,' % r
            rastText = rastText.rstrip(',')
            self.rselection.SetValue(rastText)
        box.Add(self.rselection, pos=(1, 1))

        #
        # Select an image group to histogram
        #
        label = StaticText(parent=self, id=wx.ID_ANY,
                           label=_("Select image group:"))
        box.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(2, 0))
        self.gselection = Select(self, id=wx.ID_ANY,
                                 size=globalvar.DIALOG_GSELECT_SIZE,
                                 type='group')
        if self.rasterRadio.GetValue():
            self.gselection.Disable()
        else:
            if self.group is not None:
                self.gselection.SetValue(self.group)
        box.Add(self.gselection, pos=(2, 1))

        #
        # Nsteps for FP maps and histogram type selection
        #
        label = StaticText(parent=self, id=wx.ID_ANY,
                           label=_("Number of bins (for FP maps)"))
        box.Add(label,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(3, 0))
        self.spinbins = SpinCtrl(
            parent=self, id=wx.ID_ANY, value="", pos=(
                30, 50), size=(
                100, -1), style=wx.SP_ARROW_KEYS)
        self.spinbins.SetRange(1, 1000)
        self.spinbins.SetValue(self.bins)
        box.Add(self.spinbins,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(3, 1))

        label = StaticText(parent=self, id=wx.ID_ANY,
                           label=_("Histogram type"))
        box.Add(label,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(4, 0))
        types = ['count', 'percent', 'area']
        histtype = ComboBox(parent=self, id=wx.ID_ANY, size=(250, -1),
                               choices=types, style=wx.CB_DROPDOWN)
        histtype.SetStringSelection(self.histtype)
        box.Add(histtype,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(4, 1))

        sizer.Add(box, proportion=0,
                  flag=wx.ALL, border=10)

        line = wx.StaticLine(
            parent=self, id=wx.ID_ANY, size=(
                20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(line, proportion=0, flag=wx.GROW |
                 wx.LEFT | wx.RIGHT, border=5)

        btnsizer = wx.StdDialogButtonSizer()

        btn = Button(self, wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = Button(self, wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        sizer.Add(
            btnsizer,
            proportion=0,
            flag=wx.ALIGN_RIGHT | wx.ALL,
            border=5)

        #
        # bindings
        #
        self.Bind(wx.EVT_RADIOBUTTON, self.OnHistMap, self.rasterRadio)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnHistMap, self.groupRadio)
        self.rselection.Bind(wx.EVT_TEXT, self.OnRasterSelection)
        self.gselection.Bind(wx.EVT_TEXT, self.OnGroupSelection)
        self.spinbins.Bind(wx.EVT_TEXT, self.OnSetBins)
        self.spinbins.Bind(wx.EVT_SPINCTRL, self.OnSetBins)
        histtype.Bind(wx.EVT_TEXT, self.OnSetHisttypes)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def OnHistMap(self, event):
        """Hander for radio buttons to choose between histogramming a
            single raster and an imagery group
        """
        if self.rasterRadio.GetValue() is True:
            self.maptype = 'raster'
            self.rselection.Enable()
            self.gselection.Disable()
            self.gselection.SetValue('')
        elif self.groupRadio.GetValue() is True:
            self.maptype = 'group'
            self.gselection.Enable()
            self.rselection.Disable()
            self.rselection.SetValue('')
        else:
            pass

    def OnRasterSelection(self, event):
        """Handler for selecting a single raster map
        """
        self.rasterList = []
        self.rasterList.append(event.GetString())

    def OnGroupSelection(self, event):
        """Handler for selecting imagery group
        """
        self.rasterList = []
        self.group = event.GetString()
        ret = grass.read_command('i.group',
                                 group='%s' % self.group,
                                 quiet=True,
                                 flags='g').strip().splitlines()

        if ret not in [None, '', ['']]:
            self.rasterList = ret
        else:
            wx.MessageBox(
                message=_("Selected group must be in current mapset"),
                caption=_('Invalid input'),
                style=wx.OK | wx.ICON_ERROR)

    def OnSetBins(self, event):
        """Bins for histogramming FP maps (=nsteps in r.stats)
        """
        self.bins = self.spinbins.GetValue()

    def OnSetHisttypes(self, event):
        self.histtype = event.GetString()


class TextDialog(wx.Dialog):

    def __init__(self, parent, giface, id, title, plottype='',
                 style=wx.DEFAULT_DIALOG_STYLE, **kwargs):
        """Dialog to set histogram text options: font, title
        and font size, axis labels and font size
        """
        wx.Dialog.__init__(self, parent, id, title, style=style, **kwargs)
        #
        # initialize variables
        #
        self._giface = giface
        # combo box entry lists
        self.ffamilydict = {'default': wx.FONTFAMILY_DEFAULT,
                            'decorative': wx.FONTFAMILY_DECORATIVE,
                            'roman': wx.FONTFAMILY_ROMAN,
                            'script': wx.FONTFAMILY_SCRIPT,
                            'swiss': wx.FONTFAMILY_SWISS,
                            'modern': wx.FONTFAMILY_MODERN,
                            'teletype': wx.FONTFAMILY_TELETYPE}

        self.fstyledict = {'normal': wx.FONTSTYLE_NORMAL,
                           'slant': wx.FONTSTYLE_SLANT,
                           'italic': wx.FONTSTYLE_ITALIC}

        self.fwtdict = {'normal': wx.FONTWEIGHT_NORMAL,
                        'light': wx.FONTWEIGHT_LIGHT,
                        'bold': wx.FONTWEIGHT_BOLD}

        self.parent = parent
        self.plottype = plottype

        self.ptitle = self.parent.ptitle
        self.xlabel = self.parent.xlabel
        self.ylabel = self.parent.ylabel

        self.properties = self.parent.properties  # read-only

        # font size
        self.fontfamily = self.parent.wx_font.GetFamily()
        self.fontstyle = self.parent.wx_font.GetStyle()
        self.fontweight = self.parent.wx_font.GetWeight()

        self._do_layout()

    def _do_layout(self):
        """Do layout"""
        # dialog layout
        sizer = wx.BoxSizer(wx.VERTICAL)

        box = StaticBox(parent=self, id=wx.ID_ANY,
                        label=" %s " % _("Text settings"))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        #
        # profile title
        #
        label = StaticText(
            parent=self,
            id=wx.ID_ANY,
            label=_("Profile title:"))
        gridSizer.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 0))
        self.ptitleentry = TextCtrl(
            parent=self, id=wx.ID_ANY, value="", size=(250, -1))
        # self.ptitleentry.SetFont(self.font)
        self.ptitleentry.SetValue(self.ptitle)
        gridSizer.Add(self.ptitleentry, pos=(0, 1))

        #
        # title font
        #
        tlabel = StaticText(
            parent=self,
            id=wx.ID_ANY,
            label=_("Title font size (pts):"))
        gridSizer.Add(tlabel, flag=wx.ALIGN_CENTER_VERTICAL, pos=(1, 0))
        self.ptitlesize = SpinCtrl(
            parent=self, id=wx.ID_ANY, value="", pos=(
                30, 50), size=(
                50, -1), style=wx.SP_ARROW_KEYS)
        self.ptitlesize.SetRange(5, 100)
        self.ptitlesize.SetValue(
            int(self.properties['font']['prop']['titleSize']))
        gridSizer.Add(self.ptitlesize, pos=(1, 1))

        #
        # x-axis label
        #
        label = StaticText(
            parent=self,
            id=wx.ID_ANY,
            label=_("X-axis label:"))
        gridSizer.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(2, 0))
        self.xlabelentry = TextCtrl(
            parent=self, id=wx.ID_ANY, value="", size=(250, -1))
        # self.xlabelentry.SetFont(self.font)
        self.xlabelentry.SetValue(self.xlabel)
        gridSizer.Add(self.xlabelentry, pos=(2, 1))

        #
        # y-axis label
        #
        label = StaticText(
            parent=self,
            id=wx.ID_ANY,
            label=_("Y-axis label:"))
        gridSizer.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(3, 0))
        self.ylabelentry = TextCtrl(
            parent=self, id=wx.ID_ANY, value="", size=(250, -1))
        # self.ylabelentry.SetFont(self.font)
        self.ylabelentry.SetValue(self.ylabel)
        gridSizer.Add(self.ylabelentry, pos=(3, 1))

        #
        # font size
        #
        llabel = StaticText(
            parent=self,
            id=wx.ID_ANY,
            label=_("Label font size (pts):"))
        gridSizer.Add(llabel, flag=wx.ALIGN_CENTER_VERTICAL, pos=(4, 0))
        self.axislabelsize = SpinCtrl(
            parent=self, id=wx.ID_ANY, value="", pos=(
                30, 50), size=(
                50, -1), style=wx.SP_ARROW_KEYS)
        self.axislabelsize.SetRange(5, 100)
        self.axislabelsize.SetValue(
            int(self.properties['font']['prop']['axisSize']))
        gridSizer.Add(self.axislabelsize, pos=(4, 1))

        boxSizer.Add(gridSizer)
        sizer.Add(boxSizer, flag=wx.ALL | wx.EXPAND, border=3)

        #
        # font settings
        #
        box = StaticBox(parent=self, id=wx.ID_ANY,
                        label=" %s " % _("Font settings"))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        #
        # font family
        #
        label1 = StaticText(
            parent=self,
            id=wx.ID_ANY,
            label=_("Font family:"))
        gridSizer.Add(label1, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 0))
        self.ffamilycb = ComboBox(
            parent=self, id=wx.ID_ANY, size=(250, -1),
            choices=list(self.ffamilydict.keys()),
            style=wx.CB_DROPDOWN)
        self.ffamilycb.SetStringSelection('swiss')
        for item in self.ffamilydict.items():
            if self.fontfamily == item[1]:
                self.ffamilycb.SetStringSelection(item[0])
                break
        gridSizer.Add(self.ffamilycb, pos=(0, 1), flag=wx.ALIGN_RIGHT)

        #
        # font style
        #
        label = StaticText(parent=self, id=wx.ID_ANY, label=_("Style:"))
        gridSizer.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(1, 0))
        self.fstylecb = ComboBox(
            parent=self, id=wx.ID_ANY, size=(250, -1),
            choices=list(self.fstyledict.keys()),
            style=wx.CB_DROPDOWN)
        self.fstylecb.SetStringSelection('normal')
        for item in self.fstyledict.items():
            if self.fontstyle == item[1]:
                self.fstylecb.SetStringSelection(item[0])
                break
        gridSizer.Add(self.fstylecb, pos=(1, 1), flag=wx.ALIGN_RIGHT)

        #
        # font weight
        #
        label = StaticText(parent=self, id=wx.ID_ANY, label=_("Weight:"))
        gridSizer.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(2, 0))
        self.fwtcb = ComboBox(
            parent=self, size=(250, -1),
            choices=list(self.fwtdict.keys()),
            style=wx.CB_DROPDOWN)
        self.fwtcb.SetStringSelection('normal')
        for item in self.fwtdict.items():
            if self.fontweight == item[1]:
                self.fwtcb.SetStringSelection(item[0])
                break

        gridSizer.Add(self.fwtcb, pos=(2, 1), flag=wx.ALIGN_RIGHT)

        gridSizer.AddGrowableCol(1)
        boxSizer.Add(gridSizer, flag=wx.EXPAND)
        sizer.Add(boxSizer, flag=wx.LEFT | wx.RIGHT |
                  wx.BOTTOM | wx.EXPAND, border=3)

        line = wx.StaticLine(
            parent=self, id=wx.ID_ANY, size=(
                20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(line, proportion=0, flag=wx.GROW |
                  wx.LEFT | wx.RIGHT, border=3)

        #
        # buttons
        #
        btnSave = Button(self, wx.ID_SAVE)
        btnApply = Button(self, wx.ID_APPLY)
        btnOk = Button(self, wx.ID_OK)
        btnCancel = Button(self, wx.ID_CANCEL)
        btnOk.SetDefault()

        # bindings
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnApply.SetToolTip(_("Apply changes for the current session"))
        btnOk.Bind(wx.EVT_BUTTON, self.OnOk)
        btnOk.SetToolTip(
            _("Apply changes for the current session and close dialog"))
        btnOk.SetDefault()
        btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        btnSave.SetToolTip(
            _("Apply and save changes to user settings file (default for next sessions)"))
        btnCancel.SetToolTip(_("Close dialog and ignore changes"))

        # sizers
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(btnOk)
        btnStdSizer.AddButton(btnApply)
        btnStdSizer.AddButton(btnCancel)
        btnStdSizer.Realize()

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(
            btnSave,
            proportion=0,
            flag=wx.ALL,
            border=5)
        btnSizer.Add(
            btnStdSizer,
            proportion=0,
            flag=wx.ALL,
            border=5)
        sizer.Add(
            btnSizer,
            proportion=0,
            flag=wx.ALL,
            border=5)

        #
        # bindings
        #
        self.ptitleentry.Bind(wx.EVT_TEXT, self.OnTitle)
        self.xlabelentry.Bind(wx.EVT_TEXT, self.OnXLabel)
        self.ylabelentry.Bind(wx.EVT_TEXT, self.OnYLabel)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def OnTitle(self, event):
        self.ptitle = event.GetString()

    def OnXLabel(self, event):
        self.xlabel = event.GetString()

    def OnYLabel(self, event):
        self.ylabel = event.GetString()

    def UpdateSettings(self):
        self.properties['font']['prop'][
            'titleSize'] = self.ptitlesize.GetValue()
        self.properties['font']['prop'][
            'axisSize'] = self.axislabelsize.GetValue()

        family = self.ffamilydict[self.ffamilycb.GetStringSelection()]
        self.properties['font']['prop']['family'] = family
        self.parent.wx_font.SetFamily(family)
        style = self.fstyledict[self.fstylecb.GetStringSelection()]
        self.properties['font']['prop']['style'] = style
        self.parent.wx_font.SetStyle(style)
        weight = self.fwtdict[self.fwtcb.GetStringSelection()]
        self.properties['font']['prop']['weight'] = weight
        self.parent.wx_font.SetWeight(weight)

    def OnSave(self, event):
        """Button 'Save' pressed"""
        self.OnApply(None)
        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        fileSettings[self.plottype] = UserSettings.Get(group=self.plottype)
        UserSettings.SaveToFile(fileSettings)
        self._giface.WriteLog(
            _('Plot text sizes saved to file \'%s\'.') % UserSettings.filePath)
        self.EndModal(wx.ID_OK)

    def OnApply(self, event):
        """Button 'Apply' pressed"""
        self.UpdateSettings()
        self.parent.OnPlotText(self)

    def OnOk(self, event):
        """Button 'OK' pressed"""
        self.OnApply(None)
        self.EndModal(wx.ID_OK)


class OptDialog(wx.Dialog):

    def __init__(self, parent, giface, id, title, plottype='',
                 style=wx.DEFAULT_DIALOG_STYLE, **kwargs):
        """Dialog to set various options for data plotted, including: line
        width, color, style; marker size, color, fill, and style; grid
        and legend options.
        """
        wx.Dialog.__init__(self, parent, id, title, style=style, **kwargs)

        # init variables
        self.parent = parent
        self._giface = giface
        self.linestyledict = parent.linestyledict
        self.ptfilldict = parent.ptfilldict
        self.parent = parent
        self.plottype = plottype

        self.pttypelist = ['circle',
                           'dot',
                           'square',
                           'triangle',
                           'triangle_down',
                           'cross',
                           'plus']

        self.axislist = ['min',
                         'auto',
                         'custom']

        # widgets ids
        self.wxId = {}

        self.parent = parent

        # read-only
        self.raster = self.parent.raster
        self.rasterList = self.parent.rasterList
        self.properties = self.parent.properties
        self.map = ''

        if len(self.rasterList) == 0:
            wx.MessageBox(parent=self,
                          message=_("No map or image group selected to plot."),
                          caption=_("Warning"), style=wx.OK | wx.ICON_ERROR)

        self._do_layout()

    def ConvertTuples(self, tlist):
        """Converts tuples to strings when rasterList contains raster
        pairs for scatterplot
        """
        list = []
        for i in tlist:
            i = str(i).strip('()')
            list.append(i)

        return list

    def _do_layout(self):
        """Options dialog layout
        """
        sizer = wx.BoxSizer(wx.VERTICAL)

        box = StaticBox(parent=self, id=wx.ID_ANY,
                        label=" %s " % _("Plot settings"))
        boxMainSizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)

        self.wxId['pcolor'] = 0
        self.wxId['pwidth'] = 0
        self.wxId['pstyle'] = 0
        self.wxId['psize'] = 0
        self.wxId['ptype'] = 0
        self.wxId['pfill'] = 0
        self.wxId['plegend'] = 0
        self.wxId['marker'] = {}
        self.wxId['x-axis'] = {}
        self.wxId['y-axis'] = {}

        #
        # plot line settings and point settings
        #
        if len(self.rasterList) == 0:
            return

        box = StaticBox(parent=self, id=wx.ID_ANY,
                        label=_("Map/image plotted"))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        row = 0
        choicelist = []
        for i in self.rasterList:
            choicelist.append(str(i))

        self.mapchoice = Choice(parent=self, id=wx.ID_ANY, size=(300, -1),
                                   choices=choicelist)
        self.mapchoice.SetToolTip(_("Settings for selected map"))

        if not self.map:
            self.map = self.rasterList[self.mapchoice.GetCurrentSelection()]
        else:
            self.mapchoice.SetStringSelection(str(self.map))

        gridSizer.Add(self.mapchoice, flag=wx.ALIGN_CENTER_VERTICAL,
                      pos=(row, 0), span=(1, 2))

        #
        # options for line plots (profiles and histograms)
        #
        if self.plottype != 'scatter':
            row += 1
            label = StaticText(
                parent=self,
                id=wx.ID_ANY,
                label=_("Line color"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    row,
                    0))
            color = ColourSelect(
                parent=self, id=wx.ID_ANY, colour=self.raster[
                    self.map]['pcolor'])
            self.wxId['pcolor'] = color.GetId()
            gridSizer.Add(color, pos=(row, 1))

            row += 1
            label = StaticText(
                parent=self,
                id=wx.ID_ANY,
                label=_("Line width"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    row,
                    0))
            width = SpinCtrl(parent=self, id=wx.ID_ANY, value="",
                             size=(50, -1), style=wx.SP_ARROW_KEYS)
            width.SetRange(1, 10)
            width.SetValue(self.raster[self.map]['pwidth'])
            self.wxId['pwidth'] = width.GetId()
            gridSizer.Add(width, pos=(row, 1))

            row += 1
            label = StaticText(
                parent=self,
                id=wx.ID_ANY,
                label=_("Line style"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    row,
                    0))
            style = Choice(
                parent=self, id=wx.ID_ANY, size=(
                    120, -1), choices=list(self.linestyledict.keys()))
            style.SetStringSelection(self.raster[self.map]['pstyle'])
            self.wxId['pstyle'] = style.GetId()
            gridSizer.Add(style, pos=(row, 1))

        row += 1
        label = StaticText(parent=self, id=wx.ID_ANY, label=_("Legend"))
        gridSizer.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        legend = TextCtrl(parent=self, id=wx.ID_ANY,
                          value="", size=(200, -1))
        legend.SetValue(self.raster[self.map]['plegend'])
        gridSizer.Add(legend, pos=(row, 1))
        self.wxId['plegend'] = legend.GetId()

        boxSizer.Add(gridSizer)
        boxMainSizer.Add(boxSizer, flag=wx.ALL, border=3)

        #
        # segment marker settings for profiles only
        #
        if self.plottype == 'profile':
            box = StaticBox(
                parent=self, id=wx.ID_ANY, label=" %s " %
                _("Transect segment marker settings"))

            boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)

            gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
            label = StaticText(parent=self, id=wx.ID_ANY, label=_("Color"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    0,
                    0))
            ptcolor = ColourSelect(
                parent=self,
                id=wx.ID_ANY,
                colour=self.properties['marker']['color'])
            self.wxId['marker']['color'] = ptcolor.GetId()
            gridSizer.Add(ptcolor, pos=(0, 1))

            label = StaticText(parent=self, id=wx.ID_ANY, label=_("Size"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    1,
                    0))
            ptsize = SpinCtrl(parent=self, id=wx.ID_ANY, value="",
                              size=(50, -1), style=wx.SP_ARROW_KEYS)
            ptsize.SetRange(1, 10)
            ptsize.SetValue(self.properties['marker']['size'])
            self.wxId['marker']['size'] = ptsize.GetId()
            gridSizer.Add(ptsize, pos=(1, 1))

            label = StaticText(parent=self, id=wx.ID_ANY, label=_("Fill"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    2,
                    0))
            ptfill = Choice(parent=self, id=wx.ID_ANY,
                               size=(120, -1), choices=list(self.ptfilldict.keys()))
            ptfill.SetStringSelection(self.properties['marker']['fill'])
            self.wxId['marker']['fill'] = ptfill.GetId()
            gridSizer.Add(ptfill, pos=(2, 1))

            label = StaticText(parent=self, id=wx.ID_ANY, label=_("Legend"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    3,
                    0))
            ptlegend = TextCtrl(
                parent=self, id=wx.ID_ANY, value="", size=(
                    200, -1))
            ptlegend.SetValue(self.properties['marker']['legend'])
            self.wxId['marker']['legend'] = ptlegend.GetId()
            gridSizer.Add(ptlegend, pos=(3, 1))

            label = StaticText(parent=self, id=wx.ID_ANY, label=_("Style"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    4,
                    0))
            pttype = Choice(
                parent=self, size=(200, -1),
                choices=self.pttypelist)
            pttype.SetStringSelection(self.properties['marker']['type'])
            self.wxId['marker']['type'] = pttype.GetId()
            gridSizer.Add(pttype, pos=(4, 1))

            boxSizer.Add(gridSizer)
            boxMainSizer.Add(boxSizer, flag=wx.ALL, border=3)

        #
        # point options for scatterplots
        #
        elif self.plottype == 'scatter':
            box = StaticBox(parent=self, id=wx.ID_ANY,
                            label=" %s " % _("Scatterplot points"))

            boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)

            gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
            label = StaticText(parent=self, id=wx.ID_ANY, label=_("Color"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    0,
                    0))
            ptcolor = ColourSelect(
                parent=self, id=wx.ID_ANY, colour=self.raster[
                    self.map]['pcolor'])
            self.wxId['pcolor'] = ptcolor.GetId()
            gridSizer.Add(ptcolor, pos=(0, 1))

            label = StaticText(parent=self, id=wx.ID_ANY, label=_("Size"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    1,
                    0))
            ptsize = SpinCtrl(parent=self, id=wx.ID_ANY, value="",
                              size=(50, -1), style=wx.SP_ARROW_KEYS)
            ptsize.SetRange(1, 10)
            ptsize.SetValue(self.raster[self.map]['psize'])
            self.wxId['psize'] = ptsize.GetId()
            gridSizer.Add(ptsize, pos=(1, 1))

            label = StaticText(parent=self, id=wx.ID_ANY, label=_("Fill"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    2,
                    0))
            ptfill = Choice(parent=self, id=wx.ID_ANY,
                               size=(120, -1), choices=list(self.ptfilldict.keys()))
            ptfill.SetStringSelection(self.raster[self.map]['pfill'])
            self.wxId['pfill'] = ptfill.GetId()
            gridSizer.Add(ptfill, pos=(2, 1))

            label = StaticText(parent=self, id=wx.ID_ANY, label=_("Legend"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    3,
                    0))
            ptlegend = TextCtrl(
                parent=self, id=wx.ID_ANY, value="", size=(
                    200, -1))
            ptlegend.SetValue(self.raster[self.map]['plegend'])
            self.wxId['plegend'] = ptlegend.GetId()
            gridSizer.Add(ptlegend, pos=(3, 1))

            label = StaticText(parent=self, id=wx.ID_ANY, label=_("Style"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    4,
                    0))
            pttype = Choice(
                parent=self, size=(200, -1),
                choices=self.pttypelist)
            pttype.SetStringSelection(self.raster[self.map]['ptype'])
            self.wxId['ptype'] = pttype.GetId()
            gridSizer.Add(pttype, pos=(4, 1))

            boxSizer.Add(gridSizer)
            boxMainSizer.Add(boxSizer, flag=wx.ALL, border=3)

        sizer.Add(boxMainSizer, flag=wx.ALL | wx.EXPAND, border=3)

        #
        # axis options for all plots
        #
        box = StaticBox(parent=self, id=wx.ID_ANY,
                        label=" %s " % _("Axis settings"))
        boxMainSizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)

        middleSizer = wx.BoxSizer(wx.HORIZONTAL)

        idx = 0
        for axis, atype in [(_("X-Axis"), 'x-axis'),
                            (_("Y-Axis"), 'y-axis')]:
            box = StaticBox(parent=self, id=wx.ID_ANY,
                            label=" %s " % axis)
            boxSizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
            gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

            prop = self.properties[atype]['prop']

            row = 0
            label = StaticText(parent=self, id=wx.ID_ANY, label=_("Scale"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    row,
                    0))
            type = Choice(
                parent=self, id=wx.ID_ANY, size=(
                    100, -1), choices=self.axislist)
            type.SetStringSelection(prop['type'])
            type.SetToolTip(
                _("Automatic axis scaling, custom max and min, or scale matches data range (min)"))
            self.wxId[atype]['type'] = type.GetId()
            gridSizer.Add(type, pos=(row, 1))

            row += 1
            label = StaticText(
                parent=self,
                id=wx.ID_ANY,
                label=_("Custom min"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    row,
                    0))
            min = TextCtrl(parent=self, id=wx.ID_ANY,
                           value="", size=(70, -1))
            min.SetValue(str(prop['min']))
            self.wxId[atype]['min'] = min.GetId()
            gridSizer.Add(min, pos=(row, 1))

            row += 1
            label = StaticText(
                parent=self,
                id=wx.ID_ANY,
                label=_("Custom max"))
            gridSizer.Add(
                label,
                flag=wx.ALIGN_CENTER_VERTICAL,
                pos=(
                    row,
                    0))
            max = TextCtrl(parent=self, id=wx.ID_ANY,
                           value="", size=(70, -1))
            max.SetValue(str(prop['max']))
            self.wxId[atype]['max'] = max.GetId()
            gridSizer.Add(max, pos=(row, 1))

            row += 1
            log = CheckBox(parent=self, id=wx.ID_ANY, label=_("Log scale"))
            log.SetValue(prop['log'])
            self.wxId[atype]['log'] = log.GetId()
            gridSizer.Add(log, pos=(row, 0), span=(1, 2))

            if idx == 0:
                flag = wx.ALL | wx.EXPAND
            else:
                flag = wx.TOP | wx.BOTTOM | wx.RIGHT | wx.EXPAND

            boxSizer.Add(gridSizer, flag=wx.ALL, border=3)
            boxMainSizer.Add(boxSizer, flag=flag, border=3)

            idx += 1

        middleSizer.Add(boxMainSizer, flag=wx.ALL | wx.EXPAND, border=3)

        #
        # grid & legend options for all plots
        #
        self.wxId['grid'] = {}
        self.wxId['legend'] = {}
        self.wxId['font'] = {}
        box = StaticBox(parent=self, id=wx.ID_ANY,
                        label=" %s " % _("Grid and Legend settings"))
        boxMainSizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        row = 0
        label = StaticText(parent=self, id=wx.ID_ANY, label=_("Grid color"))
        gridSizer.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        gridcolor = ColourSelect(
            parent=self,
            id=wx.ID_ANY,
            colour=self.properties['grid']['color'])
        self.wxId['grid']['color'] = gridcolor.GetId()
        gridSizer.Add(gridcolor, pos=(row, 1))

        row += 1
        gridshow = CheckBox(parent=self, id=wx.ID_ANY, label=_("Show grid"))
        gridshow.SetValue(self.properties['grid']['enabled'])
        self.wxId['grid']['enabled'] = gridshow.GetId()
        gridSizer.Add(gridshow, pos=(row, 0), span=(1, 2))

        row += 1
        label = StaticText(
            parent=self,
            id=wx.ID_ANY,
            label=_("Legend font size"))
        gridSizer.Add(label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        legendfontsize = SpinCtrl(parent=self, id=wx.ID_ANY, value="",
                                  size=(50, -1), style=wx.SP_ARROW_KEYS)
        legendfontsize.SetRange(5, 100)
        legendfontsize.SetValue(
            int(self.properties['font']['prop']['legendSize']))
        self.wxId['font']['legendSize'] = legendfontsize.GetId()
        gridSizer.Add(legendfontsize, pos=(row, 1))

        row += 1
        legendshow = CheckBox(
            parent=self,
            id=wx.ID_ANY,
            label=_("Show legend"))
        legendshow.SetValue(self.properties['legend']['enabled'])
        self.wxId['legend']['enabled'] = legendshow.GetId()
        gridSizer.Add(legendshow, pos=(row, 0), span=(1, 2))

        boxMainSizer.Add(gridSizer, flag=flag, border=3)

        middleSizer.Add(boxMainSizer, flag=wx.LEFT |
                        wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=3)

        sizer.Add(middleSizer, flag=wx.ALL, border=0)

        #
        # line & buttons
        #
        line = wx.StaticLine(
            parent=self, id=wx.ID_ANY, size=(
                20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(line, proportion=0, flag=wx.GROW |
                  wx.LEFT | wx.RIGHT, border=3)

        #
        # buttons
        #
        btnSave = Button(self, wx.ID_SAVE)
        btnApply = Button(self, wx.ID_APPLY)
        btnOk = Button(self, wx.ID_OK)
        btnCancel = Button(self, wx.ID_CANCEL)
        btnOk.SetDefault()

        # tooltips for buttons
        btnApply.SetToolTip(_("Apply changes for the current session"))
        btnOk.SetToolTip(
            _("Apply changes for the current session and close dialog"))
        btnSave.SetToolTip(
            _("Apply and save changes to user settings file (default for next sessions)"))
        btnCancel.SetToolTip(_("Close dialog and ignore changes"))

        # sizers
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(btnOk)
        btnStdSizer.AddButton(btnApply)
        btnStdSizer.AddButton(btnCancel)
        btnStdSizer.Realize()

        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(
            btnSave,
            proportion=0,
            flag=wx.ALL,
            border=5)
        btnSizer.Add(
            btnStdSizer,
            proportion=0,
            flag=wx.ALL,
            border=5)
        sizer.Add(
            btnSizer,
            proportion=0,
            flag=wx.ALL,
            border=5)

        #
        # bindings for buttons and map plot settings controls
        #
        self.mapchoice.Bind(wx.EVT_CHOICE, self.OnSetMap)
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnOk.Bind(wx.EVT_BUTTON, self.OnOk)
        btnOk.SetDefault()
        btnSave.Bind(wx.EVT_BUTTON, self.OnSave)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def OnSetMap(self, event):
        """Handler for changing map selection"""
        idx = event.GetSelection()
        self.map = self.rasterList[idx]

        # update settings controls for all plots
        self.FindWindowById(
            self.wxId['pcolor']).SetColour(
            self.raster[self.map]['pcolor'])
        self.FindWindowById(
            self.wxId['plegend']).SetValue(
            self.raster[self.map]['plegend'])

        # update settings controls for histograms and profiles
        if self.plottype != 'scatter':
            self.FindWindowById(
                self.wxId['pwidth']).SetValue(
                self.raster[self.map]['pwidth'])
            self.FindWindowById(
                self.wxId['pstyle']).SetStringSelection(
                self.raster[
                    self.map]['pstyle'])

        # update settings controls for scatterplots
        elif self.plottype == 'scatter':
            self.FindWindowById(
                self.wxId['psize']).SetValue(
                self.raster[self.map]['psize'])
            self.FindWindowById(
                self.wxId['ptype']).SetStringSelection(
                self.raster[
                    self.map]['ptype'])
            self.FindWindowById(
                self.wxId['pfill']).SetStringSelection(
                self.raster[
                    self.map]['pfill'])

        self.Refresh()

    def OnSetOpt(self, event):
        """Handler for changing any other option"""
        self.map = self.rasterList[self.mapchoice.GetCurrentSelection()]
        self.UpdateSettings()
        self.parent.SetGraphStyle()
        p = self.parent.CreatePlotList()
        self.parent.DrawPlot(p)

    def UpdateSettings(self):
        """Apply settings to each map and to entire plot"""
        self.raster[self.map]['pcolor'] = self.FindWindowById(
            self.wxId['pcolor']).GetColour().Get()
        self.properties['raster']['pcolor'] = self.raster[self.map]['pcolor']

        self.raster[self.map]['plegend'] = self.FindWindowById(
            self.wxId['plegend']).GetValue()

        if self.plottype != 'scatter':
            self.raster[
                self.map]['pwidth'] = int(
                self.FindWindowById(
                    self.wxId['pwidth']).GetValue())
            self.properties['raster'][
                'pwidth'] = self.raster[self.map]['pwidth']
            self.raster[self.map]['pstyle'] = self.FindWindowById(
                self.wxId['pstyle']).GetStringSelection()
            self.properties['raster'][
                'pstyle'] = self.raster[self.map]['pstyle']

        elif self.plottype == 'scatter':
            self.raster[
                self.map]['psize'] = self.FindWindowById(
                self.wxId['psize']).GetValue()
            self.properties['raster']['psize'] = self.raster[self.map]['psize']
            self.raster[self.map]['ptype'] = self.FindWindowById(
                self.wxId['ptype']).GetStringSelection()
            self.properties['raster']['ptype'] = self.raster[self.map]['ptype']
            self.raster[self.map]['pfill'] = self.FindWindowById(
                self.wxId['pfill']).GetStringSelection()
            self.properties['raster']['pfill'] = self.raster[self.map]['pfill']

        # update settings for entire plot
        for axis in ('x-axis', 'y-axis'):
            self.properties[axis]['prop']['type'] = self.FindWindowById(
                self.wxId[axis]['type']).GetStringSelection()
            self.properties[axis]['prop']['min'] = float(
                self.FindWindowById(self.wxId[axis]['min']).GetValue())
            self.properties[axis]['prop']['max'] = float(
                self.FindWindowById(self.wxId[axis]['max']).GetValue())
            self.properties[axis]['prop']['log'] = self.FindWindowById(
                self.wxId[axis]['log']).IsChecked()

        if self.plottype == 'profile':
            self.properties['marker']['color'] = self.FindWindowById(
                self.wxId['marker']['color']).GetColour().Get()
            self.properties['marker']['fill'] = self.FindWindowById(
                self.wxId['marker']['fill']).GetStringSelection()
            self.properties['marker']['size'] = self.FindWindowById(
                self.wxId['marker']['size']).GetValue()
            self.properties['marker']['type'] = self.FindWindowById(
                self.wxId['marker']['type']).GetStringSelection()
            self.properties['marker']['legend'] = self.FindWindowById(
                self.wxId['marker']['legend']).GetValue()

        self.properties['grid']['color'] = self.FindWindowById(
            self.wxId['grid']['color']).GetColour().Get()
        self.properties['grid']['enabled'] = self.FindWindowById(
            self.wxId['grid']['enabled']).IsChecked()

        # this makes more sense in the text properties, including for settings update. But will need to change
        # layout for controls to text dialog too.
        self.properties['font']['prop']['legendSize'] = self.FindWindowById(
            self.wxId['font']['legendSize']).GetValue()
        self.properties['legend']['enabled'] = self.FindWindowById(
            self.wxId['legend']['enabled']).IsChecked()

        self.parent.UpdateLabels()

    def OnSave(self, event):
        """Button 'Save' pressed"""
        self.OnApply(None)
        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        fileSettings[self.plottype] = UserSettings.Get(group=self.plottype)
        UserSettings.SaveToFile(fileSettings)
        self._giface.WriteLog(
            _('Plot settings saved to file \'%s\'.') % UserSettings.filePath)
        self.EndModal(wx.ID_OK)

    def OnApply(self, event):
        """Button 'Apply' pressed. Does not close dialog"""
        self.UpdateSettings()
        self.parent.SetGraphStyle()
        p = self.parent.CreatePlotList()
        self.parent.DrawPlot(p)

    def OnOk(self, event):
        """Button 'OK' pressed"""
        self.OnApply(None)
        self.EndModal(wx.ID_OK)
