"""!
@package histogram2.py

Raster histogramming using PyPlot (wx.lib.plot.py); replacement for d.hist

Classes:
 - HistFrame
 - SetRasterDialog
 - TextDialog
 - OptDialog

(C) 2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton, Arizona State University
"""

import os
import sys
import math
import random

import wx
import wx.lib.colourselect as  csel

import os
import sys

import wx

import render
import menuform
import disp_print
import utils
import gdialogs
import globalvar
import gselect
import gcmd
import toolbars
from preferences import DefaultFontDialog
from debug import Debug as Debug
from icon import Icons as Icons
from gcmd import GError
from preferences import globalSettings as UserSettings

from grass.script import core as grass
from grass.script import raster as raster

try:
    import numpy
    import wx.lib.plot as plot
except ImportError:
    msg= _("This module requires the NumPy module, which could not be "
           "imported. It probably is not installed (it's not part of the "
           "standard Python distribution). See the Numeric Python site "
           "(http://numpy.scipy.org) for information on downloading source or "
           "binaries.")
    print >> sys.stderr, "histogram2.py: " + msg

class HistFrame(wx.Frame):
    """!Mainframe for displaying profile of raster map. Uses wx.lib.plot.
    """
    def __init__(self, parent=None, id=wx.ID_ANY, title=_("GRASS Histogramming Tool"),
                 rasterList=[],
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_FRAME_STYLE):

        self.parent = parent            # MapFrame
        self.mapwin = self.parent.MapWindow
        self.Map = render.Map()         # instance of render.Map to be associated with display
        self.rasterList = rasterList    #list of rasters to histogram; could come from layer manager

        self.pstyledict = { 'solid' : wx.SOLID,
                            'dot' : wx.DOT,
                            'long-dash' : wx.LONG_DASH,
                            'short-dash' : wx.SHORT_DASH,
                            'dot-dash' : wx.DOT_DASH }

        self.ptfilldict = { 'transparent' : wx.TRANSPARENT,
                            'solid' : wx.SOLID }

        wx.Frame.__init__(self, parent, id, title, pos, size, style)

        #
        # Icon
        #
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        
        #
        # Add toolbar
        #
        self.toolbar = toolbars.Histogram2Toolbar(parent=self)
        self.SetToolBar(self.toolbar)
        
        #
        # Set the size & cursor
        #
        self.SetClientSize(size)

        #
        # Add statusbar
        #
        self.statusbar = self.CreateStatusBar(number=2, style=0)
        self.statusbar.SetStatusWidths([-2, -1])

        #
        # Define canvas
        #
        self.client = plot.PlotCanvas(self) # plot canvas settings
        
        # Create mouse event for showing cursor coords in status bar
        self.client.canvas.Bind(wx.EVT_LEFT_DOWN, self.OnMouseLeftDown)
        
        # Show closest point when enabled
        self.client.canvas.Bind(wx.EVT_MOTION, self.OnMotion)

        #
        # Init variables
        #
        self.group = ''

        self.raster = {}

        if len(self.rasterList) > 0: # set 1 raster name from layer manager if a map is selected
            self.raster[self.rasterList[0]] = UserSettings.Get(group='histogram', key='raster') # some default settings
            self.raster[self.rasterList[0]]['units'] = ''
            self.raster[self.rasterList[0]]['plegend'] = '' 
            self.raster[self.rasterList[0]]['datalist'] = [] # list of cell value,frequency pairs for plotting histogram
            self.raster[self.rasterList[0]]['pline'] = None
            colstr = str(self.raster[self.rasterList[0]]['pcolor']) # changing color string to tuple
            self.raster[self.rasterList[0]]['pcolor'] = tuple(int(colval) for colval in colstr.strip('()').split(','))

        self.plotlist = []                      # list of things to plot
        self.histogram = None                   # plot draw object
        self.ptitle = _('Histogram of')         # title of window
        self.xlabel = _("Raster cell values")   # default X-axis label
        self.ylabel = _("Cell counts")          # default Y-axis label
        self.maptype = 'raster'                 # default type of histogram to plot

        self.properties = {}                    # plot properties

        self.properties['font'] = {}
        self.properties['font']['prop'] = UserSettings.Get(group='histogram', key='font')
        self.properties['font']['wxfont'] = wx.Font(11, wx.FONTFAMILY_SWISS,
                                                    wx.FONTSTYLE_NORMAL,
                                                    wx.FONTWEIGHT_NORMAL)
        
        self.properties['grid'] = UserSettings.Get(group='histogram', key='grid')        
        colstr = str(self.properties['grid']['color']) # changing color string to tuple        
        self.properties['grid']['color'] = tuple(int(colval) for colval in colstr.strip('()').split(','))
                
        self.properties['x-axis'] = {}
        self.properties['x-axis']['prop'] = UserSettings.Get(group='histogram', key='x-axis')
        self.properties['x-axis']['axis'] = None

        self.properties['y-axis'] = {}
        self.properties['y-axis']['prop'] = UserSettings.Get(group='histogram', key='y-axis')
        self.properties['y-axis']['axis'] = None
        
        self.properties['legend'] = UserSettings.Get(group='histogram', key='legend')
        
        self.histtype = 'count' 
        self.bins = 255

        self.zoom = False  # zooming disabled
        self.drag = False  # draging disabled
        self.client.SetShowScrollbars(True) # vertical and horizontal scrollbars

        # x and y axis set to normal (non-log)
        self.client.setLogScale((False, False))
        if self.properties['x-axis']['prop']['type']:
            self.client.SetXSpec(self.properties['x-axis']['prop']['type'])
        else:
            self.client.SetXSpec('auto')
        
        if self.properties['y-axis']['prop']['type']:
            self.client.SetYSpec(self.properties['y-axis']['prop']['type'])
        else:
            self.client.SetYSpec('auto')

        #
        # Bind various events
        #
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        
        self.CentreOnScreen()
        
        self._createColorDict()
        
    def _createColorDict(self):
        """!Create color dictionary to return wx.Color tuples
        for assigning colors to images in imagery groups"""
                
        self.colorDict = {}
        for clr in grass.named_colors.iterkeys():
            if clr == 'white' or clr == 'black': continue
            r = grass.named_colors[clr][0] * 255
            g = grass.named_colors[clr][1] * 255
            b = grass.named_colors[clr][2] * 255
            self.colorDict[clr] = (r,g,b,255)
            
    def OnSelectRaster(self, event):
        """!Select raster map(s) to profile
        """
        dlg = SetRasterDialog(parent=self)

        if dlg.ShowModal() == wx.ID_OK:
            self.rasterList = dlg.rasterList
            self.group = dlg.group
            self.bins = dlg.bins
            self.histtype = dlg.histtype
            self.maptype = dlg.maptype

            # plot profile
            if len(self.rasterList) > 0:
                self.OnCreateHist(event=None)

        dlg.Destroy()

    def OnCreateHist(self, event):
        """!Main routine for creating a histogram. Uses r.stats to
        create a list of cell value and count/percent/area pairs. This is passed to
        plot to create a line graph of the histogram.
        """

        self.SetCursor(self.parent.cursors["default"])
        self.SetGraphStyle()
        self.SetRaster()
        self.DrawPlot()

    def SetGraphStyle(self):
        """!Set plot and text options
        """
        self.client.SetFont(self.properties['font']['wxfont'])
        self.client.SetFontSizeTitle(self.properties['font']['prop']['titleSize'])
        self.client.SetFontSizeAxis(self.properties['font']['prop']['axisSize'])

        self.client.SetEnableZoom(self.zoom)
        self.client.SetEnableDrag(self.drag)
        
        #
        # axis settings
        #
        if self.properties['x-axis']['prop']['type'] == 'custom':
            self.client.SetXSpec('min')
        else:
            self.client.SetXSpec(self.properties['x-axis']['prop']['type'])

        if self.properties['y-axis']['prop']['type'] == 'custom':
            self.client.SetYSpec('min')
        else:
            self.client.SetYSpec(self.properties['y-axis']['prop']['type'])

        if self.properties['x-axis']['prop']['type'] == 'custom' and \
               self.properties['x-axis']['prop']['min'] < self.properties['x-axis']['prop']['max']:
            self.properties['x-axis']['axis'] = (self.properties['x-axis']['prop']['min'],
                                                 self.properties['x-axis']['prop']['max'])
        else:
            self.properties['x-axis']['axis'] = None

        if self.properties['y-axis']['prop']['type'] == 'custom' and \
                self.properties['y-axis']['prop']['min'] < self.properties['y-axis']['prop']['max']:
            self.properties['y-axis']['axis'] = (self.properties['y-axis']['prop']['min'],
                                                 self.properties['y-axis']['prop']['max'])
        else:
            self.properties['y-axis']['axis'] = None

        self.client.SetEnableGrid(self.properties['grid']['enabled'])
        
        self.client.SetGridColour(wx.Color(self.properties['grid']['color'][0],
                                           self.properties['grid']['color'][1],
                                           self.properties['grid']['color'][2],
                                           255))

        self.client.SetFontSizeLegend(self.properties['font']['prop']['legendSize'])
        self.client.SetEnableLegend(self.properties['legend']['enabled'])

        if self.properties['x-axis']['prop']['log'] == True:
            self.properties['x-axis']['axis'] = None
            self.client.SetXSpec('min')
        if self.properties['y-axis']['prop']['log'] == True:
            self.properties['y-axis']['axis'] = None
            self.client.SetYSpec('min')
            
        self.client.setLogScale((self.properties['x-axis']['prop']['log'],
                                 self.properties['y-axis']['prop']['log']))

    def SetRaster(self):
        """!Build data list for ploting each raster."""

        #
        # populate raster dictionary
        #
        if len(self.rasterList) == 0: return  # nothing selected
        
        colorList = []
        for clr in ["blue", "green", "red", "yellow", "magenta", "cyan", \
                    "aqua", "grey", "orange", "brown", "purple", "violet", \
                    "indigo"]:
            colorList.append(self.colorDict[clr])
            
        if len(self.rasterList) > len(colorList):
            # just in case the imagery group has many maps
            diff = len(self.rasterList) - len(colorList)
            for x in range(0, diff):
                r = randint(0, 255)
                b = randint(0, 255)
                g = randint(0, 255)
                colorList.append((r,g,b,255))
        
        rastcolor = zip(self.rasterList, colorList)
        
        for r,c in rastcolor:
            if r in self.raster: continue   # only reset values if rasters have changed
            self.raster[r] = {}
            try:
                ret = raster.raster_info(r)
            except:
                continue
            self.raster[r]['datatype'] = 'CELL'
            self.raster[r]['datatype'] = ret['datatype']
            self.raster[r]['units'] = ret['units']
            self.raster[r]['plegend'] = r   # raster name to use in legend 
            self.raster[r]['datalist'] = [] # list of cell value,frequency pairs for plotting histogram
            self.raster[r]['pline'] = None
            if 'pcolor' not in self.raster[r]:
                self.raster[r]['pcolor'] = c
            if 'pwidth' not in self.raster[r]:        
                self.raster[r]['pwidth'] = 1
            if 'pstyle' not in self.raster[r]:
                self.raster[r]['pstyle'] = 'solid'

        self.ptitle = _('Histogram of') # reset window title

        #
        # create datalist for each raster map
        #
        for r in self.rasterList:
            self.raster[r]['datalist'] = self.CreateDatalist(r)
            
        #
        # update title
        #
        if self.maptype == 'group':
            self.ptitle = _('Histogram of %s') % self.group.split('@')[0] 
        else: 
            self.ptitle = _('Histogram of %s') % self.rasterList[0].split('@')[0] 

        
        #
        # set xlabel based on first raster map in list to be histogrammed
        #
        units = self.raster[self.rasterList[0]]['units']
        if units != '' and units != '(none)' and units != None:
            self.xlabel = _('Raster cell values %s') % units
        else:
            self.xlabel = _('Raster cell values') 

        #
        # set ylabel from self.histtype
        #
        if self.histtype == 'count': self.ylabel = _('Cell counts')
        if self.histtype == 'percent': self.ylabel = _('Percent of total cells')
        if self.histtype == 'area': self.ylabel = _('Area')

    def CreateDatalist(self, raster):
        """!Build a list of cell value, frequency pairs for histogram
            frequency can be in cell counts, percents, or area
        """
        datalist = []
        
        if self.histtype == 'count': freqflag = 'cn'
        if self.histtype == 'percent': freqflag = 'pn'
        if self.histtype == 'area': freqflag = 'an'
                
        try:
            ret = gcmd.RunCommand("r.stats",
                                  parent = self,
                                  input=raster,
                                  flags=freqflag,
                                  nsteps=self.bins,
                                  fs=',',
                                  quiet = True,
                                  read = True)
            
            if not ret:
                return datalist
            
            for line in ret.splitlines():
                cellval, histval = line.strip().split(',')
                histval = histval.strip()
                if self.raster[raster]['datatype'] != 'CELL':
                    cellval = cellval.split('-')[0]
                if self.histtype == 'percent':
                    histval = histval.rstrip('%')
                    
                datalist.append((cellval,histval))

            return datalist
        except gcmd.GException, e:
            gcmd.GError(parent = self,
                        message = e.value)
            return None

    #### Maybe add a barplot routine to make an 'area' graph. Could make it slow down a lot
    def DrawPlot(self):
        """!Draw line and point plot from transect datalist and
        transect segment endpoint coordinates.
        """
        # graph the cell value, frequency pairs for the histogram
        self.plotlist = []

        for r in self.rasterList:
            if len(self.raster[r]['datalist']) > 0:
                col = wx.Color(self.raster[r]['pcolor'][0],
                               self.raster[r]['pcolor'][1],
                               self.raster[r]['pcolor'][2],
                               255)
                self.raster[r]['pline'] = plot.PolyLine(self.raster[r]['datalist'],
                                           colour=col,
                                           width=self.raster[r]['pwidth'],
                                           style=self.pstyledict[self.raster[r]['pstyle']],
                                           legend=self.raster[r]['plegend'])

                self.plotlist.append(self.raster[r]['pline'])

        self.histogram = plot.PlotGraphics(self.plotlist,
                                         self.ptitle,
                                         self.xlabel,
                                         self.ylabel)

        if self.properties['x-axis']['prop']['type'] == 'custom':
            self.client.SetXSpec('min')
        else:
            self.client.SetXSpec(self.properties['x-axis']['prop']['type'])

        if self.properties['y-axis']['prop']['type'] == 'custom':
            self.client.SetYSpec('min')
        else:
            self.client.SetYSpec(self.properties['y-axis']['prop']['type'])

        self.client.Draw(self.histogram, self.properties['x-axis']['axis'],
                         self.properties['y-axis']['axis'])

    def OnZoom(self, event):
        """!Enable zooming and disable dragging
        """
        self.zoom = True
        self.drag = False
        self.client.SetEnableZoom(self.zoom)
        self.client.SetEnableDrag(self.drag)

    def OnDrag(self, event):
        """!Enable dragging and disable zooming
        """
        self.zoom = False
        self.drag = True
        self.client.SetEnableDrag(self.drag)
        self.client.SetEnableZoom(self.zoom)

    def OnRedraw(self, event):
        """!Redraw the hisogram window. Unzoom to original size
        """
        self.client.Reset()
        self.client.Redraw()

    def Update(self):
        """!Update histogram after changing options
        """
        self.SetGraphStyle()
        self.DrawPlot()

    def OnErase(self, event):
        """!Erase the histogram window
        """
        self.client.Clear()
        self.mapwin.ClearLines(self.mapwin.pdc)
        self.mapwin.ClearLines(self.mapwin.pdcTmp)
        self.mapwin.polycoords = []
        self.mapwin.Refresh()

    def SaveToFile(self, event):
        """!Save histogram to graphics file
        """
        self.client.SaveFile()

    def OnMouseLeftDown(self,event):
        s= "Left Mouse Down at Point: (%.4f, %.4f)" % self.client._getXY(event)
        self.SetStatusText(s)
        event.Skip()            #allows plotCanvas OnMouseLeftDown to be called

    def OnMotion(self, event):
        # indicate when mouse is outside the plot area
        if self.client.OnLeave(event): print 'out of area'
        #show closest point (when enbled)
        if self.client.GetEnablePointLabel() == True:
            #make up dict with info for the pointLabel
            #I've decided to mark the closest point on the closest curve
            dlst= self.client.GetClosetPoint( self.client._getXY(event), pointScaled= True)
            if dlst != []:      #returns [] if none
                curveNum, legend, pIndex, pointXY, scaledXY, distance = dlst
                #make up dictionary to pass to my user function (see DrawPointLabel)
                mDataDict= {"curveNum":curveNum, "legend":legend, "pIndex":pIndex,\
                    "pointXY":pointXY, "scaledXY":scaledXY}
                #pass dict to update the pointLabel
                self.client.UpdatePointLabel(mDataDict)
        event.Skip()           #go to next handler

    def HistOptionsMenu(self, event):
        """!Popup menu for histogram and text options
        """
        point = wx.GetMousePosition()
        popt = wx.Menu()
        # Add items to the menu
        settext = wx.MenuItem(popt, -1, 'Histogram text settings')
        popt.AppendItem(settext)
        self.Bind(wx.EVT_MENU, self.PText, settext)

        setgrid = wx.MenuItem(popt, -1, 'Histogram plot settings')
        popt.AppendItem(setgrid)
        self.Bind(wx.EVT_MENU, self.POptions, setgrid)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(popt)
        popt.Destroy()

    def NotFunctional(self):
        """!Creates a 'not functional' message dialog
        """
        dlg = wx.MessageDialog(parent = self,
                               message = _('This feature is not yet functional'),
                               caption = _('Under Construction'),
                               style = wx.OK | wx.ICON_INFORMATION)
        dlg.ShowModal()
        dlg.Destroy()

    def OnPText(self, dlg):
        """!Custom text settings for histogram plot.
        """
        self.ptitle = dlg.ptitle
        self.xlabel = dlg.xlabel
        self.ylabel = dlg.ylabel
        dlg.UpdateSettings()

        self.client.SetFont(self.properties['font']['wxfont'])
        self.client.SetFontSizeTitle(self.properties['font']['prop']['titleSize'])
        self.client.SetFontSizeAxis(self.properties['font']['prop']['axisSize'])

        if self.histogram:
            self.histogram.setTitle(dlg.ptitle)
            self.histogram.setXLabel(dlg.xlabel)
            self.histogram.setYLabel(dlg.ylabel)
        
        self.OnRedraw(event=None)
    
    def PText(self, event):
        """!Set custom text values for profile title and axis labels.
        """
        dlg = TextDialog(parent=self, id=wx.ID_ANY, title=_('Histogram text settings'))

        if dlg.ShowModal() == wx.ID_OK:
            self.OnPText(dlg)

        dlg.Destroy()

    def POptions(self, event):
        """!Set various profile options, including: line width, color,
        style; marker size, color, fill, and style; grid and legend
        options.  Calls OptDialog class.
        """
        dlg = OptDialog(parent=self, id=wx.ID_ANY, title=_('Histogram settings'))
        btnval = dlg.ShowModal()

        if btnval == wx.ID_SAVE:
            dlg.UpdateSettings()            
            self.SetGraphStyle()            
            dlg.Destroy()            
        elif btnval == wx.ID_CANCEL:
            dlg.Destroy()

    def PrintMenu(self, event):
        """!Print options and output menu
        """
        point = wx.GetMousePosition()
        printmenu = wx.Menu()
        # Add items to the menu
        setup = wx.MenuItem(printmenu, -1,'Page setup')
        printmenu.AppendItem(setup)
        self.Bind(wx.EVT_MENU, self.OnPageSetup, setup)

        preview = wx.MenuItem(printmenu, -1,'Print preview')
        printmenu.AppendItem(preview)
        self.Bind(wx.EVT_MENU, self.OnPrintPreview, preview)

        doprint = wx.MenuItem(printmenu, -1,'Print display')
        printmenu.AppendItem(doprint)
        self.Bind(wx.EVT_MENU, self.OnDoPrint, doprint)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(printmenu)
        printmenu.Destroy()

    def OnPageSetup(self, event):
        self.client.PageSetup()

    def OnPrintPreview(self, event):
        self.client.PrintPreview()

    def OnDoPrint(self, event):
        self.client.Printout()

    def OnQuit(self, event):
        self.Close(True)

    def OnCloseWindow(self, event):
        """
        Close histogram window and clean up
        """

        self.mapwin.SetCursor(self.Parent.cursors["default"])
        self.Destroy()

class SetRasterDialog(wx.Dialog):
    def __init__(self, parent, id=wx.ID_ANY, 
                 title=_("Select raster map or imagery group to histogram"),
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_DIALOG_STYLE):
        """!Dialog to select raster maps to histogram.
        """

        wx.Dialog.__init__(self, parent, id, title, pos, size, style)

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

        box = wx.GridBagSizer (hgap=3, vgap=3)
        
        #
        # select single raster or image group to histogram radio buttons
        #
        self.rasterRadio = wx.RadioButton(self, id = wx.ID_ANY, label = " %s " % _("Histogram single raster"), style = wx.RB_GROUP)
        self.groupRadio = wx.RadioButton(self, id = wx.ID_ANY, label = " %s " % _("Histogram imagery group"))
        if self.maptype == 'raster': 
            self.rasterRadio.SetValue(True)
        elif self.maptype == 'group': 
            self.groupRadio.SetValue(True)
        box.Add(item=self.rasterRadio, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 0))
        box.Add(item=self.groupRadio, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 1))
        
        #
        # Select a raster to histogram
        #
        label = wx.StaticText(parent=self, id=wx.ID_ANY, 
                              label=_("Select raster map:"))
        box.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(1, 0))
        self.rselection = gselect.Select(self, id=wx.ID_ANY,
                                   size=globalvar.DIALOG_GSELECT_SIZE,
                                   type='cell')
        if self.groupRadio.GetValue() == True: 
            self.rselection.Disable()
        else:
            if len(self.rasterList) > 0: self.rselection.SetValue(self.rasterList[0])
        box.Add(item=self.rselection, pos=(1, 1))       

        #
        # Select an image group to histogram
        #
        label = wx.StaticText(parent=self, id=wx.ID_ANY, 
                              label=_("Select image group:"))
        box.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(2, 0))
        self.gselection = gselect.Select(self, id=wx.ID_ANY,
                                   size=globalvar.DIALOG_GSELECT_SIZE,
                                   type='group')
        if self.rasterRadio.GetValue() == True: 
            self.gselection.Disable()
        else:
            if self.group != None: self.gselection.SetValue(self.group)
        box.Add(item=self.gselection, pos=(2, 1))
            
        #
        # Nsteps for FP maps and histogram type selection
        #

        label = wx.StaticText(parent=self, id=wx.ID_ANY, 
                              label=_("Number of bins (for FP maps)"))
        box.Add(item=label,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(3, 0))
        self.spinbins = wx.SpinCtrl(parent=self, id=wx.ID_ANY, value="", pos=(30, 50),
                                      size=(100,-1), style=wx.SP_ARROW_KEYS)
        self.spinbins.SetRange(1,1000)
        self.spinbins.SetValue(self.bins)
        box.Add(item=self.spinbins,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(3, 1))

        label = wx.StaticText(parent=self, id=wx.ID_ANY, 
                              label=_("Histogram type"))
        box.Add(item=label,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(4, 0))
        types = ['count', 'percent', 'area']
        histtype = wx.ComboBox(parent=self, id=wx.ID_ANY, size=(250, -1),
                                choices=types, style=wx.CB_DROPDOWN)
        histtype.SetStringSelection(self.histtype)
        box.Add(item=histtype,
                flag=wx.ALIGN_CENTER_VERTICAL, pos=(4, 1))
          
        sizer.Add(item=box, proportion=0,
                  flag=wx.ALL, border=10)

        line = wx.StaticLine(parent=self, id=wx.ID_ANY, size=(20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(item=line, proportion=0,
                  flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.RIGHT, border=5)

        btnsizer = wx.StdDialogButtonSizer()

        btn = wx.Button(self, wx.ID_OK)
        btn.SetDefault()
        btnsizer.AddButton(btn)

        btn = wx.Button(self, wx.ID_CANCEL)
        btnsizer.AddButton(btn)
        btnsizer.Realize()

        sizer.Add(item=btnsizer, proportion=0, flag=wx.ALIGN_RIGHT | wx.ALL, border=5)

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
        """!Hander for radio buttons to choose between histogramming a
            single raster and an imagery group
        """
        if self.rasterRadio.GetValue() == True:
            self.maptype = 'raster'
            self.rselection.Enable()
            self.gselection.Disable()
            self.gselection.SetValue('')
        elif self.groupRadio.GetValue() == True:
            self.maptype = 'group'
            self.gselection.Enable()
            self.rselection.Disable()
            self.rselection.SetValue('')
        else:
            pass
        
    def OnRasterSelection(self, event):
        """!Handler for selecting a single raster map
        """
        self.rasterList = []
        self.rasterList.append(event.GetString())

    def OnGroupSelection(self, event):
        """!Handler for selecting imagery group
        """
        self.rasterList = []
        self.group = event.GetString()
        self.rasterList = grass.read_command('i.group', 
                                            group='%s' % self.group, 
                                            quiet=True,
                                            flags='g').strip().split('\n')
                                                
    def OnSetBins(self, event):
        """!Bins for histogramming FP maps (=nsteps in r.stats)
        """
        self.bins = self.spinbins.GetValue()
        
    def OnSetHisttypes(self, event):
        self.histtype = event.GetString()
        print 'histtype = ' + self.histtype

class TextDialog(wx.Dialog):
    def __init__(self, parent, id, title, pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_DIALOG_STYLE):
        """!Dialog to set histogram text options: font, title
        and font size, axis labels and font size
        """
        wx.Dialog.__init__(self, parent, id, title, pos, size, style)
        #
        # initialize variables
        #
        # combo box entry lists
        self.ffamilydict = { 'default' : wx.FONTFAMILY_DEFAULT,
                             'decorative' : wx.FONTFAMILY_DECORATIVE,
                             'roman' : wx.FONTFAMILY_ROMAN,
                             'script' : wx.FONTFAMILY_SCRIPT,
                             'swiss' : wx.FONTFAMILY_SWISS,
                             'modern' : wx.FONTFAMILY_MODERN,
                             'teletype' : wx.FONTFAMILY_TELETYPE }

        self.fstyledict = { 'normal' : wx.FONTSTYLE_NORMAL,
                            'slant' : wx.FONTSTYLE_SLANT,
                            'italic' : wx.FONTSTYLE_ITALIC }

        self.fwtdict = { 'normal' : wx.FONTWEIGHT_NORMAL,
                         'light' : wx.FONTWEIGHT_LIGHT,
                         'bold' : wx.FONTWEIGHT_BOLD }

        self.parent = parent

        self.ptitle = self.parent.ptitle
        self.xlabel = self.parent.xlabel
        self.ylabel = self.parent.ylabel

        self.properties = self.parent.properties # read-only
        
        # font size
        self.fontfamily = self.properties['font']['wxfont'].GetFamily()
        self.fontstyle = self.properties['font']['wxfont'].GetStyle()
        self.fontweight = self.properties['font']['wxfont'].GetWeight()

        self._do_layout()
        
        
#### This stays with relevant changes to dictionary/list names
    def _do_layout(self):
        """!Do layout"""
        # dialog layout
        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                           label=" %s " % _("Text settings"))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        #
        # profile title
        #
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Profile title:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 0))
        self.ptitleentry = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(250,-1))
        # self.ptitleentry.SetFont(self.font)
        self.ptitleentry.SetValue(self.ptitle)
        gridSizer.Add(item=self.ptitleentry, pos=(0, 1))

        #
        # title font
        #
        tlabel = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Title font size (pts):"))
        gridSizer.Add(item=tlabel, flag=wx.ALIGN_CENTER_VERTICAL, pos=(1, 0))
        self.ptitlesize = wx.SpinCtrl(parent=self, id=wx.ID_ANY, value="", pos=(30, 50),
                                      size=(50,-1), style=wx.SP_ARROW_KEYS)
        self.ptitlesize.SetRange(5,100)
        self.ptitlesize.SetValue(int(self.properties['font']['prop']['titleSize']))
        gridSizer.Add(item=self.ptitlesize, pos=(1, 1))

        #
        # x-axis label
        #
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("X-axis label:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(2, 0))
        self.xlabelentry = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(250,-1))
        # self.xlabelentry.SetFont(self.font)
        self.xlabelentry.SetValue(self.xlabel)
        gridSizer.Add(item=self.xlabelentry, pos=(2, 1))

        #
        # y-axis label
        #
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Y-axis label:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(3, 0))
        self.ylabelentry = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(250,-1))
        # self.ylabelentry.SetFont(self.font)
        self.ylabelentry.SetValue(self.ylabel)
        gridSizer.Add(item=self.ylabelentry, pos=(3, 1))

        #
        # font size
        #
        llabel = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Label font size (pts):"))
        gridSizer.Add(item=llabel, flag=wx.ALIGN_CENTER_VERTICAL, pos=(4, 0))
        self.axislabelsize = wx.SpinCtrl(parent=self, id=wx.ID_ANY, value="", pos=(30, 50),
                                         size=(50, -1), style=wx.SP_ARROW_KEYS)
        self.axislabelsize.SetRange(5, 100) 
        self.axislabelsize.SetValue(int(self.properties['font']['prop']['axisSize']))
        gridSizer.Add(item=self.axislabelsize, pos=(4,1))

        boxSizer.Add(item=gridSizer)
        sizer.Add(item=boxSizer, flag=wx.ALL | wx.EXPAND, border=3)

        #
        # font settings
        #
        box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                           label=" %s " % _("Font settings"))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
        gridSizer.AddGrowableCol(1)

        #
        # font family
        #
        label1 = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Font family:"))
        gridSizer.Add(item=label1, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 0))
        self.ffamilycb = wx.ComboBox(parent=self, id=wx.ID_ANY, size=(250, -1),
                                choices=self.ffamilydict.keys(), style=wx.CB_DROPDOWN)
        self.ffamilycb.SetStringSelection('swiss')
        for item in self.ffamilydict.items():
            if self.fontfamily == item[1]:
                self.ffamilycb.SetStringSelection(item[0])
                break
        gridSizer.Add(item=self.ffamilycb, pos=(0, 1), flag=wx.ALIGN_RIGHT)

        #
        # font style
        #
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Style:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(1, 0))
        self.fstylecb = wx.ComboBox(parent=self, id=wx.ID_ANY, size=(250, -1),
                                    choices=self.fstyledict.keys(), style=wx.CB_DROPDOWN)
        self.fstylecb.SetStringSelection('normal')
        for item in self.fstyledict.items():
            if self.fontstyle == item[1]:
                self.fstylecb.SetStringSelection(item[0])
                break
        gridSizer.Add(item=self.fstylecb, pos=(1, 1), flag=wx.ALIGN_RIGHT)

        #
        # font weight
        #
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Weight:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(2, 0))
        self.fwtcb = wx.ComboBox(parent=self, size=(250, -1),
                                 choices=self.fwtdict.keys(), style=wx.CB_DROPDOWN)
        self.fwtcb.SetStringSelection('normal')
        for item in self.fwtdict.items():
            if self.fontweight == item[1]:
                self.fwtcb.SetStringSelection(item[0])
                break

        gridSizer.Add(item=self.fwtcb, pos=(2, 1), flag=wx.ALIGN_RIGHT)
                      
        boxSizer.Add(item=gridSizer, flag=wx.EXPAND)
        sizer.Add(item=boxSizer, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=3)

        line = wx.StaticLine(parent=self, id=wx.ID_ANY, size=(20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(item=line, proportion=0,
                  flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.RIGHT, border=3)

        #
        # buttons
        #
        btnSave = wx.Button(self, wx.ID_SAVE)
        btnApply = wx.Button(self, wx.ID_APPLY)
        btnOk = wx.Button(self, wx.ID_OK)
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnOk.SetDefault()

        # bindings
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnApply.SetToolTipString(_("Apply changes for the current session"))
        btnOk.Bind(wx.EVT_BUTTON, self.OnOk)
        btnOk.SetToolTipString(_("Apply changes for the current session and close dialog"))
        btnOk.SetDefault()
        btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        btnSave.SetToolTipString(_("Apply and save changes to user settings file (default for next sessions)"))
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        btnCancel.SetToolTipString(_("Close dialog and ignore changes"))

        # sizers
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(btnOk)
        btnStdSizer.AddButton(btnApply)
        btnStdSizer.AddButton(btnCancel)
        btnStdSizer.Realize()
        
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(item=btnSave, proportion=0, flag=wx.ALIGN_LEFT | wx.ALL, border=5)
        btnSizer.Add(item=btnStdSizer, proportion=0, flag=wx.ALIGN_RIGHT | wx.ALL, border=5)
        sizer.Add(item=btnSizer, proportion=0, flag=wx.ALIGN_RIGHT | wx.ALL, border=5)

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
        self.properties['font']['prop']['titleSize'] = self.ptitlesize.GetValue()
        self.properties['font']['prop']['axisSize'] = self.axislabelsize.GetValue()

        family = self.ffamilydict[self.ffamilycb.GetStringSelection()]
        self.properties['font']['wxfont'].SetFamily(family)
        style = self.fstyledict[self.fstylecb.GetStringSelection()]
        self.properties['font']['wxfont'].SetStyle(style)
        weight = self.fwtdict[self.fwtcb.GetStringSelection()]
        self.properties['font']['wxfont'].SetWeight(weight)

    def OnSave(self, event):
        """!Button 'Save' pressed"""
        self.UpdateSettings()
        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        fileSettings['histogram'] = UserSettings.Get(group='histogram')
        file = UserSettings.SaveToFile(fileSettings)
        self.parent.parent.GetLayerManager().goutput.WriteLog(_('Histogram settings saved to file \'%s\'.') % file)
        self.EndModal(wx.ID_OK)

    def OnApply(self, event):
        """!Button 'Apply' pressed"""
        self.UpdateSettings()
        self.parent.OnPText(self)
        
    def OnOk(self, event):
        """!Button 'OK' pressed"""
        self.UpdateSettings()
        self.EndModal(wx.ID_OK)

    def OnCancel(self, event):
        """!Button 'Cancel' pressed"""
        self.EndModal(wx.ID_CANCEL)
        
class OptDialog(wx.Dialog):
    def __init__(self, parent, id, title, pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_DIALOG_STYLE): 
                     
        """!Dialog to set various profile options, including: line
        width, color, style; marker size, color, fill, and style; grid
        and legend options.
        """
        wx.Dialog.__init__(self, parent, id, title, pos, size, style)
        # init variables
        self.pstyledict = parent.pstyledict
        self.ptfilldict = parent.ptfilldict
        
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
            wx.MessageBox(parent = self,
                              message = _("No map or image group selected to histogram."),
                              caption = _("Warning"), style = wx.OK | wx.ICON_ERROR)
            
        self._do_layout()

    def _do_layout(self):
        """!Do layout"""
        # dialog layout
        sizer = wx.BoxSizer(wx.VERTICAL)

        #
        # histogram line settings
        #
        box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                           label=" %s " % _("Histogram settings"))
        boxMainSizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)

        self.wxId['pcolor']  = 0
        self.wxId['pwidth']  = 0
        self.wxId['pstyle']  = 0
        self.wxId['plegend'] = 0

        if len(self.rasterList) > 0:
            box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                               label=_("Map/image histogrammed"))
            boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
            
            gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
            
            row = 0
            self.mapchoice = wx.Choice(parent = self, id = wx.ID_ANY, size = (300, -1),
                                       choices = self.rasterList)
            if self.map == None or self.map == '':
                self.map = self.rasterList[self.mapchoice.GetCurrentSelection()]
            else:
                self.mapchoice.SetStringSelection(self.map)
            gridSizer.Add(item=self.mapchoice, flag=wx.ALIGN_CENTER_VERTICAL, 
                          pos=(row, 0), span=(1, 2))
            
            row +=1            
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Line color"))
            gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            pcolor = csel.ColourSelect(parent=self, id=wx.ID_ANY, colour=self.raster[self.map]['pcolor'])
            self.wxId['pcolor'] = pcolor.GetId()
            gridSizer.Add(item=pcolor, pos=(row, 1))

            row += 1
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Line width"))
            gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            pwidth = wx.SpinCtrl(parent=self, id=wx.ID_ANY, value="",
                                 size=(50,-1), style=wx.SP_ARROW_KEYS)
            pwidth.SetRange(1, 10)
            pwidth.SetValue(self.raster[self.map]['pwidth'])
            self.wxId['pwidth'] = pwidth.GetId()
            gridSizer.Add(item=pwidth, pos=(row, 1))

            row +=1
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Line style"))
            gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            pstyle = wx.Choice(parent=self, id=wx.ID_ANY, 
                                 size=(120, -1), choices=self.pstyledict.keys(), style=wx.CB_DROPDOWN)
            pstyle.SetStringSelection(self.raster[self.map]['pstyle'])
            self.wxId['pstyle'] = pstyle.GetId()
            gridSizer.Add(item=pstyle, pos=(row, 1))

            row += 1
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Legend"))
            gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            plegend = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(200,-1))
            plegend.SetValue(self.raster[self.map]['plegend'])
            gridSizer.Add(item=plegend, pos=(row, 1))
            self.wxId['plegend'] = plegend.GetId()
            boxSizer.Add(item=gridSizer)
                
            flag = wx.ALL
            boxMainSizer.Add(item=boxSizer, flag=flag, border=3)
                
            sizer.Add(item=boxMainSizer, flag=wx.ALL | wx.EXPAND, border=3)

        middleSizer = wx.BoxSizer(wx.HORIZONTAL)

        #
        # axis options
        #
        box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                           label=" %s " % _("Axis settings"))
        boxMainSizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)

        self.wxId['x-axis'] = {}
        self.wxId['y-axis'] = {}

        idx = 0
        for axis, atype in [(_("X-Axis"), 'x-axis'),
                     (_("Y-Axis"), 'y-axis')]:
            box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                               label=" %s " % axis)
            boxSizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
            gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

            prop = self.properties[atype]['prop']
            
            row = 0
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Style"))
            gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            type = wx.Choice(parent=self, id=wx.ID_ANY,
                               size=(100, -1), choices=self.axislist, style=wx.CB_DROPDOWN)
            type.SetStringSelection(prop['type']) 
            self.wxId[atype]['type'] = type.GetId()
            gridSizer.Add(item=type, pos=(row, 1))
                        
            row += 1
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Custom min"))
            gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            min = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(70, -1))
            min.SetValue(str(prop['min']))
            self.wxId[atype]['min'] = min.GetId()
            gridSizer.Add(item=min, pos=(row, 1))

            row += 1
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Custom max"))
            gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            max = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(70, -1))
            max.SetValue(str(prop['max']))
            self.wxId[atype]['max'] = max.GetId()
            gridSizer.Add(item=max, pos=(row, 1))
            
            row += 1
            log = wx.CheckBox(parent=self, id=wx.ID_ANY, label=_("Log scale"))
            log.SetValue(prop['log'])
            self.wxId[atype]['log'] = log.GetId()
            gridSizer.Add(item=log, pos=(row, 0), span=(1, 2))

            if idx == 0:
                flag = wx.ALL | wx.EXPAND
            else:
                flag = wx.TOP | wx.BOTTOM | wx.RIGHT | wx.EXPAND

            boxSizer.Add(item=gridSizer, flag=wx.ALL, border=3)
            boxMainSizer.Add(item=boxSizer, flag=flag, border=3)

            idx += 1
            
        middleSizer.Add(item=boxMainSizer, flag=wx.ALL | wx.EXPAND, border=3)

        #
        # grid & legend options
        #
        self.wxId['grid'] = {}
        self.wxId['legend'] = {}
        self.wxId['font'] = {}
        box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                           label=" %s " % _("Grid and Legend settings"))
        boxMainSizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        row = 0
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Grid color"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        gridcolor = csel.ColourSelect(parent=self, id=wx.ID_ANY, colour=self.properties['grid']['color'])
        self.wxId['grid']['color'] = gridcolor.GetId()
        gridSizer.Add(item=gridcolor, pos=(row, 1))

        row +=1
        gridshow = wx.CheckBox(parent=self, id=wx.ID_ANY, label=_("Show grid"))
        gridshow.SetValue(self.properties['grid']['enabled'])
        self.wxId['grid']['enabled'] = gridshow.GetId()
        gridSizer.Add(item=gridshow, pos=(row, 0), span=(1, 2))

        row +=1
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Legend font size"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        legendfontsize = wx.SpinCtrl(parent=self, id=wx.ID_ANY, value="", 
                                     size=(50, -1), style=wx.SP_ARROW_KEYS)
        legendfontsize.SetRange(5,100)
        legendfontsize.SetValue(int(self.properties['font']['prop']['legendSize']))
        self.wxId['font']['legendSize'] = legendfontsize.GetId()
        gridSizer.Add(item=legendfontsize, pos=(row, 1))

        row += 1
        legendshow = wx.CheckBox(parent=self, id=wx.ID_ANY, label=_("Show legend"))
        legendshow.SetValue(self.properties['legend']['enabled'])
        self.wxId['legend']['enabled'] = legendshow.GetId()
        gridSizer.Add(item=legendshow, pos=(row, 0), span=(1, 2))

        boxMainSizer.Add(item=gridSizer, flag=flag, border=3)

        middleSizer.Add(item=boxMainSizer, flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border=3)

        sizer.Add(item=middleSizer, flag=wx.ALL, border=0)
        
        #
        # line & buttons
        #
        line = wx.StaticLine(parent=self, id=wx.ID_ANY, size=(20, -1), style=wx.LI_HORIZONTAL)
        sizer.Add(item=line, proportion=0,
                  flag=wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.LEFT|wx.RIGHT, border=3)

        #
        # buttons
        #
        btnSave = wx.Button(self, wx.ID_SAVE)
        btnApply = wx.Button(self, wx.ID_APPLY)
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnSave.SetDefault()

        # tooltips for buttons
        btnApply.SetToolTipString(_("Apply changes for the current session"))
        btnSave.SetToolTipString(_("Apply and save changes to user settings file (default for next sessions)"))
        btnSave.SetDefault()
        btnCancel.SetToolTipString(_("Close dialog and ignore changes"))

        # sizers
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(btnCancel)
        btnStdSizer.AddButton(btnSave)
        btnStdSizer.AddButton(btnApply)
        btnStdSizer.Realize()
        
        sizer.Add(item=btnStdSizer, proportion=0, flag=wx.ALIGN_RIGHT | wx.ALL, border=5)

        #
        # bindings for buttons and map plot settings controls
        #
        self.mapchoice.Bind(wx.EVT_CHOICE, self.OnSetMap)
        pcolor.Bind(csel.EVT_COLOURSELECT, self.OnSetOpt)
        pwidth.Bind(wx.EVT_SPINCTRL, self.OnSetOpt)
        pstyle.Bind(wx.EVT_CHOICE, self.OnSetOpt)
        plegend.Bind(wx.EVT_TEXT, self.OnSetOpt)
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def OnSetMap(self, event):
        """!Handler for changing map selection"""
        self.map = event.GetString()
        
        # update plot settings controls for selected map
        self.FindWindowById(self.wxId['pcolor']).SetColour(self.raster[self.map]['pcolor'])
        self.FindWindowById(self.wxId['pwidth']).SetValue(self.raster[self.map]['pwidth'])
        self.FindWindowById(self.wxId['pstyle']).SetStringSelection(self.raster[self.map]['pstyle'])
        self.FindWindowById(self.wxId['plegend']).SetValue(self.raster[self.map]['plegend'])
        self.Refresh()
        
    def OnSetOpt(self, event):
        """!Handler for changing any other option"""
        self.map = self.rasterList[self.mapchoice.GetCurrentSelection()]
        self.UpdateSettings()
        self.parent.SetGraphStyle()
        if self.parent.histogram:
            self.parent.DrawPlot()

    def UpdateSettings(self):
        """!Apply settings to each map and to entire plot"""
        
        # update plot settings for selected map
        self.raster[self.map]['pcolor'] = self.FindWindowById(self.wxId['pcolor']).GetColour()
        self.raster[self.map]['pwidth'] = int(self.FindWindowById(self.wxId['pwidth']).GetValue())
        self.raster[self.map]['pstyle'] = self.FindWindowById(self.wxId['pstyle']).GetStringSelection()
        self.raster[self.map]['plegend'] = self.FindWindowById(self.wxId['plegend']).GetValue()

        # update settings for entire plot
        for axis in ('x-axis', 'y-axis'):
            self.properties[axis]['prop']['type'] = self.FindWindowById(self.wxId[axis]['type']).GetStringSelection()
            self.properties[axis]['prop']['min'] = float(self.FindWindowById(self.wxId[axis]['min']).GetValue())
            self.properties[axis]['prop']['max'] = float(self.FindWindowById(self.wxId[axis]['max']).GetValue())
            self.properties[axis]['prop']['log'] = self.FindWindowById(self.wxId[axis]['log']).IsChecked()

        self.properties['grid']['color'] = self.FindWindowById(self.wxId['grid']['color']).GetColour()
        self.properties['grid']['enabled'] = self.FindWindowById(self.wxId['grid']['enabled']).IsChecked()

        self.properties['font']['prop']['legendSize'] = self.FindWindowById(self.wxId['font']['legendSize']).GetValue()
        self.properties['legend']['enabled'] = self.FindWindowById(self.wxId['legend']['enabled']).IsChecked()

    def OnSave(self, event):
        """!Button 'Save' pressed"""
        self.UpdateSettings()
        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        
        fileSettings['histogram'] = UserSettings.Get(group='histogram')
        file = UserSettings.SaveToFile(fileSettings)
        self.parent.parent.GetLayerManager().goutput.WriteLog(_('Histogram settings saved to file \'%s\'.') % file)
        self.parent.SetGraphStyle()
        if self.parent.histogram:
            self.parent.DrawPlot()
        self.Close()

    def OnApply(self, event):
        """!Button 'Apply' pressed. Does not close dialog"""
        self.UpdateSettings()
        self.parent.SetGraphStyle()
        if self.parent.histogram:
            self.parent.DrawPlot()
        
    def OnCancel(self, event):
        """!Button 'Cancel' pressed"""
        self.Close()
        
