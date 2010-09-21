"""!
@package gis_set.py

GRASS start-up screen.

Initialization module for wxPython GRASS GUI.
Location/mapset management (selection, creation, etc.).

Classes:
 - GRASSStartup
 - GListBox
 - StartUp

(C) 2006-2010 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton and Jachym Cepicky (original author)
@author Martin Landa <landa.martin gmail.com> (various updates)
"""

import os
import sys
import glob
import shutil
import copy
import platform

### i18N
import gettext
gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

from gui_modules import globalvar
if not os.getenv("GRASS_WXBUNDLED"):
    globalvar.CheckForWx()

import gui_modules.goutput
from gui_modules.ghelp import HelpFrame
from gui_modules.gcmd  import GMessage

import wx
import wx.html
import wx.lib.rcsizer as rcs
import wx.lib.filebrowsebutton as filebrowse
import wx.lib.mixins.listctrl as listmix
import wx.lib.scrolledpanel as scrolled

class GRASSStartup(wx.Frame):
    """!GRASS start-up screen"""
    def __init__(self, parent = None, id = wx.ID_ANY, style = wx.DEFAULT_FRAME_STYLE):

        #
        # GRASS variables
        #
        self.gisbase  = os.getenv("GISBASE")
        self.grassrc  = self._read_grassrc()
        self.gisdbase = self.GetRCValue("GISDBASE")

        #
        # list of locations/mapsets
        #
        self.listOfLocations = []
        self.listOfMapsets = []
        self.listOfMapsetsSelectable = []
        
        wx.Frame.__init__(self, parent = parent, id = id, style = style)
        
        self.locale = wx.Locale(language = wx.LANGUAGE_DEFAULT)
        
        self.panel = scrolled.ScrolledPanel(parent = self, id = wx.ID_ANY)
        
        # i18N
        import gettext
        gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

        #
        # graphical elements
        #
        # image
        try:
            name = os.path.join(globalvar.ETCDIR, "gui", "images", "startup_banner.png")
            self.hbitmap = wx.StaticBitmap(self.panel, wx.ID_ANY,
                                           wx.Bitmap(name = name,
                                                     type = wx.BITMAP_TYPE_PNG))
        except:
            self.hbitmap = wx.StaticBitmap(self.panel, wx.ID_ANY, wx.EmptyBitmap(530,150))

        # labels
        ### crashes when LOCATION doesn't exist
        versionFile = open(os.path.join(globalvar.ETCDIR, "VERSIONNUMBER"))
        grassVersion = versionFile.readline().split(' ')[0].rstrip('\n')
        versionFile.close()

        self.select_box = wx.StaticBox (parent = self.panel, id = wx.ID_ANY,
                                        label = " %s " % _("Choose project location and mapset"))

        self.manage_box = wx.StaticBox (parent = self.panel, id = wx.ID_ANY,
                                        label = " %s " % _("Manage"))
        self.lwelcome = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                      label = _("Welcome to GRASS GIS %s\n"
                                              "The world's leading open source GIS") % grassVersion,
                                      style = wx.ALIGN_CENTRE)
        self.ltitle = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                    label = _("Select an existing project location and mapset\n"
                                            "or define a new location"),
                                    style = wx.ALIGN_CENTRE)
        self.ldbase = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                    label = _("GIS Data Directory:"))
        self.llocation = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                       label = _("Project location\n(projection/coordinate system)"),
                                       style = wx.ALIGN_CENTRE)
        self.lmapset = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                     label = _("Accessible mapsets\n(directories of GIS files)"),
                                     style = wx.ALIGN_CENTRE)
        self.lcreate = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                     label = _("Create new mapset\nin selected location"),
                                     style = wx.ALIGN_CENTRE)
        self.ldefine = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                     label = _("Define new location"),
                                     style = wx.ALIGN_CENTRE)
        self.lmanageloc = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                        label = _("Rename/delete selected\nmapset or location"),
                                        style = wx.ALIGN_CENTRE)

        # buttons
        self.bstart = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                label = _("Start &GRASS"))
        self.bstart.SetDefault()
        self.bexit = wx.Button(parent = self.panel, id = wx.ID_EXIT)
        self.bstart.SetMinSize((180, self.bexit.GetSize()[1]))
        self.bhelp = wx.Button(parent = self.panel, id = wx.ID_HELP)
        self.bbrowse = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                 label = _("&Browse"))
        self.bmapset = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                 label = _("&Create mapset"))
        self.bwizard = wx.Button(parent = self.panel, id = wx.ID_ANY,
                                 label = _("&Location wizard"))
        self.manageloc = wx.Choice(parent = self.panel, id = wx.ID_ANY,
                                   choices = [_('Rename mapset'), _('Rename location'),
                                            _('Delete mapset'), _('Delete location')])
        self.manageloc.SetSelection(0)

        # textinputs
        self.tgisdbase = wx.TextCtrl(parent = self.panel, id = wx.ID_ANY, value = "", size = (300, -1),
                                     style = wx.TE_PROCESS_ENTER)

        # Locations
        self.lpanel = wx.Panel(parent = self.panel, id = wx.ID_ANY)
        self.lblocations = GListBox(parent = self.lpanel,
                                    id = wx.ID_ANY, size = (180, 200),
                                    choices = self.listOfLocations)
        
        self.lblocations.SetColumnWidth(0, 180)

        # TODO: sort; but keep PERMANENT on top of list
        # Mapsets
        self.mpanel = wx.Panel(parent = self.panel, id = wx.ID_ANY)
        self.lbmapsets = GListBox(parent = self.mpanel,
                                  id = wx.ID_ANY, size = (180, 200),
                                  choices = self.listOfMapsets)
        
        self.lbmapsets.SetColumnWidth(0, 180)

        # layout & properties
        self._set_properties()
        self._do_layout()

        # events
        self.bbrowse.Bind(wx.EVT_BUTTON,      self.OnBrowse)
        self.bstart.Bind(wx.EVT_BUTTON,       self.OnStart)
        self.bexit.Bind(wx.EVT_BUTTON,        self.OnExit)
        self.bhelp.Bind(wx.EVT_BUTTON,        self.OnHelp)
        self.bmapset.Bind(wx.EVT_BUTTON,      self.OnCreateMapset)
        self.bwizard.Bind(wx.EVT_BUTTON,      self.OnWizard)
        self.manageloc.Bind(wx.EVT_CHOICE,    self.OnManageLoc)
        self.lblocations.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnSelectLocation)
        self.lbmapsets.Bind(wx.EVT_LIST_ITEM_SELECTED,   self.OnSelectMapset)
        self.lbmapsets.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.OnStart)
        self.tgisdbase.Bind(wx.EVT_TEXT_ENTER, self.OnSetDatabase)
        self.Bind(wx.EVT_CLOSE,               self.OnCloseWindow)
        
    def _set_properties(self):
        """!Set frame properties"""
        self.SetTitle(_("Welcome to GRASS GIS"))
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, "grass.ico"),
                             wx.BITMAP_TYPE_ICO))

        self.lwelcome.SetForegroundColour(wx.Colour(35, 142, 35))
        self.lwelcome.SetFont(wx.Font(13, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))

        self.bstart.SetForegroundColour(wx.Colour(35, 142, 35))
        self.bstart.SetToolTipString(_("Enter GRASS session"))
        self.bstart.Enable(False)
        self.bmapset.Enable(False)
        self.manageloc.Enable(False)

        # set database
        if not self.gisdbase:
            # sets an initial path for gisdbase if nothing in GISRC
            if os.path.isdir(os.getenv("HOME")):
                self.gisdbase = os.getenv("HOME")
            else:
                self.gisdbase = os.getcwd()
        try:
            self.tgisdbase.SetValue(self.gisdbase)
        except UnicodeDecodeError:
            wx.MessageBox(parent = self, caption = _("Error"),
                          message = _("Unable to set GRASS database. "
                                      "Check your locale settings."),
                          style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
        
        self.OnSetDatabase(None)
        location = self.GetRCValue("LOCATION_NAME")
        if location == "<UNKNOWN>" or \
                not os.path.isdir(os.path.join(self.gisdbase, location)):
            location = None

        if location:
            # list of locations
            self.UpdateLocations(self.gisdbase)
            try:
                self.lblocations.SetSelection(self.listOfLocations.index(location),
                                              force = True)
                self.lblocations.EnsureVisible(self.listOfLocations.index(location))
            except ValueError:
                print >> sys.stderr, _("ERROR: Location <%s> not found") % \
                    (utils.UnicodeString(location))
                
            # list of mapsets
            self.UpdateMapsets(os.path.join(self.gisdbase, location))
            mapset = self.GetRCValue("MAPSET")
            if mapset:
                try:
                    self.lbmapsets.SetSelection(self.listOfMapsets.index(mapset),
                                                force = True)
                    self.lbmapsets.EnsureVisible(self.listOfMapsets.index(mapset))
                except ValueError:
                    self.lbmapsets.Clear()
                    print >> sys.stderr, _("ERROR: Mapset <%s> not found") % \
                        (utils.UnicodeString(mapset))
            
    def _do_layout(self):
        label_style = wx.ADJUST_MINSIZE | wx.ALIGN_CENTER_HORIZONTAL

        sizer           = wx.BoxSizer(wx.VERTICAL)
        dbase_sizer     = wx.BoxSizer(wx.HORIZONTAL)
        location_sizer  = wx.FlexGridSizer(rows = 1, cols = 2, vgap = 4, hgap = 4)
        select_boxsizer = wx.StaticBoxSizer(self.select_box, wx.VERTICAL)
        select_sizer    = wx.FlexGridSizer(rows = 2, cols = 2, vgap = 4, hgap = 4)
        manage_boxsizer = wx.StaticBoxSizer(self.manage_box, wx.VERTICAL)
        manage_sizer    = wx.BoxSizer(wx.VERTICAL)
        btns_sizer      = wx.BoxSizer(wx.HORIZONTAL)

        # gis data directory
        dbase_sizer.Add(item = self.ldbase, proportion = 0,
                        flag = wx.ALIGN_CENTER_VERTICAL |
                        wx.ALIGN_CENTER_HORIZONTAL | wx.ALL,
                        border = 3)
        dbase_sizer.Add(item = self.tgisdbase, proportion = 0,
                        flag = wx.ALIGN_CENTER_VERTICAL |
                        wx.ALIGN_CENTER_HORIZONTAL | wx.ALL,
                        border = 3)
        dbase_sizer.Add(item = self.bbrowse, proportion = 0,
                        flag = wx.ALIGN_CENTER_VERTICAL |
                        wx.ALIGN_CENTER_HORIZONTAL | wx.ALL,
                        border = 3)

        # select sizer
        select_sizer.Add(item = self.llocation, proportion = 0,
                         flag = label_style | wx.ALL,
                         border = 3)
        select_sizer.Add(item = self.lmapset, proportion = 0,
                         flag = label_style | wx.ALL,
                         border = 3)
        select_sizer.Add(item = self.lpanel, proportion = 0,
                         flag = wx.ADJUST_MINSIZE |
                         wx.ALIGN_CENTER_VERTICAL |
                         wx.ALIGN_CENTER_HORIZONTAL)
        select_sizer.Add(item = self.mpanel, proportion = 0,
                         flag = wx.ADJUST_MINSIZE |
                         wx.ALIGN_CENTER_VERTICAL |
                         wx.ALIGN_CENTER_HORIZONTAL)

        select_boxsizer.Add(item = select_sizer, proportion = 0)

        # define new location and mapset
        manage_sizer.Add(item = self.ldefine, proportion = 0,
                         flag = label_style | wx.ALL,
                         border = 3)
        manage_sizer.Add(item = self.bwizard, proportion = 0,
                         flag = label_style | wx.BOTTOM,
                         border = 5)
        manage_sizer.Add(item = self.lcreate, proportion = 0,
                         flag = label_style | wx.ALL,
                         border = 3)
        manage_sizer.Add(item = self.bmapset, proportion = 0,
                         flag = label_style | wx.BOTTOM,
                         border = 5)
        manage_sizer.Add(item = self.lmanageloc, proportion = 0,
                         flag = label_style | wx.ALL,
                         border = 3)
        manage_sizer.Add(item = self.manageloc, proportion = 0,
                         flag = label_style | wx.BOTTOM,
                         border = 5)

        manage_boxsizer.Add(item = manage_sizer, proportion = 0)

        # location sizer
        location_sizer.Add(item = select_boxsizer, proportion = 0,
                           flag = wx.ADJUST_MINSIZE |
                           wx.ALIGN_CENTER_VERTICAL |
                           wx.ALIGN_CENTER_HORIZONTAL |
                           wx.RIGHT | wx.LEFT | wx.EXPAND,
                           border = 3) # GISDBASE setting
        location_sizer.Add(item = manage_boxsizer, proportion = 0,
                           flag = wx.ADJUST_MINSIZE |
                           wx.ALIGN_TOP |
                           wx.ALIGN_CENTER_HORIZONTAL |
                           wx.RIGHT | wx.EXPAND,
                           border = 3)

        # buttons
        btns_sizer.Add(item = self.bstart, proportion = 0,
                       flag = wx.ALIGN_CENTER_HORIZONTAL |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL,
                       border = 5)
        btns_sizer.Add(item = self.bexit, proportion = 0,
                       flag = wx.ALIGN_CENTER_HORIZONTAL |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL,
                       border = 5)
        btns_sizer.Add(item = self.bhelp, proportion = 0,
                       flag = wx.ALIGN_CENTER_HORIZONTAL |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL,
                       border = 5)

        # main sizer
        sizer.Add(item = self.hbitmap,
                  proportion = 0,
                  flag = wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.ALL,
                  border = 3) # image
        sizer.Add(item = self.lwelcome, # welcome message
                  proportion = 0,
                  flag = wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.BOTTOM,
                  border = 5)
        sizer.Add(item = self.ltitle, # title
                  proportion = 0,
                  flag = wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL)
        sizer.Add(item = dbase_sizer, proportion = 0,
                  flag = wx.ALIGN_CENTER_HORIZONTAL |
                  wx.RIGHT | wx.LEFT,
                  border = 1) # GISDBASE setting
        sizer.Add(item = location_sizer, proportion = 1,
                  flag = wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.RIGHT | wx.LEFT,
                  border = 1)
        sizer.Add(item = btns_sizer, proportion = 0,
                  flag = wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.RIGHT | wx.LEFT,
                  border = 1)

        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
        sizer.SetSizeHints(self)

        self.Layout()

    def _read_grassrc(self):
        """!Read variables from $HOME/.grass7/rc file
        """
        grassrc = {}

        gisrc = os.getenv("GISRC")

        if gisrc and os.path.isfile(gisrc):
            try:
                rc = open(gisrc, "r")
                for line in rc.readlines():
                    key, val = line.split(":", 1)
                    grassrc[key.strip()] = val.strip()
            finally:
                rc.close()

        return grassrc

    def GetRCValue(self, value):
        "Return GRASS variable (read from GISRC)"""

        if self.grassrc.has_key(value):
            return self.grassrc[value]
        else:
            return None

    def OnWizard(self, event):
        """!Location wizard started"""
        from gui_modules import location_wizard
        gWizard = location_wizard.LocationWizard(parent = self,
                                                 grassdatabase = self.tgisdbase.GetValue())
        if gWizard.location !=  None:
            self.OnSetDatabase(event)
            self.UpdateMapsets(os.path.join(self.gisdbase, gWizard.location))
            self.lblocations.SetSelection(self.listOfLocations.index(gWizard.location))
            self.lbmapsets.SetSelection(0)

    def OnManageLoc(self, event):
        """!Location management choice control handler
        """
        sel = event.GetSelection()
        if sel ==  0:
            self.RenameMapset()
        elif sel ==  1:
            self.RenameLocation()
        elif sel ==  2:
            self.DeleteMapset()
        elif sel ==  3:
            self.DeleteLocation()
        
        event.Skip()
        
    def RenameMapset(self):
        """!Rename selected mapset
        """
        location = utils.UnicodeString(self.listOfLocations[self.lblocations.GetSelection()])
        mapset   = utils.UnicodeString(self.listOfMapsets[self.lbmapsets.GetSelection()])
        if mapset ==  'PERMANENT':
            GMessage(parent = self,
                     message = _('Mapset <PERMANENT> is required for valid GRASS location.\n\n'
                                 'This mapset cannot be renamed.'))
            return
        
        dlg = wx.TextEntryDialog(parent = self,
                                 message = _('Current name: %s\n\nEnter new name:') % mapset,
                                 caption = _('Rename selected mapset'))
        
        if dlg.ShowModal() ==  wx.ID_OK:
            newmapset = dlg.GetValue()
            if newmapset ==  mapset:
                dlg.Destroy()
                return
            
            if newmapset in self.listOfMapsets:
                wx.MessageBox(parent = self,
                              caption = _('Message'),
                              message = _('Unable to rename mapset.\n\n'
                                        'Mapset <%s> already exists in location.') % newmapset,
                              style = wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
            else:
                try:
                    os.rename(os.path.join(self.gisdbase, location, mapset),
                              os.path.join(self.gisdbase, location, newmapset))
                    self.OnSelectLocation(None)
                    self.lbmapsets.SetSelection(self.listOfMapsets.index(newmapset))
                except StandardError, e:
                    wx.MessageBox(parent = self,
                                  caption = _('Error'),
                                  message = _('Unable to rename mapset.\n\n%s') % e,
                                  style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
            
        dlg.Destroy()

    def RenameLocation(self):
        """!Rename selected location
        """
        location = utils.UnicodeString(self.listOfLocations[self.lblocations.GetSelection()])

        dlg = wx.TextEntryDialog(parent = self,
                                 message = _('Current name: %s\n\nEnter new name:') % location,
                                 caption = _('Rename selected location'))

        if dlg.ShowModal() ==  wx.ID_OK:
            newlocation = dlg.GetValue()
            if newlocation ==  location:
                dlg.Destroy()
                return

            if newlocation in self.listOfLocations:
                wx.MessageBox(parent = self,
                              caption = _('Message'),
                              message = _('Unable to rename location.\n\n'
                                        'Location <%s> already exists in GRASS database.') % newlocation,
                              style = wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
            else:
                try:
                    os.rename(os.path.join(self.gisdbase, location),
                              os.path.join(self.gisdbase, newlocation))
                    self.UpdateLocations(self.gisdbase)
                    self.lblocations.SetSelection(self.listOfLocations.index(newlocation))
                    self.UpdateMapsets(newlocation)
                except StandardError, e:
                    wx.MessageBox(parent = self,
                                  caption = _('Error'),
                                  message = _('Unable to rename location.\n\n%s') % e,
                                  style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
        
        dlg.Destroy()

    def DeleteMapset(self):
        """!Delete selected mapset
        """
        location = self.listOfLocations[self.lblocations.GetSelection()]
        mapset   = self.listOfMapsets[self.lbmapsets.GetSelection()]
        if mapset ==  'PERMANENT':
            GMessage(parent = self,
                     message = _('Mapset <PERMANENT> is required for valid GRASS location.\n\n'
                                 'This mapset cannot be deleted.'))
            return
        
        dlg = wx.MessageDialog(parent = self, message = _("Do you want to continue with deleting mapset <%(mapset)s> "
                                                      "from location <%(location)s>?\n\n"
                                                      "ALL MAPS included in this mapset will be "
                                                      "PERMANENTLY DELETED!") % {'mapset' : mapset,
                                                                                 'location' : location},
                               caption = _("Delete selected mapset"),
                               style = wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION)

        if dlg.ShowModal() ==  wx.ID_YES:
            try:
                shutil.rmtree(os.path.join(self.gisdbase, location, mapset))
                self.OnSelectLocation(None)
                self.lbmapsets.SetSelection(0)
            except:
                wx.MessageBox(message = _('Unable to delete mapset'))

        dlg.Destroy()

    def DeleteLocation(self):
        """
        Delete selected location
        """

        location = self.listOfLocations[self.lblocations.GetSelection()]

        dlg = wx.MessageDialog(parent = self, message = _("Do you want to continue with deleting "
                                                      "location <%s>?\n\n"
                                                      "ALL MAPS included in this location will be "
                                                      "PERMANENTLY DELETED!") % (location),
                               caption = _("Delete selected location"),
                               style = wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION)

        if dlg.ShowModal() ==  wx.ID_YES:
            try:
                shutil.rmtree(os.path.join(self.gisdbase, location))
                self.UpdateLocations(self.gisdbase)
                self.lblocations.SetSelection(0)
                self.OnSelectLocation(None)
                self.lbmapsets.SetSelection(0)
            except:
                wx.MessageBox(message = _('Unable to delete location'))

        dlg.Destroy()

    def UpdateLocations(self, dbase):
        """!Update list of locations"""
        try:
            self.listOfLocations = utils.GetListOfLocations(dbase)
        except UnicodeEncodeError:
            wx.MessageBox(parent = self, caption = _("Error"),
                          message = _("Unable to set GRASS database. "
                                      "Check your locale settings."),
                          style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
        
        self.lblocations.Clear()
        self.lblocations.InsertItems(self.listOfLocations, 0)

        if len(self.listOfLocations) > 0:
            self.lblocations.SetSelection(0)
        else:
            self.lblocations.SetSelection(wx.NOT_FOUND)

        return self.listOfLocations

    def UpdateMapsets(self, location):
        """!Update list of mapsets"""
        self.FormerMapsetSelection = wx.NOT_FOUND # for non-selectable item

        self.listOfMapsetsSelectable = list()
        self.listOfMapsets = utils.GetListOfMapsets(self.gisdbase, location)
         
        self.lbmapsets.Clear()

        # disable mapset with denied permission
        locationName = os.path.basename(location)

        try:
            ret = gcmd.RunCommand('g.mapset',
                                  read = True,
                                  flags = 'l',
                                  location = locationName,
                                  gisdbase = self.gisdbase)
            
            if not ret:
                raise gcmd.CmdError("")
            
            for line in ret.splitlines():
                self.listOfMapsetsSelectable +=  line.split(' ')
        except:
            gcmd.RunCommand("g.gisenv",
                            set =  "GISDBASE = %s" % self.gisdbase)
            gcmd.RunCommand("g.gisenv",
                            set = "LOCATION_NAME = %s" % locationName)
            gcmd.RunCommand("g.gisenv",
                            set = "MAPSET = PERMANENT")
            # first run only
            self.listOfMapsetsSelectable = copy.copy(self.listOfMapsets)
        
        disabled = []
        idx = 0
        for mapset in self.listOfMapsets:
            mapset = utils.UnicodeString(mapset)
            if mapset not in self.listOfMapsetsSelectable or \
                    os.path.isfile(os.path.join(self.gisdbase,
                                                locationName,
                                                mapset, ".gislock")):
                disabled.append(idx)
            idx +=  1
        
        self.lbmapsets.InsertItems(self.listOfMapsets, 0, disabled = disabled)
        
        return self.listOfMapsets

    def OnSelectLocation(self, event):
        """!Location selected"""
        if event:
            self.lblocations.SetSelection(event.GetIndex())
            
        if self.lblocations.GetSelection() !=  wx.NOT_FOUND:
            self.UpdateMapsets(os.path.join(self.gisdbase,
                                            self.listOfLocations[self.lblocations.GetSelection()]))
        else:
            self.listOfMapsets = []

        disabled = []
        idx = 0
        try:
            locationName = self.listOfLocations[self.lblocations.GetSelection()]
        except IndexError:
            locationName = ''
        
        for mapset in self.listOfMapsets:
            if mapset not in self.listOfMapsetsSelectable or \
                    os.path.isfile(os.path.join(self.gisdbase,
                                                locationName,
                                                mapset, ".gislock")):
                disabled.append(idx)
            idx +=  1

        self.lbmapsets.Clear()
        self.lbmapsets.InsertItems(self.listOfMapsets, 0, disabled = disabled)

        if len(self.listOfMapsets) > 0:
            self.lbmapsets.SetSelection(0)
            if locationName:
                # enable start button when location and mapset is selected
                self.bstart.Enable()
                self.bmapset.Enable()
                self.manageloc.Enable()
        else:
            self.lbmapsets.SetSelection(wx.NOT_FOUND)
            self.bstart.Enable(False)
            self.bmapset.Enable(False)
            self.manageloc.Enable(False)
        
    def OnSelectMapset(self, event):
        """!Mapset selected"""
        self.lbmapsets.SetSelection(event.GetIndex())

        if event.GetText() not in self.listOfMapsetsSelectable:
            self.lbmapsets.SetSelection(self.FormerMapsetSelection)
        else:
            self.FormerMapsetSelection = event.GetIndex()
            event.Skip()

    def OnSetDatabase(self, event):
        """!Database set"""
        self.gisdbase = self.tgisdbase.GetValue()
        
        self.UpdateLocations(self.gisdbase)

        self.OnSelectLocation(None)

    def OnBrowse(self, event):
        """'Browse' button clicked"""
        grassdata = None

        dlg = wx.DirDialog(self, _("Choose GIS Data Directory:"),
                           style = wx.DD_DEFAULT_STYLE | wx.DD_NEW_DIR_BUTTON)
        if dlg.ShowModal() ==  wx.ID_OK:
            self.gisdbase = dlg.GetPath()
            self.tgisdbase.SetValue(self.gisdbase)
            self.OnSetDatabase(event)

        dlg.Destroy()

    def OnCreateMapset(self,event):
        """!Create new mapset"""
        self.gisdbase = self.tgisdbase.GetValue()
        location = self.listOfLocations[self.lblocations.GetSelection()]

        dlg = wx.TextEntryDialog(parent = self,
                                 message = _('Enter name for new mapset:'),
                                 caption = _('Create new mapset'))

        if dlg.ShowModal() ==  wx.ID_OK:
            mapset = dlg.GetValue()
            try:
                os.mkdir(os.path.join(self.gisdbase, location, mapset))
                # copy WIND file and its permissions from PERMANENT and set permissions to u+rw,go+r
                shutil.copy(os.path.join(self.gisdbase, location, 'PERMANENT', 'WIND'),
                            os.path.join(self.gisdbase, location, mapset))
                # os.chmod(os.path.join(database,location,mapset,'WIND'), 0644)
                self.OnSelectLocation(None)
                self.lbmapsets.SetSelection(self.listOfMapsets.index(mapset))
            except StandardError, e:
                dlg = wx.MessageDialog(parent = self, message = _("Unable to create new mapset: %s") % e,
                                       caption = _("Error"), style = wx.OK | wx.ICON_ERROR)
                dlg.ShowModal()
                dlg.Destroy()
                return False
        
        self.bstart.SetFocus()
        
        return True

    def OnStart(self, event):
        """'Start GRASS' button clicked"""
        gcmd.RunCommand("g.gisenv",
                        set = "GISDBASE = %s" % \
                            self.tgisdbase.GetValue())
        gcmd.RunCommand("g.gisenv",
                        set = "LOCATION_NAME = %s" % \
                            self.listOfLocations[self.lblocations.GetSelection()])
        gcmd.RunCommand("g.gisenv",
                        set = "MAPSET = %s" % \
                            self.listOfMapsets[self.lbmapsets.GetSelection()])
        
        self.Destroy()
        sys.exit(0)

    def OnExit(self, event):
        """'Exit' button clicked"""
        self.Destroy()
        sys.exit(2)

    def OnHelp(self, event):
        """'Help' button clicked"""
        # help text in lib/init/helptext.html
        file = os.path.join(self.gisbase, "docs", "html", "helptext.html")

        helpFrame = HelpFrame(parent = self, id = wx.ID_ANY,
                              title = _("GRASS Quickstart"),
                              size = (640, 480),
                              file = file)
        helpFrame.Show(True)

        event.Skip()

    def OnCloseWindow(self, event):
        """!Close window event"""
        event.Skip()
        sys.exit(2)

class GListBox(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin):
    """!Use wx.ListCtrl instead of wx.ListBox, different style for
    non-selectable items (e.g. mapsets with denied permission)"""
    def __init__(self, parent, id, size,
                 choices, disabled = []):
        wx.ListCtrl.__init__(self, parent, id, size = size,
                             style = wx.LC_REPORT | wx.LC_NO_HEADER | wx.LC_SINGLE_SEL |
                             wx.BORDER_SUNKEN)
        
        listmix.ListCtrlAutoWidthMixin.__init__(self)
        
        self.InsertColumn(0, '')
        
        self.selected = wx.NOT_FOUND
        
        self._LoadData(choices, disabled)
        
    def _LoadData(self, choices, disabled = []):
        """!Load data into list
        
        @param choices list of item
        @param disabled list of indeces of non-selectable items
        """
        idx = 0
        for item in choices:
            index = self.InsertStringItem(sys.maxint, item)
            self.SetStringItem(index, 0, item)
            
            if idx in disabled:
                self.SetItemTextColour(idx, wx.Colour(150, 150, 150))
            idx +=  1
        
    def Clear(self):
        self.DeleteAllItems()
        
    def InsertItems(self, choices, pos, disabled = []):
        self._LoadData(choices, disabled)
        
    def SetSelection(self, item, force = False):
        if item !=  wx.NOT_FOUND and \
                (platform.system() !=  'Windows' or force):
            ### Windows -> FIXME
            self.SetItemState(item, wx.LIST_STATE_SELECTED, wx.LIST_STATE_SELECTED)
        
        self.selected = item
        
    def GetSelection(self):
        return self.selected
        
class StartUp(wx.App):
    """!Start-up application"""

    def OnInit(self):
        wx.InitAllImageHandlers()
        StartUp = GRASSStartup()
        StartUp.CenterOnScreen()
        self.SetTopWindow(StartUp)
        StartUp.Show()
        
        if StartUp.GetRCValue("LOCATION_NAME") ==  "<UNKNOWN>":
            wx.MessageBox(parent = StartUp,
                          caption = _('Starting GRASS for the first time'),
                          message = _('GRASS needs a directory in which to store its data. '
                                    'Create one now if you have not already done so. '
                                    'A popular choice is "grassdata", located in '
                                    'your home directory.'),
                          style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
            
            StartUp.OnBrowse(None)
        
        return 1

if __name__ ==  "__main__":

    if os.getenv("GISBASE") is None:
        print >> sys.stderr, "Failed to start GUI, GRASS GIS is not running."
    else:
        import gettext
        gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)

        import gui_modules.gcmd as gcmd
        import gui_modules.utils as utils

        GRASSStartUp = StartUp(0)
        GRASSStartUp.MainLoop()
