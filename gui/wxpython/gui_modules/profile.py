"""
@package profile

Profile analysis of GRASS raster maps and images.

Uses PyPlot (wx.lib.plot.py)

Classes:
 - ProfileFrame
 - SetRasterDialog
 - TextDialog
 - OptDialog

COPYRIGHT: (C) 2007-2008 by the GRASS Development Team
           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.

@author Michael Barton
Various updates: Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import math

import wx
import wx.lib.colourselect as  csel

try:
    import wx.lib.plot as plot
except:
    msg= """
    This module requires the NumPy module,
    which could not be imported.  It probably is not installed
    (it's not part of the standard Python distribution). See the
    Numeric Python site (http://numpy.scipy.org) for information on
    downloading source or binaries."""
    print >> sys.stderr, "profile.py: " + msg

import globalvar
try:
    import subprocess
except:
    CompatPath = os.path.join(globalvar.ETCWXDIR)
    sys.path.append(CompatPath)
    from compat import subprocess as subprocess

import render
import menuform
import disp_print
import gselect
import gcmd
import toolbars
from debug import Debug as Debug
from icon import Icons as Icons
from preferences import globalSettings as UserSettings
import grass

class ProfileFrame(wx.Frame):
    """
    Mainframe for displaying profile of raster map. Uses wx.lib.plot.
    """

    def __init__(self, parent=None, id=wx.ID_ANY, title=_("Profile Analysis"),
                 rasterList=[],
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_FRAME_STYLE):

        self.parent = parent # MapFrame
        self.mapwin = self.parent.MapWindow
        self.Map = render.Map()  # instance of render.Map to be associated with display

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
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_map.ico'), wx.BITMAP_TYPE_ICO))
        
        #
        # Add toolbar
        #
        self.toolbar = toolbars.ProfileToolbar(parent=self, tbframe=self).GetToolbar()
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
        # plot canvas settings
        self.client = plot.PlotCanvas(self)
        #define the function for drawing pointLabels
        self.client.SetPointLabelFunc(self.DrawPointLabel)
        # Create mouse event for showing cursor coords in status bar
        self.client.canvas.Bind(wx.EVT_LEFT_DOWN, self.OnMouseLeftDown)
        # Show closest point when enabled
        self.client.canvas.Bind(wx.EVT_MOTION, self.OnMotion)

        #
        # Init variables
        #
        # 0    -> default raster map to profile
        # 1, 2 -> optional raster map to profile
        # units -> map data units (used for y axis legend)
        self.raster = {}
        for idx in (0, 1, 2):
            self.raster[idx] = {}
            self.raster[idx]['name'] = ''
            self.raster[idx]['units'] = ''
            self.raster[idx]['plegend'] = ''
            # list of distance,value pairs for plotting profile
            self.raster[idx]['datalist'] = []
            # first (default) profile line
            self.raster[idx]['pline'] = None
            self.raster[idx]['prop'] = UserSettings.Get(group='profile', key='raster' + str(idx))

        # set raster map name (if given)
        for idx in range(len(rasterList)):
            self.raster[idx]['name'] = rasterList[idx]

        # string of coordinates for r.profile
        self.coordstr = '' 
        # segment endpoint list
        self.seglist = []
        # list of things to plot
        self.plotlist = []
        # segment endpoints data 
        self.ppoints = '' 
        # plot draw object
        self.profile = None 

        # title of window
        self.ptitle = _('Profile of')

        # determine units (axis labels)
        if self.parent.Map.projinfo['units'] != '':
            self.xlabel = _('Distance (%s)') % self.parent.Map.projinfo['units']
        else:
            self.xlabel = _("Distance along transect")
        self.ylabel = _("Cell values")

        self.properties = {}
        self.properties['font'] = {}
        self.properties['font']['prop'] = UserSettings.Get(group='profile', key='font')
        self.properties['font']['wxfont'] = wx.Font(11, wx.FONTFAMILY_SWISS,
                                                    wx.FONTSTYLE_NORMAL,
                                                    wx.FONTWEIGHT_NORMAL)
        
        self.properties['marker'] = UserSettings.Get(group='profile', key='marker')

        self.properties['grid'] = UserSettings.Get(group='profile', key='grid')
        self.properties['x-axis'] = {}
        
        self.properties['x-axis']['prop'] = UserSettings.Get(group='profile', key='x-axis')
        self.properties['x-axis']['axis'] = None

        self.properties['y-axis'] = {}
        self.properties['y-axis']['prop'] = UserSettings.Get(group='profile', key='y-axis')
        self.properties['y-axis']['axis'] = None
        
        self.properties['legend'] = UserSettings.Get(group='profile', key='legend')
        
        # zooming disabled
        self.zoom = False
        # draging disabled
        self.drag = False 

        # vertical and horizontal scrollbars
        self.client.SetShowScrollbars(True)

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

    def OnDrawTransect(self, event):
        """
        Draws transect to profile in map display
        """
        self.mapwin.polycoords = []
        self.seglist = []
        self.mapwin.ClearLines(self.mapwin.pdc)
        self.ppoints = ''

        self.parent.SetFocus()
        self.parent.Raise()
        
        self.mapwin.mouse['use'] = 'profile'
        self.mapwin.mouse['box'] = 'line'
        self.mapwin.pen = wx.Pen(colour='Red', width=2, style=wx.SHORT_DASH)
        self.mapwin.polypen = wx.Pen(colour='dark green', width=2, style=wx.SHORT_DASH)
        self.mapwin.SetCursor(self.Parent.cursors["cross"])

    def OnSelectRaster(self, event):
        """
        Select raster map(s) to profile
        """
        
        dlg = SetRasterDialog(parent=self)

        if dlg.ShowModal() == wx.ID_OK:
            for r in self.raster.keys():
                self.raster[r]['name'] = dlg.raster[r]['name']

            # plot profile
            if self.raster[0]['name'] and len(self.mapwin.polycoords) > 0:
                self.OnCreateProfile(event=None)

        dlg.Destroy()

    def SetRaster(self):
        """
        Create coordinate string for profiling. Create segment list for
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

        if self.raster[0]['name'] == '':
            return

        # title of window
        self.ptitle = _('Profile of')

        #
        # create datalist for each raster map
        #
        
        for r in self.raster.itervalues():
            if r['name'] == '':
                continue
            r['datalist'] = self.CreateDatalist(r['name'], self.coordstr)
            r['plegend'] = _('Profile of %s') % r['name']

            ret = gcmd.RunCommand('r.info',
                                  parent = self,
                                  read = True,
                                  quiet = True,
                                  flags = 'u',
                                  map = r['name'])

            if ret:
                r['units'] = p.split('\n')[0].split('=')[1]

            # update title
            self.ptitle += ' %s and' % r['name']

        self.ptitle = self.ptitle.rstrip('and')
        
        #
        # set ylabel to match units if they exist
        #
        self.ylabel = ''
        i = 0
        
        for r in self.raster.itervalues():
            if r['name'] == '':
                continue
            if r['units'] != '':
                self.ylabel = '%s (%d),' % (r['units'], i)
            i += 1
        if self.ylabel == '':
            self.ylabel = _('Raster values')
        else:
            self.ylabel = self.ylabel.rstrip(',')

        #
        # create list of coordinates for transect segment markers
        #

        if len(self.mapwin.polycoords) > 0:
            for point in self.mapwin.polycoords:
                # get value of raster cell at coordinate point
                ret = gcmd.RunCommand('r.what',
                                      parent = self,
                                      read = True,
                                      input = self.raster[0]['name'],
                                      east_north = '%d,%d' % (point[0],point[1]))
                outlist = ret.split('\n')
                val = outlist[0].split('|')[3]
                
                # calculate distance between coordinate points
                if lasteast and lastnorth:
                    dist = math.sqrt(math.pow((lasteast-point[0]),2) + math.pow((lastnorth-point[1]),2))
                cumdist += dist

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

    def SetGraphStyle(self):
        """
        Set plot and text options
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

        # self.client.SetPointLabelFunc(self.DrawPointLabel())

    def CreateDatalist(self, raster, coords):
        """
        Build a list of distance, value pairs for points along transect
        """
        datalist = []
        import subprocess
        
        try:
            p = grass.read_command("r.profile",
                       input=raster,
                       profile=coords,
                       null="nan",
                       quiet=True
                       )
            for outline in p.strip().split('\n'):
                dist, elev = outline.split(' ')
                if elev != 'nan': datalist.append((dist,elev))

            return datalist
        except gcmd.CmdError, e:
            print e
            return None

    def OnCreateProfile(self, event):
        """
        Main routine for creating a profile. Uses r.profile to create a list
        of distance,cell value pairs. This is passed to plot to create a
        line graph of the profile. If the profile transect is in multiple
        segments, these are drawn as points. Profile transect is drawn, using
        methods in mapdisp.py
        """
    
        if len(self.mapwin.polycoords) == 0 or self.raster[0]['name'] == '':
            dlg = wx.MessageDialog(parent=self,
                                   message=_('You must draw a transect to profile in the map display window.'),
                                   caption=_('Nothing to profile'),
                                   style=wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
            dlg.ShowModal()
            dlg.Destroy()
            return

        self.mapwin.SetCursor(self.parent.cursors["default"])
        self.SetCursor(self.parent.cursors["default"])
        self.SetGraphStyle()

        self.SetRaster()
            
        self.DrawPlot()

        # reset transect
        self.mapwin.mouse['begin'] = self.mapwin.mouse['end'] = (0.0,0.0)
        self.mapwin.mouse['use'] = 'pointer'
        self.mapwin.mouse['box'] = 'point'

    def DrawPlot(self):
        """
        Draw line and point plot from transect datalist and
        transect segment endpoint coordinates.
        """

        # graph the distance, value pairs for the transect
        self.plotlist = []
        for r in self.raster.itervalues():
            if len(r['datalist']) > 0:
                col = wx.Color(r['prop']['pcolor'][0],
                               r['prop']['pcolor'][1],
                               r['prop']['pcolor'][2],
                               255)
                r['pline'] = plot.PolyLine(r['datalist'],
                                           colour=col,
                                           width=r['prop']['pwidth'],
                                           style=self.pstyledict[r['prop']['pstyle']],
                                           legend=r['plegend'])

                self.plotlist.append(r['pline'])

        if len(self.seglist) > 0 :
            self.ppoints = plot.PolyMarker(self.seglist,
                                           legend=' ' + self.properties['marker']['legend'],
                                           colour=wx.Color(self.properties['marker']['color'][0],
                                                           self.properties['marker']['color'][1],
                                                           self.properties['marker']['color'][2],
                                                           255),
                                           size=self.properties['marker']['size'],
                                           fillstyle=self.ptfilldict[self.properties['marker']['fill']],
                                           marker=self.properties['marker']['type'])
            self.plotlist.append(self.ppoints)

        self.profile = plot.PlotGraphics(self.plotlist,
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

        self.client.Draw(self.profile, self.properties['x-axis']['axis'],
                         self.properties['y-axis']['axis'])

    def OnZoom(self, event):
        """
        Enable zooming and disable dragging
        """

        self.zoom = True
        self.drag = False
        self.client.SetEnableZoom(self.zoom)
        self.client.SetEnableDrag(self.drag)

    def OnDrag(self, event):
        """
        Enable dragging and disable zooming
        """

        self.zoom = False
        self.drag = True
        self.client.SetEnableDrag(self.drag)
        self.client.SetEnableZoom(self.zoom)

    def OnRedraw(self, event):
        """
        Redraw the profile window. Unzoom to original size
        """
        self.client.Reset()
        self.client.Redraw()

    def Update(self):
        """
        Update profile after changing options
        """

        self.SetGraphStyle()
        self.DrawPlot()

    def OnErase(self, event):
        """
        Erase the profile window
        """
        self.client.Clear()
        self.mapwin.ClearLines(self.mapwin.pdc)
        self.mapwin.ClearLines(self.mapwin.pdcTmp)
        self.mapwin.polycoords = []
        self.mapwin.Refresh()
        #        try:
        #            self.mapwin.pdc.ClearId(self.mapwin.lineid)
        #            self.mapwin.pdc.ClearId(self.mapwin.plineid)
        #            self.mapwin.Refresh()
        #        except:
        #            pass

    def SaveToFile(self, event):
        """
        Save profile to graphics file
        """
        self.client.SaveFile()

    def DrawPointLabel(self, dc, mDataDict):
        """This is the fuction that defines how the pointLabels are plotted
            dc - DC that will be passed
            mDataDict - Dictionary of data that you want to use for the pointLabel

            As an example I have decided I want a box at the curve point
            with some text information about the curve plotted below.
            Any wxDC method can be used.
        """
        # ----------
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
        # -----------

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

    def ProfileOptionsMenu(self, event):
        """
        Popup menu for profile and text options
        """

        point = wx.GetMousePosition()
        popt = wx.Menu()
        # Add items to the menu
        settext = wx.MenuItem(popt, -1, 'Profile text settings')
        popt.AppendItem(settext)
        self.Bind(wx.EVT_MENU, self.PText, settext)

        setgrid = wx.MenuItem(popt, -1, 'Profile plot settings')
        popt.AppendItem(setgrid)
        self.Bind(wx.EVT_MENU, self.POptions, setgrid)

        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(popt)
        popt.Destroy()

    def NotFunctional(self):
        """
        Creates a 'not functional' message dialog
        """

        dlg = wx.MessageDialog(self, 'This feature is not yet functional',
                           'Under Construction', wx.OK | wx.ICON_INFORMATION)
        dlg.ShowModal()
        dlg.Destroy()

    def PText(self, event):
        """
        Set custom text values for profile
        title and axis labels.
        """
        dlg = TextDialog(parent=self, id=wx.ID_ANY, title=_('Profile text settings'))

        if dlg.ShowModal() == wx.ID_OK:
            self.ptitle = dlg.ptitle
            self.xlabel = dlg.xlabel
            self.ylabel = dlg.ylabel
            dlg.UpdateSettings()

            self.client.SetFont(self.properties['font']['wxfont'])
            self.client.SetFontSizeTitle(self.properties['font']['prop']['titleSize'])
            self.client.SetFontSizeAxis(self.properties['font']['prop']['axisSize'])

            if self.profile:
                self.profile.setTitle(dlg.ptitle)
                self.profile.setXLabel(dlg.xlabel)
                self.profile.setYLabel(dlg.ylabel)

        dlg.Destroy()

        self.OnRedraw(event=None)

    def POptions(self, event):
        """
        Set various profile options, including: line width, color, style;
        marker size, color, fill, and style; grid and legend options.
        Calls OptDialog class.
        """
        dlg = OptDialog(parent=self, id=wx.ID_ANY, title=_('Profile settings'))

        if dlg.ShowModal() == wx.ID_OK:
            dlg.UpdateSettings()

            self.SetGraphStyle()
            if self.profile:
                self.DrawPlot()

        dlg.Destroy()

    def PrintMenu(self, event):
        """
        Print options and output menu
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
        Close profile window and clean up
        """
        self.mapwin.ClearLines()
        self.mapwin.mouse['begin'] = self.mapwin.mouse['end'] = (0.0, 0.0)
        self.mapwin.mouse['use'] = 'pointer'
        self.mapwin.mouse['box'] = 'point'
        self.mapwin.polycoords = []
        self.mapwin.SetCursor(self.Parent.cursors["default"])

        self.mapwin.UpdateMap(render=False, renderVector=False)

        self.Destroy()

class SetRasterDialog(wx.Dialog):
    def __init__(self, parent, id=wx.ID_ANY, title=_("Select raster map to profile"),
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_DIALOG_STYLE):
        """
        Dialog to select raster maps to profile.
        """

        wx.Dialog.__init__(self, parent, id, title, pos, size, style)

        self.parent = parent
        self.coordstr = self.parent.coordstr

        #         if self.coordstr == '':
        #             dlg = wx.MessageDialog(parent=self,
        #                                    message=_('You must draw a transect to profile in the map display window.'),
        #                                    caption=_('Nothing to profile'),
        #                                    style=wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
        #             dlg.ShowModal()
        #             dlg.Destroy()
        #             self.Close(True)
        #             return

        self.raster = { 0 : { 'name' : self.parent.raster[0]['name'],
                              'id' : None },
                        1 : { 'name' : self.parent.raster[1]['name'],
                              'id' : None },
                        2 : { 'name' : self.parent.raster[2]['name'],
                              'id' : None }
                        }

        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.GridBagSizer (hgap=3, vgap=3)
        
        i = 0
        for txt in [_("Select raster map 1 (required):"),
                    _("Select raster map 2 (optional):"),
                    _("Select raster map 3 (optional):")]:
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=txt)
            box.Add(item=label,
                    flag=wx.ALIGN_CENTER_VERTICAL, pos=(i, 0))
            
            selection = gselect.Select(self, id=wx.ID_ANY,
                                       size=globalvar.DIALOG_GSELECT_SIZE,
                                       type='cell')
            selection.SetValue(str(self.raster[i]['name']))
            self.raster[i]['id'] = selection.GetChildren()[0].GetId()
            selection.Bind(wx.EVT_TEXT, self.OnSelection)
            
            box.Add(item=selection, pos=(i, 1))
            
            i += 1 
            
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

        self.SetSizer(sizer)
        sizer.Fit(self)

    def OnSelection(self, event):
        id = event.GetId()
        for r in self.raster.itervalues():
            if r['id'] == id:
                r['name'] = event.GetString()
                break

class TextDialog(wx.Dialog):
    def __init__(self, parent, id, title, pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.DEFAULT_DIALOG_STYLE):
        wx.Dialog.__init__(self, parent, id, title, pos, size, style)
        """
        Dialog to set profile text options: font, title
        and font size, axis labels and font size
        """
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
        
    def _do_layout(self):
        """Do layout"""
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
        self.ptitleentry = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(200,-1))
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
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=("X-axis label:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(2, 0))
        self.xlabelentry = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(200,-1))
        # self.xlabelentry.SetFont(self.font)
        self.xlabelentry.SetValue(self.xlabel)
        gridSizer.Add(item=self.xlabelentry, pos=(2, 1))

        #
        # y-axis label
        #
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Y-axis label:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(3, 0))
        self.ylabelentry = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(200,-1))
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
        self.ffamilycb = wx.ComboBox(parent=self, id=wx.ID_ANY, size=(200, -1),
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
        self.fstylecb = wx.ComboBox(parent=self, id=wx.ID_ANY, size=(200, -1),
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
        self.fwtcb = wx.ComboBox(parent=self, size=(200, -1),
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
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnSave.SetDefault()

        # bindigs
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnApply.SetToolTipString(_("Apply changes for the current session"))
        btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        btnSave.SetToolTipString(_("Apply and save changes to user settings file (default for next sessions)"))
        btnSave.SetDefault()
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        btnCancel.SetToolTipString(_("Close dialog and ignore changes"))

        # sizers
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(btnCancel)
        btnStdSizer.AddButton(btnSave)
        btnStdSizer.AddButton(btnApply)
        btnStdSizer.Realize()
        
        sizer.Add(item=btnStdSizer, proportion=0, flag=wx.ALIGN_RIGHT | wx.ALL, border=5)

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
        """Button 'Save' pressed"""
        self.UpdateSettings()
        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        fileSettings['profile'] = UserSettings.Get(group='profile')
        file = UserSettings.SaveToFile(fileSettings)
        self.parent.parent.gismanager.goutput.WriteLog(_('Profile settings saved to file \'%s\'.') % file)
        self.Close()

    def OnApply(self, event):
        """Button 'Apply' pressed"""
        self.UpdateSettings()
        self.Close()

    def OnCancel(self, event):
        """Button 'Cancel' pressed"""
        self.Close()
        
class OptDialog(wx.Dialog):
    def __init__(self, parent, id, title, pos=wx.DefaultPosition, size=wx.DefaultSize,
            style=wx.DEFAULT_DIALOG_STYLE):
        wx.Dialog.__init__(self, parent, id, title, pos, size, style)
        """
        Dialog to set various profile options, including: line width, color, style;
        marker size, color, fill, and style; grid and legend options.
        """
        # init variables
        self.pstyledict = parent.pstyledict
        self.ptfilldict = parent.ptfilldict

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
        self.properties = self.parent.properties
        
        self._do_layout()

    def _do_layout(self):
        """Do layout"""
        # dialog layout
        sizer = wx.BoxSizer(wx.VERTICAL)

        #
        # profile line settings
        #
        box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                           label=" %s " % _("Profile line settings"))
        boxMainSizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)

        idx = 1
        self.wxId['pcolor'] = []
        self.wxId['pwidth'] = []
        self.wxId['pstyle'] = []
        self.wxId['plegend'] = []
        for r in self.raster.itervalues():
            box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                               label=" %s %d " % (_("Profile"), idx))
            boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
            
            gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
            row = 0            
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Line color"))
            gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            pcolor = csel.ColourSelect(parent=self, id=wx.ID_ANY, colour=r['prop']['pcolor'])
            self.wxId['pcolor'].append(pcolor.GetId())
            gridSizer.Add(item=pcolor, pos=(row, 1))

            row += 1
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Line width"))
            gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            pwidth = wx.SpinCtrl(parent=self, id=wx.ID_ANY, value="",
                                 size=(50,-1), style=wx.SP_ARROW_KEYS)
            pwidth.SetRange(1, 10)
            pwidth.SetValue(r['prop']['pwidth'])
            self.wxId['pwidth'].append(pwidth.GetId())
            gridSizer.Add(item=pwidth, pos=(row, 1))

            row +=1
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Line style"))
            gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            pstyle = wx.ComboBox(parent=self, id=wx.ID_ANY, 
                                 size=(120, -1), choices=self.pstyledict.keys(), style=wx.CB_DROPDOWN)
            pstyle.SetStringSelection(r['prop']['pstyle'])
            self.wxId['pstyle'].append(pstyle.GetId())
            gridSizer.Add(item=pstyle, pos=(row, 1))

            row += 1
            label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Legend"))
            gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
            plegend = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(200,-1))
            plegend.SetValue(r['plegend'])
            gridSizer.Add(item=plegend, pos=(row, 1))
            self.wxId['plegend'].append(plegend.GetId())
            boxSizer.Add(item=gridSizer)

            if idx == 0:
                flag = wx.ALL
            else:
                flag = wx.TOP | wx.BOTTOM | wx.RIGHT
            boxMainSizer.Add(item=boxSizer, flag=flag, border=3)

            idx += 1
            
        sizer.Add(item=boxMainSizer, flag=wx.ALL | wx.EXPAND, border=3)

        middleSizer = wx.BoxSizer(wx.HORIZONTAL)
        
        #
        # segment marker settings
        #
        box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                           label=" %s " % _("Transect segment marker settings"))
        boxMainSizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)

        self.wxId['marker'] = {}
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Color"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 0))
        ptcolor = csel.ColourSelect(parent=self, id=wx.ID_ANY, colour=self.properties['marker']['color'])
        self.wxId['marker']['color'] = ptcolor.GetId()
        gridSizer.Add(item=ptcolor, pos=(0, 1))

        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Size"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(1, 0))
        ptsize = wx.SpinCtrl(parent=self, id=wx.ID_ANY, value="",
                             size=(50, -1), style=wx.SP_ARROW_KEYS)
        ptsize.SetRange(1, 10)
        ptsize.SetValue(self.properties['marker']['size'])
        self.wxId['marker']['size'] = ptsize.GetId()
        gridSizer.Add(item=ptsize, pos=(1, 1))
        
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Style"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(2, 0))
        ptfill = wx.ComboBox(parent=self, id=wx.ID_ANY,
                             size=(120, -1), choices=self.ptfilldict.keys(), style=wx.CB_DROPDOWN)
        ptfill.SetStringSelection(self.properties['marker']['fill'])
        self.wxId['marker']['fill'] = ptfill.GetId()
        gridSizer.Add(item=ptfill, pos=(2, 1))
        
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Legend"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(3, 0))
        ptlegend = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(200,-1))
        ptlegend.SetValue(self.properties['marker']['legend'])
        self.wxId['marker']['legend'] = ptlegend.GetId()
        gridSizer.Add(item=ptlegend, pos=(3, 1))
                
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Type"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(4, 0))
        pttype = wx.ComboBox(parent=self, 
                             size=(200, -1), choices=self.pttypelist, style=wx.CB_DROPDOWN)
        pttype.SetStringSelection(self.properties['marker']['type'])
        self.wxId['marker']['type'] = pttype.GetId()
        gridSizer.Add(item=pttype, pos=(4, 1))

        boxMainSizer.Add(item=gridSizer, flag=wx.ALL, border=3)
        middleSizer.Add(item=boxMainSizer, flag=wx.ALL | wx.EXPAND, border=3)

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
            type = wx.ComboBox(parent=self, id=wx.ID_ANY,
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

        # bindigs
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnApply.SetToolTipString(_("Apply changes for the current session"))
        btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        btnSave.SetToolTipString(_("Apply and save changes to user settings file (default for next sessions)"))
        btnSave.SetDefault()
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        btnCancel.SetToolTipString(_("Close dialog and ignore changes"))

        # sizers
        btnStdSizer = wx.StdDialogButtonSizer()
        btnStdSizer.AddButton(btnCancel)
        btnStdSizer.AddButton(btnSave)
        btnStdSizer.AddButton(btnApply)
        btnStdSizer.Realize()
        
        sizer.Add(item=btnStdSizer, proportion=0, flag=wx.ALIGN_RIGHT | wx.ALL, border=5)

        self.SetSizer(sizer)
        sizer.Fit(self)

    def UpdateSettings(self):
        idx = 0
        for r in self.raster.itervalues():
            r['prop']['pcolor'] = self.FindWindowById(self.wxId['pcolor'][idx]).GetColour()
            r['prop']['pwidth'] = int(self.FindWindowById(self.wxId['pwidth'][idx]).GetValue())
            r['prop']['pstyle'] = self.FindWindowById(self.wxId['pstyle'][idx]).GetStringSelection()
            r['plegend'] = self.FindWindowById(self.wxId['plegend'][idx]).GetValue()
            idx +=1

        self.properties['marker']['color'] = self.FindWindowById(self.wxId['marker']['color']).GetColour()
        self.properties['marker']['fill'] = self.FindWindowById(self.wxId['marker']['fill']).GetStringSelection()
        self.properties['marker']['size'] = self.FindWindowById(self.wxId['marker']['size']).GetValue()
        self.properties['marker']['type'] = self.FindWindowById(self.wxId['marker']['type']).GetValue()
        self.properties['marker']['legend'] = self.FindWindowById(self.wxId['marker']['legend']).GetValue()

        for axis in ('x-axis', 'y-axis'):
            self.properties[axis]['prop']['type'] = self.FindWindowById(self.wxId[axis]['type']).GetValue()
            self.properties[axis]['prop']['min'] = float(self.FindWindowById(self.wxId[axis]['min']).GetValue())
            self.properties[axis]['prop']['max'] = float(self.FindWindowById(self.wxId[axis]['max']).GetValue())
            self.properties[axis]['prop']['log'] = self.FindWindowById(self.wxId[axis]['log']).IsChecked()

        self.properties['grid']['color'] = self.FindWindowById(self.wxId['grid']['color']).GetColour()
        self.properties['grid']['enabled'] = self.FindWindowById(self.wxId['grid']['enabled']).IsChecked()

        self.properties['font']['prop']['legendSize'] = self.FindWindowById(self.wxId['font']['legendSize']).GetValue()
        self.properties['legend']['enabled'] = self.FindWindowById(self.wxId['legend']['enabled']).IsChecked()

    def OnSave(self, event):
        """Button 'Save' pressed"""
        self.UpdateSettings()
        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        fileSettings['profile'] = UserSettings.Get(group='profile')
        file = UserSettings.SaveToFile(fileSettings)
        self.parent.parent.gismanager.goutput.WriteLog(_('Profile settings saved to file \'%s\'.') % file)
        self.Close()

    def OnApply(self, event):
        """Button 'Apply' pressed"""
        self.UpdateSettings()
        self.Close()

    def OnCancel(self, event):
        """Button 'Cancel' pressed"""
        self.Close()
