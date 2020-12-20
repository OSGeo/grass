"""
@package gis_set

GRASS start-up screen.

Initialization module for wxPython GRASS GUI.
Project/subproject management (selection, creation, etc.).

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
import shutil
import copy
import platform
import codecs
import getpass

from core import globalvar
import wx
import wx.lib.mixins.listctrl as listmix

from grass.script import core as grass

from core.gcmd import GMessage, GError, DecodeString, RunCommand
from core.utils import GetListOfProjects, GetListOfSubprojects
from project_wizard.dialogs import RegionDef
from gui_core.dialogs import TextEntryDialog
from gui_core.widgets import GenericValidator, StaticWrapText
from gui_core.wrap import Button, ListCtrl, StaticText, \
    StaticBox, TextCtrl, BitmapFromImage


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
        self.grassrc = self._readGisRC()
        self.gisdbase = self.GetRCValue("GISDBASE")

        #
        # list of projects/subprojects
        #
        self.listOfProjects = []
        self.listOfSubprojects = []
        self.listOfSubprojectsSelectable = []

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
        versionFile = open(os.path.join(globalvar.ETCDIR, "VERSIONNUMBER"))
        versionLine = versionFile.readline().rstrip('\n')
        versionFile.close()
        try:
            grassVersion, grassRevision = versionLine.split(' ', 1)
            if grassVersion.endswith('dev'):
                grassRevisionStr = ' (%s)' % grassRevision
            else:
                grassRevisionStr = ''
        except ValueError:
            grassVersion = versionLine
            grassRevisionStr = ''

        self.gisdbase_box = StaticBox(
            parent=self.panel, id=wx.ID_ANY, label=" %s " %
            _("1. Select GRASS GIS database directory"))
        self.project_box = StaticBox(
            parent=self.panel, id=wx.ID_ANY, label=" %s " %
            _("2. Select GRASS Project"))
        self.subproject_box = StaticBox(
            parent=self.panel, id=wx.ID_ANY, label=" %s " %
            _("3. Select GRASS Subproject"))

        self.lmessage = StaticText(parent=self.panel)
        # It is not clear if all wx versions supports color, so try-except.
        # The color itself may not be correct for all platforms/system settings
        # but in http://xoomer.virgilio.it/infinity77/wxPython/Widgets/wx.SystemSettings.html
        # there is no 'warning' color.
        try:
            self.lmessage.SetForegroundColour(wx.Colour(255, 0, 0))
        except AttributeError:
            pass

        self.gisdbase_panel = wx.Panel(parent=self.panel)
        self.project_panel = wx.Panel(parent=self.panel)
        self.subproject_panel = wx.Panel(parent=self.panel)

        self.ldbase = StaticText(
            parent=self.gisdbase_panel, id=wx.ID_ANY,
            label=_("GRASS GIS database directory contains Projects."))

        self.lproject = StaticWrapText(
            parent=self.project_panel, id=wx.ID_ANY,
            label=_("All data in one Project is in the same "
                    " coordinate reference system (projection)."
                    " One Project can be one project."
                    " Project contains Subprojects."),
            style=wx.ALIGN_LEFT)

        self.lsubproject = StaticWrapText(
            parent=self.subproject_panel, id=wx.ID_ANY,
            label=_("Subproject contains GIS data related"
                    " to one project, task within one project,"
                    " subregion or user."),
            style=wx.ALIGN_LEFT)

        try:
            for label in [self.ldbase, self.lproject, self.lsubproject]:
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
        self.bsubproject = Button(parent=self.subproject_panel, id=wx.ID_ANY,
                              # GTC New subproject
                              label=_("&New"))
        self.bsubproject.SetToolTip(_("Create a new Subproject in selected Project"))
        self.bwizard = Button(parent=self.project_panel, id=wx.ID_ANY,
                              # GTC New project
                              label=_("N&ew"))
        self.bwizard.SetToolTip(
            _(
                "Create a new project using project wizard."
                " After project is created successfully,"
                " GRASS session is started."))
        self.rename_project_button = Button(parent=self.project_panel, id=wx.ID_ANY,
                                             # GTC Rename project
                                             label=_("Ren&ame"))
        self.rename_project_button.SetToolTip(_("Rename selected project"))
        self.delete_project_button = Button(parent=self.project_panel, id=wx.ID_ANY,
                                             # GTC Delete project
                                             label=_("De&lete"))
        self.delete_project_button.SetToolTip(_("Delete selected project"))
        self.rename_subproject_button = Button(parent=self.subproject_panel, id=wx.ID_ANY,
                                           # GTC Rename subproject
                                           label=_("&Rename"))
        self.rename_subproject_button.SetToolTip(_("Rename selected subproject"))
        self.delete_subproject_button = Button(parent=self.subproject_panel, id=wx.ID_ANY,
                                           # GTC Delete subproject
                                           label=_("&Delete"))
        self.delete_subproject_button.SetToolTip(_("Delete selected subproject"))

        # textinputs
        self.tgisdbase = TextCtrl(
            parent=self.gisdbase_panel, id=wx.ID_ANY, value="", size=(
                300, -1), style=wx.TE_PROCESS_ENTER)

        # Projects
        self.lbprojects = GListBox(parent=self.project_panel,
                                    id=wx.ID_ANY, size=(180, 200),
                                    choices=self.listOfProjects)
        self.lbprojects.SetColumnWidth(0, 180)

        # TODO: sort; but keep PERMANENT on top of list
        # Subprojects
        self.lbsubprojects = GListBox(parent=self.subproject_panel,
                                  id=wx.ID_ANY, size=(180, 200),
                                  choices=self.listOfSubprojects)
        self.lbsubprojects.SetColumnWidth(0, 180)

        # layout & properties, first do layout so everything is created
        self._do_layout()
        self._set_properties(grassVersion, grassRevisionStr)

        # events
        self.bbrowse.Bind(wx.EVT_BUTTON, self.OnBrowse)
        self.bstart.Bind(wx.EVT_BUTTON, self.OnStart)
        self.bexit.Bind(wx.EVT_BUTTON, self.OnExit)
        self.bhelp.Bind(wx.EVT_BUTTON, self.OnHelp)
        self.bsubproject.Bind(wx.EVT_BUTTON, self.OnCreateSubproject)
        self.bwizard.Bind(wx.EVT_BUTTON, self.OnWizard)

        self.rename_project_button.Bind(wx.EVT_BUTTON, self.RenameProject)
        self.delete_project_button.Bind(wx.EVT_BUTTON, self.DeleteProject)
        self.rename_subproject_button.Bind(wx.EVT_BUTTON, self.RenameSubproject)
        self.delete_subproject_button.Bind(wx.EVT_BUTTON, self.DeleteSubproject)

        self.lbprojects.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnSelectProject)
        self.lbsubprojects.Bind(wx.EVT_LIST_ITEM_SELECTED, self.OnSelectSubproject)
        self.lbsubprojects.Bind(wx.EVT_LIST_ITEM_ACTIVATED, self.OnStart)
        self.tgisdbase.Bind(wx.EVT_TEXT_ENTER, self.OnSetDatabase)
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)

    def _set_properties(self, version, revision):
        """Set frame properties"""
        self.SetTitle(_("GRASS GIS %s startup%s") % (version, revision))
        self.SetIcon(wx.Icon(os.path.join(globalvar.ICONDIR, "grass.ico"),
                             wx.BITMAP_TYPE_ICO))

        self.bstart.SetToolTip(_("Enter GRASS session"))
        self.bstart.Enable(False)
        self.bsubproject.Enable(False)
        # this all was originally a choice, perhaps just subproject needed
        self.rename_project_button.Enable(False)
        self.delete_project_button.Enable(False)
        self.rename_subproject_button.Enable(False)
        self.delete_subproject_button.Enable(False)

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
        project = self.GetRCValue("LOCATION_NAME")
        if project == "<UNKNOWN>":
            return
        if not os.path.isdir(os.path.join(self.gisdbase, project)):
            project = None

        # list of projects
        self.UpdateProjects(self.gisdbase)
        try:
            self.lbprojects.SetSelection(self.listOfProjects.index(project),
                                          force=True)
            self.lbprojects.EnsureVisible(
                self.listOfProjects.index(project))
        except ValueError:
            sys.stderr.write(
                _("ERROR: Project <%s> not found\n") %
                self.GetRCValue("LOCATION_NAME"))
            if len(self.listOfProjects) > 0:
                self.lbprojects.SetSelection(0, force=True)
                self.lbprojects.EnsureVisible(0)
                project = self.listOfProjects[0]
            else:
                return

        # list of subprojects
        self.UpdateSubprojects(os.path.join(self.gisdbase, project))
        subproject = self.GetRCValue("MAPSET")
        if subproject:
            try:
                self.lbsubprojects.SetSelection(self.listOfSubprojects.index(subproject),
                                            force=True)
                self.lbsubprojects.EnsureVisible(self.listOfSubprojects.index(subproject))
            except ValueError:
                sys.stderr.write(_("ERROR: Subproject <%s> not found\n") % subproject)
                self.lbsubprojects.SetSelection(0, force=True)
                self.lbsubprojects.EnsureVisible(0)

    def _do_layout(self):
        sizer = wx.BoxSizer(wx.VERTICAL)
        self.sizer = sizer  # for the layout call after changing message
        dbase_sizer = wx.BoxSizer(wx.HORIZONTAL)

        project_subproject_sizer = wx.BoxSizer(wx.HORIZONTAL)

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

        # project and subproject lists

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
                           flag=wx.ALIGN_CENTER_HORIZONTAL | wx.ALL,
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

        project_boxsizer = layout_list_box(
            box=self.project_box,
            panel=self.project_panel,
            list_box=self.lbprojects,
            buttons=[self.bwizard, self.rename_project_button,
                     self.delete_project_button],
            description=self.lproject)
        subproject_boxsizer = layout_list_box(
            box=self.subproject_box,
            panel=self.subproject_panel,
            list_box=self.lbsubprojects,
            buttons=[self.bsubproject, self.rename_subproject_button,
                     self.delete_subproject_button],
            description=self.lsubproject)

        # project and subproject sizer
        project_subproject_sizer.Add(project_boxsizer, proportion=1,
                                  flag=wx.LEFT | wx.RIGHT | wx.EXPAND,
                                  border=3)
        project_subproject_sizer.Add(subproject_boxsizer, proportion=1,
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
                  flag=wx.ALIGN_CENTER_HORIZONTAL |
                  wx.RIGHT | wx.LEFT | wx.TOP | wx.EXPAND,
                  border=3)  # GISDBASE setting

        # warning/error message
        sizer.Add(self.lmessage,
                  proportion=0,
                  flag=wx.ALIGN_CENTER_VERTICAL |
                  wx.ALIGN_LEFT | wx.ALL | wx.EXPAND, border=5)
        sizer.Add(project_subproject_sizer, proportion=1,
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

    def _readGisRC(self):
        """Read variables from $HOME/.grass7/rc file
        """
        grassrc = {}

        gisrc = os.getenv("GISRC")

        if gisrc and os.path.isfile(gisrc):
            try:
                rc = open(gisrc, "r")
                for line in rc.readlines():
                    try:
                        key, val = line.split(":", 1)
                    except ValueError as e:
                        sys.stderr.write(
                            _('Invalid line in GISRC file (%s):%s\n' % (e, line)))
                    grassrc[key.strip()] = DecodeString(val.strip())
            finally:
                rc.close()

        return grassrc

    def _showWarning(self, text):
        """Displays a warning, hint or info message to the user.

        This function can be used for all kinds of messages except for
        error messages.

        .. note::
            There is no cleaning procedure. You should call _hideMessage when
            you know that there is everything correct now.
        """
        self.lmessage.SetLabel(text)
        self.lmessage.Wrap(self.GetClientSize()[0])
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
        self.lmessage.Wrap(self.GetClientSize()[0])
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

    def OnWizard(self, event):
        """Project wizard started"""
        from project_wizard.wizard import ProjectWizard
        gWizard = ProjectWizard(parent=self,
                                 grassdatabase=self.tgisdbase.GetValue())
        if gWizard.project is not None:
            self.tgisdbase.SetValue(gWizard.grassdatabase)
            self.OnSetDatabase(None)
            self.UpdateSubprojects(os.path.join(self.gisdbase, gWizard.project))
            self.lbprojects.SetSelection(
                self.listOfProjects.index(
                    gWizard.project))
            self.lbsubprojects.SetSelection(0)
            self.SetProject(self.gisdbase, gWizard.project, 'PERMANENT')
            if gWizard.georeffile:
                message = _("Do you want to import <%(name)s> to the newly created project?") % {
                    'name': gWizard.georeffile}
                dlg = wx.MessageDialog(parent=self, message=message, caption=_(
                    "Import data?"), style=wx.YES_NO | wx.YES_DEFAULT | wx.ICON_QUESTION)
                dlg.CenterOnParent()
                if dlg.ShowModal() == wx.ID_YES:
                    self.ImportFile(gWizard.georeffile)
                dlg.Destroy()
            if gWizard.default_region:
                defineRegion = RegionDef(self, project=gWizard.project)
                defineRegion.CenterOnParent()
                defineRegion.ShowModal()
                defineRegion.Destroy()

            if gWizard.user_subproject:
                dlg = TextEntryDialog(
                    parent=self,
                    message=_("New subproject:"),
                    caption=_("Create new subproject"),
                    defaultValue=self._getDefaultSubprojectName(),
                    validator=GenericValidator(
                        grass.legal_name,
                        self._nameValidationFailed
                    ),
                    style=wx.OK | wx.CANCEL | wx.HELP
                )
                help = dlg.FindWindowById(wx.ID_HELP)
                help.Bind(wx.EVT_BUTTON, self.OnHelp)
                if dlg.ShowModal() == wx.ID_OK:
                    subprojectName = dlg.GetValue()
                    self.CreateNewSubproject(subprojectName)

    def ImportFile(self, filePath):
        """Tries to import file as vector or raster.

        If successfull sets default region from imported map.
        """
        RunCommand('db.connect', flags='c')
        mapName = os.path.splitext(os.path.basename(filePath))[0]
        vectors = RunCommand('v.in.ogr', input=filePath, flags='l',
                             read=True)

        wx.BeginBusyCursor()
        wx.GetApp().Yield()
        if mapName in vectors:
            # vector detected
            returncode, error = RunCommand(
                'v.in.ogr', input=filePath, output=mapName, flags='e',
                getErrorMsg=True)
        else:
            returncode, error = RunCommand(
                'r.in.gdal', input=filePath, output=mapName, flags='e',
                getErrorMsg=True)
        wx.EndBusyCursor()

        if returncode != 0:
            GError(
                parent=self,
                message=_(
                    "Import of <%(name)s> failed.\n"
                    "Reason: %(msg)s") % ({
                        'name': filePath,
                        'msg': error}))
        else:
            GMessage(
                message=_(
                    "Data file <%(name)s> imported successfully. "
                    "The project's default region was set from this imported map.") % {
                    'name': filePath},
                parent=self)

    # the event can be refactored out by using lambda in bind
    def RenameSubproject(self, event):
        """Rename selected subproject
        """
        project = self.listOfProjects[self.lbprojects.GetSelection()]
        subproject = self.listOfSubprojects[self.lbsubprojects.GetSelection()]
        if subproject == 'PERMANENT':
            GMessage(
                parent=self, message=_(
                    'Subproject <PERMANENT> is required for valid GRASS project.\n\n'
                    'This subproject cannot be renamed.'))
            return

        dlg = TextEntryDialog(
            parent=self,
            message=_('Current name: %s\n\nEnter new name:') %
            subproject,
            caption=_('Rename selected subproject'),
            validator=GenericValidator(
                grass.legal_name,
                self._nameValidationFailed))

        if dlg.ShowModal() == wx.ID_OK:
            newsubproject = dlg.GetValue()
            if newsubproject == subproject:
                dlg.Destroy()
                return

            if newsubproject in self.listOfSubprojects:
                wx.MessageBox(
                    parent=self, caption=_('Message'), message=_(
                        'Unable to rename subproject.\n\n'
                        'Subproject <%s> already exists in project.') %
                    newsubproject, style=wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
            else:
                try:
                    os.rename(os.path.join(self.gisdbase, project, subproject),
                              os.path.join(self.gisdbase, project, newsubproject))
                    self.OnSelectProject(None)
                    self.lbsubprojects.SetSelection(
                        self.listOfSubprojects.index(newsubproject))
                except Exception as e:
                    wx.MessageBox(
                        parent=self,
                        caption=_('Error'),
                        message=_('Unable to rename subproject.\n\n%s') %
                        e,
                        style=wx.OK | wx.ICON_ERROR | wx.CENTRE)

        dlg.Destroy()

    def RenameProject(self, event):
        """Rename selected project
        """
        project = self.listOfProjects[self.lbprojects.GetSelection()]

        dlg = TextEntryDialog(
            parent=self,
            message=_('Current name: %s\n\nEnter new name:') %
            project,
            caption=_('Rename selected project'),
            validator=GenericValidator(
                grass.legal_name,
                self._nameValidationFailed))

        if dlg.ShowModal() == wx.ID_OK:
            newproject = dlg.GetValue()
            if newproject == project:
                dlg.Destroy()
                return

            if newproject in self.listOfProjects:
                wx.MessageBox(
                    parent=self, caption=_('Message'), message=_(
                        'Unable to rename project.\n\n'
                        'Project <%s> already exists in GRASS database.') %
                    newproject, style=wx.OK | wx.ICON_INFORMATION | wx.CENTRE)
            else:
                try:
                    os.rename(os.path.join(self.gisdbase, project),
                              os.path.join(self.gisdbase, newproject))
                    self.UpdateProjects(self.gisdbase)
                    self.lbprojects.SetSelection(
                        self.listOfProjects.index(newproject))
                    self.UpdateSubprojects(newproject)
                except Exception as e:
                    wx.MessageBox(
                        parent=self,
                        caption=_('Error'),
                        message=_('Unable to rename project.\n\n%s') %
                        e,
                        style=wx.OK | wx.ICON_ERROR | wx.CENTRE)

        dlg.Destroy()

    def DeleteSubproject(self, event):
        """Delete selected subproject
        """
        project = self.listOfProjects[self.lbprojects.GetSelection()]
        subproject = self.listOfSubprojects[self.lbsubprojects.GetSelection()]
        if subproject == 'PERMANENT':
            GMessage(
                parent=self, message=_(
                    'Subproject <PERMANENT> is required for valid GRASS project.\n\n'
                    'This subproject cannot be deleted.'))
            return

        dlg = wx.MessageDialog(
            parent=self,
            message=_(
                "Do you want to continue with deleting subproject <%(subproject)s> "
                "from project <%(project)s>?\n\n"
                "ALL MAPS included in this subproject will be "
                "PERMANENTLY DELETED!") %
            {'subproject': subproject, 'project': project},
            caption=_("Delete selected subproject"),
            style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION)

        if dlg.ShowModal() == wx.ID_YES:
            try:
                shutil.rmtree(os.path.join(self.gisdbase, project, subproject))
                self.OnSelectProject(None)
                self.lbsubprojects.SetSelection(0)
            except:
                wx.MessageBox(message=_('Unable to delete subproject'))

        dlg.Destroy()

    def DeleteProject(self, event):
        """
        Delete selected project
        """

        project = self.listOfProjects[self.lbprojects.GetSelection()]

        dlg = wx.MessageDialog(
            parent=self,
            message=_(
                "Do you want to continue with deleting "
                "project <%s>?\n\n"
                "ALL MAPS included in this project will be "
                "PERMANENTLY DELETED!") %
            (project),
            caption=_("Delete selected project"),
            style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION)

        if dlg.ShowModal() == wx.ID_YES:
            try:
                shutil.rmtree(os.path.join(self.gisdbase, project))
                self.UpdateProjects(self.gisdbase)
                self.lbprojects.SetSelection(0)
                self.OnSelectProject(None)
                self.lbsubprojects.SetSelection(0)
            except:
                wx.MessageBox(message=_('Unable to delete project'))

        dlg.Destroy()

    def UpdateProjects(self, dbase):
        """Update list of projects"""
        try:
            self.listOfProjects = GetListOfProjects(dbase)
        except (UnicodeEncodeError, UnicodeDecodeError) as e:
            GError(parent=self,
                   message=_("Unicode error detected. "
                             "Check your locale settings. Details: {0}").format(e),
                   showTraceback=False)

        self.lbprojects.Clear()
        self.lbprojects.InsertItems(self.listOfProjects, 0)

        if len(self.listOfProjects) > 0:
            self._hideMessage()
            self.lbprojects.SetSelection(0)
        else:
            self.lbprojects.SetSelection(wx.NOT_FOUND)
            self._showWarning(_("No GRASS Project found in '%s'."
                                " Create a new Project or choose different"
                                " GRASS database directory.")
                              % self.gisdbase)

        return self.listOfProjects

    def UpdateSubprojects(self, project):
        """Update list of subprojects"""
        self.FormerSubprojectSelection = wx.NOT_FOUND  # for non-selectable item

        self.listOfSubprojectsSelectable = list()
        self.listOfSubprojects = GetListOfSubprojects(self.gisdbase, project)

        self.lbsubprojects.Clear()

        # disable subproject with denied permission
        projectName = os.path.basename(project)

        ret = RunCommand('g.subproject',
                         read=True,
                         flags='l',
                         project=projectName,
                         gisdbase=self.gisdbase)

        if ret:
            for line in ret.splitlines():
                self.listOfSubprojectsSelectable += line.split(' ')
        else:
            RunCommand("g.gisenv",
                       set="GISDBASE=%s" % self.gisdbase)
            RunCommand("g.gisenv",
                       set="LOCATION_NAME=%s" % projectName)
            RunCommand("g.gisenv",
                       set="MAPSET=PERMANENT")
            # first run only
            self.listOfSubprojectsSelectable = copy.copy(self.listOfSubprojects)

        disabled = []
        idx = 0
        for subproject in self.listOfSubprojects:
            if subproject not in self.listOfSubprojectsSelectable or \
                    os.path.isfile(os.path.join(self.gisdbase,
                                                projectName,
                                                subproject, ".gislock")):
                disabled.append(idx)
            idx += 1

        self.lbsubprojects.InsertItems(self.listOfSubprojects, 0, disabled=disabled)

        return self.listOfSubprojects

    def OnSelectProject(self, event):
        """Project selected"""
        if event:
            self.lbprojects.SetSelection(event.GetIndex())

        if self.lbprojects.GetSelection() != wx.NOT_FOUND:
            self.UpdateSubprojects(
                os.path.join(
                    self.gisdbase,
                    self.listOfProjects[
                        self.lbprojects.GetSelection()]))
        else:
            self.listOfSubprojects = []

        disabled = []
        idx = 0
        try:
            projectName = self.listOfProjects[
                self.lbprojects.GetSelection()]
        except IndexError:
            projectName = ''

        for subproject in self.listOfSubprojects:
            if subproject not in self.listOfSubprojectsSelectable or \
                    os.path.isfile(os.path.join(self.gisdbase,
                                                projectName,
                                                subproject, ".gislock")):
                disabled.append(idx)
            idx += 1

        self.lbsubprojects.Clear()
        self.lbsubprojects.InsertItems(self.listOfSubprojects, 0, disabled=disabled)

        if len(self.listOfSubprojects) > 0:
            self.lbsubprojects.SetSelection(0)
            if projectName:
                # enable start button when project and subproject is selected
                self.bstart.Enable()
                self.bstart.SetFocus()
                self.bsubproject.Enable()
                # replacing disabled choice, perhaps just subproject needed
                self.rename_project_button.Enable()
                self.delete_project_button.Enable()
                self.rename_subproject_button.Enable()
                self.delete_subproject_button.Enable()
        else:
            self.lbsubprojects.SetSelection(wx.NOT_FOUND)
            self.bstart.Enable(False)
            self.bsubproject.Enable(False)
            # this all was originally a choice, perhaps just subproject needed
            self.rename_project_button.Enable(False)
            self.delete_project_button.Enable(False)
            self.rename_subproject_button.Enable(False)
            self.delete_subproject_button.Enable(False)

    def OnSelectSubproject(self, event):
        """Subproject selected"""
        self.lbsubprojects.SetSelection(event.GetIndex())

        if event.GetText() not in self.listOfSubprojectsSelectable:
            self.lbsubprojects.SetSelection(self.FormerSubprojectSelection)
        else:
            self.FormerSubprojectSelection = event.GetIndex()
            event.Skip()

    def OnSetDatabase(self, event):
        """Database set"""
        gisdbase = self.tgisdbase.GetValue()
        self._hideMessage()
        if not os.path.exists(gisdbase):
            self._showError(_("Path '%s' doesn't exist.") % gisdbase)
            return

        self.gisdbase = self.tgisdbase.GetValue()
        self.UpdateProjects(self.gisdbase)

        self.OnSelectProject(None)

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

    def OnCreateSubproject(self, event):
        """Create new subproject"""
        dlg = TextEntryDialog(
            parent=self,
            message=_('Enter name for new subproject:'),
            caption=_('Create new subproject'),
            defaultValue=self._getDefaultSubprojectName(),
            validator=GenericValidator(
                grass.legal_name,
                self._nameValidationFailed))
        if dlg.ShowModal() == wx.ID_OK:
            subproject = dlg.GetValue()
            return self.CreateNewSubproject(subproject=subproject)
        else:
            return False

    def CreateNewSubproject(self, subproject):
        if subproject in self.listOfSubprojects:
            GMessage(parent=self,
                     message=_("Subproject <%s> already exists.") % subproject)
            return False

        if subproject.lower() == 'ogr':
            dlg1 = wx.MessageDialog(
                parent=self,
                message=_(
                    "Subproject <%s> is reserved for direct "
                    "read access to OGR layers. Please consider to use "
                    "another name for your subproject.\n\n"
                    "Are you really sure that you want to create this subproject?") %
                subproject,
                caption=_("Reserved subproject name"),
                style=wx.YES_NO | wx.NO_DEFAULT | wx.ICON_QUESTION)
            ret = dlg1.ShowModal()
            dlg1.Destroy()
            if ret == wx.ID_NO:
                dlg1.Destroy()
                return False

        try:
            self.gisdbase = self.tgisdbase.GetValue()
            project = self.listOfProjects[self.lbprojects.GetSelection()]
            os.mkdir(os.path.join(self.gisdbase, project, subproject))
            # copy WIND file and its permissions from PERMANENT and set
            # permissions to u+rw,go+r
            shutil.copy(
                os.path.join(
                    self.gisdbase,
                    project,
                    'PERMANENT',
                    'WIND'),
                os.path.join(
                    self.gisdbase,
                    project,
                    subproject))
            # os.chmod(os.path.join(database,project,subproject,'WIND'), 0644)
            self.OnSelectProject(None)
            self.lbsubprojects.SetSelection(self.listOfSubprojects.index(subproject))
            self.bstart.SetFocus()

            return True
        except Exception as e:
            GError(parent=self,
                   message=_("Unable to create new subproject: %s") % e,
                   showTraceback=False)
            return False

    def OnStart(self, event):
        """'Start GRASS' button clicked"""
        dbase = self.tgisdbase.GetValue()
        project = self.listOfProjects[self.lbprojects.GetSelection()]
        subproject = self.listOfSubprojects[self.lbsubprojects.GetSelection()]

        lockfile = os.path.join(dbase, project, subproject, '.gislock')
        if os.path.isfile(lockfile):
            dlg = wx.MessageDialog(
                parent=self,
                message=_(
                    "GRASS is already running in selected subproject <%(subproject)s>\n"
                    "(file %(lock)s found).\n\n"
                    "Concurrent use not allowed.\n\n"
                    "Do you want to try to remove .gislock (note that you "
                    "need permission for this operation) and continue?") %
                {'subproject': subproject, 'lock': lockfile},
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
        self.SetProject(dbase, project, subproject)
        self.ExitSuccessfully()

    def SetProject(self, dbase, project, subproject):
        RunCommand("g.gisenv",
                   set="GISDBASE=%s" % dbase)
        RunCommand("g.gisenv",
                   set="LOCATION_NAME=%s" % project)
        RunCommand("g.gisenv",
                   set="MAPSET=%s" % subproject)

    def _getDefaultSubprojectName(self):
        """Returns default name for subproject."""
        try:
            defaultName = getpass.getuser()
            # raise error if not ascii (not valid subproject name)
            defaultName.encode('ascii')
        except:  # whatever might go wrong
            defaultName = 'user'

        return defaultName

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

    def _nameValidationFailed(self, ctrl):
        message = _(
            "Name <%(name)s> is not a valid name for project or subproject. "
            "Please use only ASCII characters excluding %(chars)s "
            "and space.") % {
            'name': ctrl.GetValue(),
            'chars': '/"\'@,=*~'}
        GError(parent=self, message=message, caption=_("Invalid name"))


class GListBox(ListCtrl, listmix.ListCtrlAutoWidthMixin):
    """Use wx.ListCtrl instead of wx.ListBox, different style for
    non-selectable items (e.g. subprojects with denied permission)"""

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
        if item !=  wx.NOT_FOUND and \
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

        if StartUp.GetRCValue("LOCATION_NAME") == "<UNKNOWN>":
            # TODO: This is not ideal, either it should be checked elsewhere
            # where other checks are performed or it should use some public
            # API. There is no reason for not exposing it.
            # TODO: another question is what should be warning, hint or message
            StartUp._showWarning(
                _(
                    'GRASS needs a directory (GRASS database) '
                    'in which to store its data. '
                    'Create one now if you have not already done so. '
                    'A popular choice is "grassdata", located in '
                    'your home directory. '
                    'Press Browse button to select the directory.'))

        return 1

if __name__ == "__main__":
    if os.getenv("GISBASE") is None:
        sys.exit("Failed to start GUI, GRASS GIS is not running.")

    GRASSStartUp = StartUp(0)
    GRASSStartUp.MainLoop()
