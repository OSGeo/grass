"""
@package gis_set.py

GRASS start-up screen.

Initialization module for wxPython GRASS GUI.
Location/mapset management (selection, creation, etc.).

Classes:
 - GRASSStartup
 - HelpWindow
 - StartUp

COPYRIGHT: (C) 2006-2008 by the GRASS Development Team
           This program is free software under the GNU General Public
           License (>=v2). Read the file COPYING that comes with GRASS
           for details.

@author Michael Barton and Jachym Cepicky (original author)
Martin Landa <landa.martin gmail.com> (various updates)
"""

import os
import sys
import glob
import shutil

### i18N
import gettext

from gui_modules import globalvar
globalvar.CheckForWx()

import wx
import wx.html
import wx.lib.rcsizer as rcs
import wx.lib.filebrowsebutton as filebrowse
import wx.lib.mixins.listctrl as listmix

class GRASSStartup(wx.Frame):
    """GRASS start-up screen"""
    def __init__(self, parent=None, id=wx.ID_ANY, style=wx.DEFAULT_FRAME_STYLE):

        #
        # GRASS variables
        #
        self.gisbase  = os.getenv("GISBASE")
        self.grassrc  = self._read_grassrc()
        self.gisdbase = self._getRCValue("GISDBASE")

        #
        # list of locations/mapsets
        #
        self.listOfLocations = []
        self.listOfMapsets = []
        self.listOfMapsetsSelectable = []
        
        wx.Frame.__init__(self, parent=parent, id=id, style=style)

	self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        #
        # graphical elements
        #
        # image
        try:
            name = os.path.join(globalvar.ETCDIR, "gintro.gif")
            self.hbitmap = wx.StaticBitmap(self.panel, wx.ID_ANY,
                                           wx.Bitmap(name=name,
                                                     type=wx.BITMAP_TYPE_GIF))
        except:
            self.hbitmap = wx.StaticBitmap(self.panel, wx.ID_ANY, wx.EmptyBitmap(530,150))

        # labels
        ### crashes when LOCATION doesn't exist
        # versionCmd = gcmd.Command(['g.version'], log=None)
        # grassVersion = versionCmd.ReadStdOutput()[0].replace('GRASS', '').strip()
        versionFile = open(os.path.join(globalvar.ETCDIR, "VERSIONNUMBER"))
        grassVersion = versionFile.readline().replace('%s' % os.linesep, '').strip()
        versionFile.close()

        self.select_box = wx.StaticBox (parent=self.panel, id=wx.ID_ANY,
                                        label=" %s " % _("Choose project location and mapset"))

        self.manage_box = wx.StaticBox (parent=self.panel, id=wx.ID_ANY,
                                        label=" %s " % _("Manage"))
        self.lwelcome = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                      label=_("Welcome to GRASS GIS %s\n"
                                              "The world's leading open source GIS") % grassVersion,
                                      style=wx.ALIGN_CENTRE)
        #self.SetFont(wx.Font(pointSize=9, family=wx.FONTFAMILY_DEFAULT,
        #                     style=wx.NORMAL, weight=wx.NORMAL))
        self.ltitle = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                    label=_("Select an existing project location and mapset\n"
                                            "or define a new location"),
                                    style=wx.ALIGN_CENTRE)
        self.ldbase = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                    label=_("GIS Data Directory:"))
        self.llocation = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                       label=_("Project location\n(projection/coordinate system)"),
                                       style=wx.ALIGN_CENTRE)
        self.lmapset = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                     label=_("Accessible mapsets\n(directories of GIS files)"),
                                     style=wx.ALIGN_CENTRE)
        self.lcreate = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                     label=_("Create new mapset\nin selected location"),
                                     style=wx.ALIGN_CENTRE)
        self.ldefine = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                     label=_("Define new location"),
                                     style=wx.ALIGN_CENTRE)
        self.lmanageloc = wx.StaticText(parent=self.panel, id=wx.ID_ANY,
                                        label=_("Rename/delete selected\nmapset or location"),
                                        style=wx.ALIGN_CENTRE)

        # buttons
        self.bstart = wx.Button(parent=self.panel, id=wx.ID_ANY,
                                label=_("Start GRASS"), size=(180, -1))
        self.bstart.SetDefault()
        self.bexit = wx.Button(parent=self.panel, id=wx.ID_EXIT)
        self.bhelp = wx.Button(parent=self.panel, id=wx.ID_HELP)
        self.bbrowse = wx.Button(parent=self.panel, id=wx.ID_ANY,
                                 label=_("Browse"))
        self.bmapset = wx.Button(parent=self.panel, id=wx.ID_ANY,
                                 label=_("Create mapset"))
        self.bwizard = wx.Button(parent=self.panel, id=wx.ID_ANY,
                                 label=_("Location wizard"))
        self.manageloc = wx.Choice(parent=self.panel, id=wx.ID_ANY,
                                   choices=[_('Rename mapset'), _('Rename location'),
                                            _('Delete mapset'), _('Delete location')])
	self.manageloc.SetSelection(0)

        # textinputs
        self.tgisdbase = wx.TextCtrl(parent=self.panel, id=wx.ID_ANY, value="", size=(300, -1),
                                     style=wx.TE_PROCESS_ENTER)

        # Locations
        self.lpanel = wx.Panel(parent=self.panel, id=wx.ID_ANY)
        self.lblocations = GListBox(parent=self.lpanel,
                                    id=wx.ID_ANY, size=(180, 200),
                                    choices=self.listOfLocations)

        # TODO: sort; but keep PERMANENT on top of list
        # Mapsets
        self.mpanel = wx.Panel(parent=self.panel, id=wx.ID_ANY)
        self.lbmapsets = GListBox(parent=self.mpanel,
                                  id=wx.ID_ANY, size=(180, 200),
                                  choices=self.listOfMapsets)

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
        self.tgisdbase.Bind(wx.EVT_TEXT_ENTER, self.OnSetDatabase)
        self.Bind(wx.EVT_CLOSE,               self.OnCloseWindow)

    def _set_properties(self):
        """Set frame properties"""
        self.SetTitle(_("Welcome to GRASS GIS"))
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCDIR, "grass.ico"),
                             wx.BITMAP_TYPE_ICO))

        self.lwelcome.SetForegroundColour(wx.Colour(35, 142, 35))
        self.lwelcome.SetFont(wx.Font(14, wx.DEFAULT, wx.NORMAL, wx.BOLD, 0, ""))

        self.bstart.SetForegroundColour(wx.Colour(35, 142, 35))
        self.bstart.SetToolTipString(_("Enter GRASS session"))
        # self.bstart.Enable(False)
        # self.bmapset.Enable(False)

        # set database
        if not self.gisdbase:
            # sets an initial path for gisdbase if nothing in GISRC
            if os.path.isdir(os.getenv("HOME")):
                self.gisdbase = os.getenv("HOME")
            else:
                self.gisdbase = os.getcwd()
        self.tgisdbase.SetValue(self.gisdbase)

        self.OnSetDatabase(None)
        location = self._getRCValue("LOCATION_NAME")
        if location == "<UNKNOWN>" or \
                not os.path.isdir(os.path.join(self.gisdbase, location)):
            location = None
        if location:
            # list of locations
            self.UpdateLocations(self.gisdbase)
            try:
                self.lblocations.SetSelection(self.listOfLocations.index(location))
            except ValueError:
                print >> sys.stderr, _("ERROR: Location <%s> not found") % \
                    (location)
                
            # list of mapsets
            self.UpdateMapsets(os.path.join(self.gisdbase,location))
            mapset = self._getRCValue("MAPSET")
            if mapset:
                try:
                    self.lbmapsets.SetSelection(self.listOfMapsets.index(mapset))
                except ValueError:
                    self.lbmapsets.Clear()
                    print >> sys.stderr, _("ERROR: Mapset <%s> not found") % \
                        (mapset)

                # self.bstart.Enable(True)

    def _do_layout(self):
        label_style = wx.ADJUST_MINSIZE | wx.ALIGN_CENTER_HORIZONTAL

        sizer           = wx.BoxSizer(wx.VERTICAL)
        dbase_sizer     = wx.BoxSizer(wx.HORIZONTAL)
        location_sizer  = wx.FlexGridSizer(rows=1, cols=2, vgap=4, hgap=4)
        select_boxsizer = wx.StaticBoxSizer(self.select_box, wx.VERTICAL)
        select_sizer    = wx.FlexGridSizer(rows=2, cols=2, vgap=4, hgap=4)
        manage_boxsizer = wx.StaticBoxSizer(self.manage_box, wx.VERTICAL)
        manage_sizer    = wx.BoxSizer(wx.VERTICAL)
        btns_sizer    = wx.BoxSizer(wx.HORIZONTAL)

        # gis data directory
        dbase_sizer.Add(item=self.ldbase, proportion=0,
                        flag=wx.ALIGN_CENTER_VERTICAL |
                        wx.ALIGN_CENTER_HORIZONTAL | wx.ALL,
                        border=5)
        dbase_sizer.Add(item=self.tgisdbase, proportion=0,
                        flag=wx.ALIGN_CENTER_VERTICAL |
                        wx.ALIGN_CENTER_HORIZONTAL | wx.ALL,
                        border=5)
        dbase_sizer.Add(item=self.bbrowse, proportion=0,
                        flag=wx.ALIGN_CENTER_VERTICAL |
                        wx.ALIGN_CENTER_HORIZONTAL | wx.ALL,
                        border=5)

        # select sizer
        select_sizer.Add(item=self.llocation, proportion=0,
                         flag=label_style | wx.ALL,
                         border=5)
        select_sizer.Add(item=self.lmapset, proportion=0,
                         flag=label_style | wx.ALL,
                         border=5)
        select_sizer.Add(item=self.lpanel, proportion=0,
                         flag=wx.ADJUST_MINSIZE |
                         wx.ALIGN_CENTER_VERTICAL |
                         wx.ALIGN_CENTER_HORIZONTAL)
        select_sizer.Add(item=self.mpanel, proportion=0,
                         flag=wx.ADJUST_MINSIZE |
                         wx.ALIGN_CENTER_VERTICAL |
                         wx.ALIGN_CENTER_HORIZONTAL)

        select_boxsizer.Add(item=select_sizer, proportion=0)

        # define new location and mapset
        manage_sizer.Add(item=self.ldefine, proportion=0,
                         flag=label_style | wx.ALL,
                         border=5)
        manage_sizer.Add(item=self.bwizard, proportion=0,
                         flag=label_style | wx.BOTTOM,
                         border=8)
        manage_sizer.Add(item=self.lcreate, proportion=0,
                         flag=label_style | wx.ALL,
                         border=5)
        manage_sizer.Add(item=self.bmapset, proportion=0,
                         flag=label_style | wx.BOTTOM,
                         border=8)
        manage_sizer.Add(item=self.lmanageloc, proportion=0,
                         flag=label_style | wx.ALL,
                         border=5)
        manage_sizer.Add(item=self.manageloc, proportion=0,
                         flag=label_style | wx.BOTTOM,
                         border=8)

        manage_boxsizer.Add(item=manage_sizer, proportion=0)

        # location sizer
        location_sizer.Add(item=select_boxsizer, proportion=0,
                           flag=wx.ADJUST_MINSIZE |
                           wx.ALIGN_CENTER_VERTICAL |
                           wx.ALIGN_CENTER_HORIZONTAL |
                           wx.RIGHT | wx.LEFT | wx.EXPAND,
                           border=5) # GISDBASE setting
        location_sizer.Add(item=manage_boxsizer, proportion=0,
                           flag=wx.ADJUST_MINSIZE |
                           wx.ALIGN_TOP |
                           wx.ALIGN_CENTER_HORIZONTAL |
                           wx.RIGHT | wx.EXPAND,
                           border=5)

        # buttons
        btns_sizer.Add(item=self.bstart, proportion=0,
                       flag=wx.ALIGN_CENTER_HORIZONTAL |
                       wx.ALL,
                       border=10)
        btns_sizer.Add(item=self.bexit, proportion=0,
                       flag=wx.ALIGN_CENTER_HORIZONTAL |
                       wx.ALL,
                       border=10)
        btns_sizer.Add(item=self.bhelp, proportion=0,
                       flag=wx.ALIGN_CENTER_HORIZONTAL |
                       wx.ALL,
                       border=10)

        # main sizer
        sizer.Add(item=self.hbitmap, proportion=0,
                  flag=wx.ADJUST_MINSIZE |
                  wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.ALL,
                  border=5) # image
        sizer.Add(item=self.lwelcome, # welcome message
                  proportion=0,
                  flag= wx.ADJUST_MINSIZE |
                  wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.EXPAND |
                  wx.BOTTOM,
                  border=10)
        sizer.Add(item=self.ltitle, # title
                  proportion=0,
                  flag=wx.ADJUST_MINSIZE |
                  wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.EXPAND |
                  wx.BOTTOM,
                  border=5)
        sizer.Add(item=dbase_sizer, proportion=0,
                  flag=wx.ADJUST_MINSIZE |
                  wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.RIGHT | wx.LEFT,
                  border=5) # GISDBASE setting
        sizer.Add(item=location_sizer, proportion=1,
                  flag=wx.ADJUST_MINSIZE |
                  wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.RIGHT | wx.LEFT,
                  border=5)
        sizer.Add(item=btns_sizer, proportion=0,
                  flag=wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.RIGHT | wx.LEFT,
                  border=5)

        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
        sizer.SetSizeHints(self)

        self.Layout()

    def _read_grassrc(self):
        """
        Read variables from $HOME/.grassrc7 file
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

    def _getRCValue(self, value):
        "Return GRASS variable (read from GISRC)"""

        if self.grassrc.has_key(value):
            return self.grassrc[value]
        else:
            return None

    def OnWizard(self, event):
        """Location wizard started"""
        from gui_modules import location_wizard
        gWizard = location_wizard.LocationWizard(self, self.tgisdbase.GetValue())
        if gWizard.location != None:
            self.OnSetDatabase(event)
            self.UpdateMapsets(os.path.join(self.gisdbase, gWizard.location))
            self.lblocations.SetSelection(self.listOfLocations.index(gWizard.location))
            self.lbmapsets.SetSelection(0)

    def OnManageLoc(self, event):
        """
        Location management choice control handler
        """

        if event.GetString() == 'Rename mapset':
            self.RenameMapset()
        elif event.GetString() == 'Rename location':
            self.RenameLocation()
        elif event.GetString() == 'Delete mapset':
            self.DeleteMapset()
        elif event.GetString() == 'Delete location':
            self.DeleteLocation()

    def RenameMapset(self):
        """
        Rename selected mapset
        """

        location = self.listOfLocations[self.lblocations.GetSelection()]
        mapset   = self.listOfMapsets[self.lbmapsets.GetSelection()]

        dlg = wx.TextEntryDialog(parent=self,
                                 message=_('Current name: %s\nEnter new name:') % mapset,
                                 caption=_('Rename selected mapset'))

        if dlg.ShowModal() == wx.ID_OK:
            newmapset = dlg.GetValue()
            if newmapset != mapset:
                try:
                    os.rename(os.path.join(self.gisdbase, location, mapset),
                              os.path.join(self.gisdbase, location, newmapset))
                    self.OnSelectLocation(None)
                    self.lbmapsets.SetSelection(self.listOfMapsets.index(newmapset))
                except:
                    wx.MessageBox(message=_('Unable to rename mapset'))

        dlg.Destroy()

    def RenameLocation(self):
        """
        Rename selected location
        """

        location = self.listOfLocations[self.lblocations.GetSelection()]

        dlg = wx.TextEntryDialog(parent=self,
                                 message=_('Current name: %s\nEnter new name:') % location,
                                 caption=_('Rename selected location'))

        if dlg.ShowModal() == wx.ID_OK:
            newlocation = dlg.GetValue()
            if newlocation != location:
                mapset = self.listOfMapsets[self.lbmapsets.GetSelection()]
                try:
                    os.rename(os.path.join(self.gisdbase, location),
                              os.path.join(self.gisdbase, newlocation))
                    self.UpdateLocations(self.gisdbase)
                    self.lblocations.SetSelection(self.listOfLocations.index(newlocation))
                    self.UpdateMapsets(newlocation)
                except:
                    wx.MessageBox(message=_('Unable to rename location'))

        dlg.Destroy()

    def DeleteMapset(self):
        """
        Delete selected mapset
        """

        location = self.listOfLocations[self.lblocations.GetSelection()]
        mapset   = self.listOfMapsets[self.lbmapsets.GetSelection()]

        dlg = wx.MessageDialog(parent=self, message=_("Do you want to continue with deleting mapset <%(mapset)s> "
                                                      "from location <%(location)s>?\n\n"
                                                      "ALL MAPS included in this mapset will be "
                                                      "PERMANENTLY DELETED!") % {'mapset' : mapset,
                                                                                 'location' : location},
                               caption=_("Delete selected mapset"),
                               style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION)

        if dlg.ShowModal() == wx.ID_YES:
            try:
                shutil.rmtree(os.path.join(self.gisdbase, location, mapset))
                self.OnSelectLocation(None)
                self.lbmapsets.SetSelection(0)
            except:
                wx.MessageBox(message=_('Unable to delete mapset'))

        dlg.Destroy()

    def DeleteLocation(self):
        """
        Delete selected location
        """

        location = self.listOfLocations[self.lblocations.GetSelection()]

        dlg = wx.MessageDialog(parent=self, message=_("Do you want to continue with deleting "
                                                      "location <%s>?\n\n"
                                                      "ALL MAPS included in this location will be "
                                                      "PERMANENTLY DELETED!") % (location),
                               caption=_("Delete selected location"),
                               style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION)

        if dlg.ShowModal() == wx.ID_YES:
            try:
                shutil.rmtree(os.path.join(self.gisdbase, location))
                self.UpdateLocations(self.gisdbase)
                self.lblocations.SetSelection(0)
                self.OnSelectLocation(None)
                self.lbmapsets.SetSelection(0)
            except:
                wx.MessageBox(message=_('Unable to delete location'))

        dlg.Destroy()

    def UpdateLocations(self, dbase):
        """Update list of locations"""
        self.listOfLocations = []

        for location in glob.glob(os.path.join(dbase, "*")):
            try:
                if os.path.join(location, "PERMANENT") in glob.glob(os.path.join(location, "*")):
                    self.listOfLocations.append(os.path.basename(location))
            except:
                pass

        utils.ListSortLower(self.listOfLocations)

        self.lblocations.Clear()
        self.lblocations.InsertItems(self.listOfLocations, 0)

        if len(self.listOfLocations) > 0:
            self.lblocations.SetSelection(0)
        else:
            self.lblocations.SetSelection(wx.NOT_FOUND)

        return self.listOfLocations

    def UpdateMapsets(self, location):
        """Update list of mapsets"""
        self.FormerMapsetSelection = wx.NOT_FOUND # for non-selectable item

        self.listOfMapsets = []
        self.listOfMapsetsSelectable = []
        
        for mapset in glob.glob(os.path.join(self.gisdbase, location, "*")):
            if os.path.isdir(mapset) and \
                    os.path.isfile(os.path.join(self.gisdbase, location, mapset, "WIND")) and \
                    os.path.basename(mapset) != 'PERMANENT':
                self.listOfMapsets.append(os.path.basename(mapset))
        
        utils.ListSortLower(self.listOfMapsets)
        self.listOfMapsets.insert(0, 'PERMANENT')
 
        self.lbmapsets.Clear()

        # disable mapset with denied permission
        locationName = os.path.basename(location)

        try:
            mapsets = gcmd.Command(['g.mapset',
                                    '-l',
                                    'location=%s' % locationName,
                                    'gisdbase=%s' % self.gisdbase],
                                   stderr=None)

            for line in mapsets.ReadStdOutput():
                self.listOfMapsetsSelectable += line.split(' ')
        except gcmd.CmdError:
            gcmd.Command(["g.gisenv",
                          "set=GISDBASE=%s" % self.gisdbase])
            gcmd.Command(["g.gisenv",
                          "set=LOCATION_NAME=%s" % locationName])
            gcmd.Command(["g.gisenv",
                          "set=MAPSET=PERMANENT"])

        disabled = []
        idx = 0
        for mapset in self.listOfMapsets:
            if mapset not in self.listOfMapsetsSelectable:
                disabled.append(idx)
            idx += 1

        self.lbmapsets.InsertItems(self.listOfMapsets, 0, disabled=disabled)
        
        return self.listOfMapsets

    def OnSelectLocation(self, event):
        """Location selected"""
        if event:
            self.lblocations.SetSelection(event.GetIndex())
            
        if self.lblocations.GetSelection() != wx.NOT_FOUND:
            self.UpdateMapsets(os.path.join(self.gisdbase,
                                            self.listOfLocations[self.lblocations.GetSelection()]))
        else:
            self.listOfMapsets = []

        disabled = []
        idx = 0
        for mapset in self.listOfMapsets:
            if mapset not in self.listOfMapsetsSelectable:
                disabled.append(idx)
            idx += 1

        self.lbmapsets.Clear()
        self.lbmapsets.InsertItems(self.listOfMapsets, 0, disabled=disabled)

        if len(self.listOfMapsets) > 0:
            self.lbmapsets.SetSelection(0)
        else:
            self.lbmapsets.SetSelection(wx.NOT_FOUND)

    def OnSelectMapset(self, event):
        """Mapset selected"""
        self.lbmapsets.SetSelection(event.GetIndex())

        if event.GetText() not in self.listOfMapsetsSelectable:
            self.lbmapsets.SetSelection(self.FormerMapsetSelection)
        else:
            self.FormerMapsetSelection = event.GetIndex()
            event.Skip()

    def OnSetDatabase(self, event):
        """Database set"""
        self.gisdbase = self.tgisdbase.GetValue()
        
        self.UpdateLocations(self.gisdbase)

        self.OnSelectLocation(None)

    def OnBrowse(self, event):
        """'Browse' button clicked"""
        grassdata = None

        dlg = wx.DirDialog(self, _("Choose GIS Data Directory:"),
                           style=wx.DD_DEFAULT_STYLE | wx.DD_NEW_DIR_BUTTON)
        if dlg.ShowModal() == wx.ID_OK:
            self.gisdbase = dlg.GetPath()
            self.tgisdbase.SetValue(self.gisdbase)
            self.OnSetDatabase(event)

        dlg.Destroy()

    def OnCreateMapset(self,event):
        """Create new mapset"""
        self.gisdbase = self.tgisdbase.GetValue()
        location = self.listOfLocations[self.lblocations.GetSelection()]

        dlg = wx.TextEntryDialog(parent=self,
                                 message=_('Enter name for new mapset:'),
                                 caption=_('Create new mapset'))

        if dlg.ShowModal() == wx.ID_OK:
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
                dlg = wx.MessageDialog(parent=self, message=_("Unable to create new mapset: %s") % e,
                                       caption=_("Error"), style=wx.OK | wx.ICON_ERROR)
                dlg.ShowModal()
                dlg.Destroy()
                return False

        return True

    def OnStart(self, event):
        """'Start GRASS' button clicked"""
        gcmd.Command(["g.gisenv",
                      "set=GISDBASE=%s" % self.tgisdbase.GetValue()])
        gcmd.Command(["g.gisenv",
                      "set=LOCATION_NAME=%s" % self.listOfLocations[self.lblocations.GetSelection()]])
        gcmd.Command(["g.gisenv",
                      "set=MAPSET=%s" % self.listOfMapsets[self.lbmapsets.GetSelection()]])

        self.Destroy()
        sys.exit(0)

    def OnExit(self, event):
        """'Exit' button clicked"""
        self.Destroy()
        sys.exit (2)

    def OnHelp(self, event):
        """'Help' button clicked"""
        # help text in lib/init/helptext.html
        file=os.path.join(self.gisbase, "docs", "html", "helptext.html")

        helpFrame = HelpWindow(parent=self, id=wx.ID_ANY,
                               title=_("GRASS Quickstart"),
                               size=(640, 480),
                               file=file)
        helpFrame.Show(True)

        event.Skip()

    def OnCloseWindow(self, event):
        """Close window event"""
        event.Skip()
        sys.exit(2)

class HelpWindow(wx.Frame):
    """GRASS Quickstart help window"""
    def __init__(self, parent, id, title, size, file):

        wx.Frame.__init__(self, parent=parent, id=id, title=title, size=size)

        sizer = wx.BoxSizer(wx.VERTICAL)

        # text
        helpFrame = wx.html.HtmlWindow(parent=self, id=wx.ID_ANY)
        helpFrame.SetStandardFonts (size = 10)
        helpFrame.SetBorders(10)
        wx.InitAllImageHandlers()

        helpFrame.LoadFile(file)
        self.Ok = True

        sizer.Add(item=helpFrame, proportion=1, flag=wx.EXPAND)

        self.SetAutoLayout(True)
        self.SetSizer(sizer)
        #        sizer.Fit(self)
        #        sizer.SetSizeHints(self)
        self.Layout()

class GListBox(wx.ListCtrl, listmix.ListCtrlAutoWidthMixin):
    """Use wx.ListCtrl instead of wx.ListBox, different style for
    non-selectable items (e.g. mapsets with denied permission)"""
    def __init__(self, parent, id, size,
                 choices, disabled=[]):

        wx.ListCtrl.__init__(self, parent, id, size=size,
                             style=wx.LC_REPORT | wx.LC_NO_HEADER | wx.LC_SINGLE_SEL |
                             wx.BORDER_SUNKEN)

        listmix.ListCtrlAutoWidthMixin.__init__(self)
        
        self.InsertColumn(0, '')

        self.selected = wx.NOT_FOUND
        
        self.__LoadData(choices, disabled)
        
    def __LoadData(self, choices, disabled=[]):
        """
        Load data into list

        @param choices list of item
        @param disabled list of indeces of non-selectable items
        """
        idx = 0
        for item in choices:
            index = self.InsertStringItem(sys.maxint, item)
            self.SetStringItem(index, 0, item)
            if idx in disabled:
                self.SetItemTextColour(idx, wx.Colour(150, 150, 150))
            idx += 1

    def Clear(self):
        self.DeleteAllItems()

    def InsertItems(self, choices, pos, disabled=[]):
        self.__LoadData(choices, disabled)

    def SetSelection(self, item):
        if item != wx.NOT_FOUND:
            self.SetItemState(item, wx.LIST_STATE_SELECTED, wx.LIST_STATE_SELECTED)
        self.selected = item
        
    def GetSelection(self):
        return self.selected
        
class StartUp(wx.App):
    """Start-up application"""

    def OnInit(self):
        wx.InitAllImageHandlers()
        StartUp = GRASSStartup()
        StartUp.CenterOnScreen()
        self.SetTopWindow(StartUp)
        StartUp.Show()
        return 1

if __name__ == "__main__":

    if os.getenv("GISBASE") is None:
        print >> sys.stderr, "Failed to start GUI, GRASS GIS is not running."
    else:
        import gettext
        gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode=True)

        import gui_modules.gcmd as gcmd
        import gui_modules.utils as utils

        GRASSStartUp = StartUp(0)
        GRASSStartUp.MainLoop()
