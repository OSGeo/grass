"""
@package georect.py

@brief Georectification module for GRASS GIS. Includes ground control
point management and interactive point and click GCP creation

Classes:
 - GeorectWizard
 - LocationPage
 - GroupPage
 - DispMapPage
 - GCP
 - GCPList
 - VectGroup
 - EditGCP
 - GrSettingsDialog

(C) 2006-2008 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton
@author Updated by Martin Landa <landa.martin gmail.com>
"""

import os
import sys
import tempfile
import shutil
import time

import wx
from wx.lib.mixins.listctrl import CheckListCtrlMixin, ListCtrlAutoWidthMixin, TextEditMixin
import wx.lib.colourselect as  csel
import wx.wizard as wiz

import grass

import globalvar
import mapdisp
import render
import toolbars
import menuform
import gselect
import gcmd
import utils
from debug import Debug as Debug
from icon import Icons as Icons
from location_wizard import TitledPage as TitledPage
from preferences import globalSettings as UserSettings

try:
    import subprocess # Not needed if GRASS commands could actually be quiet
except:
    CompatPath = globalvar.ETCWXDIR
    sys.path.append(CompatPath)
    from compat import subprocess

gmpath = os.path.join(globalvar.ETCWXDIR, "icons")
sys.path.append(gmpath)

#
# global variables
#
global xy_map
global maptype

xy_map = ''
maptype = 'cell'

class GeorectWizard(object):
    """
    Start wizard here and finish wizard here
    """

    def __init__(self, parent):
        self.parent = parent # GMFrame

        #
        # get environmental variables
        #
        self.grassdatabase = grass.gisenv()['GISDBASE']

        #
        # read original environment settings
        #
        self.orig_gisrc = os.environ['GISRC']
        self.gisrc_dict = {}
        try:
            f = open(self.orig_gisrc, 'r')
            for line in f.readlines():
                line = line.replace('\n', '').strip()
                if len(line) < 1:
                    continue
                print line
                key, value = line.split(':', 1)
                self.gisrc_dict[key.strip()] = value.strip()
        finally:
            f.close()
            
        self.currentlocation = self.gisrc_dict['LOCATION_NAME']
        self.currentmapset = self.gisrc_dict['MAPSET']
        # location for xy map to georectify
        self.newlocation = ''
        # mapset for xy map to georectify
        self.newmapset = '' 

        # GISRC file for source location/mapset of map(s) to georectify
        self.new_gisrc = ''

        #
        # define wizard pages
        #
        self.wizard = wiz.Wizard(parent=parent, id=wx.ID_ANY, title=_("Setup for georectification"))
        self.startpage = LocationPage(self.wizard, self)
        self.grouppage = GroupPage(self.wizard, self)
        self.mappage = DispMapPage(self.wizard, self)

        #
        # set the initial order of the pages
        #
        self.startpage.SetNext(self.grouppage)
        self.grouppage.SetPrev(self.startpage)
        self.grouppage.SetNext(self.mappage)
        self.mappage.SetPrev(self.grouppage)

        #
        # do pages layout
        #
        self.startpage.DoLayout()
        self.grouppage.DoLayout()
        self.mappage.DoLayout()
        self.wizard.FitToPage(self.startpage)

        # self.Bind(wx.EVT_CLOSE,    self.Cleanup)
        # self.parent.Bind(wx.EVT_ACTIVATE, self.OnGLMFocus)

        success = False

        #
        # run wizard
        #
        if self.wizard.RunWizard(self.startpage):
            success = self.OnWizFinished()
            if success == False:
                wx.MessageBox(parent=self.parent,
                              message=_("Georectifying setup canceled."),
                              caption=_("Georectify"),
                              style=wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
                self.Cleanup()
        else:
            wx.MessageBox(parent=self.parent,
                          message=_("Georectifying setup canceled."),
                          caption=_("Georectify"),
                          style=wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
            self.Cleanup()

        #
        # start display showing xymap
        #
        if success != False:
            # instance of render.Map to be associated with display
            self.Map = render.Map(gisrc=self.new_gisrc) 
            
            global maptype
            global xy_map

            #
            # add layer to map
            #
            if maptype == 'cell':
                rendertype = 'raster'
                cmdlist = ['d.rast', 'map=%s' % xy_map]
            else: # -> vector layer
                rendertype = 'vector'
                cmdlist = ['d.vect', 'map=%s' % xy_map]
            
            self.Map.AddLayer(type=rendertype, command=cmdlist, l_active=True,
                              name=utils.GetLayerNameFromCmd(cmdlist),
                              l_hidden=False, l_opacity=1.0, l_render=False)
            
            #
            # start GCP form
            #
            self.gcpmgr = GCP(self.parent, grwiz=self)
            self.gcpmgr.Show()

            #
            # start map display
            #
            self.xy_mapdisp = mapdisp.MapFrame(self.gcpmgr, title=_("Set ground control points (GCPs)"),
                                               size=globalvar.MAP_WINDOW_SIZE,
                                               toolbars=["georect"],
                                               Map=self.Map, lmgr=self.parent)

            self.gcpmgr.SetMapDisplay(self.xy_mapdisp)
            
            self.mapwin = self.xy_mapdisp.MapWindow
            
            # set mouse characteristics
            self.mapwin.mouse['box'] = 'point'
            self.mapwin.mouse["use"] == "pointer"
            self.mapwin.zoomtype = 0
            self.mapwin.pen = wx.Pen(colour='black', width=2, style=wx.SOLID)
            self.mapwin.SetCursor(self.xy_mapdisp.cursors["cross"])

            #
            # show new display & draw map
            #
            self.xy_mapdisp.Show()
        else:
            self.Cleanup()
                            
    def SetSrcEnv(self, location, mapset):
        """Create environment to use for location and mapset
        that are the source of the file(s) to georectify

        @param location source location
        @param mapset source mapset

        @return False on error
        @return True on success
        """
        
        self.newlocation = location
        self.newmapset = mapset
        
        # check to see if we are georectifying map in current working location/mapset
        if self.newlocation == self.currentlocation and self.newmapset == self.currentmapset:
            return False
        
        self.gisrc_dict['LOCATION_NAME'] = location
        self.gisrc_dict['MAPSET'] = mapset
        
        self.new_gisrc = utils.GetTempfile()

        try:
            f = open(self.new_gisrc, mode='w')        
            for line in self.gisrc_dict.items():
                f.write(line[0] + ": " + line[1] + "\n")
        finally:
            f.close()

        return True

    def SwitchEnv(self, grc):
        """
        Switches between original working location/mapset and
        location/mapset that is source of file(s) to georectify
        """
        # check to see if we are georectifying map in current working location/mapset
        if self.newlocation == self.currentlocation and self.newmapset == self.currentmapset:
            return False

        if grc == 'original':
            os.environ["GISRC"] = str(self.orig_gisrc)
        elif grc == 'new':
            os.environ["GISRC"] = str(self.new_gisrc)

        return True
    
    def OnWizFinished(self):
        # self.Cleanup()

        return True
        
    def OnGLMFocus(self, event):
        """Layer Manager focus"""
        # self.SwitchEnv('original')
        
        event.Skip()

    def Cleanup(self):
        """Return to current location and mapset"""
        self.SwitchEnv('original')
        self.parent.georectifying = None

        if hasattr(self, "xy_mapdisp"):
            self.xy_mapdisp.Close()
            self.xy_mapdisp = None

        self.wizard.Destroy()

class LocationPage(TitledPage):
    """
    Set map type (raster or vector) to georectify and
    select location/mapset of map(s) to georectify.
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Select map type and location/mapset"))

        self.parent = parent
        self.grassdatabase = self.parent.grassdatabase
        
        self.xylocation = ''
        self.xymapset = ''

        tmplist = os.listdir(self.grassdatabase)
        self.locList = []
        self.mapsetList = []
        
        #
        # create a list of valid locations
        #
        for item in tmplist:
            if os.path.isdir(os.path.join(self.grassdatabase, item)) and \
                    os.path.exists(os.path.join(self.grassdatabase, item, 'PERMANENT')):
                self.locList.append(item)

        utils.ListSortLower(self.locList)
        
        #
        # layout
        #
        self.sizer.AddGrowableCol(2)
        # map type
        self.rb_maptype = wx.RadioBox(parent=self, id=wx.ID_ANY,
                                      label=' %s ' % _("Map type to georectify"),
                                      choices=[_('raster'), _('vector')],
                                      majorDimension=wx.RA_SPECIFY_COLS)
        self.sizer.Add(item=self.rb_maptype,
                       flag=wx.ALIGN_CENTER | wx.ALL | wx.EXPAND, border=5,
                       pos=(1, 1), span=(1, 2))

        # location
        self.sizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=_('Select source location:')),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(2, 1))
        self.cb_location = wx.ComboBox(parent=self, id=wx.ID_ANY, 
                                     choices = self.locList, size=(300, -1),
                                     style=wx.CB_DROPDOWN | wx.CB_READONLY)
        self.sizer.Add(item=self.cb_location,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(2, 2))

        # mapset
        self.sizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=_('Select source mapset:')),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(3, 1))
        self.cb_mapset = wx.ComboBox(parent=self, id=wx.ID_ANY,
                                     choices = self.mapsetList, size=(300, -1),
                                     style=wx.CB_DROPDOWN | wx.CB_READONLY)
        self.sizer.Add(item=self.cb_mapset,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(3,2))

        #
        # bindings
        #
        self.Bind(wx.EVT_RADIOBOX, self.OnMaptype, self.rb_maptype)
        self.Bind(wx.EVT_COMBOBOX, self.OnLocation, self.cb_location)
        self.Bind(wx.EVT_COMBOBOX, self.OnMapset, self.cb_mapset)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        # self.Bind(wx.EVT_CLOSE, self.parent.Cleanup)

    def OnMaptype(self,event):
        """Change map type"""
        global maptype

        if event.GetInt() == 0:
            maptype = 'cell'
        else:
            maptype = 'vector'
        
    def OnLocation(self, event):
        """Sets source location for map(s) to georectify"""
        self.xylocation = event.GetString()
        
        #create a list of valid mapsets
        tmplist = os.listdir(os.path.join(self.grassdatabase, self.xylocation))
        self.mapsetList = []
        for item in tmplist:
            if os.path.isdir(os.path.join(self.grassdatabase, self.xylocation, item)) and \
                os.path.exists(os.path.join(self.grassdatabase, self.xylocation, item, 'WIND')):
                if item != 'PERMANENT':
                    self.mapsetList.append(item)

        self.xymapset = 'PERMANENT'
        utils.ListSortLower(self.mapsetList)
        self.mapsetList.insert(0, 'PERMANENT')
        self.cb_mapset.SetItems(self.mapsetList)
        self.cb_mapset.SetStringSelection(self.xymapset)
        
        if not wx.FindWindowById(wx.ID_FORWARD).IsEnabled():
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

    def OnMapset(self, event):
        """Sets source mapset for map(s) to georectify"""
        if self.xylocation == '':
            wx.MessageBox(_('You must select a valid location before selecting a mapset'))
            return

        self.xymapset = event.GetString()
        
        if not wx.FindWindowById(wx.ID_FORWARD).IsEnabled():
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

    def OnPageChanging(self, event=None):
        if event.GetDirection() and \
               (self.xylocation == '' or self.xymapset == ''):
            wx.MessageBox(_('You must select a valid location and mapset in order to continue'))
            event.Veto()
            return
        
        self.parent.SetSrcEnv(self.xylocation, self.xymapset)
        
    def OnEnterPage(self, event=None):
        if self.xylocation == '' or self.xymapset == '':
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

class GroupPage(TitledPage):
    """
    Set group to georectify. Create group if desired.
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard, _("Select image/map group to georectify"))

        self.parent = parent
        
        self.grassdatabase = self.parent.grassdatabase
        self.groupList = []
        
        self.xylocation = ''
        self.xymapset = ''
        self.xygroup = ''

        # default extension
        self.extension = 'georect' + str(os.getpid())

        #
        # layout
        #
        self.sizer.AddGrowableCol(2)
        # group
        self.sizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=_('Select group:')),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(1, 1))
        self.cb_group = wx.ComboBox(parent=self, id=wx.ID_ANY,
                                    choices=self.groupList, size=(350, -1),
                                    style=wx.CB_DROPDOWN | wx.CB_READONLY)
        self.sizer.Add(item=self.cb_group,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(1, 2))
        
        # create group               
        self.sizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=_('Create group if none exists')),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(2, 1))
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        self.btn_mkgroup = wx.Button(parent=self, id=wx.ID_ANY, label=_("Create/edit group..."))
        self.btn_vgroup = wx.Button(parent=self, id=wx.ID_ANY, label=_("Add vector map to group..."))
        self.btn_vgroup.Hide()
        btnSizer.Add(item=self.btn_mkgroup,
                     flag=wx.RIGHT, border=5)

        btnSizer.Add(item=self.btn_vgroup,
                     flag=wx.LEFT, border=5)
        
        self.sizer.Add(item=btnSizer,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(2, 2))
        
        # extension
        self.sizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=_('Extension for output maps:')),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(3, 1))
        self.ext_txt = wx.TextCtrl(parent=self, id=wx.ID_ANY, value="", size=(350,-1))
        self.ext_txt.SetValue(self.extension)
        self.sizer.Add(item=self.ext_txt,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(3, 2))

        #
        # bindings
        #
        self.Bind(wx.EVT_COMBOBOX, self.OnGroup, self.cb_group)
        self.Bind(wx.EVT_TEXT, self.OnExtension, self.ext_txt)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wx.EVT_CLOSE, self.parent.Cleanup)

    def OnGroup(self, event):        
        self.xygroup = event.GetString()
        
    def OnMkGroup(self, event):
        """Create new group in source location/mapset"""
        menuform.GUI().ParseCommand(['i.group'],
                                    completed=(self.GetOptData, None, ''),
                                    parentframe=self.parent.parent, modal=True)

    def OnVGroup(self, event):
        """Add vector maps to group"""
        dlg = VectGroup(parent = self,
                        id = wx.ID_ANY,
                        grassdb = self.grassdatabase,
                        location = self.xylocation,
                        mapset = self.xymapset,
                        group = self.xygroup)

        if dlg.ShowModal() != wx.ID_OK:
            return

        dlg.MakeVGroup()
        self.OnEnterPage()
        
    def GetOptData(self, dcmd, layer, params, propwin):
        """Process i.group"""
        # update the page
        if dcmd:
            gcmd.RunCommand(utils.CmdToTuple(dcmd))
        
        self.OnEnterPage()
        self.Update()
        
    def OnExtension(self, event):
        self.extension = event.GetString()

    def OnPageChanging(self, event=None):
        if event.GetDirection() and self.xygroup == '':
            wx.MessageBox(_('You must select a valid image/map group in order to continue'))
            event.Veto()
            return

        if event.GetDirection() and self.extension == '':
            wx.MessageBox(_('You must enter an map name extension in order to continue'))
            event.Veto()
            return

    def OnEnterPage(self, event=None):
        global maptype
        
        self.groupList = []

        self.xylocation = self.parent.gisrc_dict['LOCATION_NAME']
        self.xymapset = self.parent.gisrc_dict['MAPSET']

        # create a list of groups in selected mapset
        if os.path.isdir(os.path.join(self.grassdatabase,
                                      self.xylocation,
                                      self.xymapset,
                                      'group')):
            tmplist = os.listdir(os.path.join(self.grassdatabase,
                                              self.xylocation,
                                              self.xymapset,
                                              'group'))
            for item in tmplist:
                if os.path.isdir(os.path.join(self.grassdatabase,
                                              self.xylocation,
                                              self.xymapset,
                                              'group',
                                              item)):
                    self.groupList.append(item)
        
        if maptype == 'cell':
            self.btn_vgroup.Hide()
            self.Bind(wx.EVT_BUTTON, self.OnMkGroup, self.btn_mkgroup)

        elif maptype == 'vector':
            self.btn_vgroup.Show()
            self.Bind(wx.EVT_BUTTON, self.OnMkGroup, self.btn_mkgroup)
            self.Bind(wx.EVT_BUTTON, self.OnVGroup, self.btn_vgroup)
        
        utils.ListSortLower(self.groupList)
        self.cb_group.SetItems(self.groupList)
        
        if len(self.groupList) > 0 and \
                self.xygroup == '':
            self.cb_group.SetSelection(0)
            self.xygroup = self.groupList[0]
        
        if self.xygroup == '' or \
                self.extension == '':
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
        
        # switch to source
        self.parent.SwitchEnv('new')
    
class DispMapPage(TitledPage):
    """
    Select ungeoreferenced map to display for interactively
    setting ground control points (GCPs).
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard,
                            _("Select image/map to display for ground control point (GCP) creation"))

        self.parent = parent
        global maptype

        #
        # layout
        #
        self.sizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=_('Select display image/map:')),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(1, 1))
        
        self.selection = gselect.Select(self, id=wx.ID_ANY,
                                        size=globalvar.DIALOG_GSELECT_SIZE)
        
        self.sizer.Add(item=self.selection,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(1, 2))

        #
        # bindings
        #
        self.selection.Bind(wx.EVT_TEXT, self.OnSelection)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wx.EVT_CLOSE, self.parent.Cleanup)

    def OnSelection(self,event):
        """Map to display selected"""
        global xy_map

        xy_map = event.GetString()

        if xy_map == '':
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

    def OnPageChanging(self, event=None):
        global xy_map

        if event.GetDirection() and xy_map == '':
            wx.MessageBox(_('You must select a valid image/map in order to continue'))
            event.Veto()
            return

        self.parent.SwitchEnv('original')
        
    def OnEnterPage(self, event=None):
        global maptype
        global xy_map
        
        self.selection.SetElementList(maptype,
                                      mapsets = [self.parent.newmapset, ])

        if xy_map == '':
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

class GCP(wx.Frame):
    """
    Manages ground control points for georectifying. Calculates RMS statics.
    Calls i.rectify or v.transform to georectify map.
    """

    def __init__(self, parent, grwiz, mapdisp=None, id=wx.ID_ANY,
                 title=_("Create & manage ground control points"),
                 size=wx.DefaultSize):

        wx.Frame.__init__(self, parent, id, title, size=(625, 300))

        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_map.ico'), wx.BITMAP_TYPE_ICO))
        
        #
        # init variables
        #
        self.parent = parent # GMFrame
        self.parent.georectifying = self
        
        self.mapdisp = mapdisp # XY-location Map Display
        self.grwiz = grwiz # GR Wizard

        self.grassdatabase = self.grwiz.grassdatabase

        self.currentlocation = self.grwiz.currentlocation
        self.currentmapset = self.grwiz.currentmapset

        self.newlocation = self.grwiz.newlocation
        self.newmapset = self.grwiz.newmapset

        self.xylocation = self.grwiz.gisrc_dict['LOCATION_NAME']
        self.xymapset = self.grwiz.gisrc_dict['MAPSET']
        self.xygroup = self.grwiz.grouppage.xygroup
        self.extension = self.grwiz.grouppage.extension

        self.file = {
            'points' : os.path.join(self.grassdatabase,
                                    self.xylocation,
                                    self.xymapset,
                                    'group',
                                    self.xygroup,
                                    'POINTS'),
            'rgrp' : os.path.join(self.grassdatabase,
                                  self.xylocation,
                                  self.xymapset,
                                  'group',
                                  self.xygroup,
                                  'REF'),
            'vgrp' : os.path.join(self.grassdatabase,
                                  self.xylocation,
                                  self.xymapset,
                                  'group',
                                  self.xygroup,
                                  'VREF'),
            'target' : os.path.join(self.grassdatabase,
                                    self.xylocation,
                                    self.xymapset,
                                    'group',
                                    self.xygroup,
                                    'TARGET'),
            }
        
        # polynomial order transformation for georectification
        self.gr_order = 1 
        # number of GCPs selected to be used for georectification (checked)
        self.GCPcount = 0
        # forward RMS error
        self.fwd_rmserror = 0.0
        # backward RMS error
        self.bkw_rmserror = 0.0
        # list map coords and ID of map display they came from
        self.mapcoordlist = [] 

        self.SetTarget(self.xygroup, self.currentlocation, self.currentmapset)

        #
        # toolbar and display for xy map
        #
        self.toolbar = toolbars.GCPToolbar(parent=self, tbframe=self).GetToolbar()
        self.SetToolBar(self.toolbar)
        
        self.SetMapDisplay(self.mapdisp)

        #
        # statusbar 
        #
        self.CreateStatusBar(number=1)
        
        # can put guage into custom statusbar for progress if can figure out how to get progress text from i.rectify
        # self.gr_gauge = wx.Gauge(self, -1, 100, (-1,-1), (100, 25))
        # self.gr_guage.Pulse()

        panel = wx.Panel(parent=self)

        #
        # do layout
        #
        sizer = wx.BoxSizer(wx.VERTICAL)

        self.rb_grmethod = wx.RadioBox(parent=panel, id=wx.ID_ANY,
                                       label=" %s " % _("Select rectification method for rasters"),
                                       choices=[_('1st order'), _('2nd order'), _('3rd order')],
                                       majorDimension=wx.RA_SPECIFY_COLS)
        sizer.Add(item=self.rb_grmethod, proportion=0,
                       flag=wx.EXPAND | wx.ALL, border=5)
        
        self.clip_to_region = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                label=_("clip to computational region in target location"))
        sizer.Add(item=self.clip_to_region, proportion=0,
                       flag=wx.EXPAND | wx.ALL, border=5)

        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % _("Ground Control Points"))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        # initialize list control for GCP management
        self.list = GCPList(parent=panel, gcp=self)

        boxSizer.Add(item=self.list, proportion=1,
                     flag=wx.EXPAND | wx.ALL, border=3)

        sizer.Add(item=boxSizer, proportion=1,
                  flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)

        #
        # bindigs
        #
        self.Bind(wx.EVT_RADIOBOX, self.OnGRMethod, self.rb_grmethod)
        self.Bind(wx.EVT_ACTIVATE, self.OnFocus)
        self.Bind(wx.EVT_CLOSE, self.OnQuit)

        panel.SetSizer(sizer)
        # sizer.Fit(self)

    def __del__(self):
        """Disable georectification mode"""
        self.parent.georectifying = None
        
    def SetMapDisplay(self, win):
        self.mapdisp = win
        if self.mapdisp:
             self.list.LoadData()

    def SetTarget(self, tgroup, tlocation, tmapset):
        """
        Sets rectification target to current location and mapset
        """
        # check to see if we are georectifying map in current working location/mapset
        if self.newlocation == self.currentlocation and self.newmapset == self.currentmapset:
            gcmd.RunCommand('i.target',
                            parent = self,
                            flags = 'c',
                            group = tgroup)
        else:
            self.grwiz.SwitchEnv('new')
            gcmd.RunCommand('i.target',
                            parent = self,
                            group = tgroup,
                            location = tlocation,
                            mapset = tmapset)
            
        self.grwiz.SwitchEnv('original')

    def AddGCP(self, event):
        """
        Appends an item to GCP list
        """
        self.list.AddGCPItem()
        # x, y, MapWindow instance
        self.mapcoordlist.append({ 'gcpcoord' : (0.0, 0.0, None),
                                   'mapcoord' : (0.0, 0.0, None) })

    def DeleteGCP(self, event):
        """
        Deletes selected item in GCP list
        """
        minNumOfItems = self.OnGRMethod(None)

        if self.list.GetItemCount() <= minNumOfItems:
            wx.MessageBox(parent=self, message=_("At least %d GCPs required. Operation cancelled.") % minNumOfItems,
                          caption=_("Delete GCP"), style=wx.OK | wx.ICON_INFORMATION)
            return

        item = self.list.DeleteGCPItem()
        del self.mapcoordlist[item]

    def ClearGCP(self, event):
        """
        Clears all values in selected item of GCP list and unchecks it
        """
        index = self.list.GetSelected()

        for i in range(4):
            self.list.SetStringItem(index, i, '0.0')
        self.list.SetStringItem(index, 4, '')
        self.list.SetStringItem(index, 5, '')
        self.list.CheckItem(index, False)

        self.mapcoordlist[index] = { 'gcpcoord' : (0.0, 0.0, None),
                                     'mapcoord' : (0.0, 0.0, None) }

    def DrawGCP(self, coordtype):
        """
        Updates GCP and map coord maps and redraws
        active (checked) GCP markers
        """
        col = UserSettings.Get(group='georect', key='symbol', subkey='color')
        wxCol = wx.Colour(col[0], col[1], col[2], 255)
        wpx = UserSettings.Get(group='georect', key='symbol', subkey='width')
        font = self.GetFont()

        idx = 0
        for gcp in self.mapcoordlist:
            mapWin = gcp[coordtype][2]
            if not self.list.IsChecked(idx) or not mapWin:
                idx += 1
                continue

            mapWin.pen = wx.Pen(colour=wxCol, width=wpx, style=wx.SOLID)
            mapWin.polypen = wx.Pen(colour=wxCol, width=wpx, style=wx.SOLID) # ?
            coord = mapWin.Cell2Pixel((gcp[coordtype][0], gcp[coordtype][1]))
            mapWin.DrawCross(pdc=mapWin.pdcTmp, coords=coord,
                             size=5, text={ 'text' : '%s' % str(idx + 1),
                                            'active' : True,
                                            'font' : font,
                                            'color': wxCol,
                                            'coords': [coord[0] + 5,
                                                       coord[1] + 5,
                                                       5,
                                                       5]})
            
            idx += 1
            
    def SetGCPData(self, coordtype, coord, mapdisp=None, check=True):
        """
        Inserts coordinates from mouse click on map
        into selected item of GCP list and checks it for use
        """
        
        index = self.list.GetSelected()
        if index == wx.NOT_FOUND:
            return

        coord0 = str(coord[0])
        coord1 = str(coord[1])

        if coordtype == 'gcpcoord':
            self.list.SetStringItem(index, 0, coord0)
            self.list.SetStringItem(index, 1, coord1)
            self.mapcoordlist[index]['gcpcoord'] = (coord[0], coord[1], mapdisp)
        elif coordtype == 'mapcoord':
            self.list.SetStringItem(index, 2, coord0)
            self.list.SetStringItem(index, 3, coord1)
            
        self.mapcoordlist[index][coordtype] = (coord[0], coord[1], mapdisp)

        self.list.CheckItem(index, check)

        # self.list.ResizeColumns()

    def SaveGCPs(self, event):
        """
        Make a POINTS file or save GCP coordinates to existing POINTS file
        """
        
        self.GCPcount = 0
        try:
            f = open(self.file['points'], mode='w')
            # use os.linesep or '\n' here ???
            f.write('# Ground Control Points File\n')
            f.write("# \n")
            f.write("# target location: " + self.currentlocation + '\n')
            f.write("# target mapset: " + self.currentmapset + '\n')
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
                f.write(coord0 + ' ' + coord1 + '     ' + coord2 + ' ' + coord3 + '     ' + check + '\n')

            self.parent.goutput.WriteLog(_('POINTS file <%s> saved') % self.file['points'])
            self.SetStatusText(_('POINTS file saved'))
        except IOError, err:
            wx.MessageBox(parent=self,
                          message="%s <%s>. %s%s" % (_("Writing POINTS file failed"),
                                                     self.file['points'], os.linesep, err),
                          caption=_("Error"), style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            return

        f.close()

    def ReadGCPs(self):
        """
        Reads GCPs and georectified coordinates from POINTS file
        """
        
        self.GCPcount = 0

        sourceMapWin = self.mapdisp.MapWindow
        targetMapWin = self.parent.curr_page.maptree.mapdisplay.MapWindow
            
        try:
            f = open(self.file['points'], 'r')
            GCPcnt = 0
            
            for line in f.readlines():
                if line[0] == '#' or line =='':
                    continue
                line = line.replace('\n', '').strip()
                coords = map(float, line.split())
                if coords[4] == 1:
                    check = True
                    self.GCPcount +=1
                else:
                    check = False
                index = self.AddGCP(event=None)
                self.SetGCPData('gcpcoord', (coords[0], coords[1]), sourceMapWin, check)
                self.SetGCPData('mapcoord', (coords[2], coords[3]), targetMapWin, check)

        except IOError, err:
            wx.MessageBox(parent=self,
                          message="%s <%s>. %s%s" % (_("Reading POINTS file failed"),
                                                     self.file['points'], os.linesep, err),
                          caption=_("Error"), style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            return

        f.close()

        #
        # draw GCPs (source and target)
        #
        sourceMapWin.UpdateMap(render=False, renderVector=False)
        if targetMapWin:
            targetMapWin.UpdateMap(render=False, renderVector=False)

        #
        # calculate RMS
        #
        # FIXME auto calculation on load is not working
        #if self.CheckGCPcount():
        #    self.RMSError(self.xygroup, self.gr_order)

    def ReloadGCPs(self, event):
        """Reload data from file"""
        self.list.LoadData()
    
    def OnFocus(self, event):
        # self.grwiz.SwitchEnv('new')
        pass
        
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
                wx.MessageBox(parent=self,
                              caption=_("RMS Error"),
                              message=_('Insufficient points defined and active (checked) '
                                        'for selected rectification method.\n'
                                        '3+ points needed for 1st order,\n'
                                        '6+ points for 2nd order, and\n'
                                        '10+ points for 3rd order.'),
                              style=wx.ICON_INFORMATION | wx.ID_OK | wx.CENTRE)
                return False
        else:
            return True

    def OnGeorect(self, event):
        """
        Georectifies map(s) in group using i.rectify or v.transform
        """
        global maptype
        self.SaveGCPs(None)
        
        if self.CheckGCPcount(msg=True) == False:
            return

        if maptype == 'cell':
            self.grwiz.SwitchEnv('new')
            cmdlist = ['i.rectify','-a','group=%s' % self.xygroup,
                       'extension=%s' % self.extension,'order=%s' % self.gr_order]
            if self.clip_to_region:
                cmdlist.append('-c')
            
            self.parent.goutput.RunCmd(cmdlist, compReg=False,
                                       switchPage=True)
            
            time.sleep(.1)
            self.grwiz.SwitchEnv('original')

        elif maptype == 'vector':
            # loop through all vectors in VREF
            # and move resulting vector to target location
            f = open(self.file['vgrp'])
            vectlist = []
            try:
                for vect in f.readlines():
                    vect = vect.strip('\n')
                    if len(vect) < 1:
                        continue
                    vectlist.append(vect)
            finally:
                f.close()
            for vect in vectlist:
                self.grwiz.SwitchEnv('new')
                self.outname = vect + '_' + self.extension
                self.parent.goutput.WriteLog(text = _('Transforming <%s>...') % vect,
                                             switchPage = True)
                xyLayer = []
                for layer in grass.vector_db(map = vect).itervalues():
                    xyLayer.append((layer['driver'],
                                    layer['database'],
                                    layer['table']))
                self.parent.goutput.RunCmd(['v.transform',
                                            '--o',
                                            'input=%s' % vect,
                                            'output=%s' % self.outname,
                                            'pointsfile=%s' % self.file['points']],
                                           switchPage = True,
                                           onDone = self.OnGeorectDone)

                dbConnect = grass.db_connection()
                for layer in xyLayer:                
                    self.parent.goutput.RunCmd(['db.copy',
                                                '--q',
                                                '--o',
                                                'from_driver=%s' % layer[0],
                                                'from_database=%s' % layer[1],
                                                'from_table=%s' % layer[2],
                                                'to_driver=%s' % dbConnect['driver'],
                                                'to_database=%s' % dbConnect['database'],
                                                'to_table=%s' % layer[2] + '_' + self.extension])
            
    def OnGeorectDone(self, **kargs):
        """Print final message"""
        global maptype
        if maptype == 'cell':
            return
        
        returncode = kargs['returncode']
        
        xyvpath = os.path.join(self.grassdatabase,
                               self.xylocation,
                               self.xymapset,
                               'vector',
                               self.outname)
        vpath = os.path.join(self.grassdatabase,
                             self.currentlocation,
                             self.currentmapset,
                             'vector',
                             self.outname)
        
        if os.path.isdir(vpath):
            self.parent.goutput.WriteWarning(_('Vector map <%s> already exists. '
                                               'Change extension name and '
                                               'georectify again.') % self.outname)
        else:
            if returncode == 0:
                if not os.path.isdir(os.path.join(self.grassdatabase,
                                                  self.currentlocation,
                                                  self.currentmapset,
                                                  'vector')):
                    os.mkdir(os.path.join(self.grassdatabase,
                                          self.currentlocation,
                                          self.currentmapset,
                                          'vector'))
                shutil.move(xyvpath, vpath)        
                
                self.parent.goutput.WriteCmdLog(_('Vector map <%s> georectified '
                                                  'successfully') % self.outname)
                # copy attributes
                self.parent.goutput.WriteLog(_('Copying attributes...'))
                
            else:
                self.parent.goutput.WriteError(_('Georectification of vector map <%s> failed') %
                                               self.outname)
                
        del self.outname
        self.grwiz.SwitchEnv('original')
        
    def OnSettings(self, event):
        """Georectifier settings"""
        dlg = GrSettingsDialog(parent=self, id=wx.ID_ANY, title=_('Georectifier settings'))
        
        if dlg.ShowModal() == wx.ID_OK:
            pass
        
        dlg.Destroy()

    def OnQuit(self, event):
        """Quit georectifier"""
        self.grwiz.Cleanup()

        self.Destroy()

        event.Skip()

    def OnGRMethod(self, event):
        """
        sets transformation order for georectifying
        """
        if event:
            self.gr_order = event.GetInt() + 1

        numOfItems = self.list.GetItemCount()
        minNumOfItems = numOfItems
        
        if self.gr_order == 1:
            minNumOfItems = 3
            # self.SetStatusText(_('Insufficient points, 3+ points needed for 1st order'))

        elif self.gr_order == 2:
            minNumOfItems = 6
            diff = 6 - numOfItems
            # self.SetStatusText(_('Insufficient points, 6+ points needed for 2nd order'))

        elif self.gr_order == 3:
            minNumOfItems = 10
            # self.SetStatusText(_('Insufficient points, 10+ points needed for 3rd order'))

        for i in range(minNumOfItems - numOfItems):
            self.AddGCP(None)

        return minNumOfItems
    
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
        
        # get list of forward and reverse rms error values for each point
        self.grwiz.SwitchEnv('new')
        
        ret = gcmd.RunCommand('g.transform',
                              parent = self,
                              read = True,
                              group = xygroup,
                              order = order)
        
        self.grwiz.SwitchEnv('original')

        if ret:
            errlist = ret.splitlines()
        
        if errlist == []:
            return
        
        # insert error values into GCP list for checked items
        i = 0
        sumsq_fwd_err = 0.0
        sumsq_bkw_err = 0.0
        
        for index in range(self.list.GetItemCount()):
            if self.list.IsChecked(index):
                fwd_err, bkw_err = errlist[i].split()
                self.list.SetStringItem(index, 4, fwd_err)
                self.list.SetStringItem(index, 5, bkw_err)
                sumsq_fwd_err += float(fwd_err)**2
                sumsq_bkw_err += float(bkw_err)**2
                i += 1
            else:
                self.list.SetStringItem(index, 4, '')
                self.list.SetStringItem(index, 5, '')
        
        # calculate RMS error
        self.fwd_rmserror = round((sumsq_fwd_err/i)**0.5,4)
        self.bkw_rmserror = round((sumsq_bkw_err/i)**0.5,4)
        self.list.ResizeColumns()

        self.SetStatusText(_('RMS error for selected points forward: %(fwd)s backward: %(bkw)s') % \
                           { 'fwd' : self.fwd_rmserror, 'bkw' : self.bkw_rmserror })
        
class GCPList(wx.ListCtrl,
              CheckListCtrlMixin,
              ListCtrlAutoWidthMixin):
              
    def __init__(self, parent, gcp, id=wx.ID_ANY,
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.LC_REPORT | wx.SUNKEN_BORDER | wx.LC_HRULES |
                 wx.LC_SINGLE_SEL):

        wx.ListCtrl.__init__(self, parent, id, pos, size, style)

        # Mixin settings
        CheckListCtrlMixin.__init__(self)
        ListCtrlAutoWidthMixin.__init__(self)
        # TextEditMixin.__init__(self)

        self.gcp = gcp # GCP class

        # tracks whether list items are checked or not
        self.CheckList = [] 

        self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.OnItemActivated)

        self._Create()

        self.selected = wx.NOT_FOUND

    def _Create(self):
        idx_col = 0
        for col in (_('use| X coord'),
                    _('Y coord'),
                    _('E coord'),
                    _('N coord'),
                    _('Forward error'),
                    _('Backward error')):
            self.InsertColumn(idx_col, col)
            idx_col += 1
        
    def LoadData(self):
        """Load data into list"""
        self.DeleteAllItems()

        if os.path.isfile(self.gcp.file['points']):
            self.gcp.ReadGCPs()
        else:
            # 3 gcp is minimum
            for i in range(3):
                self.gcp.AddGCP(None)

        # select first point by default
        self.selected = 0
        self.SetItemState(self.selected,
                          wx.LIST_STATE_SELECTED,
                          wx.LIST_STATE_SELECTED)

        self.ResizeColumns()

    def OnCheckItem(self, index, flag):
        """Item is checked/unchecked"""
        pass
    
    def AddGCPItem(self):
        """
        Appends an item to GCP list
        """
        self.Append(['0.0',
                     '0.0',
                     '0.0',
                     '0.0',
                     '',
                     ''])

        self.selected = self.GetItemCount() - 1

        self.SetItemState(self.selected,
                          wx.LIST_STATE_SELECTED,
                          wx.LIST_STATE_SELECTED)

        self.ResizeColumns()

        return self.selected

    def DeleteGCPItem(self):
        """
        Deletes selected item in GCP list
        """
        if self.selected == wx.NOT_FOUND:
            return

        self.DeleteItem(self.selected)

        if self.GetItemCount() > 0:
            self.selected = self.GetItemCount() - 1
            self.SetItemState(self.selected,
                              wx.LIST_STATE_SELECTED,
                              wx.LIST_STATE_SELECTED)
        else:
            self.selected = wx.NOT_FOUND

        return self.selected
        
    def ResizeColumns(self):
        """Resize columns"""
        minWidth = 90
        for i in range(self.GetColumnCount()):
            self.SetColumnWidth(i, wx.LIST_AUTOSIZE)
            if self.GetColumnWidth(i) < minWidth:
                self.SetColumnWidth(i, minWidth)

        self.SendSizeEvent()

    def GetSelected(self):
        """Get index of selected item"""
        return self.selected

    def OnItemSelected(self, event):
        self.selected = event.GetIndex()
        
    def OnItemActivated(self, event):
        """
        When item double clicked, open editor to update coordinate values
        """
        coords = []
        index = event.GetIndex()

        for i in range(4):
            coords.append(self.GetItem(index, i).GetText())
        
        dlg = EditGPC(parent=self, id=wx.ID_ANY, data=coords)
        if dlg.ShowModal() == wx.ID_OK:
            values = dlg.GetValues() # string
            
            if len(values) == 0:
                wx.MessageBox(parent=self,
                              caption=_("Edit GCP"),
                              message=_("Invalid coordinate value. Operation cancelled."),
                              style=wx.CENTRE | wx.ICON_ERROR | wx.ID_OK)
            else:
                for i in range(len(values)):
                    if values[i] != coords[i]:
                        self.SetStringItem(index, i, values[i])
                mapdisp = self.gcp.mapcoordlist[index]['gcpcoord'][2]
                self.gcp.mapcoordlist[index]['gcpcoord'] = (float(values[0]), float(values[1]), mapdisp)
                mapdisp = self.gcp.mapcoordlist[index]['mapcoord'][2]
                self.gcp.mapcoordlist[index]['mapcoord'] = (float(values[0]), float(values[1]), mapdisp)
        
class VectGroup(wx.Dialog):
    """
    Dialog to create a vector group (VREF file) for georectifying

    @todo Replace by g.group
    """
    def __init__(self, parent, id, grassdb, location, mapset, group,
                 style=wx.DEFAULT_DIALOG_STYLE):
        
        wx.Dialog.__init__(self, parent, id, style=style,
                           title = _("Create vector map group"))
        
        self.grassdatabase = grassdb
        self.xylocation = location
        self.xymapset = mapset
        self.xygroup = group
        
        #
        # get list of valid vector directories
        #
        vectlist = os.listdir(os.path.join(self.grassdatabase,
                                           self.xylocation,
                                           self.xymapset,
                                           'vector'))
        for dir in vectlist:
            if not os.path.isfile(os.path.join(self.grassdatabase,
                                           self.xylocation,
                                           self.xymapset,
                                           'vector',
                                           dir,
                                           'coor')):
                vectlist.remove(dir)
        
        utils.ListSortLower(vectlist)
        
        # path to vref file
        self.vgrpfile = os.path.join(self.grassdatabase,
                                     self.xylocation,
                                     self.xymapset,
                                     'group',
                                     self.xygroup,
                                     'VREF')
        
        #
        # buttons
        #
        self.btnCancel = wx.Button(parent = self,
                                   id = wx.ID_CANCEL)
        self.btnOK = wx.Button(parent = self,
                                   id = wx.ID_OK)
        self.btnOK.SetDefault()


        #
        # list of vector maps
        #
        self.listMap = wx.CheckListBox(parent = self, id = wx.ID_ANY,
                                      choices = vectlist)
        
        if os.path.isfile(self.vgrpfile):
            f = open(self.vgrpfile)
            try:
                checked = []
                for line in f.readlines():
                    line = line.replace('\n', '')
                    if len(line) < 1:
                        continue
                    checked.append(line)
                self.listMap.SetCheckedStrings(checked)
            finally:
                f.close()
                
        line = wx.StaticLine(parent = self,
                             id = wx.ID_ANY, size = (20, -1),
                             style = wx.LI_HORIZONTAL)

        #
        # layout
        #
        sizer = wx.BoxSizer(wx.VERTICAL)
        
        box = wx.BoxSizer(wx.HORIZONTAL)
        box.Add(item = wx.StaticText(parent = self, id = wx.ID_ANY,
                                     label = _('Select vector map(s) to add to group:')),
                flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT,
                border = 5)

        box.Add(item = self.listMap,
                flag = wx.ALIGN_CENTER_VERTICAL | wx.ALIGN_RIGHT | wx.LEFT,
                border = 5)

        
        sizer.Add(box, flag = wx.ALIGN_RIGHT | wx.ALL,
                  border = 3)
        
        sizer.Add(item = line, proportion = 0,
                  flag = wx.GROW | wx.ALIGN_CENTER_VERTICAL | wx.LEFT | wx.RIGHT,
                  border = 5)
        
        # buttons
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOK)
        btnSizer.Realize()

        sizer.Add(item = btnSizer, proportion = 0,
                  flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER,
                  border = 5)
        
        self.SetSizer(sizer)
        sizer.Fit(self)
        self.Layout()
        
    def MakeVGroup(self):
        """Create VREF file"""
        vgrouplist = []
        for item in range(self.listMap.GetCount()):
            if not self.listMap.IsChecked(item):
                continue
            vgrouplist.append(self.listMap.GetString(item))
        
        f = open(self.vgrpfile, mode='w')
        try:
            for vect in vgrouplist:
                f.write(vect + '\n')
        finally:
            f.close()
        
class EditGPC(wx.Dialog):
    def __init__(self, parent, data, id=wx.ID_ANY,
                 title=_("Edit GCP"),
                 style=wx.DEFAULT_DIALOG_STYLE):
        """Dialog for editing GPC and map coordinates in list control"""

        wx.Dialog.__init__(self, parent, id, title=title, style=style)

        panel = wx.Panel(parent=self)

        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s " % _("Ground Control Point"))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        # source coordinates
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
       
        self.xcoord = wx.TextCtrl(parent=panel, id=wx.ID_ANY, size=(150, -1))
        self.ycoord = wx.TextCtrl(parent=panel, id=wx.ID_ANY, size=(150, -1))
        self.ncoord = wx.TextCtrl(parent=panel, id=wx.ID_ANY, size=(150, -1))
        self.ecoord = wx.TextCtrl(parent=panel, id=wx.ID_ANY, size=(150, -1))

        row = 0
        col = 0
        idx = 0
        for label, win in ((_("X:"), self.xcoord),
                           (_("Y:"), self.ycoord),
                           (_("E:"), self.ecoord),
                           (_("N:"), self.ncoord)):
            label = wx.StaticText(parent=panel, id=wx.ID_ANY,
                                  label=label)
            gridSizer.Add(item=label,
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          pos=(row, col))

            col += 1
            win.SetValue(str(data[idx]))

            gridSizer.Add(item=win,
                          pos=(row, col))

            col += 1
            idx += 1

            if col > 3:
                row += 1
                col = 0

        boxSizer.Add(item=gridSizer, proportion=1,
                  flag=wx.EXPAND | wx.ALL, border=5)

        sizer.Add(item=boxSizer, proportion=1,
                  flag=wx.EXPAND | wx.ALL, border=5)

        #
        # buttons
        #
        self.btnCancel = wx.Button(panel, wx.ID_CANCEL)
        self.btnOk = wx.Button(panel, wx.ID_OK)
        self.btnOk.SetDefault()

        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(self.btnCancel)
        btnSizer.AddButton(self.btnOk)
        btnSizer.Realize()

        sizer.Add(item=btnSizer, proportion=0,
                  flag=wx.ALIGN_RIGHT | wx.ALL, border=5)

        panel.SetSizer(sizer)
        sizer.Fit(self)

    def GetValues(self, columns=None):
        """Return list of values (as strings).
        """
        valuelist = []
        try:
            float(self.xcoord.GetValue())
            float(self.ycoord.GetValue())
            float(self.ecoord.GetValue())
            float(self.ncoord.GetValue())
        except ValueError:
            return valuelist

        valuelist.append(self.xcoord.GetValue())
        valuelist.append(self.ycoord.GetValue())
        valuelist.append(self.ecoord.GetValue())
        valuelist.append(self.ncoord.GetValue())

        return valuelist

class GrSettingsDialog(wx.Dialog):
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
        self.parent = parent

        self.symbol = {}

        self._do_layout()
        
    def _do_layout(self):
        """Do layout"""
        # dialog layout
        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.StaticBox(parent=self, id=wx.ID_ANY,
                           label=" %s " % _("Symbol settings"))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
        gridSizer.AddGrowableCol(1)

        #
        # symbol color
        #
        row = 0
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Color:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        col = UserSettings.Get(group='georect', key='symbol', subkey='color')
        colWin = csel.ColourSelect(parent=self, id=wx.ID_ANY,
                                   colour=wx.Colour(col[0],
                                                    col[1],
                                                    col[2],
                                                    255))
        self.symbol['color'] = colWin.GetId()
        gridSizer.Add(item=colWin,
                      flag=wx.ALIGN_RIGHT,
                      pos=(row, 1))

        #
        # symbol width
        #
        row += 1
        label = wx.StaticText(parent=self, id=wx.ID_ANY, label=_("Width:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        width = int(UserSettings.Get(group='georect', key='symbol', subkey='width'))
        widWin = wx.SpinCtrl(parent=self, id=wx.ID_ANY,
                             min=1, max=10)
        widWin.SetValue(width)
        self.symbol['width'] = widWin.GetId()
        gridSizer.Add(item=widWin,
                      flag=wx.ALIGN_RIGHT,
                      pos=(row, 1))
        
        boxSizer.Add(item=gridSizer, flag=wx.EXPAND)
        sizer.Add(item=boxSizer, flag=wx.EXPAND | wx.ALL, border=5)

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
        UserSettings.Set(group='georect', key='symbol', subkey='color',
                         value=wx.FindWindowById(self.symbol['color']).GetColour())
        UserSettings.Set(group='georect', key='symbol', subkey='width',
                         value=wx.FindWindowById(self.symbol['width']).GetValue())

    def OnSave(self, event):
        """Button 'Save' pressed"""
        self.UpdateSettings()
        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        fileSettings['georect'] = UserSettings.Get(group='georect')
        file = UserSettings.SaveToFile(fileSettings)
        self.parent.parent.goutput.WriteLog(_('Georectifier settings saved to file \'%s\'.') % file)
        self.Close()

    def OnApply(self, event):
        """Button 'Apply' pressed"""
        self.UpdateSettings()
        self.Close()

    def OnCancel(self, event):
        """Button 'Cancel' pressed"""
        self.Close()
