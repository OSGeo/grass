"""!
@package gcp.manager

@brief Georectification module for GRASS GIS. Includes ground control
point management and interactive point and click GCP creation

Classes:
 - manager::GCPWizard
 - manager::LocationPage
 - manager::GroupPage
 - manager::DispMapPage
 - manager::GCP
 - manager::GCPList
 - manager::VectGroup
 - manager::EditGCP
 - manager::GrSettingsDialog

(C) 2006-2012 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Original author Michael Barton
@author Original version improved by Martin Landa <landa.martin gmail.com>
@author Rewritten by Markus Metz redesign georectfier -> GCP Manage
@author Support for GraphicsSet added by Stepan Turek <stepan.turek seznam.cz> (2012)
"""

import os
import sys
import shutil
import time
from copy import copy

import wx
from wx.lib.mixins.listctrl import CheckListCtrlMixin, ColumnSorterMixin, ListCtrlAutoWidthMixin
import wx.lib.colourselect as csel
import wx.wizard           as wiz

import grass.script as grass

from core              import globalvar
from core              import utils
from core.render       import Map
from core.utils import _
from gui_core.gselect  import Select, LocationSelect, MapsetSelect
from gui_core.dialogs  import GroupDialog
from core.gcmd         import RunCommand, GMessage, GError, GWarning
from core.settings     import UserSettings
from gcp.mapdisplay    import MapFrame

from location_wizard.wizard   import TitledPage as TitledPage

#
# global variables
#
global src_map
global tgt_map
global maptype

src_map = ''
tgt_map = { 'raster' : '',
            'vector' : '' }
maptype = 'raster'

def getSmallUpArrowImage():
    stream = open(os.path.join(globalvar.ETCIMGDIR, 'small_up_arrow.png'), 'rb')
    try:
        img = wx.ImageFromStream(stream)
    finally:
        stream.close()
    return img

def getSmallDnArrowImage():
    stream = open(os.path.join(globalvar.ETCIMGDIR, 'small_down_arrow.png'), 'rb')
    try:
        img = wx.ImageFromStream(stream)
    finally:
        stream.close()
    stream.close()
    return img

class GCPWizard(object):
    """
    Start wizard here and finish wizard here
    """

    def __init__(self, parent, giface):
        self.parent = parent # GMFrame
        self._giface = giface

        #
        # get environmental variables
        #
        self.grassdatabase = grass.gisenv()['GISDBASE']
        
        #
        # read original environment settings
        #
        self.target_gisrc = os.environ['GISRC']
        self.gisrc_dict = {}
        try:
            f = open(self.target_gisrc, 'r')
            for line in f.readlines():
                line = line.replace('\n', '').strip()
                if len(line) < 1:
                    continue
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

        global maptype
        global src_map
        global tgt_map

        #src_map = ''
        #tgt_map = ''
        maptype = 'raster'

        # GISRC file for source location/mapset of map(s) to georectify
        self.source_gisrc = ''
        self.src_maps = []

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
                GMessage(parent = self.parent,
                         message = _("Georectifying setup canceled."))
                self.Cleanup()
        else:
            GMessage(parent = self.parent,
                     message = _("Georectifying setup canceled."))
            self.Cleanup()

        #
        # start GCP display
        #
        if success != False:
            # instance of render.Map to be associated with display
            self.SwitchEnv('source')
            self.SrcMap = Map(gisrc=self.source_gisrc) 
            self.SwitchEnv('target')
            self.TgtMap = Map(gisrc=self.target_gisrc)
            self.Map = self.SrcMap
            
            #
            # add layer to source map
            #
            if maptype == 'raster':
                rendertype = 'raster'
                cmdlist = ['d.rast', 'map=%s' % src_map]
            else: # -> vector layer
                rendertype = 'vector'
                cmdlist = ['d.vect', 'map=%s' % src_map]
            
            self.SwitchEnv('source')
            name, found = utils.GetLayerNameFromCmd(cmdlist)
            self.SrcMap.AddLayer(ltype=rendertype, command=cmdlist, active=True,
                                 name=name, hidden=False, opacity=1.0, render=False)

            self.SwitchEnv('target')
            if tgt_map['raster']:
                #
                # add raster layer to target map
                #
                rendertype = 'raster'
                cmdlist = ['d.rast', 'map=%s' % tgt_map['raster']]
                
                name, found = utils.GetLayerNameFromCmd(cmdlist)
                self.TgtMap.AddLayer(ltype=rendertype, command=cmdlist, active=True,
                                     name=name, hidden=False, opacity=1.0, render=False)
            
            if tgt_map['vector']:
                #
                # add raster layer to target map
                #
                rendertype = 'vector'
                cmdlist = ['d.vect', 'map=%s' % tgt_map['vector']]
                
                name, found = utils.GetLayerNameFromCmd(cmdlist)
                self.TgtMap.AddLayer(ltype=rendertype, command=cmdlist, active=True,
                                     name=name, hidden=False, opacity=1.0, render=False)
            
            #
            # start GCP Manager
            #
            self.gcpmgr = GCP(self.parent, giface = self._giface, 
                              grwiz=self, size=globalvar.MAP_WINDOW_SIZE,
                              toolbars=["gcpdisp"],
                              Map=self.SrcMap, lmgr=self.parent)

            # load GCPs
            self.gcpmgr.InitMapDisplay()
            self.gcpmgr.CenterOnScreen()
            self.gcpmgr.Show()
            # need to update AUI here for wingrass
            self.gcpmgr._mgr.Update()
        else:
            self.Cleanup()
                            
    def SetSrcEnv(self, location, mapset):
        """!Create environment to use for location and mapset
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
        
        self.source_gisrc = utils.GetTempfile()

        try:
            f = open(self.source_gisrc, mode='w')        
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

        if grc == 'target':
            os.environ['GISRC'] = str(self.target_gisrc)
        elif grc == 'source':
            os.environ['GISRC'] = str(self.source_gisrc)

        return True
    
    def OnWizFinished(self):
        # self.Cleanup()

        return True
        
    def OnGLMFocus(self, event):
        """!Layer Manager focus"""
        # self.SwitchEnv('target')
        
        event.Skip()

    def Cleanup(self):
        """!Return to current location and mapset"""
        self.SwitchEnv('target')
        self.parent.gcpmanagement = None

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
        
        #
        # layout
        #
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
        self.cb_location = LocationSelect(parent = self, gisdbase = self.grassdatabase)
        self.sizer.Add(item=self.cb_location,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(2, 2))

        # mapset
        self.sizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=_('Select source mapset:')),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(3, 1))
        self.cb_mapset = MapsetSelect(parent = self, gisdbase = self.grassdatabase,
                                              setItems = False)
        self.sizer.Add(item=self.cb_mapset,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(3,2))
        self.sizer.AddGrowableCol(2)

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
        """!Change map type"""
        global maptype

        if event.GetInt() == 0:
            maptype = 'raster'
        else:
            maptype = 'vector'
        
    def OnLocation(self, event):
        """!Sets source location for map(s) to georectify"""
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
        """!Sets source mapset for map(s) to georectify"""
        if self.xylocation == '':
            GMessage(_('You must select a valid location '
                       'before selecting a mapset'),
                     parent = self)
            return

        self.xymapset = event.GetString()
        
        if not wx.FindWindowById(wx.ID_FORWARD).IsEnabled():
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

    def OnPageChanging(self, event=None):
        if event.GetDirection() and \
               (self.xylocation == '' or self.xymapset == ''):
            GMessage(_('You must select a valid location '
                            'and mapset in order to continue'),
                          parent = self)
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
        self.extension = '_georect' + str(os.getpid())

        #
        # layout
        #
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

        self.sizer.AddGrowableCol(2)
        #
        # bindings
        #
        self.Bind(wx.EVT_COMBOBOX, self.OnGroup, self.cb_group)
        self.Bind(wx.EVT_TEXT, self.OnExtension, self.ext_txt)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wx.EVT_CLOSE, self.parent.Cleanup)

        # hide vector group button by default
        self.btn_vgroup.Hide()

    def OnGroup(self, event):        
        self.xygroup = event.GetString()
        
    def OnMkGroup(self, event):
        """!Create new group in source location/mapset"""
        dlg = GroupDialog(parent = self, defaultGroup = self.xygroup)

        dlg.ShowModal()
        gr = dlg.GetSelectedGroup()
        if gr in dlg.GetExistGroups():
            self.xygroup = gr
        else:
            gr = ''
        dlg.Destroy()
        
        self.OnEnterPage()
        self.Update()
        
    def OnVGroup(self, event):
        """!Add vector maps to group"""
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
        
    def OnExtension(self, event):
        self.extension = self.ext_txt.GetValue()

    def OnPageChanging(self, event=None):
        if event.GetDirection() and self.xygroup == '':
            GMessage(_('You must select a valid image/map '
                       'group in order to continue'),
                     parent = self)
            event.Veto()
            return

        if event.GetDirection() and self.extension == '':
            GMessage(_('You must enter an map name '
                       'extension in order to continue'),
                     parent = self)
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
        
        if maptype == 'raster':
            self.btn_vgroup.Hide()
            self.Bind(wx.EVT_BUTTON, self.OnMkGroup, self.btn_mkgroup)

        elif maptype == 'vector':
            self.btn_vgroup.Show()
            self.Bind(wx.EVT_BUTTON, self.OnMkGroup, self.btn_mkgroup)
            self.Bind(wx.EVT_BUTTON, self.OnVGroup, self.btn_vgroup)
        
        utils.ListSortLower(self.groupList)
        self.cb_group.SetItems(self.groupList)
        
        if len(self.groupList) > 0:
            if self.xygroup and self.xygroup in self.groupList:
                self.cb_group.SetStringSelection(self.xygroup)
            else:
                self.cb_group.SetSelection(0)
                self.xygroup = self.groupList[0]
        
        if self.xygroup == '' or \
                self.extension == '':
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)
        
        # switch to source
        self.parent.SwitchEnv('source')
    
class DispMapPage(TitledPage):
    """
    Select ungeoreferenced map to display for interactively
    setting ground control points (GCPs).
    """
    def __init__(self, wizard, parent):
        TitledPage.__init__(self, wizard,
                            _("Select maps to display for ground control point (GCP) creation"))

        self.parent = parent
        global maptype

        #
        # layout
        #
        self.sizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=_('Select source map to display:')),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(1, 1))
        
        self.srcselection = Select(self, id=wx.ID_ANY,
                                   size=globalvar.DIALOG_GSELECT_SIZE, type=maptype, updateOnPopup = False)
        
        self.sizer.Add(item=self.srcselection,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(1, 2))

        self.sizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=_('Select target raster map to display:')),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(2, 1))

        self.tgtrastselection = Select(self, id=wx.ID_ANY,
                                   size=globalvar.DIALOG_GSELECT_SIZE, type='raster', updateOnPopup = False)

        self.sizer.Add(item=self.tgtrastselection,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(2, 2))

        self.sizer.Add(item=wx.StaticText(parent=self, id=wx.ID_ANY, label=_('Select target vector map to display:')),
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(3, 1))

        self.tgtvectselection = Select(self, id=wx.ID_ANY,
                                   size=globalvar.DIALOG_GSELECT_SIZE, type='vector', updateOnPopup = False)
        
        self.sizer.Add(item=self.tgtvectselection,
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5,
                       pos=(3, 2))

        #
        # bindings
        #
        self.srcselection.Bind(wx.EVT_TEXT, self.OnSrcSelection)
        self.tgtrastselection.Bind(wx.EVT_TEXT, self.OnTgtRastSelection)
        self.tgtvectselection.Bind(wx.EVT_TEXT, self.OnTgtVectSelection)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(wiz.EVT_WIZARD_PAGE_CHANGED, self.OnEnterPage)
        self.Bind(wx.EVT_CLOSE, self.parent.Cleanup)

    def OnSrcSelection(self, event):
        """!Source map to display selected"""
        global src_map
        global maptype

        src_map = self.srcselection.GetValue()

        if src_map == '':
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

        try:
        # set computational region to match selected map and zoom display to region
            if maptype == 'raster':
                p = RunCommand('g.region', 'rast=src_map')
            elif maptype == 'vector':
                p = RunCommand('g.region', 'vect=src_map')
            
            if p.returncode == 0:
                print 'returncode = ', str(p.returncode)
                self.parent.Map.region = self.parent.Map.GetRegion()
        except:
            pass

    def OnTgtRastSelection(self, event):
        """!Source map to display selected"""
        global tgt_map

        tgt_map['raster'] = self.tgtrastselection.GetValue()

    def OnTgtVectSelection(self,event):
        """!Source map to display selected"""
        global tgt_map

        tgt_map['vector'] = self.tgtvectselection.GetValue()

    def OnPageChanging(self, event=None):
        global src_map
        global tgt_map

        if event.GetDirection() and (src_map == ''):
            GMessage(_('You must select a source map '
                       'in order to continue'),
                     parent = self)
            event.Veto()
            return

        self.parent.SwitchEnv('target')
        
    def OnEnterPage(self, event=None):
        global maptype
        global src_map
        global tgt_map

        self.srcselection.SetElementList(maptype)

        if maptype == 'raster':
            ret = RunCommand('i.group',
                             parent = self,
                             read = True,
                             group = self.parent.grouppage.xygroup,
                             flags = 'g')            

            if ret:
                self.parent.src_maps = ret.splitlines()
            else:
                GError(parent = self,
                       message = _('No maps in selected group <%s>.\n'
                                   'Please edit group or select another group.') %
                       self.parent.grouppage.xygroup)
                return

        elif maptype == 'vector':
            grassdatabase = self.parent.grassdatabase
            xylocation = self.parent.gisrc_dict['LOCATION_NAME']
            xymapset = self.parent.gisrc_dict['MAPSET']
            # make list of vectors to georectify from VREF

            vgrpfile = os.path.join(grassdatabase,
                                     xylocation,
                                     xymapset,
                                     'group',
                                     self.parent.grouppage.xygroup,
                                     'VREF')

            f = open(vgrpfile)
            try:
                for vect in f.readlines():
                    vect = vect.strip('\n')
                    if len(vect) < 1:
                        continue
                    self.parent.src_maps.append(vect)
            finally:
                f.close()
                
            if len(self.parent.src_maps) < 1:
                GError(parent = self,
                       message = _('No maps in selected group <%s>.\n'
                                   'Please edit group or select another group.') %
                       self.parent.grouppage.xygroup)
                return

        # filter out all maps not in group
        self.srcselection.tcp.GetElementList(elements = self.parent.src_maps)
        src_map = self.parent.src_maps[0]
        self.srcselection.SetValue(src_map)

        self.parent.SwitchEnv('target')
        self.tgtrastselection.SetElementList('raster')
        self.tgtrastselection.GetElementList()
        self.tgtvectselection.SetElementList('vector')
        self.tgtvectselection.GetElementList()
        self.parent.SwitchEnv('source')

        if src_map == '':
            wx.FindWindowById(wx.ID_FORWARD).Enable(False)
        else:
            wx.FindWindowById(wx.ID_FORWARD).Enable(True)

class GCP(MapFrame, ColumnSorterMixin):
    """!
    Manages ground control points for georectifying. Calculates RMS statistics.
    Calls i.rectify or v.rectify to georectify map.
    """
    def __init__(self, parent, giface, grwiz = None, id = wx.ID_ANY,
                 title = _("Manage Ground Control Points"),
                 size = (700, 300), toolbars = ["gcpdisp"], Map = None, lmgr = None):

        self.grwiz = grwiz # GR Wizard
        self._giface = giface

        if tgt_map['raster'] == '' and tgt_map['vector'] == '':
            self.show_target = False
        else:
            self.show_target = True
        
        #wx.Frame.__init__(self, parent, id, title, size = size, name = "GCPFrame")
        MapFrame.__init__(self, parent = parent, giface = self._giface, title = title, size = size,
                            Map = Map, toolbars = toolbars, lmgr = lmgr, name = 'GCPMapWindow')

        #
        # init variables
        #
        self.parent = parent # GMFrame
        self.parent.gcpmanagement = self
        
        #
        # register data structures for drawing GCP's
        #
        self.pointsToDrawTgt = self.TgtMapWindow.RegisterGraphicsToDraw(graphicsType = "point", setStatusFunc = self.SetGCPSatus)
        self.pointsToDrawSrc = self.SrcMapWindow.RegisterGraphicsToDraw(graphicsType = "point", setStatusFunc = self.SetGCPSatus)
        
        # window resized
        self.resize = False

        self.grassdatabase = self.grwiz.grassdatabase

        self.currentlocation = self.grwiz.currentlocation
        self.currentmapset = self.grwiz.currentmapset

        self.newlocation = self.grwiz.newlocation
        self.newmapset = self.grwiz.newmapset

        self.xylocation = self.grwiz.gisrc_dict['LOCATION_NAME']
        self.xymapset = self.grwiz.gisrc_dict['MAPSET']
        self.xygroup = self.grwiz.grouppage.xygroup
        self.src_maps = self.grwiz.src_maps
        self.extension = self.grwiz.grouppage.extension
        self.outname = ''
        self.VectGRList = []

        self.file = {
            'points' : os.path.join(self.grassdatabase,
                                    self.xylocation,
                                    self.xymapset,
                                    'group',
                                    self.xygroup,
                                    'POINTS'),
            'points_bak' : os.path.join(self.grassdatabase,
                                    self.xylocation,
                                    self.xymapset,
                                    'group',
                                    self.xygroup,
                                    'POINTS_BAK'),
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

        # make a backup of the current points file
        if os.path.exists(self.file['points']):
            shutil.copy(self.file['points'], self.file['points_bak'])

        # polynomial order transformation for georectification
        self.gr_order = 1 
        # interpolation method for georectification
        self.gr_method = 'nearest'
        # region clipping for georectified map
        self.clip_to_region = False
        # number of GCPs selected to be used for georectification (checked)
        self.GCPcount = 0
        # forward RMS error
        self.fwd_rmserror = 0.0
        # backward RMS error
        self.bkw_rmserror = 0.0
        # list map coords and ID of map display they came from
        self.mapcoordlist = []
        self.mapcoordlist.append([ 0,        # GCP number
                                   0.0,      # source east
                                   0.0,      # source north
                                   0.0,      # target east
                                   0.0,      # target north
                                   0.0,      # forward error
                                   0.0 ] )   # backward error

        # init vars to highlight high RMS errors
        self.highest_only = True
        self.show_unused =  True
        self.highest_key = -1
        self.rmsthresh = 0
        self.rmsmean = 0
        self.rmssd = 0

        self.SetTarget(self.xygroup, self.currentlocation, self.currentmapset)

        self.itemDataMap = None

        # images for column sorting
        # CheckListCtrlMixin must set an ImageList first
        self.il = self.list.GetImageList(wx.IMAGE_LIST_SMALL)

        SmallUpArrow = wx.BitmapFromImage(getSmallUpArrowImage())            
        SmallDnArrow = wx.BitmapFromImage(getSmallDnArrowImage())            
        self.sm_dn = self.il.Add(SmallDnArrow)
        self.sm_up = self.il.Add(SmallUpArrow)

        # set mouse characteristics
        self.mapwin = self.SrcMapWindow
        self.mapwin.mouse['box'] = 'point'
        self.mapwin.mouse["use"] == "pointer"
        self.mapwin.zoomtype = 0
        self.mapwin.pen = wx.Pen(colour='black', width=2, style=wx.SOLID)
        self.mapwin.SetCursor(self.cursors["cross"])

        self.mapwin = self.TgtMapWindow
        
        # set mouse characteristics
        self.mapwin.mouse['box'] = 'point'
        self.mapwin.mouse["use"] == "pointer"
        self.mapwin.zoomtype = 0
        self.mapwin.pen = wx.Pen(colour='black', width=2, style=wx.SOLID)
        self.mapwin.SetCursor(self.cursors["cross"])

        #
        # show new display & draw map
        #
        if self.show_target:
            self.MapWindow = self.TgtMapWindow
            self.Map = self.TgtMap
            self.OnZoomToMap(None)

        self.MapWindow = self.SrcMapWindow
        self.Map = self.SrcMap
        self.OnZoomToMap(None)

        #
        # bindings
        #
        self.Bind(wx.EVT_ACTIVATE, self.OnFocus)
        self.Bind(wx.EVT_SIZE,     self.OnSize)
        self.Bind(wx.EVT_IDLE,     self.OnIdle)
        self.Bind(wx.EVT_CLOSE,    self.OnQuit)
        
        self.SetSettings()

    def __del__(self):
        """!Disable GCP manager mode"""
        self.parent.gcpmanagement = None
        
    def CreateGCPList(self):
        """!Create GCP List Control"""

        return GCPList(parent=self, gcp=self)

    # Used by the ColumnSorterMixin, see wx/lib/mixins/listctrl.py
    def GetListCtrl(self):
        return self.list
        
    def GetMapCoordList(self):
        return self.mapcoordlist

    # Used by the ColumnSorterMixin, see wx/lib/mixins/listctrl.py
    def GetSortImages(self):
        return (self.sm_dn, self.sm_up)

    def GetFwdError(self):
        return self.fwd_rmserror
        
    def GetBkwError(self):
        return self.bkw_rmserror
                
    def InitMapDisplay(self):
        self.list.LoadData()
        
        # initialize column sorter
        self.itemDataMap = self.mapcoordlist
        ncols = self.list.GetColumnCount()
        ColumnSorterMixin.__init__(self, ncols)
        # init to ascending sort on first click
        self._colSortFlag = [1] * ncols

    def SetTarget(self, tgroup, tlocation, tmapset):
        """
        Sets rectification target to current location and mapset
        """
        # check to see if we are georectifying map in current working location/mapset
        if self.newlocation == self.currentlocation and self.newmapset == self.currentmapset:
            RunCommand('i.target',
                       parent = self,
                       flags = 'c',
                       group = tgroup)
        else:
            self.grwiz.SwitchEnv('source')
            RunCommand('i.target',
                       parent = self,
                       group = tgroup,
                       location = tlocation,
                       mapset = tmapset)
            self.grwiz.SwitchEnv('target')

    def AddGCP(self, event):
        """
        Appends an item to GCP list
        """
        keyval = self.list.AddGCPItem() + 1
        # source east, source north, target east, target north, forward error, backward error
        self.mapcoordlist.append([ keyval,             # GCP number
                                   0.0,                # source east
                                   0.0,                # source north
                                   0.0,                # target east
                                   0.0,                # target north
                                   0.0,                # forward error
                                   0.0 ] )             # backward error

        if self.statusbarManager.GetMode() == 8: # go to
            self.StatusbarUpdate()

    def DeleteGCP(self, event):
        """
        Deletes selected item in GCP list
        """
        minNumOfItems = self.OnGROrder(None)

        if self.list.GetItemCount() <= minNumOfItems:
            GMessage(parent = self,
                     message=_("At least %d GCPs required. Operation canceled.") % minNumOfItems)
            return

        key = self.list.DeleteGCPItem()
        del self.mapcoordlist[key]

        # update key and GCP number
        for newkey in range(key, len(self.mapcoordlist)):
            index = self.list.FindItemData(-1, newkey + 1)
            self.mapcoordlist[newkey][0] = newkey
            self.list.SetStringItem(index, 0, str(newkey))
            self.list.SetItemData(index, newkey)

        # update selected
        if self.list.GetItemCount() > 0:
            if self.list.selected < self.list.GetItemCount():
                self.list.selectedkey = self.list.GetItemData(self.list.selected)
            else:
                self.list.selected = self.list.GetItemCount() - 1
                self.list.selectedkey = self.list.GetItemData(self.list.selected)
                
            self.list.SetItemState(self.list.selected,
                              wx.LIST_STATE_SELECTED,
                              wx.LIST_STATE_SELECTED)
        else:
            self.list.selected = wx.NOT_FOUND
            self.list.selectedkey = -1

        self.UpdateColours()

        if self.statusbarManager.GetMode() == 8: # go to
            self.StatusbarUpdate()
            if self.list.selectedkey > 0:
                self.statusbarManager.SetProperty('gotoGCP', self.list.selectedkey)

    def ClearGCP(self, event):
        """
        Clears all values in selected item of GCP list and unchecks it
        """
        index = self.list.GetSelected()
        key = self.list.GetItemData(index)

        for i in range(1, 5):
            self.list.SetStringItem(index, i, '0.0')
        self.list.SetStringItem(index, 5, '')
        self.list.SetStringItem(index, 6, '')
        self.list.CheckItem(index, False)

        # GCP number, source E, source N, target E, target N, fwd error, bkwd error
        self.mapcoordlist[key] = [key, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0]

    def SetSettings(self):
        """!Sets settings for drawing of GCP's.
        """
        self.highest_only = UserSettings.Get(group='gcpman', key='rms', subkey='highestonly')
        self.show_unused =  UserSettings.Get(group='gcpman', key='symbol', subkey='unused')
        
        colours = { "color"  : "default", 
                    "hcolor" : "highest", 
                    "scolor" : "selected",
                    "ucolor" : "unused" }
        wpx = UserSettings.Get(group = 'gcpman', key = 'symbol', subkey = 'width')
        
        for k, v in colours.iteritems():
            col = UserSettings.Get(group='gcpman', key='symbol', subkey= k)
            self.pointsToDrawSrc.GetPen(v).SetColour(wx.Colour(col[0], col[1], col[2], 255)) # TODO GetPen neni to spatne? 
            self.pointsToDrawTgt.GetPen(v).SetColour(wx.Colour(col[0], col[1], col[2], 255))
            
            self.pointsToDrawSrc.GetPen(v).SetWidth(wpx)
            self.pointsToDrawTgt.GetPen(v).SetWidth(wpx)
        
        spx = UserSettings.Get(group = 'gcpman', key = 'symbol', subkey = 'size')
        self.pointsToDrawSrc.SetPropertyVal("size", int(spx))
        self.pointsToDrawTgt.SetPropertyVal("size", int(spx))
        
        font = self.GetFont()
        font.SetPointSize(int(spx) + 2)
        
        textProp = {}
        textProp['active'] = True
        textProp['font'] = font
        self.pointsToDrawSrc.SetPropertyVal("text", textProp)
        self.pointsToDrawTgt.SetPropertyVal("text", copy(textProp))
        
    def SetGCPSatus(self, item, itemIndex):
        """!Before GCP is drawn, decides it's colour and whether it
        will be drawed.
        """
        key = self.list.GetItemData(itemIndex)
        itemIndex += 1 # incremented because of itemDataMap (has one more item) - will be changed
        
        if not self.list.IsChecked(key - 1):
                wxPen = "unused"
                if not self.show_unused:
                    item.SetPropertyVal('hide', True)
                else:
                    item.SetPropertyVal('hide', False)
        
        else:
            item.SetPropertyVal('hide', False)
            if self.highest_only == True:
                if itemIndex == self.highest_key:
                    wxPen = "highest"
                else:
                    wxPen = "default"
            else:
                if (gcp[5] > self.rmsthresh):
                    wxPen = "highest"
                else:
                    wxPen = "default"
        
        if itemIndex == self.list.selectedkey:
            wxPen = "selected"
        
        item.SetPropertyVal('label', str(itemIndex))
        item.SetPropertyVal('penName', wxPen)

    def SetGCPData(self, coordtype, coord, mapdisp=None, confirm=False):
        """!Inserts coordinates from file, mouse click on map, or
        after editing into selected item of GCP list and checks it for
        use.
        """
        index = self.list.GetSelected()
        if index == wx.NOT_FOUND:
            return

        coord0 = coord[0]
        coord1 = coord[1]

        key = self.list.GetItemData(index)
        if confirm:
            if self.MapWindow == self.SrcMapWindow:
                currloc = _("source")
            else:
                currloc = _("target")
            ret = wx.MessageBox(parent = self,
                                caption = _("Set GCP coordinates"),
                                message = _('Set %(coor)s coordinates for GCP No. %(key)s? \n\n'
                                            'East: %(coor0)s \n'
                                            'North: %(coor1)s') % \
                                    { 'coor' : currloc,
                                      'key' : str(key),
                                      'coor0' : str(coord0),
                                      'coor1' : str(coord1) },
                                style = wx.ICON_QUESTION | wx.YES_NO | wx.CENTRE)
            
            # for wingrass
            if os.name == 'nt':
                self.MapWindow.SetFocus()
            if ret == wx.NO:
                return
        
        if coordtype == 'source':
            self.list.SetStringItem(index, 1, str(coord0))
            self.list.SetStringItem(index, 2, str(coord1))
            self.mapcoordlist[key][1] = coord[0]
            self.mapcoordlist[key][2] = coord[1]
            self.pointsToDrawSrc.GetItem(key - 1).SetCoords([coord0, coord1])
            
        elif coordtype == 'target':
            self.list.SetStringItem(index, 3, str(coord0))
            self.list.SetStringItem(index, 4, str(coord1))
            self.mapcoordlist[key][3] = coord[0]
            self.mapcoordlist[key][4] = coord[1]
            self.pointsToDrawTgt.GetItem(key - 1).SetCoords([coord0, coord1])
        
        self.list.SetStringItem(index, 5, '0')
        self.list.SetStringItem(index, 6, '0')
        self.mapcoordlist[key][5] = 0.0
        self.mapcoordlist[key][6] = 0.0
        
        # self.list.ResizeColumns()
        
    def SaveGCPs(self, event):
        """!Make a POINTS file or save GCP coordinates to existing
        POINTS file
        """
        self.GCPcount = 0
        try:
            f = open(self.file['points'], mode='w')
            # use os.linesep or '\n' here ???
            f.write('# Ground Control Points File\n')
            f.write("# \n")
            f.write("# target location: " + self.currentlocation + '\n')
            f.write("# target mapset: " + self.currentmapset + '\n')
            f.write("#\tsource\t\ttarget\t\tstatus\n")
            f.write("#\teast\tnorth\teast\tnorth\t(1=ok, 0=ignore)\n")
            f.write("#-----------------------     -----------------------     ---------------\n")

            for index in range(self.list.GetItemCount()):
                if self.list.IsChecked(index) == True:
                    check = "1"
                    self.GCPcount += 1
                else:
                    check = "0"
                coord0 = self.list.GetItem(index, 1).GetText()
                coord1 = self.list.GetItem(index, 2).GetText()
                coord2 = self.list.GetItem(index, 3).GetText()
                coord3 = self.list.GetItem(index, 4).GetText()
                f.write(coord0 + ' ' + coord1 + '     ' + coord2 + ' ' + coord3 + '     ' + check + '\n')

        except IOError, err:
            GError(parent = self,
                   message="%s <%s>. %s%s" % (_("Writing POINTS file failed"),
                                              self.file['points'], os.linesep, err))
            return

        f.close()

        # if event != None save also to backup file
        if event:
            shutil.copy(self.file['points'], self.file['points_bak'])
            self._giface.WriteLog(_('POINTS file saved for group <%s>') % self.xygroup)
            #self.SetStatusText(_('POINTS file saved'))

    def ReadGCPs(self):
        """
        Reads GCPs and georectified coordinates from POINTS file
        """
        
        self.GCPcount = 0

        sourceMapWin = self.SrcMapWindow
        targetMapWin = self.TgtMapWindow
        
        if not sourceMapWin:
            GError(parent = self,
                   message = "%s. %s%s" % (_("source mapwin not defined"),
                                           os.linesep, err))
        
        if not targetMapWin:
            GError(parent = self,
                   message="%s. %s%s" % (_("target mapwin not defined"),
                                         os.linesep, err))
        
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

                self.AddGCP(event=None)
                self.SetGCPData('source', (coords[0], coords[1]), sourceMapWin)
                self.SetGCPData('target', (coords[2], coords[3]), targetMapWin)
                index = self.list.GetSelected()
                if index != wx.NOT_FOUND:
                    self.list.CheckItem(index, check)
                GCPcnt += 1

        except IOError, err:
            GError(parent = self,
                   message = "%s <%s>. %s%s" % (_("Reading POINTS file failed"),
                                                self.file['points'], os.linesep, err))
            return

        f.close()

        if GCPcnt == 0:
            # 3 gcp is minimum
            for i in range(3):
                self.AddGCP(None)

        if self.CheckGCPcount():
            # calculate RMS
            self.RMSError(self.xygroup, self.gr_order)

    def ReloadGCPs(self, event):
        """!Reload data from file"""

        # use backup
        shutil.copy(self.file['points_bak'], self.file['points'])

        # delete all items in mapcoordlist
        self.mapcoordlist = []
        self.mapcoordlist.append([ 0,        # GCP number
                                   0.0,      # source east
                                   0.0,      # source north
                                   0.0,      # target east
                                   0.0,      # target north
                                   0.0,      # forward error
                                   0.0 ] )   # backward error

        self.list.LoadData()
        self.itemDataMap = self.mapcoordlist

        if self._col != -1:
            self.list.ClearColumnImage(self._col)
        self._colSortFlag = [1] * self.list.GetColumnCount()

        # draw GCPs (source and target)
        sourceMapWin = self.SrcMapWindow
        sourceMapWin.UpdateMap(render=False, renderVector=False)
        if self.show_target:
            targetMapWin = self.TgtMapWindow
            targetMapWin.UpdateMap(render=False, renderVector=False)
    
    def OnFocus(self, event):
        # self.grwiz.SwitchEnv('source')
        pass
        
    def OnRMS(self, event):
        """
        RMS button handler
        """
        self.RMSError(self.xygroup,self.gr_order)

        sourceMapWin = self.SrcMapWindow
        sourceMapWin.UpdateMap(render=False, renderVector=False)
        if self.show_target:
            targetMapWin = self.TgtMapWindow
            targetMapWin.UpdateMap(render=False, renderVector=False)
        
    def CheckGCPcount(self, msg=False):
        """
        Checks to make sure that the minimum number of GCPs have been defined and
        are active for the selected transformation order
        """
        if (self.GCPcount < 3 and self.gr_order == 1) or \
            (self.GCPcount < 6 and self.gr_order == 2) or \
            (self.GCPcount < 10 and self.gr_order == 3):
            if msg:
                GWarning(parent = self,
                         message=_('Insufficient points defined and active (checked) '
                                   'for selected rectification method.\n'
                                   '3+ points needed for 1st order,\n'
                                   '6+ points for 2nd order, and\n'
                                   '10+ points for 3rd order.'))
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

        if maptype == 'raster':
            self.grwiz.SwitchEnv('source')

            if self.clip_to_region:
                flags = "ac"
            else:
                flags = "a"

            busy = wx.BusyInfo(message=_("Rectifying images, please wait..."),
                               parent=self)
            wx.Yield()

            ret, msg = RunCommand('i.rectify',
                                  parent = self,
                                  getErrorMsg = True,
                                  quiet = True,
                                  group = self.xygroup,
                                  extension = self.extension,
                                  order = self.gr_order,
                                  method=self.gr_method,
                                  flags = flags)

            busy.Destroy()

            # provide feedback on failure
            if ret != 0:
                print >> sys.stderr, msg
                
        elif maptype == 'vector':
            # loop through all vectors in VREF

            self.grwiz.SwitchEnv('source')
            
            # make list of vectors to georectify from VREF
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
                               
            # georectify each vector in VREF using v.rectify
            for vect in vectlist:
                self.outname = str(vect.split('@')[0]) + self.extension
                self._giface.WriteLog(text = _('Transforming <%s>...') % vect,
                                             switchPage = True)
                ret = msg = ''
                
                busy = wx.BusyInfo(message=_("Rectifying vector map <%s>, please wait...") % vect,
                                   parent=self)
                wx.Yield()

                ret, msg = RunCommand('v.rectify',
                                      parent = self,
                                      getErrorMsg = True,
                                      quiet = True,
                                      input = vect,
                                      output = self.outname,
                                      group = self.xygroup,
                                      order = self.gr_order)

                busy.Destroy()

                # provide feedback on failure
                if ret != 0:
                    print >> sys.stderr, msg
                                                   
        self.grwiz.SwitchEnv('target')

    def OnGeorectDone(self, **kargs):
        """!Print final message"""
        global maptype
        if maptype == 'raster':
            return
        
        returncode = kargs['returncode']
        
        if returncode == 0:
            self.VectGRList.append(self.outname)
            print '*****vector list = ' + str(self.VectGRList)
        else:
            self._giface.WriteError(_('Georectification of vector map <%s> failed') %
                                      self.outname)

         
    def OnSettings(self, event):
        """!GCP Manager settings"""
        dlg = GrSettingsDialog(parent=self, giface=self._giface,
                               id=wx.ID_ANY, title=_('GCP Manager settings'))

        if dlg.ShowModal() == wx.ID_OK:
            pass
        
        dlg.Destroy()

    def UpdateColours(self, srcrender=False, srcrenderVector=False,
                            tgtrender=False, tgtrenderVector=False):
        """!update colours"""
        highest_fwd_err = 0.0
        self.highest_key = 0
        highest_idx = 0

        for index in range(self.list.GetItemCount()):
            if self.list.IsChecked(index):
                key = self.list.GetItemData(index)
                fwd_err = self.mapcoordlist[key][5]

                if self.highest_only == True:
                    self.list.SetItemTextColour(index, wx.BLACK)
                    if highest_fwd_err < fwd_err:
                        highest_fwd_err = fwd_err
                        self.highest_key = key
                        highest_idx = index
                elif self.rmsthresh > 0:
                    if (fwd_err > self.rmsthresh):
                        self.list.SetItemTextColour(index, wx.RED)
                    else:
                        self.list.SetItemTextColour(index, wx.BLACK)
            else:
                self.list.SetItemTextColour(index, wx.BLACK)
        
        if self.highest_only and highest_fwd_err > 0.0:
            self.list.SetItemTextColour(highest_idx, wx.RED)

        sourceMapWin = self.SrcMapWindow
        sourceMapWin.UpdateMap(render=srcrender, renderVector=srcrenderVector)
        if self.show_target:
            targetMapWin = self.TgtMapWindow
            targetMapWin.UpdateMap(render=tgtrender, renderVector=tgtrenderVector)

    def OnQuit(self, event):
        """!Quit georectifier"""
        ret = wx.MessageBox(parent=self,
                      caption=_("Quit GCP Manager"),
                      message=_('Save ground control points?'),
                      style=wx.ICON_QUESTION | wx.YES_NO | wx.CANCEL | wx.CENTRE)

        if ret != wx.CANCEL:
            if ret == wx.YES:
                self.SaveGCPs(None)
            elif ret == wx.NO:
                # restore POINTS file from backup
                if os.path.exists(self.file['points_bak']):
                    shutil.copy(self.file['points_bak'], self.file['points'])

            if os.path.exists(self.file['points_bak']):
                os.unlink(self.file['points_bak'])

            self.SrcMap.Clean()
            self.TgtMap.Clean()

            self.grwiz.Cleanup()

            self.Destroy()

        #event.Skip()

    def OnGROrder(self, event):
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
        Uses m.transform to calculate forward and backward error for each used GCP
        in POINTS file and insert error values into GCP list.
        Calculates total forward and backward RMS error for all used points
        """
        # save GCPs to points file to make sure that all checked GCPs are used
        self.SaveGCPs(None)
        #self.SetStatusText('')
        
        if self.CheckGCPcount(msg=True) == False:
            return
        
        # get list of forward and reverse rms error values for each point
        self.grwiz.SwitchEnv('source')
        
        ret = RunCommand('m.transform',
                         parent = self,
                         read = True,
                         group = xygroup,
                         order = order)
        
        self.grwiz.SwitchEnv('target')

        if ret:
            errlist = ret.splitlines()
        else:
            GError(parent = self,
                   message=_('Could not calculate RMS Error.\n'
                             'Possible error with m.transform.'))
            return
        
        # insert error values into GCP list for checked items
        sdfactor = float(UserSettings.Get(group='gcpman', key='rms', subkey='sdfactor'))
        GCPcount = 0
        sumsq_fwd_err = 0.0
        sumsq_bkw_err = 0.0
        sum_fwd_err = 0.0
        highest_fwd_err = 0.0
        self.highest_key = 0
        highest_idx = 0
        
        for index in range(self.list.GetItemCount()):
            key = self.list.GetItemData(index)
            if self.list.IsChecked(index):
                fwd_err, bkw_err = errlist[GCPcount].split()
                self.list.SetStringItem(index, 5, fwd_err)
                self.list.SetStringItem(index, 6, bkw_err)
                self.mapcoordlist[key][5] = float(fwd_err)
                self.mapcoordlist[key][6] = float(bkw_err)
                self.list.SetItemTextColour(index, wx.BLACK)
                if self.highest_only:
                    if highest_fwd_err < float(fwd_err):
                        highest_fwd_err = float(fwd_err)
                        self.highest_key = key
                        highest_idx = index
                        
                sumsq_fwd_err += float(fwd_err)**2
                sumsq_bkw_err += float(bkw_err)**2
                sum_fwd_err += float(fwd_err)
                GCPcount += 1
            else:
                self.list.SetStringItem(index, 5, '')
                self.list.SetStringItem(index, 6, '')
                self.mapcoordlist[key][5] = 0.0
                self.mapcoordlist[key][6] = 0.0
                self.list.SetItemTextColour(index, wx.BLACK)

        # SD
        if GCPcount > 0:
            sum_fwd_err /= GCPcount
            self.rmsmean = sum_fwd_err /GCPcount
            self.rmssd = (((sumsq_fwd_err/GCPcount) - self.rmsmean**2)**0.5)
            self.rmsthresh = self.rmsmean + sdfactor * self.rmssd
        else:
            self.rmsthresh = 0
            self.rmsmean = 0
            self.rmssd = 0

        if self.highest_only and highest_fwd_err > 0.0:
            self.list.SetItemTextColour(highest_idx, wx.RED)
        elif GCPcount > 0 and self.rmsthresh > 0 and not self.highest_only:
            for index in range(self.list.GetItemCount()):
                if self.list.IsChecked(index):
                    key = self.list.GetItemData(index)
                    if (self.mapcoordlist[key][5] > self.rmsthresh):
                        self.list.SetItemTextColour(index, wx.RED)
            
        # calculate global RMS error (geometric mean)
        self.fwd_rmserror = round((sumsq_fwd_err/GCPcount)**0.5,4)
        self.bkw_rmserror = round((sumsq_bkw_err/GCPcount)**0.5,4)
        self.list.ResizeColumns()

    def GetNewExtent(self, region, map = None):

        coord_file = utils.GetTempfile()
        newreg = { 'n' : 0.0, 's' : 0.0, 'e' : 0.0, 'w' : 0.0,}

        try:
            f = open(coord_file, mode='w')
            # NW corner        
            f.write(str(region['e']) + " " + str(region['n']) + "\n")
            # NE corner        
            f.write(str(region['e']) + " " + str(region['s']) + "\n")
            # SW corner        
            f.write(str(region['w']) + " " + str(region['n']) + "\n")
            # SE corner        
            f.write(str(region['w']) + " " + str(region['s']) + "\n")
        finally:
            f.close()

        # save GCPs to points file to make sure that all checked GCPs are used
        self.SaveGCPs(None)

        order = self.gr_order
        self.gr_order = 1

        if self.CheckGCPcount(msg=True) == False:
            self.gr_order = order
            return
        
        self.gr_order = order

        # get list of forward and reverse rms error values for each point
        self.grwiz.SwitchEnv('source')
        
        if map == 'source':
            ret = RunCommand('m.transform',
                             parent = self,
                             read = True,
                             group = self.xygroup,
                             order = 1,
                             format = 'dst',
                             coords = coord_file)

        elif map == 'target':
            ret = RunCommand('m.transform',
                             parent = self,
                             read = True,
                             group = self.xygroup,
                             order = 1,
                             flags = 'r',
                             format = 'src',
                             coords = coord_file)

        os.unlink(coord_file)
        
        self.grwiz.SwitchEnv('target')

        if ret:
            errlist = ret.splitlines()
        else:
            GError(parent = self,
                   message=_('Could not calculate new extends.\n'
                             'Possible error with m.transform.'))
            return

        # fist corner
        e, n = errlist[0].split()
        fe = float(e)
        fn = float(n)
        newreg['n'] = fn
        newreg['s'] = fn
        newreg['e'] = fe
        newreg['w'] = fe
        # other three corners
        for i in range(1, 4):
            e, n = errlist[i].split()
            fe = float(e)
            fn = float(n)
            if fe < newreg['w']:
                newreg['w'] = fe
            if fe > newreg['e']:
                newreg['e'] = fe
            if fn < newreg['s']:
                newreg['s'] = fn
            if fn > newreg['n']:
                newreg['n'] = fn

        return newreg

    def OnHelp(self, event):
        """!Show GCP Manager manual page"""
        self._giface.Help(entry = 'wxGUI.GCP_Manager')

    def OnUpdateActive(self, event):

        if self.activemap.GetSelection() == 0:
            self.MapWindow = self.SrcMapWindow
            self.Map = self.SrcMap
        else:
            self.MapWindow = self.TgtMapWindow
            self.Map = self.TgtMap

        self.UpdateActive(self.MapWindow)
        # for wingrass
        if os.name == 'nt':
            self.MapWindow.SetFocus()

    def UpdateActive(self, win):

        # optionally disable tool zoomback tool
        self.GetMapToolbar().Enable('zoomback', enable = (len(self.MapWindow.zoomhistory) > 1))

        if self.activemap.GetSelection() != (win == self.TgtMapWindow):
            self.activemap.SetSelection(win == self.TgtMapWindow)
        self.StatusbarUpdate()

    def AdjustMap(self, newreg):
        """!Adjust map window to new extents
        """

        # adjust map window
        self.Map.region['n'] = newreg['n']
        self.Map.region['s'] = newreg['s']
        self.Map.region['e'] = newreg['e']
        self.Map.region['w'] = newreg['w']

        self.MapWindow.ZoomHistory(self.Map.region['n'], self.Map.region['s'],
                 self.Map.region['e'], self.Map.region['w'])

        # LL locations
        if self.Map.projinfo['proj'] == 'll':
            if newreg['n'] > 90.0:
                newreg['n'] = 90.0
            if newreg['s'] < -90.0:
                newreg['s'] = -90.0
        
        ce = newreg['w'] + (newreg['e'] - newreg['w']) / 2
        cn = newreg['s'] + (newreg['n'] - newreg['s']) / 2
        
        # calculate new center point and display resolution
        self.Map.region['center_easting'] = ce
        self.Map.region['center_northing'] = cn
        self.Map.region["ewres"] = (newreg['e'] - newreg['w']) / self.Map.width
        self.Map.region["nsres"] = (newreg['n'] - newreg['s']) / self.Map.height
        self.Map.AlignExtentFromDisplay()

        self.MapWindow.ZoomHistory(self.Map.region['n'], self.Map.region['s'],
                 self.Map.region['e'], self.Map.region['w'])

        if self.MapWindow.redrawAll is False:
            self.MapWindow.redrawAll = True

        self.MapWindow.UpdateMap()
        self.StatusbarUpdate()

    def OnZoomToSource(self, event):
        """!Set target map window to match extents of source map window
        """

        if not self.MapWindow == self.TgtMapWindow:
            self.MapWindow = self.TgtMapWindow
            self.Map = self.TgtMap
            self.UpdateActive(self.TgtMapWindow)

        # get new N, S, E, W for target
        newreg = self.GetNewExtent(self.SrcMap.region, 'source')
        if newreg:
            self.AdjustMap(newreg)

    def OnZoomToTarget(self, event):
        """!Set source map window to match extents of target map window
        """

        if not self.MapWindow == self.SrcMapWindow:
            self.MapWindow = self.SrcMapWindow
            self.Map = self.SrcMap
            self.UpdateActive(self.SrcMapWindow)

        # get new N, S, E, W for target
        newreg = self.GetNewExtent(self.TgtMap.region, 'target')
        if newreg:
            self.AdjustMap(newreg)

    def OnZoomMenuGCP(self, event):
        """!Popup Zoom menu
        """
        point = wx.GetMousePosition()
        zoommenu = wx.Menu()
        # Add items to the menu

        zoomsource = wx.MenuItem(zoommenu, wx.ID_ANY, _('Adjust source display to target display'))
        zoommenu.AppendItem(zoomsource)
        self.Bind(wx.EVT_MENU, self.OnZoomToTarget, zoomsource)

        zoomtarget = wx.MenuItem(zoommenu, wx.ID_ANY, _('Adjust target display to source display'))
        zoommenu.AppendItem(zoomtarget)
        self.Bind(wx.EVT_MENU, self.OnZoomToSource, zoomtarget)

        # Popup the menu. If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(zoommenu)
        zoommenu.Destroy()
        
    def OnSize(self, event):
        """!Adjust Map Windows after GCP Map Display has been resized
        """
        # re-render image on idle
        self.resize = time.clock()

    def OnIdle(self, event):
        """!GCP Map Display resized, adjust Map Windows
        """
        if self.GetMapToolbar():
            if self.resize and self.resize + 0.2 < time.clock():
                srcwidth, srcheight = self.SrcMapWindow.GetSize()
                tgtwidth, tgtheight = self.TgtMapWindow.GetSize()
                srcwidth = (srcwidth + tgtwidth) / 2
                if self.show_target:
                    self._mgr.GetPane("target").Hide()
                    self._mgr.Update()
                self._mgr.GetPane("source").BestSize((srcwidth, srcheight))
                self._mgr.GetPane("target").BestSize((srcwidth, tgtheight))
                if self.show_target:
                    self._mgr.GetPane("target").Show()
                self._mgr.Update()
                self.resize = False
            elif self.resize:
                event.RequestMore()
        pass

class GCPList(wx.ListCtrl,
              CheckListCtrlMixin,
              ListCtrlAutoWidthMixin):
              
    def __init__(self, parent, gcp, id=wx.ID_ANY,
                 pos=wx.DefaultPosition, size=wx.DefaultSize,
                 style=wx.LC_REPORT | wx.SUNKEN_BORDER | wx.LC_HRULES |
                 wx.LC_SINGLE_SEL):

        wx.ListCtrl.__init__(self, parent, id, pos, size, style)

        self.gcp = gcp # GCP class
        self.render = True

        # Mixin settings
        CheckListCtrlMixin.__init__(self)
        ListCtrlAutoWidthMixin.__init__(self)
        # TextEditMixin.__init__(self)

        # tracks whether list items are checked or not
        self.CheckList = [] 

        self._Create()

        self.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnItemSelected)
        self.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.OnItemActivated)
        self.Bind(wx.EVT_LIST_COL_CLICK, self.OnColClick)

        self.selected = wx.NOT_FOUND
        self.selectedkey = -1

    def _Create(self):

        if 0:
            # normal, simple columns
            idx_col = 0
            for col in (_('use'),
                _('source E'),
                _('source N'),
                _('target E'),
                _('target N'),
                _('Forward error'),
                _('Backward error')):
                self.InsertColumn(idx_col, col)
                idx_col += 1
        else:
            # the hard way: we want images on the column header
            info = wx.ListItem()
            info.SetMask(wx.LIST_MASK_TEXT | wx.LIST_MASK_IMAGE | wx.LIST_MASK_FORMAT)
            info.SetImage(-1)
            info.m_format = wx.LIST_FORMAT_LEFT

            idx_col = 0
            for lbl in (_('use'),
                _('source E'),
                _('source N'),
                _('target E'),
                _('target N'),
                _('Forward error'),
                _('Backward error')):
                info.SetText(lbl)
                self.InsertColumnInfo(idx_col, info)
                idx_col += 1

    def LoadData(self):
        """!Load data into list"""
        self.DeleteAllItems()

        self.render = False
        if os.path.isfile(self.gcp.file['points']):
            self.gcp.ReadGCPs()
        else:
            # 3 gcp is minimum
            for i in range(3):
                self.gcp.AddGCP(None)

        # select first point by default
        self.selected = 0
        self.selectedkey = self.GetItemData(self.selected)
        self.SetItemState(self.selected,
                          wx.LIST_STATE_SELECTED,
                          wx.LIST_STATE_SELECTED)

        self.ResizeColumns()
        self.render = True

        self.EnsureVisible(self.selected)

    def OnCheckItem(self, index, flag):
        """!Item is checked/unchecked"""

        if self.render:
            # redraw points
            sourceMapWin = self.gcp.SrcMapWindow
            sourceMapWin.UpdateMap(render=False, renderVector=False)
            if self.gcp.show_target:
                targetMapWin = self.gcp.TgtMapWindow
                targetMapWin.UpdateMap(render=False, renderVector=False)
    
    def AddGCPItem(self):
        """
        Appends an item to GCP list
        """
        self.selectedkey = self.GetItemCount() + 1

        self.Append([str(self.selectedkey),    # GCP number
                     '0.0',                # source E
                     '0.0',                # source N
                     '0.0',                # target E
                     '0.0',                # target N
                     '',                   # forward error
                     ''])                  # backward error

        self.selected = self.GetItemCount() - 1
        self.SetItemData(self.selected, self.selectedkey)

        self.SetItemState(self.selected,
                          wx.LIST_STATE_SELECTED,
                          wx.LIST_STATE_SELECTED)

        self.ResizeColumns()
        
        self.gcp.pointsToDrawSrc.AddItem(coords = [0,0], label = str(self.selectedkey))
        self.gcp.pointsToDrawTgt.AddItem(coords = [0,0], label = str(self.selectedkey))
        
        self.EnsureVisible(self.selected)

        return self.selected

    def DeleteGCPItem(self):
        """!Deletes selected item in GCP list.
        """
        if self.selected == wx.NOT_FOUND:
            return

        key = self.GetItemData(self.selected)
        self.DeleteItem(self.selected)
        
        if self.selected != wx.NOT_FOUND:
            item = self.gcp.pointsToDrawSrc.GetItem(key - 1)
            self.gcp.pointsToDrawSrc.DeleteItem(item)
            
            item = self.gcp.pointsToDrawTgt.GetItem(key - 1)
            self.gcp.pointsToDrawTgt.DeleteItem(item)
        
        return key
        
    def ResizeColumns(self):
        """!Resize columns"""
        minWidth = [90, 120]
        for i in range(self.GetColumnCount()):
            self.SetColumnWidth(i, wx.LIST_AUTOSIZE)
            # first column is checkbox, don't set to minWidth
            if i > 0 and self.GetColumnWidth(i) < minWidth[i > 4]:
                self.SetColumnWidth(i, minWidth[i > 4])
        
        self.SendSizeEvent()

    def GetSelected(self):
        """!Get index of selected item"""
        return self.selected

    def OnItemSelected(self, event):
        """!Item selected
        """
        if self.render and self.selected != event.GetIndex():
            self.selected = event.GetIndex()
            self.selectedkey = self.GetItemData(self.selected)
            sourceMapWin = self.gcp.SrcMapWindow
            sourceMapWin.UpdateMap(render=False, renderVector=False)
            if self.gcp.show_target:
                targetMapWin = self.gcp.TgtMapWindow
                targetMapWin.UpdateMap(render=False, renderVector=False)
        event.Skip()

    def OnItemActivated(self, event):
        """
        When item double clicked, open editor to update coordinate values
        """
        coords = []
        index = event.GetIndex()
        key = self.GetItemData(index)
        changed = False

        for i in range(1, 5):
            coords.append(self.GetItem(index, i).GetText())

        dlg = EditGCP(parent=self, id=wx.ID_ANY, data=coords, gcpno=key)

        if dlg.ShowModal() == wx.ID_OK:
            values = dlg.GetValues() # string
            
            if len(values) == 0:
                GError(parent = self,
                       message=_("Invalid coordinate value. Operation canceled."))
            else:
                for i in range(len(values)):
                    if values[i] != coords[i]:
                        self.SetStringItem(index, i + 1, values[i])
                        changed = True

                if changed:
                    # reset RMS and update mapcoordlist
                    self.SetStringItem(index, 5, '')
                    self.SetStringItem(index, 6, '')
                    key = self.GetItemData(index)
                    self.gcp.mapcoordlist[key] = [key,
                                                  float(values[0]),
                                                  float(values[1]),
                                                  float(values[2]),
                                                  float(values[3]),
                                                  0.0,
                                                  0.0]
                    
                    self.gcp.pointsToDrawSrc.GetItem(key - 1).SetCoords([float(values[0]), 
                                                                         float(values[1])])
                    self.gcp.pointsToDrawTgt.GetItem(key - 1).SetCoords([float(values[2]), 
                                                                         float(values[3])])
                    self.gcp.UpdateColours()
                    
    def OnColClick(self, event):
        """!ListCtrl forgets selected item..."""
        self.selected = self.FindItemData(-1, self.selectedkey)
        self.SetItemState(self.selected,
                          wx.LIST_STATE_SELECTED,
                          wx.LIST_STATE_SELECTED)
        
        event.Skip()

class VectGroup(wx.Dialog):
    """!Dialog to create a vector group (VREF file) for georectifying

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
        """!Create VREF file"""
        vgrouplist = []
        for item in range(self.listMap.GetCount()):
            if not self.listMap.IsChecked(item):
                continue
            vgrouplist.append(self.listMap.GetString(item) + '@' + self.xymapset)
        
        f = open(self.vgrpfile, mode='w')
        try:
            for vect in vgrouplist:
                f.write(vect + '\n')
        finally:
            f.close()
        
class EditGCP(wx.Dialog):
    def __init__(self, parent, data, gcpno, id=wx.ID_ANY,
                 title=_("Edit GCP"),
                 style=wx.DEFAULT_DIALOG_STYLE):
        """!Dialog for editing GPC and map coordinates in list control"""

        wx.Dialog.__init__(self, parent, id, title=title, style=style)

        panel = wx.Panel(parent=self)

        sizer = wx.BoxSizer(wx.VERTICAL)

        box = wx.StaticBox (parent=panel, id=wx.ID_ANY,
                            label=" %s %s " % (_("Ground Control Point No."), str(gcpno)))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)

        # source coordinates
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
       
        self.xcoord = wx.TextCtrl(parent=panel, id=wx.ID_ANY, size=(150, -1))
        self.ycoord = wx.TextCtrl(parent=panel, id=wx.ID_ANY, size=(150, -1))
        self.ecoord = wx.TextCtrl(parent=panel, id=wx.ID_ANY, size=(150, -1))
        self.ncoord = wx.TextCtrl(parent=panel, id=wx.ID_ANY, size=(150, -1))

        # swap source N, target E
        tmp_coord = data[1]
        data[1] = data[2]
        data[2] = tmp_coord
        
        row = 0
        col = 0
        idx = 0
        for label, win in ((_("source E:"), self.xcoord),
                           (_("target E:"), self.ecoord),
                           (_("source N:"), self.ycoord),
                           (_("target N:"), self.ncoord)):
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
        """!Return list of values (as strings).
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
    def __init__(self, parent, id, giface, title, pos=wx.DefaultPosition, size=wx.DefaultSize,
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
        self.new_src_map = src_map
        self.new_tgt_map = { 'raster' : tgt_map['raster'],
                             'vector' : tgt_map['vector'] }
        self.sdfactor = 0

        self.symbol = {}
        
        self.methods = ["nearest",
                        "linear",
                        "linear_f",
                        "cubic", 
                        "cubic_f",
                        "lanczos",
                        "lanczos_f"]

        # notebook
        notebook = wx.Notebook(parent=self, id=wx.ID_ANY, style=wx.BK_DEFAULT)
        self.__CreateSymbologyPage(notebook)
        self.__CreateRectificationPage(notebook)

        # buttons
        btnSave = wx.Button(self, wx.ID_SAVE)
        btnApply = wx.Button(self, wx.ID_APPLY)
        btnClose = wx.Button(self, wx.ID_CLOSE)
        btnApply.SetDefault()

        # bindings
        btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
        btnApply.SetToolTipString(_("Apply changes for the current session"))
        btnSave.Bind(wx.EVT_BUTTON, self.OnSave)
        btnSave.SetToolTipString(_("Apply and save changes to user settings file (default for next sessions)"))
        btnClose.Bind(wx.EVT_BUTTON, self.OnClose)
        btnClose.SetToolTipString(_("Close dialog"))

        # sizers
        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
        btnSizer.Add(btnApply, flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.Add(btnSave, flag=wx.LEFT | wx.RIGHT, border=5)
        btnSizer.Add(btnClose, flag=wx.LEFT | wx.RIGHT, border=5)
        
        # sizers
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item=notebook, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        mainSizer.Add(item=btnSizer, proportion=0,
                       flag=wx.ALIGN_RIGHT | wx.ALL, border=5)
        #              flag=wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border=5)

        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
        
    def __CreateSymbologyPage(self, notebook):
        """!Create notebook page with symbology settings"""

        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("Symbology"))

        sizer = wx.BoxSizer(wx.VERTICAL)

        rmsgridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        # highlight only highest forward RMS error
        self.highlighthighest = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                label=_("Highlight highest RMS error only"))
        hh = UserSettings.Get(group='gcpman', key='rms', subkey='highestonly')
        self.highlighthighest.SetValue(hh)
        rmsgridSizer.Add(item=self.highlighthighest, flag=wx.ALIGN_CENTER_VERTICAL, pos=(0, 0))

        # RMS forward error threshold
        rmslabel = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Highlight RMS error > M + SD * factor:"))
        rmslabel.SetToolTip(wx.ToolTip(_("Highlight GCPs with an RMS error larger than \n"
                              "mean + standard deviation * given factor. \n"
                              "Recommended values for this factor are between 1 and 2.")))
        rmsgridSizer.Add(item=rmslabel, flag=wx.ALIGN_CENTER_VERTICAL, pos=(1, 0))
        sdfactor = UserSettings.Get(group='gcpman', key='rms', subkey='sdfactor')
        self.rmsWin = wx.TextCtrl(parent=panel, id=wx.ID_ANY,
                       size=(70,-1), style=wx.TE_NOHIDESEL)
        self.rmsWin.SetValue("%s" % str(sdfactor))
        if (self.parent.highest_only == True):
           self.rmsWin.Disable()

        self.symbol['sdfactor'] = self.rmsWin.GetId()
        rmsgridSizer.Add(item=self.rmsWin, flag=wx.ALIGN_RIGHT, pos=(1, 1))
        rmsgridSizer.AddGrowableCol(1)
        sizer.Add(item=rmsgridSizer, flag=wx.EXPAND | wx.ALL, border=5)

        box = wx.StaticBox(parent=panel, id=wx.ID_ANY,
                           label=" %s " % _("Symbol settings"))
        boxSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)

        #
        # general symbol color
        #
        row = 0
        label = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Color:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        col = UserSettings.Get(group='gcpman', key='symbol', subkey='color')
        colWin = csel.ColourSelect(parent=panel, id=wx.ID_ANY,
                                   colour=wx.Colour(col[0],
                                                    col[1],
                                                    col[2],
                                                    255))
        self.symbol['color'] = colWin.GetId()
        gridSizer.Add(item=colWin,
                      flag=wx.ALIGN_RIGHT,
                      pos=(row, 1))

        #
        # symbol color for high forward RMS error
        #
        row += 1
        label = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Color for high RMS error:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        hcol = UserSettings.Get(group='gcpman', key='symbol', subkey='hcolor')
        hcolWin = csel.ColourSelect(parent=panel, id=wx.ID_ANY,
                                   colour=wx.Colour(hcol[0],
                                                    hcol[1],
                                                    hcol[2],
                                                    255))
        self.symbol['hcolor'] = hcolWin.GetId()
        gridSizer.Add(item=hcolWin,
                      flag=wx.ALIGN_RIGHT,
                      pos=(row, 1))

        #
        # symbol color for selected GCP
        #
        row += 1
        label = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Color for selected GCP:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        scol = UserSettings.Get(group='gcpman', key='symbol', subkey='scolor')
        scolWin = csel.ColourSelect(parent=panel, id=wx.ID_ANY,
                                   colour=wx.Colour(scol[0],
                                                    scol[1],
                                                    scol[2],
                                                    255))
        self.symbol['scolor'] = scolWin.GetId()
        gridSizer.Add(item=scolWin,
                      flag=wx.ALIGN_RIGHT,
                      pos=(row, 1))

        #
        # symbol color for unused GCP
        #
        row += 1
        label = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Color for unused GCPs:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        ucol = UserSettings.Get(group='gcpman', key='symbol', subkey='ucolor')
        ucolWin = csel.ColourSelect(parent=panel, id=wx.ID_ANY,
                                   colour=wx.Colour(ucol[0],
                                                    ucol[1],
                                                    ucol[2],
                                                    255))
        self.symbol['ucolor'] = ucolWin.GetId()
        gridSizer.Add(item=ucolWin,
                      flag=wx.ALIGN_RIGHT,
                      pos=(row, 1))

        # show unused GCPs
        row += 1
        self.showunused = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                label=_("Show unused GCPs"))
        shuu = UserSettings.Get(group='gcpman', key='symbol', subkey='unused')
        self.showunused.SetValue(shuu)
        gridSizer.Add(item=self.showunused, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))

        #
        # symbol size
        #
        row += 1
        label = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Symbol size:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        symsize = int(UserSettings.Get(group='gcpman', key='symbol', subkey='size'))
        sizeWin = wx.SpinCtrl(parent=panel, id=wx.ID_ANY,
                             min=1, max=20)
        sizeWin.SetValue(symsize)
        self.symbol['size'] = sizeWin.GetId()
        gridSizer.Add(item=sizeWin,
                      flag=wx.ALIGN_RIGHT,
                      pos=(row, 1))
        
        #
        # symbol width
        #
        row += 1
        label = wx.StaticText(parent=panel, id=wx.ID_ANY, label=_("Line width:"))
        gridSizer.Add(item=label, flag=wx.ALIGN_CENTER_VERTICAL, pos=(row, 0))
        width = int(UserSettings.Get(group='gcpman', key='symbol', subkey='width'))
        widWin = wx.SpinCtrl(parent=panel, id=wx.ID_ANY,
                             min=1, max=10)
        widWin.SetValue(width)
        self.symbol['width'] = widWin.GetId()
        gridSizer.Add(item=widWin,
                      flag=wx.ALIGN_RIGHT,
                      pos=(row, 1))
        gridSizer.AddGrowableCol(1)
        
        boxSizer.Add(item=gridSizer, flag=wx.EXPAND)
        sizer.Add(item=boxSizer, flag=wx.EXPAND | wx.ALL, border=5)

        #
        # maps to display
        #
        # source map to display
        self.srcselection = Select(panel, id=wx.ID_ANY,
                                   size=globalvar.DIALOG_GSELECT_SIZE, type='maptype', updateOnPopup = False)
        self.parent.grwiz.SwitchEnv('source')
        self.srcselection.SetElementList(maptype)
        # filter out all maps not in group
        self.srcselection.tcp.GetElementList(elements = self.parent.src_maps)

        # target map(s) to display
        self.parent.grwiz.SwitchEnv('target')
        self.tgtrastselection = Select(panel, id=wx.ID_ANY,
                                   size=globalvar.DIALOG_GSELECT_SIZE, type='raster', updateOnPopup = False)
        self.tgtrastselection.SetElementList('cell')
        self.tgtrastselection.GetElementList()

        self.tgtvectselection = Select(panel, id=wx.ID_ANY,
                                   size=globalvar.DIALOG_GSELECT_SIZE, type='vector', updateOnPopup = False)
        self.tgtvectselection.SetElementList('vector')
        self.tgtvectselection.GetElementList()

        sizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY, label=_('Select source map to display:')),
                       proportion=0, flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)
        sizer.Add(item=self.srcselection, proportion=0, 
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)
        self.srcselection.SetValue(src_map)
        sizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY, label=_('Select target raster map to display:')),
                       proportion=0, flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)
        sizer.Add(item=self.tgtrastselection, proportion=0, 
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)
        self.tgtrastselection.SetValue(tgt_map['raster'])
        sizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY, label=_('Select target vector map to display:')),
                       proportion=0, flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)
        sizer.Add(item=self.tgtvectselection, proportion=0, 
                       flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)
        self.tgtvectselection.SetValue(tgt_map['vector'])

        # bindings
        self.highlighthighest.Bind(wx.EVT_CHECKBOX, self.OnHighlight)
        self.rmsWin.Bind(wx.EVT_TEXT, self.OnSDFactor)
        self.srcselection.Bind(wx.EVT_TEXT, self.OnSrcSelection)
        self.tgtrastselection.Bind(wx.EVT_TEXT, self.OnTgtRastSelection)
        self.tgtvectselection.Bind(wx.EVT_TEXT, self.OnTgtVectSelection)

        panel.SetSizer(sizer)
        
        return panel

    def __CreateRectificationPage(self, notebook):
        """!Create notebook page with symbology settings"""

        panel = wx.Panel(parent=notebook, id=wx.ID_ANY)
        notebook.AddPage(page=panel, text=_("Rectification"))

        sizer = wx.BoxSizer(wx.VERTICAL)

        # transformation order
        self.rb_grorder = wx.RadioBox(parent=panel, id=wx.ID_ANY,
                                       label=" %s " % _("Select rectification order"),
                                       choices=[_('1st order'), _('2nd order'), _('3rd order')],
                                       majorDimension=wx.RA_SPECIFY_COLS)
        sizer.Add(item=self.rb_grorder, proportion=0,
                       flag=wx.EXPAND | wx.ALL, border=5)
        self.rb_grorder.SetSelection(self.parent.gr_order - 1)

        # interpolation method
        gridSizer = wx.GridBagSizer(vgap=5, hgap=5)
        gridSizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY, label=_('Select interpolation method:')),
                       pos=(0,0), flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)
        self.grmethod = wx.Choice(parent=panel, id=wx.ID_ANY,
                                  choices = self.methods)
        gridSizer.Add(item=self.grmethod, pos=(0,1),
                       flag=wx.ALIGN_RIGHT, border=5)
        self.grmethod.SetStringSelection(self.parent.gr_method)
        gridSizer.AddGrowableCol(1)
        sizer.Add(item=gridSizer, flag=wx.EXPAND | wx.ALL, border=5)

        # clip to region
        self.check = wx.CheckBox(parent=panel, id=wx.ID_ANY,
                                label=_("clip to computational region in target location"))
        sizer.Add(item=self.check, proportion=0,
                       flag=wx.EXPAND | wx.ALL, border=5)
        self.check.SetValue(self.parent.clip_to_region)

        # extension
        sizer.Add(item=wx.StaticText(parent=panel, id=wx.ID_ANY, label=_('Extension for output maps:')),
                       proportion=0, flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)
        self.ext_txt = wx.TextCtrl(parent=panel, id=wx.ID_ANY, value="", size=(350,-1))
        self.ext_txt.SetValue(self.parent.extension)
        sizer.Add(item=self.ext_txt,
                       proportion=0, flag=wx.ALIGN_LEFT | wx.ALIGN_CENTER_VERTICAL | wx.ALL, border=5)

        # bindings
        self.ext_txt.Bind(wx.EVT_TEXT, self.OnExtension)
        self.Bind(wx.EVT_RADIOBOX, self.parent.OnGROrder, self.rb_grorder)
        self.Bind(wx.EVT_CHOICE, self.OnMethod, self.grmethod)
        self.Bind(wx.EVT_CHECKBOX, self.OnClipRegion, self.check)

        panel.SetSizer(sizer)
        
        return panel

    def OnHighlight(self, event):
        """!Checkbox 'highlighthighest' checked/unchecked"""
        if self.highlighthighest.IsChecked():
            self.parent.highest_only = True
            self.rmsWin.Disable()
        else:
            self.parent.highest_only = False
            self.rmsWin.Enable()

    def OnSDFactor(self,event):
        """!New factor for RMS threshold = M + SD * factor"""
        try:
            self.sdfactor = float(self.rmsWin.GetValue())
        except ValueError:
            return
        if self.sdfactor <= 0:
            GError(parent = self,
                   message=_('RMS threshold factor must be > 0'))
        elif self.sdfactor < 1:
            GError(parent = self,
                   message=_('RMS threshold factor is < 1\n'
                             'Too many points might be highlighted'))
        
    def OnSrcSelection(self,event):
        """!Source map to display selected"""
        global src_map

        tmp_map = self.srcselection.GetValue()
        
        if not tmp_map == '' and not tmp_map == src_map:
            self.new_src_map = tmp_map

    def OnTgtRastSelection(self,event):
        """!Target map to display selected"""
        global tgt_map

        self.new_tgt_map['raster'] = self.tgtrastselection.GetValue()

    def OnTgtVectSelection(self,event):
        """!Target map to display selected"""
        global tgt_map

        self.new_tgt_map['vector'] = self.tgtvectselection.GetValue()

    def OnMethod(self, event):
        self.parent.gr_method = self.methods[event.GetSelection()]

    def OnClipRegion(self, event):
        self.parent.clip_to_region = event.IsChecked()
        
    def OnExtension(self, event):
        self.parent.extension = self.ext_txt.GetValue()

    def UpdateSettings(self):
        global src_map
        global tgt_map
        global maptype

        layers = None

        UserSettings.Set(group = 'gcpman', key = 'rms', subkey = 'highestonly',
                         value = self.highlighthighest.GetValue())
        
        if self.sdfactor > 0:
            UserSettings.Set(group='gcpman', key='rms', subkey='sdfactor',
                             value=self.sdfactor)
            
            self.parent.sdfactor = self.sdfactor
            if self.parent.rmsthresh > 0:
                self.parent.rmsthresh = self.parent.rmsmean + self.parent.sdfactor * self.parent.rmssd
        
        UserSettings.Set(group='gcpman', key='symbol', subkey='color',
                         value=tuple(wx.FindWindowById(self.symbol['color']).GetColour()))
        UserSettings.Set(group='gcpman', key='symbol', subkey='hcolor',
                         value=tuple(wx.FindWindowById(self.symbol['hcolor']).GetColour()))
        UserSettings.Set(group='gcpman', key='symbol', subkey='scolor',
                         value=tuple(wx.FindWindowById(self.symbol['scolor']).GetColour()))
        UserSettings.Set(group='gcpman', key='symbol', subkey='ucolor',
                         value=tuple(wx.FindWindowById(self.symbol['ucolor']).GetColour()))
        UserSettings.Set(group='gcpman', key='symbol', subkey='unused',
                         value=self.showunused.GetValue())
        UserSettings.Set(group='gcpman', key='symbol', subkey='size',
                         value=wx.FindWindowById(self.symbol['size']).GetValue())
        UserSettings.Set(group='gcpman', key='symbol', subkey='width',
                         value=wx.FindWindowById(self.symbol['width']).GetValue())
        
        srcrender = False
        srcrenderVector = False
        tgtrender = False
        tgtrenderVector = False
        reload_target = False
        if self.new_src_map != src_map:
            # remove old layer
            layers = self.parent.grwiz.SrcMap.GetListOfLayers()
            self.parent.grwiz.SrcMap.DeleteLayer(layers[0])
            
            src_map = self.new_src_map
            if maptype == 'raster':
                cmdlist = ['d.rast', 'map=%s' % src_map]
                srcrender = True
            else:
                cmdlist = ['d.vect', 'map=%s' % src_map]
                srcrenderVector = True
            self.parent.grwiz.SwitchEnv('source')
            name, found = utils.GetLayerNameFromCmd(cmdlist)
            self.parent.grwiz.SrcMap.AddLayer(ltype=maptype, command=cmdlist, active=True,
                              name=name, hidden=False, opacity=1.0, render=False)

            self.parent.grwiz.SwitchEnv('target')

        if self.new_tgt_map['raster'] != tgt_map['raster'] or \
           self.new_tgt_map['vector'] != tgt_map['vector']:
            # remove all layers
            layers = self.parent.grwiz.TgtMap.GetListOfLayers()
            while layers:
                self.parent.grwiz.TgtMap.DeleteLayer(layers[0])
                del layers[0]
                layers = self.parent.grwiz.TgtMap.GetListOfLayers()
            #self.parent.grwiz.TgtMap.DeleteAllLayers()
            reload_target = True
            tgt_map['raster'] = self.new_tgt_map['raster']
            tgt_map['vector'] = self.new_tgt_map['vector']

            if tgt_map['raster'] != '':
                cmdlist = ['d.rast', 'map=%s' % tgt_map['raster']]
                name, found = utils.GetLayerNameFromCmd(cmdlist)
                self.parent.grwiz.TgtMap.AddLayer(ltype='raster', command=cmdlist, active=True,
                                  name=name, hidden=False, opacity=1.0, render=False)

                tgtrender = True

            if tgt_map['vector'] != '':
                cmdlist = ['d.vect', 'map=%s' % tgt_map['vector']]
                name, found = utils.GetLayerNameFromCmd(cmdlist)
                self.parent.grwiz.TgtMap.AddLayer(ltype='vector', command=cmdlist, active=True,
                                  name=name, hidden=False, opacity=1.0, render=False)

                tgtrenderVector = True

        if tgt_map['raster'] == '' and tgt_map['vector'] == '':
            if self.parent.show_target == True:
                self.parent.show_target = False
                self.parent._mgr.GetPane("target").Hide()
                self.parent._mgr.Update()
                self.parent.activemap.SetSelection(0)
                self.parent.activemap.Enable(False)
                self.parent.GetMapToolbar().Enable('zoommenu', enable = False)
        else:
            if self.parent.show_target == False:
                self.parent.show_target = True
                self.parent._mgr.GetPane("target").Show()
                self.parent._mgr.Update()
                self.parent.activemap.SetSelection(0)
                self.parent.activemap.Enable(True)
                self.parent.GetMapToolbar().Enable('zoommenu', enable = True)
                self.parent.TgtMapWindow.ZoomToMap(layers = self.parent.TgtMap.GetListOfLayers())
        
        self.parent.UpdateColours(srcrender, srcrenderVector, tgtrender, tgtrenderVector)
        self.parent.SetSettings()        

    def OnSave(self, event):
        """!Button 'Save' pressed"""
        self.UpdateSettings()
        fileSettings = {}
        UserSettings.ReadSettingsFile(settings=fileSettings)
        fileSettings['gcpman'] = UserSettings.Get(group='gcpman')
        file = UserSettings.SaveToFile(fileSettings)
        self._giface.WriteLog(_('GCP Manager settings saved to file \'%s\'.') % file)
        #self.Close()

    def OnApply(self, event):
        """!Button 'Apply' pressed"""
        self.UpdateSettings()
        #self.Close()

    def OnClose(self, event):
        """!Button 'Cancel' pressed"""
        self.Close()
