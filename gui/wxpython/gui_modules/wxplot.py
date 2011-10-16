"""!
@package wxplot.py

Iinteractive plotting using PyPlot (wx.lib.plot.py). 

Classes:
 - AbstractPlotFrame
 - HistFrame
 - ProfileFrame

(C) 2011 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton, Arizona State University
"""

import os
import sys
import math

import wx
import wx.lib.colourselect as  csel

import globalvar
import gcmd
from render      import Map
from toolbars    import Histogram2Toolbar
from toolbars    import ProfileToolbar
from toolbars    import ScatterplotToolbar
from preferences import globalSettings as UserSettings

import wxplot_dialogs as dialogs

from grass.script import core   as grass
from grass.script import raster as raster

try:
    import numpy
    import wx.lib.plot as plot
except ImportError:
    msg = _("This module requires the NumPy module, which could not be "
            "imported. It probably is not installed (it's not part of the "
            "standard Python distribution). See the Numeric Python site "
            "(http://numpy.scipy.org) for information on downloading source or "
            "binaries.")
    print >> sys.stderr, "wxplot.py: " + msg

class AbstractPlotFrame(wx.Frame):
    """!Abstract PyPlot display frame class"""
    def __init__(self, parent = None, id = wx.ID_ANY, size = (700, 300),
                 style = wx.DEFAULT_FRAME_STYLE, rasterList = [],  **kwargs):

        wx.Frame.__init__(self, parent, id, size = size, style = style, **kwargs)
        
        self.parent = parent            # MapFrame
        self.mapwin = self.parent.MapWindow
        self.Map    = Map()             # instance of render.Map to be associated with display
        self.rasterList = rasterList    #list of rasters to plot
        self.raster = {}    # dictionary of raster maps and their plotting parameters
        self.plottype = ''
        
        self.linestyledict = { 'solid' : wx.SOLID,
                            'dot' : wx.DOT,
                            'long-dash' : wx.LONG_DASH,
                            'short-dash' : wx.SHORT_DASH,
                            'dot-dash' : wx.DOT_DASH }

        self.ptfilldict = { 'transparent' : wx.TRANSPARENT,
                            'solid' : wx.SOLID }

        #
        # Icon
        #
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
                
        #
        # Add statusbar
        #
        self.statusbar = self.CreateStatusBar(number = 2, style = 0)
        self.statusbar.SetStatusWidths([-2, -1])

        #
        # Define canvas and settings
        #
        # 
        self.client = plot.PlotCanvas(self)

        #define the function for drawing pointLabels
        self.client.SetPointLabelFunc(self.DrawPointLabel)

        # Create mouse event for showing cursor coords in status bar
        self.client.canvas.Bind(wx.EVT_LEFT_DOWN, self.OnMouseLeftDown)

        # Show closest point when enabled
        self.client.canvas.Bind(wx.EVT_MOTION, self.OnMotion)

        self.plotlist = []      # list of things to plot
        self.plot = None        # plot draw object
        self.ptitle = ""        # title of window
        self.xlabel = ""        # default X-axis label
        self.ylabel = ""        # default Y-axis label

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

    def InitPlotOpts(self, plottype):
        """!Initialize options for entire plot
        """
        
        self.plottype = plottype                # histogram, profile, or scatter

        self.properties = {}                    # plot properties
        self.properties['font'] = {}
        self.properties['font']['prop'] = UserSettings.Get(group = self.plottype, key = 'font')
        self.properties['font']['wxfont'] = wx.Font(11, wx.FONTFAMILY_SWISS,
                                                    wx.FONTSTYLE_NORMAL,
                                                    wx.FONTWEIGHT_NORMAL)
        
        if self.plottype == 'profile':
            self.properties['marker'] = UserSettings.Get(group = self.plottype, key = 'marker')
            # changing color string to tuple for markers/points
            colstr = str(self.properties['marker']['color'])
            self.properties['marker']['color'] = tuple(int(colval) for colval in colstr.strip('()').split(','))

        self.properties['grid'] = UserSettings.Get(group = self.plottype, key = 'grid')        
        colstr = str(self.properties['grid']['color']) # changing color string to tuple        
        self.properties['grid']['color'] = tuple(int(colval) for colval in colstr.strip('()').split(','))
                
        self.properties['x-axis'] = {}
        self.properties['x-axis']['prop'] = UserSettings.Get(group = self.plottype, key = 'x-axis')
        self.properties['x-axis']['axis'] = None

        self.properties['y-axis'] = {}
        self.properties['y-axis']['prop'] = UserSettings.Get(group = self.plottype, key = 'y-axis')
        self.properties['y-axis']['axis'] = None
        
        self.properties['legend'] = UserSettings.Get(group = self.plottype, key = 'legend')

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
        
    def InitRasterOpts(self, rasterList, plottype):
        """!Initialize or update raster dictionary for plotting
        """

        rdict = {} # initialize a dictionary

        for r in rasterList:
            idx = rasterList.index(r)
            
            try:
                ret = raster.raster_info(r)
            except:
                continue
                # if r.info cannot parse map, skip it
                
            self.raster[r] = UserSettings.Get(group = plottype, key = 'raster') # some default settings
            rdict[r] = {} # initialize sub-dictionaries for each raster in the list

            if ret['units'] == '(none)' or ret['units'] == '' or ret['units'] == None:
                rdict[r]['units'] = ''
            else:
                self.raster[r]['units'] = ret['units']

            rdict[r]['plegend'] = r.split('@')[0]
            rdict[r]['datalist'] = [] # list of cell value,frequency pairs for plotting histogram
            rdict[r]['pline'] = None
            rdict[r]['datatype'] = ret['datatype']
            rdict[r]['pwidth'] = 1
            rdict[r]['pstyle'] = 'solid'
            
            if idx <= len(self.colorList):
                rdict[r]['pcolor'] = self.colorDict[self.colorList[idx]]
            else:
                r = randint(0, 255)
                b = randint(0, 255)
                g = randint(0, 255)
                rdict[r]['pcolor'] = ((r,g,b,255))
 
            
        return rdict
            
    def InitRasterPairs(self, rasterList, plottype):
        """!Initialize or update raster dictionary with raster pairs for
            bivariate scatterplots
        """
        
        if len(rasterList) == 0: return

        rdict = {} # initialize a dictionary
        for rpair in rasterList:
            idx = rasterList.index(rpair)
            
            try:
                ret0 = raster.raster_info(rpair[0])
                ret1 = raster.raster_info(rpair[1])

            except:
                continue
                # if r.info cannot parse map, skip it

            self.raster[rpair] = UserSettings.Get(group = plottype, key = 'rasters') # some default settings
            rdict[rpair] = {} # initialize sub-dictionaries for each raster in the list
            rdict[rpair][0] = {}
            rdict[rpair][1] = {}

            if ret0['units'] == '(none)' or ret['units'] == '' or ret['units'] == None:
                rdict[rpair][0]['units'] = ''
            else:
                self.raster[rpair][0]['units'] = ret0['units']

            if ret1['units'] == '(none)' or ret['units'] == '' or ret['units'] == None:
                rdict[rpair][1]['units'] = ''
            else:
                self.raster[rpair][1]['units'] = ret1['units']

            rdict[rpair]['plegend'] = rpair[0].split('@')[0] + ' vs ' + rpair[1].split('@')[0]
            rdict[rpair]['datalist'] = [] # list of cell value,frequency pairs for plotting histogram
            rdict[rpair]['ptype'] = 'dot'
            rdict[rpair][0]['datatype'] = ret0['datatype']
            rdict[rpair][1]['datatype'] = ret1['datatype']
            rdict[rpair]['psize'] = 1
            rdict[rpair]['pfill'] = 'solid'
            
            if idx <= len(self.colorList):
                rdict[rpair]['pcolor'] = self.colorDict[self.colorList[idx]]
            else:
                r = randint(0, 255)
                b = randint(0, 255)
                g = randint(0, 255)
                rdict[rpair]['pcolor'] = ((r,g,b,255))
            
        return rdict

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
            self.client.SetYSpec(self.properties['y-axis']['prop'])

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

    def DrawPlot(self, plotlist):
        """!Draw line and point plot from list plot elements.
        """
        self.plot = plot.PlotGraphics(plotlist,
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

        self.client.Draw(self.plot, self.properties['x-axis']['axis'],
                         self.properties['y-axis']['axis'])
                
    def DrawPointLabel(self, dc, mDataDict):
        """!This is the fuction that defines how the pointLabels are
            plotted dc - DC that will be passed mDataDict - Dictionary
            of data that you want to use for the pointLabel

            As an example I have decided I want a box at the curve
            point with some text information about the curve plotted
            below.  Any wxDC method can be used.
        """
        dc.SetPen(wx.Pen(wx.BLACK))
        dc.SetBrush(wx.Brush( wx.BLACK, wx.SOLID ) )

        sx, sy = mDataDict["scaledXY"] #scaled x,y of closest point
        dc.DrawRectangle( sx-5,sy-5, 10, 10)  #10by10 square centered on point
        px,py = mDataDict["pointXY"]
        cNum = mDataDict["curveNum"]
        pntIn = mDataDict["pIndex"]
        legend = mDataDict["legend"]
        #make a string to display
        s = "Crv# %i, '%s', Pt. (%.2f,%.2f), PtInd %i" %(cNum, legend, px, py, pntIn)
        dc.DrawText(s, sx , sy+1)

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
        """!Redraw the plot window. Unzoom to original size
        """
        self.client.Reset()
        self.client.Redraw()
       
    def OnErase(self, event):
        """!Erase the plot window
        """
        self.client.Clear()
        self.mapwin.ClearLines(self.mapwin.pdc)
        self.mapwin.ClearLines(self.mapwin.pdcTmp)
        self.mapwin.polycoords = []
        self.mapwin.Refresh()

    def SaveToFile(self, event):
        """!Save plot to graphics file
        """
        self.client.SaveFile()

    def OnMouseLeftDown(self,event):
        self.SetStatusText(_("Left Mouse Down at Point: (%.4f, %.4f)") % \
                               self.client._getXY(event))
        event.Skip() # allows plotCanvas OnMouseLeftDown to be called

    def OnMotion(self, event):
        """!Indicate when mouse is outside the plot area
        """
        if self.client.OnLeave(event): print 'out of area'
        #show closest point (when enbled)
        if self.client.GetEnablePointLabel() == True:
            #make up dict with info for the pointLabel
            #I've decided to mark the closest point on the closest curve
            dlst =  self.client.GetClosetPoint( self.client._getXY(event), pointScaled =  True)
            if dlst != []:      #returns [] if none
                curveNum, legend, pIndex, pointXY, scaledXY, distance = dlst
                #make up dictionary to pass to my user function (see DrawPointLabel)
                mDataDict =  {"curveNum":curveNum, "legend":legend, "pIndex":pIndex,\
                    "pointXY":pointXY, "scaledXY":scaledXY}
                #pass dict to update the pointLabel
                self.client.UpdatePointLabel(mDataDict)
        event.Skip()           #go to next handler        
 
 
    def PlotOptionsMenu(self, event):
        """!Popup menu for plot and text options
        """
        point = wx.GetMousePosition()
        popt = wx.Menu()
        # Add items to the menu
        settext = wx.MenuItem(popt, wx.ID_ANY, _('Text settings'))
        popt.AppendItem(settext)
        self.Bind(wx.EVT_MENU, self.PlotText, settext)

        setgrid = wx.MenuItem(popt, wx.ID_ANY, _('Plot settings'))
        popt.AppendItem(setgrid)
        self.Bind(wx.EVT_MENU, self.PlotOptions, setgrid)

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

    def OnPlotText(self, dlg):
        """!Custom text settings for histogram plot.
        """
        self.ptitle = dlg.ptitle
        self.xlabel = dlg.xlabel
        self.ylabel = dlg.ylabel
        dlg.UpdateSettings()

        self.client.SetFont(self.properties['font']['wxfont'])
        self.client.SetFontSizeTitle(self.properties['font']['prop']['titleSize'])
        self.client.SetFontSizeAxis(self.properties['font']['prop']['axisSize'])

        if self.plot:
            self.plot.setTitle(dlg.ptitle)
            self.plot.setXLabel(dlg.xlabel)
            self.plot.setYLabel(dlg.ylabel)
        
        self.OnRedraw(event = None)
    
    def PlotText(self, event):
        """!Set custom text values for profile title and axis labels.
        """
        dlg = dialogs.TextDialog(parent = self, id = wx.ID_ANY, 
                                 plottype = self.plottype, 
                                 title = _('Histogram text settings'))

        if dlg.ShowModal() == wx.ID_OK:
            self.OnPlotText(dlg)

        dlg.Destroy()

    def PlotOptions(self, event):
        """!Set various profile options, including: line width, color,
        style; marker size, color, fill, and style; grid and legend
        options.  Calls OptDialog class.
        """
        dlg = dialogs.OptDialog(parent = self, id = wx.ID_ANY, 
                                plottype = self.plottype, 
                                title = _('Plot settings'))
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
        for title, handler in ((_("Page setup"), self.OnPageSetup),
                               (_("Print preview"), self.OnPrintPreview),
                               (_("Print display"), self.OnDoPrint)):
            item = wx.MenuItem(printmenu, wx.ID_ANY, title)
            printmenu.AppendItem(item)
            self.Bind(wx.EVT_MENU, handler, item)
        
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
        """!Close plot window and clean up
        """
        try:
            self.mapwin.ClearLines()
            self.mapwin.mouse['begin'] = self.mapwin.mouse['end'] = (0.0, 0.0)
            self.mapwin.mouse['use'] = 'pointer'
            self.mapwin.mouse['box'] = 'point'
            self.mapwin.polycoords = []
            self.mapwin.UpdateMap(render = False, renderVector = False)
        except:
            pass
        
        self.mapwin.SetCursor(self.Parent.cursors["default"])
        self.Destroy()
        
class HistFrame(AbstractPlotFrame):
    def __init__(self, parent, id, pos, style, size, rasterList = []):
        """!Mainframe for displaying histogram of raster map. Uses wx.lib.plot.
        """
        AbstractPlotFrame.__init__(self, parent)
        
        self.toolbar = Histogram2Toolbar(parent = self)
        self.SetToolBar(self.toolbar)
        self.SetLabel(_("GRASS Histogramming Tool"))

        #
        # Init variables
        #
        self.rasterList = rasterList
        self.plottype = 'histogram'
        self.group = '' 
        self.ptitle = _('Histogram of')         # title of window
        self.xlabel = _("Raster cell values")   # default X-axis label
        self.ylabel = _("Cell counts")          # default Y-axis label
        self.maptype = 'raster'                 # default type of histogram to plot
        self.histtype = 'count' 
        self.bins = 255
        self.colorList = ["blue", "green", "red", "yellow", "magenta", "cyan", \
                    "aqua", "black", "grey", "orange", "brown", "purple", "violet", \
                    "indigo"]
        
        if len(self.rasterList) > 0: # set raster name(s) from layer manager if a map is selected
            self.InitRasterOpts(self.rasterList, self.plottype)

        self._initOpts()

    def _initOpts(self):
        """!Initialize plot options
        """
        self.InitPlotOpts('histogram')            

    def OnCreateHist(self, event):
        """!Main routine for creating a histogram. Uses r.stats to
        create a list of cell value and count/percent/area pairs. This is passed to
        plot to create a line graph of the histogram.
        """
        self.SetCursor(self.parent.cursors["default"])
        self.SetGraphStyle()
        self.SetupHistogram()
        p = self.CreatePlotList()
        self.DrawPlot(p)

    def OnSelectRaster(self, event):
        """!Select raster map(s) to profile
        """
        dlg = dialogs.HistRasterDialog(parent = self)

        if dlg.ShowModal() == wx.ID_OK:
            self.rasterList = dlg.rasterList
            self.group = dlg.group
            self.bins = dlg.bins
            self.histtype = dlg.histtype
            self.maptype = dlg.maptype
            self.raster = self.InitRasterOpts(self.rasterList, self.plottype)

            # plot histogram
            if len(self.rasterList) > 0:
                self.OnCreateHist(event = None)

        dlg.Destroy()

    def SetupHistogram(self):
        """!Build data list for ploting each raster
        """

        #
        # populate raster dictionary
        #
        if len(self.rasterList) == 0: return  # nothing selected
        
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
                                  input = raster,
                                  flags = freqflag,
                                  nsteps = self.bins,
                                  fs = ',',
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
        
    def CreatePlotList(self):
        """!Make list of elements to plot
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
                                                        colour = col,
                                                        width = self.raster[r]['pwidth'],
                                                        style = self.linestyledict[self.raster[r]['pstyle']],
                                                        legend = self.raster[r]['plegend'])

                self.plotlist.append(self.raster[r]['pline'])
          
        if len(self.plotlist) > 0:        
            return self.plotlist
        else:
            return None

    def Update(self):
        """!Update histogram after changing options
        """
        self.SetGraphStyle()
        p = self.CreatePlotList()
        self.DrawPlot(p)
 
    def OnStats(self, event):
        """!Displays regression information in messagebox
        """
        message = []
        title = _('Statistics for Map(s) Histogrammed')

        for r in self.rasterList:
            rast = r.split('@')[0] 
            ret = grass.read_command('r.univar', map = r, flags = 'e', quiet = True)
            stats = _('Statistics for %s\n\n%s\n') % (rast, ret)
            message.append(stats)
            
        stats = dialogs.PlotStatsFrame(self, id = wx.ID_ANY, message = message, 
                                      title = title)

        if stats.Show() == wx.ID_CLOSE:
            stats.Destroy()       
        
class ProfileFrame(AbstractPlotFrame):
    """!Mainframe for displaying profile of one or more raster maps. Uses wx.lib.plot.
    """
    def __init__(self, parent, id, pos, style, size, rasterList = []):

        AbstractPlotFrame.__init__(self, parent)

        self.toolbar = ProfileToolbar(parent = self)
        self.SetToolBar(self.toolbar)
        self.SetLabel(_("GRASS Profile Analysis Tool"))

        #
        # Init variables
        #
        self.rasterList = rasterList
        self.plottype = 'profile'
        self.coordstr = ''              # string of coordinates for r.profile
        self.seglist = []               # segment endpoint list
        self.ppoints = ''               # segment endpoints data         
        self.transect_length = 0.0      # total transect length        
        self.ptitle = _('Profile of')   # title of window
        self.raster = {}

        self.colorList =  ["blue", "red", "green", "yellow", "magenta", "cyan", \
                    "aqua", "black", "grey", "orange", "brown", "purple", "violet", \
                    "indigo"]

        if len(self.rasterList) > 0: # set raster name(s) from layer manager if a map is selected
            self.InitRasterOpts(self.rasterList, self.plottype)
            
        
        self._initOpts()

        # determine units (axis labels)
        if self.parent.Map.projinfo['units'] != '':
            self.xlabel = _('Distance (%s)') % self.parent.Map.projinfo['units']
        else:
            self.xlabel = _("Distance along transect")
        self.ylabel = _("Cell values")
        
    def _initOpts(self):
        """!Initialize plot options
        """
        self.InitPlotOpts('profile')

    def OnDrawTransect(self, event):
        """!Draws transect to profile in map display
        """
        self.mapwin.polycoords = []
        self.seglist = []
        self.mapwin.ClearLines(self.mapwin.pdc)
        self.ppoints = ''

        self.parent.SetFocus()
        self.parent.Raise()
        
        self.mapwin.mouse['use'] = 'profile'
        self.mapwin.mouse['box'] = 'line'
        self.mapwin.pen = wx.Pen(colour = 'Red', width = 2, style = wx.SHORT_DASH)
        self.mapwin.polypen = wx.Pen(colour = 'dark green', width = 2, style = wx.SHORT_DASH)
        self.mapwin.SetCursor(self.Parent.cursors["cross"])

    def OnSelectRaster(self, event):
        """!Select raster map(s) to profile
        """
        dlg = dialogs.ProfileRasterDialog(parent = self)

        if dlg.ShowModal() == wx.ID_OK:
            self.rasterList = dlg.rasterList
            self.raster = self.InitRasterOpts(self.rasterList, self.plottype)
            
            # plot profile
            if len(self.mapwin.polycoords) > 0 and len(self.rasterList) > 0:
                self.OnCreateProfile(event = None)

        dlg.Destroy()

    def SetupProfile(self):
        """!Create coordinate string for profiling. Create segment list for
           transect segment markers.
        """

        #
        # create list of coordinate points for r.profile
        #                
        dist = 0
        cumdist = 0
        self.coordstr = ''
        lasteast = lastnorth = None
        
        if len(self.mapwin.polycoords) > 0:
            for point in self.mapwin.polycoords:
                # build string of coordinate points for r.profile
                if self.coordstr == '':
                    self.coordstr = '%d,%d' % (point[0], point[1])
                else:
                    self.coordstr = '%s,%d,%d' % (self.coordstr, point[0], point[1])

        if len(self.rasterList) == 0:
            return

        # title of window
        self.ptitle = _('Profile of')

        #
        # create list of coordinates for transect segment markers
        #
        if len(self.mapwin.polycoords) > 0:
            self.seglist = []
            for point in self.mapwin.polycoords:
                # get value of raster cell at coordinate point
                ret = gcmd.RunCommand('r.what',
                                      parent = self,
                                      read = True,
                                      input = self.rasterList[0],
                                      east_north = '%d,%d' % (point[0],point[1]))
                
                val = ret.splitlines()[0].split('|')[3]
                if val == None or val == '*': continue
                val = float(val)
                
                # calculate distance between coordinate points
                if lasteast and lastnorth:
                    dist = math.sqrt(math.pow((lasteast-point[0]),2) + math.pow((lastnorth-point[1]),2))
                cumdist += dist
                
                #store total transect length
                self.transect_length = cumdist

                # build a list of distance,value pairs for each segment of transect
                self.seglist.append((cumdist,val))
                lasteast = point[0]
                lastnorth = point[1]

            # delete first and last segment point
            try:
                self.seglist.pop(0)
                self.seglist.pop()
            except:
                pass

        #
        # create datalist of dist/value pairs and y labels for each raster map
        #    
        self.ylabel = ''
        i = 0

        for r in self.raster.iterkeys():
            self.raster[r]['datalist'] = []
            datalist = self.CreateDatalist(r, self.coordstr)
            if len(datalist) > 0:   
                self.raster[r]['datalist'] = datalist

                # update ylabel to match units if they exist           
                if self.raster[r]['units'] != '':
                    self.ylabel += '%s (%d),' % (r['units'], i)
                i += 1

                # update title
                self.ptitle += ' %s ,' % r.split('@')[0]

        self.ptitle = self.ptitle.rstrip(',')
            
        if self.ylabel == '':
            self.ylabel = _('Raster values')
        else:
            self.ylabel = self.ylabel.rstrip(',')

    def CreateDatalist(self, raster, coords):
        """!Build a list of distance, value pairs for points along transect using r.profile
        """
        datalist = []
        
        # keep total number of transect points to 500 or less to avoid 
        # freezing with large, high resolution maps
        region = grass.region()
        curr_res = min(float(region['nsres']),float(region['ewres']))
        transect_rec = 0
        if self.transect_length / curr_res > 500:
            transect_res = self.transect_length / 500
        else: transect_res = curr_res
        
        ret = gcmd.RunCommand("r.profile",
                              parent = self,
                              input = raster,
                              profile = coords,
                              res = transect_res,
                              null = "nan",
                              quiet = True,
                              read = True)
        
        if not ret:
            return []
            
        for line in ret.splitlines():
            dist, elev = line.strip().split(' ')
            if dist == None or dist == '' or dist == 'nan' or \
                elev == None or elev == '' or elev == 'nan':
                continue
            dist = float(dist)
            elev = float(elev)
            datalist.append((dist,elev))

        return datalist

    def OnCreateProfile(self, event):
        """!Main routine for creating a profile. Uses r.profile to
        create a list of distance,cell value pairs. This is passed to
        plot to create a line graph of the profile. If the profile
        transect is in multiple segments, these are drawn as
        points. Profile transect is drawn, using methods in mapdisp.py
        """
            
        if len(self.mapwin.polycoords) == 0 or len(self.rasterList) == 0:
            dlg = wx.MessageDialog(parent = self,
                                   message = _('You must draw a transect to profile in the map display window.'),
                                   caption = _('Nothing to profile'),
                                   style = wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
            dlg.ShowModal()
            dlg.Destroy()
            return

        self.mapwin.SetCursor(self.parent.cursors["default"])
        self.SetCursor(self.parent.cursors["default"])
        self.SetGraphStyle()
        self.SetupProfile()
        p = self.CreatePlotList()
        self.DrawPlot(p)

        # reset transect
        self.mapwin.mouse['begin'] = self.mapwin.mouse['end'] = (0.0,0.0)
        self.mapwin.mouse['use'] = 'pointer'
        self.mapwin.mouse['box'] = 'point'

    def CreatePlotList(self):
        """!Create a plot data list from transect datalist and
            transect segment endpoint coordinates.
        """
        # graph the distance, value pairs for the transect
        self.plotlist = []

        # Add segment marker points to plot data list
        if len(self.seglist) > 0 :
            self.ppoints = plot.PolyMarker(self.seglist,
                                           legend = ' ' + self.properties['marker']['legend'],
                                           colour = wx.Color(self.properties['marker']['color'][0],
                                                           self.properties['marker']['color'][1],
                                                           self.properties['marker']['color'][2],
                                                           255),
                                           size = self.properties['marker']['size'],
                                           fillstyle = self.ptfilldict[self.properties['marker']['fill']],
                                           marker = self.properties['marker']['type'])
            self.plotlist.append(self.ppoints)

        # Add profile distance/elevation pairs to plot data list for each raster profiled
        for r in self.rasterList:
            col = wx.Color(self.raster[r]['pcolor'][0],
                           self.raster[r]['pcolor'][1],
                           self.raster[r]['pcolor'][2],
                           255)
            self.raster[r]['pline'] = plot.PolyLine(self.raster[r]['datalist'],
                                                    colour = col,
                                                    width = self.raster[r]['pwidth'],
                                                    style = self.linestyledict[self.raster[r]['pstyle']],
                                                    legend = self.raster[r]['plegend'])

            self.plotlist.append(self.raster[r]['pline'])

        if len(self.plotlist) > 0:        
            return self.plotlist
        else:
            return None

    def Update(self):
        """!Update profile after changing options
        """
        self.SetGraphStyle()
        p = self.CreatePlotList()
        self.DrawPlot(p)

    def SaveProfileToFile(self, event):
        """!Save r.profile data to a csv file
        """    
        wildcard = _("Comma separated value (*.csv)|*.csv")
        
        dlg = wx.FileDialog(parent = self,
                            message = _("Path and prefix (for raster name) to save profile values..."),
                            defaultDir = os.getcwd(), 
                            defaultFile = "",  wildcard = wildcard, style = wx.SAVE)
        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            
            for r in self.rasterList:
                pfile = path+'_'+str(r['name'])+'.csv'
                try:
                    file = open(pfile, "w")
                except IOError:
                    wx.MessageBox(parent = self,
                                  message = _("Unable to open file <%s> for writing.") % pfile,
                                  caption = _("Error"), style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
                    return False
                for datapair in self.raster[r]['datalist']:
                    file.write('%d,%d\n' % (float(datapair[0]),float(datapair[1])))
                                        
                file.close()

        dlg.Destroy()
        
    def OnStats(self, event):
        """!Displays regression information in messagebox
        """
        message = []
        title = _('Statistics for Profile(s)')

        for r in self.raster.iterkeys():
            try:
                rast = r.split('@')[0] 
                statstr = 'Profile of %s\n\n' % rast

                iterable = (i[1] for i in self.raster[r]['datalist'])            
                a = numpy.fromiter(iterable, numpy.float)
                
                statstr += 'n: %f\n' % a.size
                statstr += 'minimum: %f\n' % numpy.amin(a)
                statstr += 'maximum: %f\n' % numpy.amax(a)
                statstr += 'range: %f\n' % numpy.ptp(a)
                statstr += 'mean: %f\n' % numpy.mean(a)
                statstr += 'standard deviation: %f\n' % numpy.std(a)
                statstr += 'variance: %f\n' % numpy.var(a)
                cv = numpy.std(a)/numpy.mean(a)
                statstr += 'coefficient of variation: %f\n' % cv
                statstr += 'sum: %f\n' % numpy.sum(a)
                statstr += 'median: %f\n' % numpy.median(a)
                statstr += 'distance along transect: %f\n\n' % self.transect_length       
                message.append(statstr)
            except:
                pass
            
        stats = dialogs.PlotStatsFrame(self, id = wx.ID_ANY, message = message, 
                                      title = title)

        if stats.Show() == wx.ID_CLOSE:
            stats.Destroy()       
        
class ScatterFrame(AbstractPlotFrame):
    """!Mainframe for displaying bivariate scatter plot of two raster maps. Uses wx.lib.plot.
    """
    def __init__(self, parent, id, pos, style, size, rasterList = []):

        AbstractPlotFrame.__init__(self, parent)
        
        self.toolbar = ScatterplotToolbar(parent = self)
        self.SetToolBar(self.toolbar)
        self.SetLabel(_("GRASS Bivariate Scatterplot Tool"))

        #
        # Init variables
        #
        self.rasterList = rasterList
        self.plottype = 'scatter'
        self.ptitle = _('Bivariate Scatterplot')     # title of window
        self.xlabel = _("Raster cell values")           # default X-axis label
        self.ylabel = _("Raster cell values")           # default Y-axis label
        self.maptype = 'raster'                         # default type of scatterplot
        self.scattertype = 'normal' 
        self.bins = 255
        self.colorList = ["blue", "red", "black", "green", "yellow", "magenta", "cyan", \
                    "aqua", "grey", "orange", "brown", "purple", "violet", \
                    "indigo"]
        
        if len(self.rasterList) > 1: # set raster name(s) from layer manager if a map is selected
            self.InitRasterOpts(self.rasterList, 'scatter')

        self._initOpts()

    def _initOpts(self):
        """!Initialize plot options
        """
        self.InitPlotOpts('scatter')            

    def OnCreateScatter(self, event):
        """!Main routine for creating a scatterplot. Uses r.stats to
        create a list of cell value pairs. This is passed to
        plot to create a scatterplot.
        """
        self.SetCursor(self.parent.cursors["default"])
        self.SetGraphStyle()
        self.SetupScatterplot()
        p = self.CreatePlotList()
        self.DrawPlot(p)

    def OnSelectRaster(self, event):
        """!Select raster map(s) to profile
        """
        dlg = dialogs.ScatterRasterDialog(parent = self)

        if dlg.ShowModal() == wx.ID_OK:
            rlist = dlg.rasterList
            if rlist < 2: 
                dlg.Destroy()
                return                        # need at least 2 rasters for scatterplot

            self.bins = dlg.bins                        # bins for r.stats with float and dcell maps
            self.scattertype = dlg.scattertype          # scatterplot or bubbleplot
            self.rasterList = self.CreatePairs(rlist)   # list of raster pairs (tuples)
            self.raster = self.InitRasterPairs(self.rasterList, 'scatter') # dictionary of raster pairs

            # plot histogram
            if len(self.rasterList) > 0:
                self.OnCreateScatter(event = None)

        dlg.Destroy()
        
    def CreatePairs(self, rlist):
        """!Transforms list of rasters into tuples of raster pairs
        """
        rasterList = []
        next = 'first'
        for r in rlist:
            if next == 'first':
                first = r
                next = 'second'
            else:
                second = r
                t = (first, second)
                rasterList.append(t)
                next = 'first'
                first = second = ''
                
        return rasterList

    def SetupScatterplot(self):
        """!Build data list for ploting each raster
        """

        #
        # initialize title string
        #
        self.ptitle = _('Bivariate Scatterplot of ')        

        #
        # create a datalist for plotting for each raster pair
        #
        if len(self.rasterList) == 0: return  # at least 1 pair of maps needed to plot        
        
        for rpair in self.rasterList:
            self.raster[rpair]['datalist'] = self.CreateDatalist(rpair)
            
            # update title
            self.ptitle += '%s vs %s, ' % (rpair[0].split('@')[0], rpair[1].split('@')[0])

        self.ptitle = self.ptitle.strip(', ')
        
        #
        # set xlabel & ylabel based on raster maps of first pair to be plotted
        #
        units = self.raster[self.rasterList[0]][0]['units']
        if units != '' and units != '(none)' and units != None:
            self.xlabel = _('Raster cell values %s') % units
        else:
            self.xlabel = _('Raster cell values') 

        units = self.raster[self.rasterList[0]][1]['units']
        if units != '' and units != '(none)' and units != None:
            self.ylabel = _('Raster cell values %s') % units
        else:
            self.ylabel = _('Raster cell values') 

    def CreateDatalist(self, rpair):
        """!Build a list of cell value, frequency pairs for histogram
            frequency can be in cell counts, percents, or area
        """
        datalist = []
        
        if self.scattertype == 'bubble': 
            freqflag = 'cn'
        else:
            freqflag = 'n'
                
        try:
            ret = gcmd.RunCommand("r.stats",
                                  parent = self,
                                  input = '%s,%s' % rpair,
                                  flags = freqflag,
                                  nsteps = self.bins,
                                  fs = ',',
                                  quiet = True,
                                  read = True)
            
            if not ret:
                return datalist
            
            for line in ret.splitlines():
                rast1, rast2 = line.strip().split(',')
                rast1 = rast1.strip()
                if '-' in rast1: rast1 = rast1.split('-')[0]
                rast2 = rast2.strip()
                if '-' in rast2: rast2 = rast2.split('-')[0]
                
                rast1 = rast1.encode('ascii', 'ignore')
                rast2 = rast2.encode('ascii', 'ignore')
                    
                datalist.append((rast1,rast2))

            return datalist
        except gcmd.GException, e:
            gcmd.GError(parent = self,
                        message = e.value)
            return None
        
    def CreatePlotList(self):
        """!Make list of elements to plot
        """
        # graph the cell value, frequency pairs for the histogram
        self.plotlist = []

        for rpair in self.rasterList:
            if 'datalist' not in self.raster[rpair] or \
                self.raster[rpair]['datalist'] == None: return
            
            if len(self.raster[rpair]['datalist']) > 0:
                col = wx.Color(self.raster[rpair]['pcolor'][0],
                               self.raster[rpair]['pcolor'][1],
                               self.raster[rpair]['pcolor'][2],
                               255)
                scatterpoints = plot.PolyMarker(self.raster[rpair]['datalist'],
                                                legend = ' ' + self.raster[rpair]['plegend'],
                                                colour = col,size = self.raster[rpair]['psize'],
                                                fillstyle = self.ptfilldict[self.raster[rpair]['pfill']],
                                                marker = self.raster[rpair]['ptype'])

                self.plotlist.append(scatterpoints)
          
        if len(self.plotlist) > 0:        
            return self.plotlist
        else:
            return None

    def Update(self):
        """!Update histogram after changing options
        """
        self.SetGraphStyle()
        p = self.CreatePlotList()
        self.DrawPlot(p)
    
    def OnRegression(self, event):
        """!Displays regression information in messagebox
        """
        message = []
        title = _('Regression Statistics for Scatterplot(s)')

        for rpair in self.rasterList:
            if isinstance(rpair, tuple) == False: continue
            rast1, rast2 = rpair
            rast1 = rast1.split('@')[0] 
            rast2 = rast2.split('@')[0] 
            ret = grass.parse_command('r.regression.line', 
                                      map1 = rast1, 
                                      map2 = rast2, 
                                      flags = 'g', quiet = True,
                                      parse = (grass.parse_key_val, { 'sep' : '=' }))
            eqtitle = _('Regression equation for %s vs. %s:\n\n')  % (rast1, rast2)
            eq = _('   %s = %s + %s(%s)\n\n') % (rast2, ret['a'], ret['b'], rast1)
            num = _('N = %s\n') % ret['N']
            rval = _('R = %s\n') % ret['R']
            rsq = _('R-squared = %f\n') % pow(float(ret['R']), 2)
            ftest = _('F = %s\n') % ret['F']
            str = eqtitle + eq + num + rval + rsq + ftest
            message.append(str)
            
        stats = dialogs.PlotStatsFrame(self, id = wx.ID_ANY, message = message, 
                                      title = title)

        if stats.Show() == wx.ID_CLOSE:
            stats.Destroy()       