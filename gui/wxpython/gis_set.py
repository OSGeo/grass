"""
@package gis_set

GRASS start-up screen.

Initialization module for wxPython GRASS GUI.
Location/mapset management (selection, creation, etc.).

Classes:
 - gis_set::GRASSStartup
 - gis_set::GListBox
 - gis_set::StartUp

(C) 2006-2014 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Michael Barton and Jachym Cepicky (original author)
@author Martin Landa <landa.martin gmail.com> (various updates)
"""

import os
import sys
import copy
import platform

# i18n is taken care of in the grass library code.
# So we need to import it before any of the GUI code.

from core import globalvar
import wx
# import adv and html before wx.App is created, otherwise
# we get annoying "Debug: Adding duplicate image handler for 'Windows bitmap file'"
# during download location dialog start up, remove when not needed
import wx.adv
import wx.html
import wx.lib.mixins.listctrl as listmix

from grass.grassdb.checks import get_lockfile_if_present

from core.gcmd import GError, RunCommand
from core.utils import GetListOfLocations, GetListOfMapsets
from startup.utils import (
    get_possible_database_path,
    create_database_directory,
    create_startup_location_in_grassdb)
from startup.guiutils import (SetSessionMapset,
                              create_mapset_interactively,
                              create_location_interactively,
                              rename_mapset_interactively,
                              rename_location_interactively,
                              delete_mapset_interactively,
                              delete_location_interactively,
                              download_location_interactively)
import startup.guiutils as sgui
from gui_core.widgets import StaticWrapText
from gui_core.wrap import Button, ListCtrl, StaticText, StaticBox, \
    TextCtrl, BitmapFromImage


class GRASSStartup(wx.Frame):
    exit_success = 0
    # 2 is file not found from python interpreter
    exit_user_requested = 5

    """GRASS start-up screen"""

    def __init__(self, parent=None, id=wx.ID_ANY,
                 style=wx.DEFAULT_FRAME_STYLE):

        #
        # GRASS variables
        #
        self.gisbase = os.getenv("GISBASE")
        self.grassrc = sgui.read_gisrc()
        self.gisdbase = self.GetRCValue("GISDBASE")

        #
        # list of locations/mapsets
        #
        self.listOfLocations = []
        self.listOfMapsets = []
        self.listOfMapsetsSelectable = []

        wx.Frame.__init__(self, parent=parent, id=id, style=style)

        self.locale = wx.Locale(language=wx.LANGUAGE_DEFAULT)

        # scroll panel was used here but not properly and is probably not need
        # as long as it is not high too much
        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        # i18N

        #
        # graphical elements
        #
        # image
        try:
            if os.getenv('ISISROOT'):
                name = os.path.join(
                    globalvar.GUIDIR,
                    "images",
                    "startup_banner_isis.png")
            else:
                name = os.path.join(
                    globalvar.GUIDIR, "images", "startup_banner.png")
            self.hbitmap = wx.StaticBitmap(self.panel, wx.ID_ANY,
                                           wx.Bitmap(name=name,
                                                     type=wx.BITMAP_TYPE_PNG))
        except:
            self.hbitmap = wx.StaticBitmap(
                self.panel, wx.ID_ANY, BitmapFromImage(
                    wx.EmptyImage(530, 150)))

        # labels
        # crashes when LOCATION doesn't exist
        # get version & revision
        grassVersion, grassRevisionStr = sgui.GetVersion()

        self.gisdbase_box = StaticBox(
            parent=self.panel, id=wx.ID_ANY, label=" %s " %
            _("1. Select GRASS GIS database directory"))
        self.location_box = StaticBox(
            parent=self.panel, id=wx.ID_ANY, label=" %s " %
            _("2. Select GRASS Location"))
        self.mapset_box = StaticBox(
            parent=self.panel, id=wx.ID_ANY, label=" %s " %
            _("3. Select GRASS Mapset"))

        self.lmessage = StaticWrapText(parent=self.panel)
        # It is not clear if all wx versions supports color, so try-except.
        # The color itself may not be correct for all platforms/system settings
        # but in http://xoomer.virgilio.it/infinity77/wxPython/Widgets/wx.SystemSettings.html
        # there is no 'warning' color.
        try:
            self.lmessage.SetForegroundColour(wx.Colour(255, 0, 0))
        except AttributeError:
            pass

        self.gisdbase_panel = wx.Panel(parent=self.panel)
        self.location_panel = wx.Panel(parent=self.panel)
        self.mapset_panel = wx.Panel(parent=self.panel)

        self.ldbase = StaticText(
            parent=self.gisdbase_panel, id=wx.ID_ANY,
            label=_("GRASS GIS database directory contains Locations."))

        self.llocation = StaticWrapText(
            parent=self.location_panel, id=wx.ID_ANY,
            label=_("All data in one Location is in the same "
                    " coordinate reference system (projection)."
                    " One Location can be one project."
                    " Location contains Mapsets."),
            style=wx.ALIGN_LEFT)

        self.lmapset = StaticWrapText(
            parent=self.mapset_panel, id=wx.ID_ANY,
            label=_("Mapset contains GIS data related"
                    " to one project, task within one project,"
                    " subregion or user."),
            style=wx.ALIGN_LEFT)

        try:
            for label in [self.ldbase, self.llocation, self.lmapset]:
                label.SetForegroundColour(
                    wx.SystemSettings.GetColour(wx.SYS_COLOUR_GRAYTEXT))
        except AttributeError:
            # for explanation of try-except see above
            pass

        # buttons
        self.bstart = Button(parent=self.panel, id=wx.ID_ANY,
                             label=_("Start &GRASS session"))
        self.bstart.SetDefault()
        self.bexit = Button(parent=self.panel, id=wx.ID_EXIT)
        self.bstart.SetMinSize((180, self.bexit.GetSize()[1]))
        self.bhelp = Button(parent=self.panel, id=wx.ID_HELP)
        self.bbrowse = Button(parent=self.gisdbase_panel, id=wx.ID_ANY,
                              label=_("&Browse"))
        self.bmapset = Button(parent=self.mapset_panel, id=wx.ID_ANY,
                              # GTC New mapset
                              label=_("&New"))
        self.bmapset.SetToolTip(_("Create a new Mapset in selected Location"))
        self.bwizard = Button(parent=self.location_panel, id=wx.ID_ANY,
                              # GTC New location
                              label=_("N&ew"))
        self.bwizard.SetToolTip(
            _(
                "Create a new location using location wizard."
                " After location is created successfully,"
                " GRASS session is started."))
        self.rename_location_button = Button(parent=self.location_panel, id=wx.ID_ANY,
                                             # GTC Rename location
                                             label=_("Ren&ame"))
        self.rename_location_button.SetToolTip(_("Rename selected location"))
        self.delete_location_button = Button(parent=self.location_panel, id=wx.ID_ANY,
                                             # GTC Delete location
                                             label=_("De&lete"))
        self.delete_location_button.SetToolTip(_("Delete selected location"))
        self.download_location_button = Button(parent=self.location_panel, id=wx.ID_ANY,
                                             label=_("Do&wnload"))
        self.download_location_button.SetToolTip(_("Download sample location"))

        self.rename_mapset_button = Button(parent=self.mapset_panel, id=wx.ID_ANY,
                                           # GTC Rename mapset
                                           label=_("&Rename"))
        self.rename_mapset_button.SetToolTip(_("Rename selected mapset"))
        self.delete_mapset_button = Button(parent=self.mapset_panel, id=wx.ID_ANY,
                                           # GTC Delete mapset
                                           label=_("&Delete"))
        self.delete_mapset_button.SetToolTip(_("Delete selected mapset"))

        # textinputs
        self.tgisdbase = TextCtrl(
            parent=self.gisdbase_panel, id=wx.ID_ANY, value="", size=(
                300, -1), style=wx.TE_PROCESS_ENTER)

        # Locations
        self.lblocations = GListBox(parent=self.location_panel,
                                    id=wx.ID_ANY, size=(180, 200),
                                    choices=self.listOfLocations)
        self.lblocations.SetColumnWidth(0, 180)

        # TODO: sort; but keep PERMANENT on top of list
        # Mapsets
        self.lbmapsets = GListBox(parent=self.mapset_panel,
                                  id=wx.ID_ANY, size=(180, 200),
                                  choices=self.listOfMapsets)
        self.lbmapsets.SetColumnWidth(0, 180)

        # layout & properties, first do layout so everything is created
        self._do_layout()
        self._set_properties(grassVersion, grassRevisionStr)

        # events
        self.bbrowse.Bind(wx.EVT_BUTTON, self.OnBrowse)
        self.bstart.Bind(wx.EVT_BUTTON, self.OnStart)
        self.bexit.Bind(wx.EVT_BUTTON, self.OnExit)
        self.bhelp.Bind(wx.EVT_BUTTON, self.OnHelp)
        self.bmapset.Bind(wx.EVT_BUTTON, self.OnCreateMapset)
        self.bwizard.Bind(wx.EVT_BUTTON, self.OnCreateLocation)

        self.rename_location_button.Bind(wx.EVT_BUTTON, self.OnRenameLocation)
        self.delete_location_button.Bind(wx.EVT_BUTTON, self.OnDeleteLocation)
        self.download_location_button.Bind(wx.EVT_BUTTON, self.OnDownloadLocation)
        self.rename_mapset_button.Bind(wx.EVT_BUTTON, self.OnRenameMapset)
        self.delete_mapset_button.Bind(wx.EVT_BUTTON, self.OnDeleteMapset)

        self.lblocations.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnSelectLocation)
        self.lbmapsets.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnSelectMapset)
        self.lbmapsets.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.OnStart)
        self.tgisdbase.Bind(wx.EVT_TEXT_ENTER, self.OnSetDatabase)
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

    def _set_properties(self, version, revision):
        """Set frame properties

        :param version: Version in the form of X.Y.Z
        :param revision: Version control revision with leading space

        *revision* should be an empty string in case of release and
        otherwise it needs a leading space to be separated from the rest
        of the title.
        """
        self.SetTitle(_("GRASS GIS %s Startup%s") % (version, revision))
        self.SetIcon(wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"),
                             wx.BITMAP_TYPE_ICO))

        self.bstart.SetToolTip(_("Enter GRASS session"))
        self.bstart.Enable(False)
        self.bmapset.Enable(False)
        # this all was originally a choice, perhaps just mapset needed
        self.rename_location_button.Enable(False)
        self.delete_location_button.Enable(False)
        self.rename_mapset_button.Enable(False)
        self.delete_mapset_button.Enable(False)

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
            wx.MessageBox(parent=self, caption=_("Error"),
                          message=_("Unable to set GRASS database. "
                                    "Check your locale settings."),
                          style=wx.OK | wx.ICON_ERROR | wx.CENTRE)

        self.OnSetDatabase(None)
        location = self.GetRCValue("LOCATION_NAME")
        if location == "<UNKNOWN>" or location is None:
            return
        if not os.path.isdir(os.path.join(self.gisdbase, location)):
            location = None

        # list of locations
        self.UpdateLocations(self.gisdbase)
        try:
            self.lblocations.SetSelection(self.listOfLocations.index(location),
                                          force=True)
            self.lblocations.EnsureVisible(
                self.listOfLocations.index(location))
        except ValueError:
            sys.stderr.write(
                _("ERROR: Location <%s> not found\n") %
                self.GetRCValue("LOCATION_NAME"))
            if len(self.listOfLocations) > 0:
                self.lblocations.SetSelection(0, force=True)
                self.lblocations.EnsureVisible(0)
                location = self.listOfLocations[0]
            else:
                return

        # list of mapsets
        self.UpdateMapsets(os.path.join(self.gisdbase, location))
        mapset = self.GetRCValue("MAPSET")
        if mapset:
            try:
                self.lbmapsets.SetSelection(self.listOfMapsets.index(mapset),
                                            force=True)
                self.lbmapsets.EnsureVisible(self.listOfMapsets.index(mapset))
            except ValueError:
                sys.stderr.write(_("ERROR: Mapset <%s> not found\n") % mapset)
                self.lbmapsets.SetSelection(0, force=True)
                self.lbmapsets.EnsureVisible(0)

    def _do_layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer = sizer  # for the layout call after changing message
        dbase_sizer = wx.BoxSizer(wx.HORIZONTAL)

        location_mapset_sizer = wx.BoxSizer(wx.HORIZONTAL)

        gisdbase_panel_sizer = wx.BoxSizer(wx.VERTICAL)
        gisdbase_boxsizer = wx.StaticBoxSizer(self.gisdbase_box, wx.VERTICAL)

        btns_sizer = wx.BoxSizer(wx.HORIZONTAL)

        self.gisdbase_panel.SetSizer(gisdbase_panel_sizer)

        # gis data directory

        gisdbase_boxsizer.Add(self.gisdbase_panel, proportion=1,
                              flag=wx.EXPAND | wx.ALL,
                              border=1)

        gisdbase_panel_sizer.Add(dbase_sizer, proportion=1,
                                 flag=wx.EXPAND | wx.ALL,
                                 border=1)
        gisdbase_panel_sizer.Add(self.ldbase, proportion=0,
                                 flag=wx.EXPAND | wx.ALL,
                                 border=1)

        dbase_sizer.Add(self.tgisdbase, proportion=1,
                        flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                        border=1)
        dbase_sizer.Add(self.bbrowse, proportion=0,
                        flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                        border=1)

        gisdbase_panel_sizer.Fit(self.gisdbase_panel)

        # location and mapset lists

        def layout_list_box(box, panel, list_box, buttons, description):
            panel_sizer = wx.BoxSizer(wx.VERTICAL)
            main_sizer = wx.BoxSizer(wx.HORIZONTAL)
            box_sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
            buttons_sizer = wx.BoxSizer(wx.VERTICAL)

            panel.SetSizer(panel_sizer)
            panel_sizer.Fit(panel)

            main_sizer.Add(list_box, proportion=1,
                           flag=wx.EXPAND | wx.ALL,
                           border=1)
            main_sizer.Add(buttons_sizer, proportion=0,
                           flag=wx.ALL,
                           border=1)
            for button in buttons:
                buttons_sizer.Add(button, proportion=0,
                                  flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                                  border=3)
            box_sizer.Add(panel, proportion=1,
                          flag=wx.EXPAND | wx.ALL,
                          border=1)
            panel_sizer.Add(main_sizer, proportion=1,
                            flag=wx.EXPAND | wx.ALL,
                            border=1)
            panel_sizer.Add(description, proportion=0,
                            flag=wx.EXPAND | wx.ALL,
                            border=1)
            return box_sizer

        location_boxsizer = layout_list_box(
            box=self.location_box,
            panel=self.location_panel,
            list_box=self.lblocations,
            buttons=[self.bwizard, self.rename_location_button,
                     self.delete_location_button,
                     self.download_location_button],
            description=self.llocation)
        mapset_boxsizer = layout_list_box(
            box=self.mapset_box,
            panel=self.mapset_panel,
            list_box=self.lbmapsets,
            buttons=[self.bmapset, self.rename_mapset_button,
                     self.delete_mapset_button],
            description=self.lmapset)

        # location and mapset sizer
        location_mapset_sizer.Add(location_boxsizer, proportion=1,
                                  flag=wx.LEFT | wx.RIGHT | wx.EXPAND,
                                  border=3)
        location_mapset_sizer.Add(mapset_boxsizer, proportion=1,
                                  flag=wx.RIGHT | wx.EXPAND,
                                  border=3)

        # buttons
        btns_sizer.Add(self.bstart, proportion=0,
                       flag=wx.ALIGN_CENTER_HORIZONTAL |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL,
                       border=5)
        btns_sizer.Add(self.bexit, proportion=0,
                       flag=wx.ALIGN_CENTER_HORIZONTAL |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL,
                       border=5)
        btns_sizer.Add(self.bhelp, proportion=0,
                       flag=wx.ALIGN_CENTER_HORIZONTAL |
                       wx.ALIGN_CENTER_VERTICAL |
                       wx.ALL,
                       border=5)

        # main sizer
        sizer.Add(self.hbitmap,
                  proportion=0,
                  flag=wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.ALL,
                  border=3)  # image
        sizer.Add(gisdbase_boxsizer, proportion=0,
                  flag=wx.RIGHT | wx.LEFT | wx.TOP | wx.EXPAND,
                  border=3)  # GISDBASE setting

        # warning/error message
        sizer.Add(self.lmessage,
                  proportion=0,
                  flag=wx.ALIGN_LEFT | wx.ALL | wx.EXPAND, border=5)
        sizer.Add(location_mapset_sizer, proportion=1,
                  flag=wx.RIGHT | wx.LEFT | wx.EXPAND,
                  border=1)
        sizer.Add(btns_sizer, proportion=0,
                  flag=wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_CENTER_HORIZONTAL |
                  wx.RIGHT | wx.LEFT,
                  border=3)

        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(sizer)
        sizer.Fit(self.panel)
        sizer.SetSizeHints(self)
        self.Layout()

    def _showWarning(self, text):
        """Displays a warning, hint or info message to the user.

        This function can be used for all kinds of messages except for
        error messages.

        .. note::
            There is no cleaning procedure. You should call _hideMessage when
            you know that there is everything correct now.
        """
        self.lmessage.SetLabel(text)
        self.sizer.Layout()

    def _showError(self, text):
        """Displays a error message to the user.

        This function should be used only when something serious and unexpected
        happens, otherwise _showWarning should be used.

        .. note::
            There is no cleaning procedure. You should call _hideMessage when
            you know that there is everything correct now.
        """
        self.lmessage.SetLabel(_("Error: {text}").format(text=text))
        self.sizer.Layout()

    def _hideMessage(self):
        """Clears/hides the error message."""
        # we do no hide widget
        # because we do not want the dialog to change the size
        self.lmessage.SetLabel("")
        self.sizer.Layout()

    def GetRCValue(self, value):
        """Return GRASS variable (read from GISRC)
        """
        if value in self.grassrc:
            return self.grassrc[value]
        else:
            return None

    def SuggestDatabase(self):
        """Suggest (set) possible GRASS Database value"""
        # only if nothing is set (<UNKNOWN> comes from init script)
        if self.GetRCValue("LOCATION_NAME") != "<UNKNOWN>":
            return
        path = get_possible_database_path()

        # If nothing found, try to create GRASS directory and copy startup loc
        if path is None:
            grassdb = create_database_directory()
            location = "world_latlong_wgs84"
            if create_startup_location_in_grassdb(grassdb,
                                                  location):
                self.SetLocation(grassdb, location, "PERMANENT")
                self.ExitSuccessfully()

        if path:
            try:
                self.tgisdbase.SetValue(path)
            except UnicodeDecodeError:
                # restore previous state
                # wizard gives error in this case, we just ignore
                path = None
                self.tgisdbase.SetValue(self.gisdbase)
            # if we still have path
            if path:
                self.gisdbase = path
                self.OnSetDatabase(None)
        else:
            # nothing found
            # TODO: should it be warning, hint or message?
            self._showWarning(_(
                'GRASS needs a directory (GRASS database) '
                'in which to store its data. '
                'Create one now if you have not already done so. '
                'A popular choice is "grassdata", located in '
                'your home directory. '
                'Press Browse button to select the directory.'))

    def OnCreateLocation(self, event):
        """Location wizard started"""
        grassdatabase, location, mapset = (
            create_location_interactively(self, self.gisdbase)
        )
        if location is not None:
            self.OnSelectLocation(None)
            self.lbmapsets.SetSelection(self.listOfMapsets.index(mapset))
            self.bstart.SetFocus()
            self.tgisdbase.SetValue(grassdatabase)
            self.OnSetDatabase(None)
            self.UpdateMapsets(os.path.join(grassdatabase, location))
            self.lblocations.SetSelection(
                self.listOfLocations.index(location))
            self.lbmapsets.SetSelection(0)
            self.SetLocation(grassdatabase, location, mapset)

    # the event can be refactored out by using lambda in bind
    def OnRenameMapset(self, event):
        """Rename selected mapset
        """
        location = self.listOfLocations[self.lblocations.GetSelection()]
        mapset = self.listOfMapsets[self.lbmapsets.GetSelection()]
        try:
            newmapset = rename_mapset_interactively(self, self.gisdbase,
                                                    location, mapset)
            if newmapset:
                self.OnSelectLocation(None)
                self.lbmapsets.SetSelection(
                    self.listOfMapsets.index(newmapset))
        except Exception as e:
            GError(parent=self,
                   message=_("Unable to rename mapset: %s") % e,
                   showTraceback=False)

    def OnRenameLocation(self, event):
        """Rename selected location
        """
        location = self.listOfLocations[self.lblocations.GetSelection()]
        try:
            newlocation = rename_location_interactively(self, self.gisdbase,
                                                        location)
            if newlocation:
                self.UpdateLocations(self.gisdbase)
                self.lblocations.SetSelection(
                    self.listOfLocations.index(newlocation))
                self.UpdateMapsets(newlocation)
        except Exception as e:
            GError(parent=self,
                   message=_("Unable to rename location: %s") % e,
                   showTraceback=False)

    def OnDeleteMapset(self, event):
        """
        Delete selected mapset
        """
        location = self.listOfLocations[self.lblocations.GetSelection()]
        mapset = self.listOfMapsets[self.lbmapsets.GetSelection()]
        if (delete_mapset_interactively(self, self.gisdbase, location, mapset)):
            self.OnSelectLocation(None)
            self.lbmapsets.SetSelection(0)

    def OnDeleteLocation(self, event):
        """
        Delete selected location
        """
        location = self.listOfLocations[self.lblocations.GetSelection()]
        try:
            if (delete_location_interactively(self, self.gisdbase, location)):
                self.UpdateLocations(self.gisdbase)
                self.lblocations.SetSelection(0)
                self.OnSelectLocation(None)
                self.lbmapsets.SetSelection(0)
        except Exception as e:
            GError(parent=self,
                   message=_("Unable to delete location: %s") % e,
                   showTraceback=False)

    def OnDownloadLocation(self, event):
        """
        Download location online
        """
        grassdatabase, location, mapset = download_location_interactively(
            self, self.gisdbase
        )
        if location:
            # get the new location to the list
            self.UpdateLocations(grassdatabase)
            # seems to be used in similar context
            self.UpdateMapsets(os.path.join(grassdatabase, location))
            self.lblocations.SetSelection(
                self.listOfLocations.index(location))
            # wizard does this as well, not sure if needed
            self.SetLocation(grassdatabase, location, mapset)
            # seems to be used in similar context
            self.OnSelectLocation(None)

    def UpdateLocations(self, dbase):
        """Update list of locations"""
        try:
            self.listOfLocations = GetListOfLocations(dbase)
        except (UnicodeEncodeError, UnicodeDecodeError) as e:
            GError(parent=self,
                   message=_("Unicode error detected. "
                             "Check your locale settings. Details: {0}").format(e),
                   showTraceback=False)

        self.lblocations.Clear()
        self.lblocations.InsertItems(self.listOfLocations, 0)

        if len(self.listOfLocations) > 0:
            self._hideMessage()
            self.lblocations.SetSelection(0)
        else:
            self.lblocations.SetSelection(wx.NOT_FOUND)
            self._showWarning(_("No GRASS Location found in '%s'."
                                " Create a new Location or choose different"
                                " GRASS database directory.")
                              % self.gisdbase)

        return self.listOfLocations

    def UpdateMapsets(self, location):
        """Update list of mapsets"""
        self.FormerMapsetSelection = wx.NOT_FOUND  # for non-selectable item

        self.listOfMapsetsSelectable = list()
        self.listOfMapsets = GetListOfMapsets(self.gisdbase, location)

        self.lbmapsets.Clear()

        # disable mapset with denied permission
        locationName = os.path.basename(location)

        ret = RunCommand('g.mapset',
                         read=True,
                         flags='l',
                         location=locationName,
                         gisdbase=self.gisdbase)

        if ret:
            for line in ret.splitlines():
                self.listOfMapsetsSelectable += line.split(' ')
        else:
            self.SetLocation(self.gisdbase, locationName, "PERMANENT")
            # first run only
            self.listOfMapsetsSelectable = copy.copy(self.listOfMapsets)

        disabled = []
        idx = 0
        for mapset in self.listOfMapsets:
            if mapset not in self.listOfMapsetsSelectable or \
                    get_lockfile_if_present(self.gisdbase,
                                            locationName, mapset):
                disabled.append(idx)
            idx += 1

        self.lbmapsets.InsertItems(self.listOfMapsets, 0, disabled=disabled)

        return self.listOfMapsets

    def OnSelectLocation(self, event):
        """Location selected"""
        if event:
            self.lblocations.SetSelection(event.GetIndex())

        if self.lblocations.GetSelection() != wx.NOT_FOUND:
            self.UpdateMapsets(
                os.path.join(
                    self.gisdbase,
                    self.listOfLocations[
                        self.lblocations.GetSelection()]))
        else:
            self.listOfMapsets = []

        disabled = []
        idx = 0
        try:
            locationName = self.listOfLocations[
                self.lblocations.GetSelection()]
        except IndexError:
            locationName = ''

        for mapset in self.listOfMapsets:
            if mapset not in self.listOfMapsetsSelectable or \
                    get_lockfile_if_present(self.gisdbase,
                                            locationName, mapset):
                disabled.append(idx)
            idx += 1

        self.lbmapsets.Clear()
        self.lbmapsets.InsertItems(self.listOfMapsets, 0, disabled=disabled)

        if len(self.listOfMapsets) > 0:
            self.lbmapsets.SetSelection(0)
            if locationName:
                # enable start button when location and mapset is selected
                self.bstart.Enable()
                self.bstart.SetFocus()
                self.bmapset.Enable()
                # replacing disabled choice, perhaps just mapset needed
                self.rename_location_button.Enable()
                self.delete_location_button.Enable()
                self.rename_mapset_button.Enable()
                self.delete_mapset_button.Enable()
        else:
            self.lbmapsets.SetSelection(wx.NOT_FOUND)
            self.bstart.Enable(False)
            self.bmapset.Enable(False)
            # this all was originally a choice, perhaps just mapset needed
            self.rename_location_button.Enable(False)
            self.delete_location_button.Enable(False)
            self.rename_mapset_button.Enable(False)
            self.delete_mapset_button.Enable(False)

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
        gisdbase = self.tgisdbase.GetValue()
        self._hideMessage()
        if not os.path.exists(gisdbase):
            self._showError(_("Path '%s' doesn't exist.") % gisdbase)
            return

        self.gisdbase = self.tgisdbase.GetValue()
        self.UpdateLocations(self.gisdbase)

        self.OnSelectLocation(None)

    def OnBrowse(self, event):
        """'Browse' button clicked"""
        if not event:
            defaultPath = os.getenv('HOME')
        else:
            defaultPath = ""

        dlg = wx.DirDialog(parent=self, message=_("Choose GIS Data Directory"),
                           defaultPath=defaultPath, style=wx.DD_DEFAULT_STYLE)

        if dlg.ShowModal() == wx.ID_OK:
            self.gisdbase = dlg.GetPath()
            self.tgisdbase.SetValue(self.gisdbase)
            self.OnSetDatabase(event)

        dlg.Destroy()

    def OnCreateMapset(self, event):
        """Create new mapset"""
        gisdbase = self.tgisdbase.GetValue()
        location = self.listOfLocations[self.lblocations.GetSelection()]
        try:
            mapset = create_mapset_interactively(self, gisdbase, location)
            if mapset:
                self.OnSelectLocation(None)
                self.lbmapsets.SetSelection(self.listOfMapsets.index(mapset))
                self.bstart.SetFocus()
        except Exception as e:
            GError(parent=self,
                   message=_("Unable to create new mapset: %s") % e,
                   showTraceback=False)

    def OnStart(self, event):
        """'Start GRASS' button clicked"""
        dbase = self.tgisdbase.GetValue()
        location = self.listOfLocations[self.lblocations.GetSelection()]
        mapset = self.listOfMapsets[self.lbmapsets.GetSelection()]

        lockfile = get_lockfile_if_present(dbase, location, mapset)
        if lockfile:
            dlg = wx.MessageDialog(
                parent=self,
                message=_(
                    "GRASS is already running in selected mapset <%(mapset)s>\n"
                    "(file %(lock)s found).\n\n"
                    "Concurrent use not allowed.\n\n"
                    "Do you want to try to remove .gislock (note that you "
                    "need permission for this operation) and continue?") %
                {'mapset': mapset, 'lock': lockfile},
                caption=_("Lock file found"),
                style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE)

            ret = dlg.ShowModal()
            dlg.Destroy()
            if ret == wx.ID_YES:
                dlg1 = wx.MessageDialog(
                    parent=self,
                    message=_(
                        "ARE YOU REALLY SURE?\n\n"
                        "If you really are running another GRASS session doing this "
                        "could corrupt your data. Have another look in the processor "
                        "manager just to be sure..."),
                    caption=_("Lock file found"),
                    style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION | wx.CENTRE)

                ret = dlg1.ShowModal()
                dlg1.Destroy()

                if ret == wx.ID_YES:
                    try:
                        os.remove(lockfile)
                    except IOError as e:
                        GError(_("Unable to remove '%(lock)s'.\n\n"
                                 "Details: %(reason)s") % {'lock': lockfile, 'reason': e})
                else:
                    return
            else:
                return
        self.SetLocation(dbase, location, mapset)
        self.ExitSuccessfully()

    def SetLocation(self, dbase, location, mapset):
        SetSessionMapset(dbase, location, mapset)

    def ExitSuccessfully(self):
        self.Destroy()
        sys.exit(self.exit_success)

    def OnExit(self, event):
        """'Exit' button clicked"""
        self.Destroy()
        sys.exit(self.exit_user_requested)

    def OnHelp(self, event):
        """'Help' button clicked"""

        # help text in lib/init/helptext.html
        RunCommand('g.manual', entry='helptext')

    def OnCloseWindow(self, event):
        """Close window event"""
        event.Skip()
        sys.exit(self.exit_user_requested)


class GListBox(ListCtrl, listmix.ListCtrlAutoWidthMixin):
    """Use wx.ListCtrl instead of wx.ListBox, different style for
    non-selectable items (e.g. mapsets with denied permission)"""

    def __init__(self, parent, id, size,
                 choices, disabled=[]):
        ListCtrl.__init__(
            self, parent, id, size=size, style=wx.LC_REPORT | wx.LC_NO_HEADER |
            wx.LC_SINGLE_SEL | wx.BORDER_SUNKEN)

        listmix.ListCtrlAutoWidthMixin.__init__(self)

        self.InsertColumn(0, '')

        self.selected = wx.NOT_FOUND

        self._LoadData(choices, disabled)

    def _LoadData(self, choices, disabled=[]):
        """Load data into list

        :param choices: list of item
        :param disabled: list of indices of non-selectable items
        """
        idx = 0
        count = self.GetItemCount()
        for item in choices:
            index = self.InsertItem(count + idx, item)
            self.SetItem(index, 0, item)

            if idx in disabled:
                self.SetItemTextColour(idx, wx.Colour(150, 150, 150))
            idx += 1

    def Clear(self):
        self.DeleteAllItems()

    def InsertItems(self, choices, pos, disabled=[]):
        self._LoadData(choices, disabled)

    def SetSelection(self, item, force=False):
        if item != wx.NOT_FOUND and \
                (platform.system() != 'Windows' or force):
            # Windows -> FIXME
            self.SetItemState(
                item,
                wx.LIST_STATE_SELECTED,
                wx.LIST_STATE_SELECTED)

        self.selected = item

    def GetSelection(self):
        return self.selected


class StartUp(wx.App):
    """Start-up application"""

    def OnInit(self):
        StartUp = GRASSStartup()
        StartUp.CenterOnScreen()
        self.SetTopWindow(StartUp)
        StartUp.Show()
        StartUp.SuggestDatabase()

        return 1


if __name__ == "__main__":
    if os.getenv("GISBASE") is None:
        sys.exit("Failed to start GUI, GRASS GIS is not running.")

    GRASSStartUp = StartUp(0)
    GRASSStartUp.MainLoop()
