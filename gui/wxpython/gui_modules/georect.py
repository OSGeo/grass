"""
MODULE:    georect.py

CLASSES:
    * Georectify
    * GCP
    * GRMap

PURPOSE:   Georectification module for GRASS GIS. Includes ground control
            point management and interactive point and click GCP creation

AUTHORS:   The GRASS Development Team
           Michael Barton

COPYRIGHT: (C) 2006-2007 by the GRASS Development Team
           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.
"""

# recheck once completed to see how many of these are still needed
import os
import sys
import time
import glob
import math
import tempfile
import shutil

import wx
import wx.aui
import wx.lib.filebrowsebutton as filebrowse
from wx.lib.mixins.listctrl import CheckListCtrlMixin, ListCtrlAutoWidthMixin, TextEditMixin
import wx.wizard as wiz
import wx.grid as gridlib

from threading import Thread

import globalvar
import mapdisp
import render
import toolbars
import menuform
import gselect
import disp_print
import gcmd
import utils
import menuform
from debug import Debug as Debug
from icon import Icons as Icons

try:
    import subprocess # Not needed if GRASS commands could actually be quiet
except:
    CompatPath = globalvar.ETCWXDIR
    sys.path.append(CompatPath)
    from compat import subprocess

gmpath = os.path.join(globalvar.ETCWXDIR, "icons")
sys.path.append(gmpath)

import images
imagepath = images.__path__[0]
sys.path.append(imagepath)

# global variables
global xy_map

global maptype

xy_map = ''
maptype = 'cell'


class TitledPage(wiz.WizardPageSimple):
    """
    Class to make wizard pages. Generic methods to make
    labels, text entries, and buttons.
    """
    def __init__(self, parent, title):
        wiz.WizardPageSimple.__init__(self, parent)

        self.title = wx.StaticText(self,-1,title)
        self.title.SetFont(wx.Font(13, wx.SWISS, wx.NORMAL, wx.BOLD))
        self.sizer = wx.BoxSizer(wx.VERTICAL)

        tmpsizer = wx.BoxSizer(wx.VERTICAL)

        tmpsizer.Add(self.title, 0, wx.ALIGN_CENTRE|wx.ALL, 5)
        tmpsizer.AddSpacer(10)
        tmpsizer.Add(wx.StaticLine(self, -1), 0, wx.EXPAND|wx.ALL, 0)
        tmpsizer.Add(self.sizer, wx.EXPAND)

        self.SetSizer(tmpsizer)
        self.SetAutoLayout(True)
        tmpsizer.Fit(self)

class GeorectWizard(object):
    """
    Start wizard here and finish wizard here
    """

    def __init__(self, parent):
        #
        # define wizard image
        #
        # file = "loc_wizard.png"
        #file = "loc_wizard_qgis.png"
        #imagePath = os.path.join(globalvar.ETCWXDIR, "images",
        #                         file)
        #wizbmp = wx.Image(imagePath, wx.BITMAP_TYPE_PNG)
        ## wizbmp.Rescale(250,600)
        #wizbmp = wizbmp.ConvertToBitmap()

        self.parent = parent

        #set environmental variables
        cmdlist = ['g.gisenv', 'get=GISDBASE']
        #global grassdatabase
        p = gcmd.Command(cmdlist)
        self.grassdatabase = p.ReadStdOutput()[0]
        
        # read original environment settings
        #self.orig_env = os.environ.copy()
        self.orig_gisrc = os.environ['GISRC']
        f = open(self.orig_gisrc)
        self.gisrc_dict = {}
        try:
            for line in f:
                if line != '':
                    line = line.strip(' \n')
                    self.gisrc_dict[line.split(':')[0]] = line.split(':')[1].strip()
        finally:
            f.close()
            
        self.currentlocation = self.gisrc_dict['LOCATION_NAME']
        self.currentmapset = self.gisrc_dict['MAPSET']
        self.newlocation = '' #location for xy map to georectify
        self.newmapset = '' #mapset for xy map to georectify
        
        self.new_gisrc = '' #GISRC file for source location/mapset of map(s) to georectify
        
        # define wizard pages
        self.wizard = wiz.Wizard(parent, -1, "Setup for georectification")
        self.startpage = LocationPage(self.wizard, self)
        self.grouppage = GroupPage(self.wizard, self)
        self.mappage = DispMapPage(self.wizard, self)

        # Set the initial order of the pages
        self.startpage.SetNext(self.grouppage)

        self.grouppage.SetPrev(self.startpage)
        self.grouppage.SetNext(self.mappage)

        self.mappage.SetPrev(self.grouppage)

        self.wizard.FitToPage(self.startpage)

        #self.Bind(wx.EVT_CLOSE,    self.Cleanup)
        self.parent.Bind(wx.EVT_ACTIVATE, self.OnGLMFocus)

        success = False

        if self.wizard.RunWizard(self.startpage):
            success = self.onWizFinished()
            if success == True:
                pass
            else:
                wx.MessageBox("Georectifying setup canceled.")
                self.Cleanup()
        else:
            wx.MessageBox("Georectifying setup canceled.")
            self.Cleanup()

        # start display showing xymap
        if success != False:
            self.Map = render.Map()    # instance of render.Map to be associated with display
    
            global maptype
            global xy_map
    
            if maptype == 'cell':
                rendertype = 'raster'
                cmdlist = ['d.rast', 'map=%s' % xy_map]
            elif maptype == 'vector':
                rendertype = 'vector'
                cmdlist = ['d.vect', 'map=%s' % xy_map]
    
            self.Map.AddLayer(type=rendertype, command=cmdlist,l_active=True,
                              l_hidden=False, l_opacity=1, l_render=False)
                
            self.xy_mapdisp = mapdisp.MapFrame(self.parent, title="Set ground control points (GCPs)",
                 pos=wx.DefaultPosition, size=(640,480),
                 style=wx.DEFAULT_FRAME_STYLE, toolbars=["georect"],
                 Map=self.Map, gismgr=self.parent, georect=True)
            
            self.mapwin = self.xy_mapdisp.MapWindow
            
            # set mouse characteristics
            self.mapwin.mouse['box'] = 'point'
            self.mapwin.mouse["use"] == "pointer"
            self.mapwin.zoomtype = 0
            self.mapwin.pen = wx.Pen(colour='black', width=2, style=wx.SOLID)
            self.mapwin.SetCursor(self.xy_mapdisp.cursors["cross"])
            
            # draw selected xy map
            self.xy_mapdisp.MapWindow.UpdateMap()

            #show new display
            self.xy_mapdisp.Show()
            self.xy_mapdisp.Refresh()
            self.xy_mapdisp.Update()
    
            # start GCP form
            self.gcpmgr = GCP(self.parent, grwiz=self)
            self.gcpmgr.Show()
            self.gcpmgr.Refresh()
            self.gcpmgr.Update()
        else:
            self.Cleanup()
                            
    def SetSrcEnv(self, location, mapset):
        """Create environment to use for location and mapset
        that are the source of the file(s) to georectify"""
        
        self.newlocation = location
        self.newmapset = mapset
        
        # check to see if we are georectifying map in current working location/mapset
        if self.newlocation == self.currentlocation and self.newmapset == self.currentmapset:
            return
        
        self.gisrc_dict['LOCATION_NAME'] = location
        self.gisrc_dict['MAPSET'] = mapset
        self.new_gisrc = utils.GetTempfile()     
        f = open(self.new_gisrc, mode='w')        
        for line in self.gisrc_dict.items():
            f.write(line[0]+": "+line[1]+"\n")
        f.close()

    def SwitchEnv(self, grc):
        """
        Switches between original working location/mapset and
        location/mapset that is source of file(s) to georectify
        """

        # check to see if we are georectifying map in current working location/mapset
        if self.newlocation == self.currentlocation and self.newmapset == self.currentmapset:
            return

        if grc == 'original':
            os.environ["GISRC"] = str(self.orig_gisrc)
        elif grc == 'new':
            os.environ["GISRC"] = str(self.new_gisrc)
        
    def onWizFinished(self):
        return True
        self.Cleanup()
        
    def OnGLMFocus(self, event):
        #self.SwitchEnv('original')
        pass

    def Cleanup(self):
        # return to current location and mapset
        self.SwitchEnv('original')
        self.parent.georectifying = False
        try:
            self.xy_mapdisp.Destroy()
            self.wizard.Destroy()
        except:
            pass

class LocationPage(TitledPage):
    """
    Set map type (raster or vector) to georectify and
    select location/mapset of map(s) to georectify.
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, "Select map type and location/mapset")

        self.parent = parent
        self.grassdatabase = self.parent.grassdatabase
        
        self.xylocation = ''
        self.xymapset = ''

        tmplist = os.listdir(self.grassdatabase)

        self.locList = []

        # Create a list of valid locations
        for item in tmplist:
            if os.path.isdir(os.path.join(self.grassdatabase,item)) and \
                os.path.exists(os.path.join(self.grassdatabase,item,'PERMANENT')):
                self.locList.append(item)

        self.mapsetList = []

        box = wx.BoxSizer(wx.HORIZONTAL)
        self.rb_maptype = wx.RadioBox(self, -1, "Map type to georectify",
                                   wx.DefaultPosition, wx.DefaultSize,
                                   ['raster','vector'], 2, wx.RA_SPECIFY_COLS)
        box.Add(self.rb_maptype, 0, wx.ALIGN_CENTER|wx.ALL, 5)
        self.sizer.Add(box, 0, wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)

        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(self, -1, 'select location:',
                style=wx.ALIGN_RIGHT)
        box.Add(label, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.cb_location = wx.ComboBox(self, wx.ID_ANY, "",
                                     wx.DefaultPosition,
                                     wx.DefaultSize,
                                     choices = self.locList,
                                     style=wx.CB_DROPDOWN|wx.CB_READONLY)
        box.Add(self.cb_location, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.sizer.Add(box, 0, wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)

        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(self, -1, 'select mapset:',
                style=wx.ALIGN_RIGHT)
        box.Add(label, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.cb_mapset = wx.ComboBox(self, wx.ID_ANY, "",
                                     wx.DefaultPosition,
                                     wx.DefaultSize,
                                     choices = self.mapsetList,
                                     style=wx.CB_DROPDOWN|wx.CB_READONLY)
        box.Add(self.cb_mapset, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.sizer.Add(box, 0, wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)

        self.Bind(wx.EVT_RADIOBOX, self.OnMaptype, self.rb_maptype)
        self.Bind(wx.EVT_COMBOBOX, self.OnLocation, self.cb_location)
        self.Bind(wx.EVT_COMBOBOX, self.OnMapset, self.cb_mapset)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.onPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnPageChanged)
        self.Bind(wx.EVT_CLOSE, self.parent.Cleanup)

    def OnMaptype(self,event):
        global maptype

        if event.GetInt() == 0:
            maptype = 'cell'
        elif event.GetInt() == 1:
            maptype = 'vector'

    def OnLocation(self, event):
        """Sets source location for map(s) to georectify"""

        self.xylocation = event.GetString()
        
        #create a list of valid mapsets
        tmplist = os.listdir(os.path.join(self.grassdatabase,self.xylocation))
        self.mapsetList = []
        for item in tmplist:
            if os.path.isdir(os.path.join(self.grassdatabase,self.xylocation,item)) and \
                os.path.exists(os.path.join(self.grassdatabase,self.xylocation,item,'WIND')):
                self.mapsetList.append(item)

        self.cb_mapset.SetItems(self.mapsetList)

    def OnMapset(self, event):
        """Sets source mapset for map(s) to georectify"""

        if self.xylocation == '':
            wx.MessageBox('You must select a valid location before selecting a mapset')
            return

        self.xymapset = event.GetString()

    def onPageChanging(self,event=None):

        if event.GetDirection() and (self.xylocation == '' or self.xymapset == ''):
            wx.MessageBox('You must select a valid location and mapset in order to continue')
            event.Veto()
            return
        else:
            self.parent.SetSrcEnv(self.xylocation, self.xymapset)

    def OnPageChanged(self,event=None):
        pass

class GroupPage(TitledPage):
    """
    Set group to georectify. Create group if desired.
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, "Select image/map group to georectify")

        self.parent = parent
        self.groupList = []
        self.grassdatabase = self.parent.grassdatabase
        self.xylocation = ''
        self.xymapset = ''
        self.xygroup = ''
        
        self.extension = 'georect'+str(os.getpid())
        
        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(self, -1, 'select group:',
                style=wx.ALIGN_RIGHT)
        box.Add(label, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.cb_group = wx.ComboBox(self, wx.ID_ANY, "",
                                     wx.DefaultPosition,
                                     wx.DefaultSize,
                                     choices = self.groupList,
                                     style=wx.CB_DROPDOWN|wx.CB_READONLY)
        box.Add(self.cb_group, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.sizer.Add(box, 0, wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)

        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(self, -1, 'Create group if none exists', style=wx.ALIGN_LEFT)
        box.Add(label, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.btn_mkgroup = wx.Button(self, wx.ID_ANY, "Create/edit group ...")
        box.Add(self.btn_mkgroup, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.sizer.Add(box, 0, wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)

        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(self, -1, 'Extension for output maps:', style=wx.ALIGN_LEFT)
        box.Add(label, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.ext_txt = wx.wx.TextCtrl(self, -1, "", size=(150,-1))
        self.ext_txt.SetValue(self.extension)
        box.Add(self.ext_txt, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.sizer.Add(box, 0, wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)

        self.Bind(wx.EVT_COMBOBOX, self.OnGroup, self.cb_group)
        self.Bind(wx.EVT_BUTTON, self.OnMkGroup, self.btn_mkgroup)
        self.Bind(wx.EVT_TEXT, self.OnExtension, self.ext_txt)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.onPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnPageChanged)
        self.Bind(wx.EVT_CLOSE, self.parent.Cleanup)

    def OnGroup(self, event):        
        self.xygroup = event.GetString()
        
    def OnMkGroup(self, event):
        global maptype
        
        self.parent.SwitchEnv('new')
        if maptype == 'cell':
            cmdlist = ['i.group']       
            menuform.GUI().ParseCommand(cmdlist, parentframe=self.parent.parent)
        elif maptype == 'vector':
            dlg = VectGroup(self, wx.ID_ANY, self.grassdatabase, self.xylocation, self.xymapset, self.xygroup)
            if dlg.ShowModal() == wx.ID_OK:
                dlg.MakeVGroup()
                
        #refresh combobox list
        try:
            tmplist = os.listdir(os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'group'))
        except:
            return
        if tmplist != []:
            for item in tmplist:
                if os.path.isdir(os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'group',item)):
                    self.groupList.append(item)

            self.cb_group.SetItems(self.groupList)
        
    def OnExtension(self, event):
        self.extension = event.GetString()

    def onPageChanging(self,event=None):
        if event.GetDirection() and self.xygroup == '':
            wx.MessageBox('You must select a valid image/map group in order to continue')
            event.Veto()
            return

        if event.GetDirection() and self.extension == '':
            wx.MessageBox('You must enter an map name extension in order to continue')
            event.Veto()
            return

    def OnPageChanged(self,event=None):
        self.groupList = []
        tmplist = []
        self.xylocation = self.parent.gisrc_dict['LOCATION_NAME']
        self.xymapset = self.parent.gisrc_dict['MAPSET']

        # create a list of groups in selected mapset
        tmplist = os.listdir(os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'group'))

        if event.GetDirection() and self.xygroup == '':
            if tmplist == []:
                wx.MessageBox('No map/imagery groups exist to georectify. You will need to create one')
            else:
                for item in tmplist:
                    if os.path.isdir(os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'group',item)):
                        self.groupList.append(item)
    
                self.cb_group.SetItems(self.groupList)
                
class DispMapPage(TitledPage):
    """
    Select ungeoreferenced map to display for interactively
    setting ground control points (GCPs).
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, "Select image/map to display for ground control point (GCP) creation")

        self.parent = parent
        global maptype

        self.parent = parent

        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(self, -1, 'Select display image/map:', style=wx.ALIGN_LEFT)
        box.Add(label, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.selection = gselect.Select(self, id=wx.ID_ANY, size=(300,-1),
                                              type=maptype )
        box.Add(self.selection, 0, wx.ALIGN_LEFT|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)
        self.sizer.Add(box, 0, wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)

        self.selection.Bind(wx.EVT_TEXT, self.OnSelection)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.onPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnPageChanged)
        self.Bind(wx.EVT_CLOSE, self.parent.Cleanup)

    def OnSelection(self,event):
        global xy_map

        xy_map = event.GetString()

    def onPageChanging(self,event=None):
        global xy_map

        if event.GetDirection() and xy_map == '':
            wx.MessageBox('You must select a valid image/map in order to continue')
            event.Veto()
            return

    def OnPageChanged(self,event=None):
        global maptype

        if event.GetDirection():
            # switch to xy location if coming into the page from preceding
            self.parent.SwitchEnv('new')
            self.selection.SetElementList(maptype)
        else:
            # switch back to current location if leaving the page
            self.parent.SwitchEnv('original')

class GCP(wx.Frame):
    """
    Manages ground control points for georectifying. Calculates RMS statics.
    Calls i.rectify or v.transform to georectify map.
    """

    def __init__(self,parent,id=-1,title="Create & manage ground control points",
                 size=wx.DefaultSize, grwiz=None):
        wx.Frame.__init__(self, parent, id , title, size=(600,300))
        self.Centre(wx.HORIZONTAL)

        toolbar = self.__createToolBar()
        
        self.parent = parent
        self.grwiz = grwiz
        self.grassdatabase = self.grwiz.grassdatabase
        self.currentlocation = self.grwiz.currentlocation
        self.currentmapset = self.grwiz.currentmapset
        self.newlocation = self.grwiz.newlocation
        self.newmapset = self.grwiz.newmapset
        self.xylocation = self.grwiz.gisrc_dict['LOCATION_NAME']
        self.xymapset = self.grwiz.gisrc_dict['MAPSET']
        self.xygroup = self.grwiz.grouppage.xygroup
        self.extension = self.grwiz.grouppage.extension
        self.pointsfile = os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'group',self.xygroup,'POINTS')
        self.rgrpfile = os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'group',self.xygroup,'REF')
        self.vgrpfile = os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'group',self.xygroup,'VREF')
        self.targetfile = os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'group',self.xygroup,'TARGET')
        
        self.SetTarget(self.xygroup, self.currentlocation, self.currentmapset)

        self.gr_order = 1 #polynomial order transformation for georectification
        self.selected = 0 #gcp list item selected
        self.GCPcount = 0 #number of GCPs selected to be used for georectification (checked)
        self.fwd_rmserror = 0.0 # forward RMS error
        self.bkw_rmserror = 0.0 # backward RMS error
        self.mapcoordlist = [(0000000.00,0000000.00,'')] #list map coords and ID of map display they came from
        
        self.CreateStatusBar(3,1,-1,'gcpstatusbar')
        self.SetStatusText('RMS error for selected points',0)
        
        # can put guage into custom statusbar for progress if can figure out how to get progress text from i.rectify
        #self.gr_gauge = wx.Gauge(self, -1, 100, (-1,-1), (100, 25))
        #self.gr_guage.Pulse()

        p = wx.Panel(self, -1, style=0)

        self.sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.BoxSizer(wx.HORIZONTAL)
        self.rb_grmethod = wx.RadioBox(p, -1, "Select rectification method for rasters ",
                                   wx.DefaultPosition, wx.DefaultSize,
                                   ['1st order','2nd order', '3rd order'], 3, wx.RA_SPECIFY_COLS)
        box.Add(self.rb_grmethod, 0, wx.ALIGN_CENTER|wx.ALL, 5)
        self.sizer.Add(box, 0, wx.GROW|wx.ALIGN_CENTER_VERTICAL|wx.ALL, 5)

        # initialize list control for GCP management
        self.list = CheckListCtrl(p, -1, style=wx.LC_REPORT | wx.SUNKEN_BORDER | wx.LC_HRULES)
        self.list.InsertColumn(0, 'use| X coord', width=120)
        self.list.InsertColumn(1, 'Y coord')
        self.list.InsertColumn(2, 'N coord')
        self.list.InsertColumn(3, 'E coord')
        self.list.InsertColumn(4, 'Forward error')
        self.list.InsertColumn(5, 'Backward error')
        
        if os.path.isfile(self.pointsfile):
            self.ReadGCPs()
            self.ResizeColumns()
        elif self.list.GetItemCount() == 0:
            # initialize 3 blank lines in the GCP list (minimum for georectification)
            i = ('0000000.00','0000000.00','0000000.00','0000000.00','','')
            index = self.list.InsertStringItem(sys.maxint, i[0])
            self.list.SetStringItem(index, 1, i[1])
            self.list.SetStringItem(index, 2, i[2])
            self.list.SetStringItem(index, 3, i[3])
            self.list.SetStringItem(index, 4, i[4])
            self.list.SetStringItem(index, 5, i[5])
            self.list.CheckItem(0, True)
            self.AddGCP(None)
            self.AddGCP(None)
            self.ResizeColumns()

        self.sizer.Add(self.list, 1, wx.EXPAND|wx.ALL, 5)
        p.SetSizer(self.sizer)

        self.Bind(wx.EVT_RADIOBOX, self.OnGRMethod, self.rb_grmethod)
        self.list.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.list.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.OnItemActivated)
        self.Bind(wx.EVT_ACTIVATE, self.OnFocus)
        self.Bind(wx.EVT_CLOSE, self.grwiz.Cleanup)

    def __createToolBar(self):
        """Creates toolbar"""

        toolbar = self.CreateToolBar()
        for each in self.toolbarData():
            self.addToolbarButton(toolbar, *each)
        toolbar.Realize()
        
    def OnFocus(self, event):
        self.grwiz.SwitchEnv('new')
        
    def ResizeColumns(self):
        for i in range(6):
            self.list.SetColumnWidth(i, wx.LIST_AUTOSIZE)
        
    def SetTarget(self, tgroup, tlocation, tmapset):
        """
        Sets rectification target to current location and mapset
        """

       # check to see if we are georectifying map in current working location/mapset
        if self.newlocation == self.currentlocation and self.newmapset == self.currentmapset:
            cmdlist = ['i.target', 'c', 'group=%s' % tgroup]
        else:
            self.grwiz.SwitchEnv('new')
            cmdlist = ['i.target', 'group=%s' % tgroup, 'location=%s' % tlocation, 'mapset=%s' % tmapset]
        gcmd.Command(cmd=cmdlist)

    def addToolbarButton(self, toolbar, label, icon, help, handler):
        """Adds button to the given toolbar"""

        if not label:
            toolbar.AddSeparator()
            return
        tool = toolbar.AddLabelTool(id=wx.ID_ANY, label=label, bitmap=icon, shortHelp=help)
        self.Bind(wx.EVT_TOOL, handler, tool)

    def toolbarData(self):

        return   (
                 ('savegcp', Icons["savefile"].GetBitmap(), 'Save GCPs to POINTS file', self.SaveGCPs),
                 ('addgcp',  wx.ArtProvider.GetBitmap(wx.ART_NEW, wx.ART_TOOLBAR, (16,16)), 'Add new GCP', self.AddGCP),
                 ('deletegcp',  wx.ArtProvider.GetBitmap(wx.ART_DELETE, wx.ART_TOOLBAR, (16,16)), 'Delete selected GCP', self.DeleteGCP),
                 ('cleargcp', Icons["cleargcp"].GetBitmap(), Icons["cleargcp"].GetLabel(), self.ClearGCP),
                 ('refreshgcp', Icons["refreshgcp"].GetBitmap(), Icons["refreshgcp"].GetLabel(), self.RefreshGCPMarks),
                 ('rms', Icons["rms"].GetBitmap(), Icons["rms"].GetLabel(), self.OnRMS),
                 ('georect',  Icons["georect"].GetBitmap(),  Icons["georect"].GetLabel(),  self.OnGeorect),
                 ('quit',  wx.ArtProvider.GetBitmap(wx.ART_QUIT, wx.ART_TOOLBAR, (16,16)), 'Quit georectification module', self.OnQuit)
                  )

    def SaveGCPs(self, event):
        """
        Make a POINTS file or save GCP coordinates to existing POINTS file
        """
        
        self.GCPcount = 0
        f = open(self.pointsfile, mode='w')
        try:
            f.write('# Ground Control Points File\n')
            f.write("# \n")
            f.write("# target location: "+self.currentlocation+'\n')
            f.write("# target mapset: "+self.currentmapset+'\n')
            f.write("#unrectified xy     georectified east north     1=use gcp point\n")
            f.write("#--------------     -----------------------     ---------------\n")

            for index in range(self.list.GetItemCount()):
                if self.list.IsChecked(index) == True:
                    check = "1"
                    self.GCPcount += 1
                else:
                    check = "0"
                coord0 = self.list.GetItem(index, 0).GetText()
                coord1 = self.list.GetItem(index, 1).GetText()
                coord2 = self.list.GetItem(index, 2).GetText()
                coord3 = self.list.GetItem(index, 3).GetText()
                f.write(coord0+' '+coord1+'     '+coord2+' '+coord3+'     '+check+'\n')
        finally:
            f.close()
        pass

    def DeleteGCP(self, event):
        """
        Deletes selected item in GCP list
        """
        
        self.list.DeleteItem(self.selected)
        del self.mapcoordlist[self.selected]

    def AddGCP(self, event):
        """
        Appends an item to GCP list
        """
        
        self.list.Append(['0000000.00','0000000.00','0000000.00','0000000.00','',''])
        index = self.list.GetItemCount() - 1
        self.mapcoordlist.append((0000000.00,0000000.00,''))
        self.list.SetItemState(index, wx.LIST_STATE_SELECTED, wx.LIST_STATE_SELECTED)
        self.ResizeColumns()
        return index
        
    def SetGCPData(self, coordtype, coord, mapdisp=None, check=True):
        """
        Inserts coordinates from mouse click on map
        into selected item of GCP list and checks it for use
        """
        
        index = self.selected
        coord0 = str(coord[0])
        coord1 = str(coord[1])

        if coordtype == 'gcpcoord':
            self.list.SetStringItem(index, 0, coord0)
            self.list.SetStringItem(index, 1, coord1)
        if coordtype == 'mapcoord':
            self.list.SetStringItem(index, 2, coord0)
            self.list.SetStringItem(index, 3, coord1)
            self.mapcoordlist[index] = (coord[0], coord[1], mapdisp)
        self.list.CheckItem(index, check)
        self.ResizeColumns()

    def ClearGCP(self, event):
        """
        Clears all values in selected item of GCP list and unchecks it
        """
        
        index = self.selected
        for i in range(4):
            self.list.SetStringItem(index, i, '0000000.00')
        self.list.SetStringItem(index, 4, '')
        self.list.SetStringItem(index, 5, '')
        self.mapcoordlist[index] = (0000000.00,0000000.00,'')
        self.list.CheckItem(index, False)
        
    def ReadGCPs(self):
        """
        Reads GCPs and georectified coordinates from POINTS file
        """
        
        self.GCPcount = 0
        f = open(self.pointsfile)
        try:
            GCPcnt = 0
            for line in f:
                if line[0] != '#' and line !='':
                    line = line.strip(' \n')
                    coords = line.split()
                    if coords[4] == '1':
                        check = True
                        self.GCPcount +=1
                    else:
                        check = False
                    index = self.AddGCP(event=None)
                    for i in range(4):
                        self.list.SetStringItem(index, i, coords[i])
                    self.list.CheckItem(index, check)
        finally:
            f.close()
        self.RefreshGCPMarks(None)

        if self.CheckGCPcount():
            self.RMSError(self.xygroup, self.gr_order)
            
    def OnRMS(self, event):
        """
        RMS button handler
        """
        self.RMSError(self.xygroup,self.gr_order)
        
    def CheckGCPcount(self, msg=False):
        """
        Checks to make sure that the minimum number of GCPs have been defined and
        are active for the selected transformation order
        """
        if (self.GCPcount < 3 and self.gr_order == 1) or \
            (self.GCPcount < 6 and self.gr_order == 2) or \
            (self.GCPcount < 10 and self.gr_order == 3):
            if msg:
                s1 = 'Insufficient points defined and active (checked)\n'
                s2 = 'for selected rectification method.\n'
                s3 = '3+ points needed for 1st order,\n'
                s4 = '6+ points for 2nd order, and\n'
                s5 = '10+ points for 3rd order.'
                wx.MessageBox('%s%s%s%s%s' % (s1, s2, s3, s4, s5))
            return False
        else:
            return True

    def OnGeorect(self, event):
        """
        Georectifies map(s) in group using i.rectify or v.transform
        """
        global maptype
        self.SaveGCPs()
        
        if self.CheckGCPcount(msg=True) == False:
            return
                
        if maptype == 'cell':
            cmdlist = ['i.rectify', '-ca', 'group=%s' % self.xygroup, 'extension=%s' % self.extension, 'order=%s' % self.gr_order]
            p = gcmd.Command(cmd=cmdlist)
            stdout = p.ReadStdOutput()
            stderr = p.ReadErrOutput()
            msg = err = ''
            if p.returncode == 0:
                for line in stdout:
                    msg = msg+line+'\n'
                for line in stderr:
                    err = err+line+'\n'
                wx.MessageBox('All maps georectified successfully\n'+msg+'\n'+err)
            else:
                for line in stderr:
                    err = err+line+'\n'
                wx.MessageBox(err)
        elif maptype == 'vector':
            # loop through all vectors in VREF and move resulting vector to target location
            f = open(self.vgrpfile)
            vectlist = []
            try:
                for vect in f:
                    vect = vect.strip(' \n')
                    vectlist.append(vect)
            finally:
                f.close()
            for vect in vectlist:
                outname = vect+'_'+self.extension
                cmdlist = ['v.transform', '--q', 'input=%s' % vect, 'output=%s' % outname, 'pointsfile=%s' % self.pointsfile]
                                
                p = gcmd.Command(cmd=cmdlist)
                stdout = p.ReadStdOutput()
                msg = '****'+outname+'****\n'
                for line in stdout:
                    msg = msg+line+'\n'
                wx.MessageBox(msg)
                
            if p.returncode == 0:
                wx.MessageBox("All maps were georectified successfully")
                for vect in vectlist:
                    outname = vect+'_'+self.extension
                    xyvpath = os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'vector',outname)
                    vpath = os.path.join(self.grassdatabase,self.currentlocation,self.currentmapset,'vector',outname)
                    if os.path.isfile(vpath):
                        wx.MessageBox("%s already exists. Change extension name and georectify again" % outname)
                    else:
                        shutil.move(xyvpath, vpath)                
                                    
                wx.MessageBox('For vector files with attribute tables, you will need to manually copy the tables to the new location')
            else:
                wx.MessageBox('Some maps were not georectified successfully')
        else:
            return

    def OnQuit(self, event):
        self.Destroy()
        self.grwiz.Cleanup()

    def OnItemSelected(self, event):
        self.selected = event.GetIndex()
        
    def OnItemActivated(self, event):
        """
        When item double clicked, open editor to update coordinate values
        """
        coords = []
        index = event.m_itemIndex
        for i in range(4):
            coords.append(self.list.GetItem(index, i).GetText())
        
        dlg = EditGPC(self, -1, data=coords)
        if dlg.ShowModal() == wx.ID_OK:
            values = dlg.GetValues() # string
            
            for i in range(4):
                if values[i] != coords[i]:
                    self.list.SetStringItem(index, i, values[i])
                    

    def RefreshGCPMarks(self, event):
        """
        Updates GCP and map coord maps and redraws
        active (checked) GCP markers
        """
        self.grwiz.SwitchEnv("new")
        self.grwiz.mapwin.UpdateMap()
        self.grwiz.SwitchEnv("original")
        mapid = ''
        for map in self.mapcoordlist:
            if map[2] != mapid and map[2] != '':
                map[2].UpdateMap()
                mapid = map[2]
        for index in range(self.list.GetItemCount()):
            if self.list.IsChecked(index) and (
                (self.list.GetItem(index, 0).GetText() != '0000000.00' and 
                 self.list.GetItem(index, 1).GetText() != '0000000.00') or 
                (self.list.GetItem(index, 2).GetText() != '0000000.00' and 
                 self.list.GetItem(index, 3).GetText() != '0000000.00')
                ):
                coord0 = float(self.list.GetItem(index, 0).GetText())
                coord1 = float(self.list.GetItem(index, 1).GetText())
                coord2 = float(self.list.GetItem(index, 2).GetText())
                coord3 = float(self.list.GetItem(index, 3).GetText())
                
                self.grwiz.SwitchEnv("new")
                pxcoord1 = self.grwiz.mapwin.Cell2Pixel((coord0, coord1))
                self.grwiz.mapwin.DrawCross(pdc=self.grwiz.mapwin.pdcTmp,
                                            coords=pxcoord1, size=5)
                if self.mapcoordlist != [] and self.mapcoordlist[index][2] != '':
                    self.grwiz.SwitchEnv("original")
                    pxcoord2 = self.mapcoordlist[index][2].Cell2Pixel((coord2, coord3))
                    self.mapcoordlist[index][2].DrawCross(pdc=self.mapcoordlist[index][2].pdcTmp,
                                                      coords=pxcoord2, size=5)
                
    def OnGRMethod(self, event):
        """
        sets transformation order for georectifying
        """
        self.gr_order = event.GetInt() + 1
        
    def RMSError(self, xygroup, order):
        """
        Uses g.transform to calculate forward and backward error for each used GCP
        in POINTS file and insert error values into GCP list.
        Calculates total forward and backward RMS error for all used points
        """
        
        # save GCPs to points file to make sure that all checked GCPs are used
        self.SaveGCPs(None)
        
        if self.CheckGCPcount(msg=True) == False:
            return
        
        #get list of forward and reverse rms error values for each point
        self.grwiz.SwitchEnv('new')
        cmdlist = ['g.transform', 'group=%s' % xygroup, 'order=%s' % order]
        p = gcmd.Command(cmdlist)
        errlist = p.ReadStdOutput()
        if errlist == []:
            return
        
        #insert error values into GCP list for checked items
        i = 0
        sumsq_fwd_err = 0.0
        sumsq_bkw_err = 0.0
        for index in range(self.list.GetItemCount()):
            if self.list.IsChecked(index):
                fwd_err,bkw_err = errlist[i].split()
                self.list.SetStringItem(index, 4, fwd_err)
                self.list.SetStringItem(index, 5, bkw_err)
                sumsq_fwd_err += float(fwd_err)**2
                sumsq_bkw_err += float(bkw_err)**2
                i += 1
            else:
                self.list.SetStringItem(index, 4, '')
                self.list.SetStringItem(index, 5, '')
        
        #calculate RMS error
        self.fwd_rmserror = round((sumsq_fwd_err/i)**0.5,4)
        self.bkw_rmserror = round((sumsq_bkw_err/i)**0.5,4)
        self.ResizeColumns()
        self.SetStatusText('forward: %s' % self.fwd_rmserror,1)
        self.SetStatusText('backward: %s' % self.bkw_rmserror,2)

class CheckListCtrl(wx.ListCtrl, CheckListCtrlMixin, ListCtrlAutoWidthMixin):
    def __init__(self, parent, ID=-1, pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=0):
        wx.ListCtrl.__init__(self, parent, -ID, pos, size, style)
        CheckListCtrlMixin.__init__(self)
        ListCtrlAutoWidthMixin.__init__(self)
        
        self.CheckList = [] # tracks whether list items are checked or not
        #self.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.OnItemActivated)

    # this is called by the base class when an item is checked/unchecked
    def OnCheckItem(self, index, flag):
        pass
    

class VectGroup(wx.Dialog):
    """
    Dialog to create a vector group (VREF file) for georectifying
    """

    def __init__(self, parent, id, grassdb, location, mapset, group,
                style=wx.DEFAULT_DIALOG_STYLE):
  
        wx.Dialog.__init__(self, parent, id, style=style)
        
        self.grassdatabase = grassdb
        self.xylocation = location
        self.xymapset = mapset
        self.xygroup = group
        
        # get list of valid vector directories
        vectlist = os.listdir(os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'vector'))
        for dir in vectlist:
            if os.path.isfile(os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'vector',dir,'coor')):
                pass
            else:
                vectlist.remove(dir)
        
        self.vgrouplist = []
        self.vgrpfile = os.path.join(self.grassdatabase,self.xylocation,self.xymapset,'group',self.xygroup,'VREF')
        if os.path.isfile(self.vgrpfile):
            f = open(self.vgrpfile)
            try:
                for line in f:
                    if line != '':
                        self.vgrouplist.append(line.strip(' \n'))
            finally:
                f.close()
        
        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnSubmit = wx.Button(self, wx.ID_OK)
        self.btnSubmit.SetDefault()

        sizer = wx.BoxSizer(wx.VERTICAL)
        
        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(parent=self, id=wx.ID_ANY,
                              label='Select vector map(s) to add to group:')
        box.Add(label, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT, border=5)
        self.addmap = wx.CheckListBox(self, -1, wx.DefaultPosition, wx.DefaultSize, vectlist)
        box.Add(self.addmap, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT| wx.LEFT, border=5)
        sizer.Add(box, flag=wx.ALIGN_RIGHT | wx.ALL, border=3)
        
        box = wx.BoxSizer(wx.HORIZONTAL)
        label = wx.StaticText(parent=self, id=wx.ID_ANY,
                              label='Select vector map(s) to remove from group:')
        box.Add(label, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT, border=5)
        self.remmap = wx.CheckListBox(self, -1, wx.DefaultPosition, wx.DefaultSize, self.vgrouplist)
        box.Add(self.remmap, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT| wx.LEFT, border=5)
        sizer.Add(box, flag=wx.ALIGN_RIGHT | wx.ALL, border=3)

        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnSubmit)
        btnSizer.Realize()

        sizer.Add(item=btnSizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        self.SetSizer(sizer)
        sizer.Fit(self)
        self.Layout()
        
        self.Bind(wx.EVT_CHECKLISTBOX, self.AddVect, self.addmap)
        self.Bind(wx.EVT_CHECKLISTBOX, self.RemoveVect, self.remmap)

    def AddVect(self, event):
        index = event.GetSelection()
        label = self.addmap.GetString(index)
        if self.addmap.IsChecked(index):
            self.vgrouplist.append(label)
        self.addmap.SetSelection(index)    
        event.Skip()
        
    def RemoveVect(self, event):
        index = event.GetSelection()
        label = self.remmap.GetString(index)
        if self.remmap.IsChecked(index):
            self.vgrouplist.remove(label)
        self.remmap.SetSelection(index)    
        event.Skip()
        
    def MakeVGroup(self):
        f = open(self.vgrpfile, mode='w')
        for vect in self.vgrouplist:
            f.write(vect+'\n')
         

class EditGPC(wx.Dialog):
    """Dialog for editing GPC and map coordinates in list control"""
    def __init__(self, parent, id, data, 
                style=wx.DEFAULT_DIALOG_STYLE):
  
        wx.Dialog.__init__(self, parent, id, style=style)

        self.btnCancel = wx.Button(self, wx.ID_CANCEL)
        self.btnSubmit = wx.Button(self, wx.ID_OK)
        self.btnSubmit.SetDefault()

        sizer = wx.BoxSizer(wx.VERTICAL)
       
        box = wx.BoxSizer(wx.HORIZONTAL)
        xlabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                              label='X:')
        box.Add(xlabel, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT, border=5)
        self.xcoord = wx.TextCtrl(parent=self, id=wx.ID_ANY,
                            value=data[0], size=(150, -1))
        box.Add(self.xcoord, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT| wx.LEFT, border=5)
        ylabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                              label='Y:')
        box.Add(ylabel, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT| wx.LEFT, border=5)
        self.ycoord = wx.TextCtrl(parent=self, id=wx.ID_ANY,
                            value=data[1], size=(150, -1))
        box.Add(self.ycoord, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT| wx.LEFT | wx.RIGHT, border=5)
        sizer.Add(box, flag=wx.ALL, border=3)

        box = wx.BoxSizer(wx.HORIZONTAL)
        elabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                              label='E:')
        box.Add(elabel, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT| wx.LEFT, border=5)
        self.ecoord = wx.TextCtrl(parent=self, id=wx.ID_ANY,
                            value=data[2], size=(150, -1))
        box.Add(self.ecoord, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT| wx.LEFT, border=5)
        nlabel = wx.StaticText(parent=self, id=wx.ID_ANY,
                              label='N:')
        box.Add(nlabel, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT| wx.LEFT, border=5)
        self.ncoord = wx.TextCtrl(parent=self, id=wx.ID_ANY,
                            value=data[3], size=(150, -1))
        box.Add(self.ncoord, flag=wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_LEFT| wx.LEFT | wx.RIGHT, border=5)
        sizer.Add(box, flag=wx.ALL, border=3)

        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnSubmit)
        btnSizer.Realize()

        sizer.Add(item=btnSizer, proportion=0,
                  flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        self.SetSizer(sizer)
        sizer.Fit(self)
        self.Layout()

    def GetValues(self, columns=None):
        """Return list of values (as strings).
        """
        valuelist = []
        valuelist.append(self.xcoord.GetValue())
        valuelist.append(self.ycoord.GetValue())
        valuelist.append(self.ecoord.GetValue())
        valuelist.append(self.ncoord.GetValue())

        return valuelist

