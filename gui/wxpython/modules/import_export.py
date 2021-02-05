"""
@package modules.import_export

@brief Import/export dialogs used in wxGUI.

List of classes:
 - :class:`ImportDialog`
 - :class:`GdalImportDialog`
 - :class:`OgrImportDialog`
 - :class:`GdalOutputDialog`
 - :class:`DxfImportDialog`
 - :class:`ReprojectionDialog`

(C) 2008-2016 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com> (GroupDialog, SymbolDialog)
"""

import os

import wx
from core import globalvar
import wx.lib.filebrowsebutton as filebrowse

from grass.script import core as grass
from grass.script import task as gtask

from core.gcmd import GError, GMessage, GWarning, RunCommand
from gui_core.forms import CmdPanel
from gui_core.gselect import GdalSelect
from gui_core.widgets import GListCtrl, GNotebook, LayersList, \
    LayersListValidator
from gui_core.wrap import Button, CloseButton, StaticText, StaticBox
from core.utils import GetValidLayerName
from core.settings import UserSettings, GetDisplayVectSettings


class ImportDialog(wx.Dialog):
    """Dialog for bulk import of various data (base class)"""

    def __init__(self, parent, giface, itype,
                 id=wx.ID_ANY, title=_("Multiple import"),
                 style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        self.parent = parent    # GMFrame
        self._giface = giface  # used to add layers
        self.importType = itype
        self.options = dict()   # list of options
        self.options_par = dict()

        self.commandId = -1  # id of running command

        wx.Dialog.__init__(self, parent, id, title, style=style,
                           name="MultiImportDialog")

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        self.layerBox = StaticBox(parent=self.panel, id=wx.ID_ANY)
        if self.importType == 'gdal':
            label = _("List of raster layers")
        elif self.importType == 'ogr':
            label = _("List of vector layers")
        else:
            label = _("List of %s layers") % self.importType.upper()
        self.layerBox.SetLabel(" %s - %s " %
                               (label, _("right click to (un)select all")))

        # list of layers
        columns = [_('Layer id'),
                   _('Layer name'),
                   _('Name for output GRASS map (editable)')]
        if itype == 'ogr':
            columns.insert(2, _('Feature type'))
            columns.insert(3, _('Projection match'))
        elif itype == 'gdal':
            columns.insert(2, _('Projection match'))

        self.list = LayersList(parent=self.panel, columns=columns)
        self.list.LoadData()

        self.override = wx.CheckBox(
            parent=self.panel, id=wx.ID_ANY,
            label=_("Override projection check (use current location's projection)"))

        self.overwrite = wx.CheckBox(
            parent=self.panel, id=wx.ID_ANY,
            label=_("Allow output files to overwrite existing files"))
        self.overwrite.SetValue(
            UserSettings.Get(
                group='cmd',
                key='overwrite',
                subkey='enabled'))
        self.overwrite.Bind(wx.EVT_CHECKBOX, self.OnCheckOverwrite)
        if UserSettings.Get(group='cmd', key='overwrite',
                            subkey='enabled'):
            self.list.validate = False

        self.add = wx.CheckBox(parent=self.panel, id=wx.ID_ANY)
        self.closeOnFinish = wx.CheckBox(parent=self.panel, id=wx.ID_ANY,
                                         label=_("Close dialog on finish"))
        self.closeOnFinish.SetValue(
            UserSettings.Get(
                group='cmd',
                key='closeDlg',
                subkey='enabled'))

        #
        # buttons
        #
        # cancel
        self.btn_close = CloseButton(parent=self.panel)
        self.btn_close.SetToolTip(_("Close dialog"))
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        # run
        self.btn_run = Button(
            parent=self.panel, id=wx.ID_OK, label=_("&Import"))
        self.btn_run.SetToolTip(_("Import selected layers"))
        self.btn_run.SetDefault()
        self.btn_run.Bind(wx.EVT_BUTTON, self.OnRun)

        self.Bind(wx.EVT_CLOSE, lambda evt: self.Destroy())

        self.notebook = GNotebook(parent=self,
                                  style=globalvar.FNPageDStyle)

        self.notebook.AddPage(page=self.panel,
                              text=_('Source settings'),
                              name='source')

        self.createSettingsPage()

    def createSettingsPage(self):

        self._blackList = {
            'enabled': True,
            'items': {
                self._getCommand(): {
                    'params': self._getBlackListedParameters(),
                    'flags': self._getBlackListedFlags()}}}

        grass_task = gtask.parse_interface(self._getCommand(),
                                           blackList=self._blackList)

        self.advancedPagePanel = CmdPanel(
            parent=self, giface=None, task=grass_task, frame=None)

        self.notebook.AddPage(page=self.advancedPagePanel,
                              text=_('Import settings'),
                              name='settings')

    def doLayout(self):
        """Do layout"""
        dialogSizer = wx.BoxSizer(wx.VERTICAL)

        # dsn input
        dialogSizer.Add(self.dsnInput, proportion=0,
                        flag=wx.EXPAND)

        #
        # list of DXF layers
        #
        layerSizer = wx.StaticBoxSizer(self.layerBox, wx.HORIZONTAL)

        layerSizer.Add(self.list, proportion=1,
                       flag=wx.ALL | wx.EXPAND, border=5)

        dialogSizer.Add(
            layerSizer,
            proportion=1,
            flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
            border=5)

        dialogSizer.Add(self.override, proportion=0,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)

        dialogSizer.Add(self.overwrite, proportion=0,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)

        dialogSizer.Add(self.add, proportion=0,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)

        dialogSizer.Add(self.closeOnFinish, proportion=0,
                        flag=wx.LEFT | wx.RIGHT | wx.BOTTOM, border=5)
        #
        # buttons
        #
        btnsizer = wx.BoxSizer(orient=wx.HORIZONTAL)

        btnsizer.Add(self.btn_close, proportion=0,
                     flag=wx.LEFT | wx.RIGHT | wx.ALIGN_CENTER,
                     border=10)

        btnsizer.Add(self.btn_run, proportion=0,
                     flag=wx.RIGHT | wx.ALIGN_CENTER,
                     border=10)

        dialogSizer.Add(
            btnsizer,
            proportion=0,
            flag=wx.BOTTOM | wx.ALIGN_RIGHT,
            border=10)

        # dialogSizer.SetSizeHints(self.panel)
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)

        # auto-layout seems not work here - FIXME
        size = wx.Size(globalvar.DIALOG_GSELECT_SIZE[0] + 322, 550)
        self.SetMinSize(size)
        self.SetSize((size.width, size.height + 100))
        # width = self.GetSize()[0]
        # self.list.SetColumnWidth(col = 1, width = width / 2 - 50)
        self.Layout()

    def _getCommand(self):
        """Get command"""
        raise NotImplementedError()

    def _getBlackListedParameters(self):
        """Get parameters which will not be showed in Settings page"""
        raise NotImplementedError()

    def _getBlackListedFlags(self):
        """Get flags which will not be showed in Settings page"""
        raise NotImplementedError()

    def _nameValidationFailed(self, layers_list):
        """Output map name validation callback

        :param layers_list: LayersList class instance
        """
        if isinstance(layers_list.output_map, list):
            maps = [
                '<{}>'.format(m) for m in layers_list.output_map]
            message = _(
                "Output map names %(names)s exist. "
            ) % {
                'names': ', '.join(maps)}
        else:
            message = _(
                "Output map name <%(name)s> exist. "
                ) % {
                'name': layers_list.output_map}
        GError(parent=self, message=message, caption=_("Invalid name"))

    def _validateOutputMapName(self):
        """Enable/disable output map name validation according the
        overwrite state"""
        if not self.overwrite.IsChecked():
            if not self.list.GetValidator().\
               Validate(win=self.list, validate_all=True):
                return False
        return True

    def OnClose(self, event=None):
        """Close dialog"""
        self.Close()

    def OnRun(self, event):
        """Import/Link data (each layes as separate vector map)"""
        pass

    def OnCheckOverwrite(self, event):
        """Check/uncheck overwrite checkbox widget"""
        if self.overwrite.IsChecked():
            self.list.validate = False
        else:
            self.list.validate = True

    def AddLayers(self, returncode, cmd=None, userData=None):
        """Add imported/linked layers into layer tree"""
        if not self.add.IsChecked() or returncode != 0:
            return

        # TODO: if importing map creates more map the folowing does not work
        # * do nothing if map does not exist or
        # * try to determine names using regexp or
        # * persuade import tools to report map names
        self.commandId += 1
        layer, output = self.list.GetLayers()[self.commandId][:2]

        if '@' not in output:
            name = output + '@' + grass.gisenv()['MAPSET']
        else:
            name = output

        # add imported layers into layer tree
        # an alternative would be emit signal (mapCreated) and (optionally)
        # connect to this signal
        llist = self._giface.GetLayerList()
        if self.importType == 'gdal':
            if userData:
                nBands = int(userData.get('nbands', 1))
            else:
                nBands = 1

            if UserSettings.Get(group='rasterLayer',
                                key='opaque', subkey='enabled'):
                nFlag = True
            else:
                nFlag = False

            for i in range(1, nBands + 1):
                nameOrig = name
                if nBands > 1:
                    mapName, mapsetName = name.split('@')
                    mapName += '.%d' % i
                    name = mapName + '@' + mapsetName

                cmd = ['d.rast',
                       'map=%s' % name]
                if nFlag:
                    cmd.append('-n')

                llist.AddLayer(ltype='raster',
                               name=name, checked=True,
                               cmd=cmd)
                name = nameOrig
        else:
            llist.AddLayer(ltype='vector',
                           name=name, checked=True,
                           cmd=['d.vect',
                                'map=%s' % name] + GetDisplayVectSettings())

        self._giface.GetMapWindow().ZoomToMap()

    def OnAbort(self, event):
        """Abort running import

        .. todo::
            not yet implemented
        """
        pass

    def OnCmdDone(self, event):
        """Do what has to be done after importing"""
        pass

    def _getLayersToReprojetion(self, projMatch_idx, grassName_idx):
        """If there are layers with different projection from loation projection,
           show dialog to user to explicitly select layers which will be reprojected..."""
        differentProjLayers = []
        data = self.list.GetData(checked=True)

        for itm in data:

            layerId = itm[-1]

            # select only layers with different projetion
            if self.layersData[layerId][projMatch_idx] == 0:
                dt = [itm[0], itm[grassName_idx]]
                differentProjLayers.append(tuple(dt))

        layers = self.list.GetLayers()

        if not self.link and \
           differentProjLayers and \
           not self.override.IsChecked(): # '-o' not in self.getSettingsPageCmd():

            dlg = ReprojectionDialog(
                parent=self,
                giface=self._giface,
                data=differentProjLayers)

            ret = dlg.ShowModal()

            if ret == wx.ID_OK:

                # do not import unchecked layers
                for itm in reversed(list(dlg.GetData(checked=False))):
                    idx = itm[-1]
                    layers.pop(idx)
            else:
                return None

        return layers

    def getSettingsPageCmd(self):

        return self.advancedPagePanel.createCmd(
            ignoreErrors=True, ignoreRequired=True)


class GdalImportDialog(ImportDialog):

    def __init__(self, parent, giface, link=False):
        """Dialog for bulk import of various raster data

        .. todo::
            split importing logic from gui code

        :param parent: parent window
        :param link: True for linking data otherwise importing data
        """
        self._giface = giface
        self.link = link

        self.layersData = []

        ImportDialog.__init__(self, parent, giface=giface, itype='gdal')

        self.list.SetValidator(
            LayersListValidator(
                condition='raster',
                callback=self._nameValidationFailed))

        if link:
            self.SetTitle(_("Link external raster data"))
        else:
            self.SetTitle(_("Import raster data"))

        self.dsnInput = GdalSelect(parent=self, panel=self.panel,
                                   ogr=False, link=link)
        self.dsnInput.AttachSettings()
        self.dsnInput.reloadDataRequired.connect(self.reload)

        if link:
            self.add.SetLabel(_("Add linked layers into layer tree"))
        else:
            self.add.SetLabel(_("Add imported layers into layer tree"))

        self.add.SetValue(
            UserSettings.Get(
                group='cmd',
                key='addNewLayer',
                subkey='enabled'))

        if link:
            self.btn_run.SetLabel(_("&Link"))
            self.btn_run.SetToolTip(_("Link selected layers"))
        else:
            self.btn_run.SetLabel(_("&Import"))
            self.btn_run.SetToolTip(_("Import selected layers"))

        self.doLayout()

    def reload(self, data, listData):

        self.list.LoadData(listData)
        self.list.SelectAll(select=True)
        self.layersData = data

    def OnRun(self, event):
        """Import/Link data (each layes as separate vector map)"""
        self.commandId = -1
        data = self.list.GetLayers()

        data = self._getLayersToReprojetion(2, 3)

        if data is None:
            return

        if not data:
            GMessage(_("No layers selected. Operation canceled."),
                     parent=self)
            return

        if not self._validateOutputMapName():
            return

        dsn = self.dsnInput.GetDsn()
        ext = self.dsnInput.GetFormatExt()

        for layer, output, listId in data:
            userData = {}

            if self.dsnInput.GetType() == 'dir':
                idsn = os.path.join(dsn, layer)
            else:
                idsn = dsn

            # check number of bands
            nBandsStr = RunCommand('r.in.gdal',
                                   flags='p',
                                   input=idsn, read=True)
            nBands = -1
            if nBandsStr:
                try:
                    nBands = int(nBandsStr.rstrip('\n'))
                except:
                    pass
            if nBands < 0:
                GWarning(_("Unable to determine number of raster bands"),
                         parent=self)
                nBands = 1

            userData['nbands'] = nBands
            cmd = self.getSettingsPageCmd()
            cmd.append('input=%s' % idsn)
            cmd.append('output=%s' % output)

            if self.override.IsChecked():
                cmd.append('-o')

            if self.overwrite.IsChecked():
                cmd.append('--overwrite')

            if UserSettings.Get(group='cmd', key='overwrite',
                                subkey='enabled') and '--overwrite' not in cmd:
                cmd.append('--overwrite')

            # run in Layer Manager
            self._giface.RunCmd(
                cmd,
                onDone=self.OnCmdDone,
                userData=userData,
                addLayer=False)

    def OnCmdDone(self, event):
        """Load layers and close if required"""
        if not hasattr(self, 'AddLayers'):
            return

        self.AddLayers(event.returncode, event.cmd, event.userData)

        if event.returncode == 0 and self.closeOnFinish.IsChecked():
            self.Close()

    def _getCommand(self):
        """Get command"""
        if self.link:
            return 'r.external'
        else:
            return 'r.import'

    def _getBlackListedParameters(self):
        """Get flags which will not be showed in Settings page"""
        return ['input', 'output']

    def _getBlackListedFlags(self):
        """Get flags which will not be showed in Settings page"""
        return ['overwrite', 'o']


class OgrImportDialog(ImportDialog):

    def __init__(self, parent, giface, link=False):
        """Dialog for bulk import of various vector data

        .. todo::
            split importing logic from gui code

        :param parent: parent window
        :param link: True for linking data otherwise importing data
        """
        self._giface = giface
        self.link = link

        self.layersData = []

        ImportDialog.__init__(self, parent, giface=giface, itype='ogr')

        self.list.SetValidator(
            LayersListValidator(
                condition='vector',
                callback=self._nameValidationFailed))

        if link:
            self.SetTitle(_("Link external vector data"))
        else:
            self.SetTitle(_("Import vector data"))

        self.dsnInput = GdalSelect(parent=self, panel=self.panel,
                                   ogr=True, link=link)
        self.dsnInput.AttachSettings()
        self.dsnInput.reloadDataRequired.connect(self.reload)

        if link:
            self.add.SetLabel(_("Add linked layers into layer tree"))
        else:
            self.add.SetLabel(_("Add imported layers into layer tree"))

        self.add.SetValue(
            UserSettings.Get(
                group='cmd',
                key='addNewLayer',
                subkey='enabled'))

        if link:
            self.btn_run.SetLabel(_("&Link"))
            self.btn_run.SetToolTip(_("Link selected layers"))
        else:
            self.btn_run.SetLabel(_("&Import"))
            self.btn_run.SetToolTip(_("Import selected layers"))

        self.doLayout()

    def reload(self, data, listData):

        self.list.LoadData(listData)
        self.list.SelectAll(select=True)
        self.layersData = data

    def OnRun(self, event):
        """Import/Link data (each layes as separate vector map)"""
        self.commandId = -1
        data = self.list.GetLayers()

        data = self._getLayersToReprojetion(3, 4)

        if data is None:
            return

        if not data:
            GMessage(_("No layers selected. Operation canceled."),
                     parent=self)
            return

        if not self._validateOutputMapName():
            return

        dsn = self.dsnInput.GetDsn()
        ext = self.dsnInput.GetFormatExt()

        # determine data driver for PostGIS links
        self.popOGR = False
        if  self.dsnInput.GetType() == 'db' and \
                self.dsnInput.GetFormat() == 'PostgreSQL' and \
                'GRASS_VECTOR_OGR' not in os.environ:
            self.popOGR = True
            os.environ['GRASS_VECTOR_OGR'] = '1'

        for layer, output, listId in data:
            userData = {}

            if ext and layer.rfind(ext) > -1:
                layer = layer.replace('.' + ext, '')
            if '|' in layer:
                layer, geometry = layer.split('|', 1)
            else:
                geometry = None

                # TODO: v.import has no geometry option
                # if geometry:
                #    cmd.append('geometry=%s' % geometry)

            cmd = self.getSettingsPageCmd()
            cmd.append('input=%s' % dsn)
            cmd.append('layer=%s' % layer)
            cmd.append('output=%s' % output)

            if self.override.IsChecked():
                cmd.append('-o')

            if self.overwrite.IsChecked():
                cmd.append('--overwrite')

            # TODO options
            if UserSettings.Get(group='cmd', key='overwrite',
                                subkey='enabled') and '--overwrite' not in cmd:
                cmd.append('--overwrite')

            # run in Layer Manager
            self._giface.RunCmd(
                cmd,
                onDone=self.OnCmdDone,
                userData=userData,
                addLayer=False)

    def OnCmdDone(self, event):
        """Load layers and close if required"""
        if not hasattr(self, 'AddLayers'):
            return

        self.AddLayers(event.returncode, event.cmd, event.userData)

        if self.popOGR:
            os.environ.pop('GRASS_VECTOR_OGR')

        if event.returncode == 0 and self.closeOnFinish.IsChecked():
            self.Close()

    def _getCommand(self):
        """Get command"""
        if self.link:
            return 'v.external'
        else:
            return 'v.import'

    def _getBlackListedParameters(self):
        """Get parametrs which will not be showed in Settings page"""
        return ['input', 'output', 'layer']

    def _getBlackListedFlags(self):
        """Get flags which will not be showed in Settings page"""
        return ['overwrite', 'o', 'l', 'f']


class GdalOutputDialog(wx.Dialog):

    def __init__(self, parent, id=wx.ID_ANY, ogr=False,
                 style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, *kwargs):
        """Dialog for setting output format for rasters/vectors

        .. todo::
            Split into GdalOutputDialog and OgrOutputDialog

        :param parent: parent window
        :param id: window id
        :param ogr: True for OGR (vector) otherwise GDAL (raster)
        :param style: window style
        :param *kwargs: other wx.Dialog's arguments
        """
        self.parent = parent  # GMFrame
        self.ogr = ogr
        wx.Dialog.__init__(self, parent, id=id, style=style, *kwargs)
        if self.ogr:
            self.SetTitle(_("Define output format for vector data"))
        else:
            self.SetTitle(_("Define output format for raster data"))

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        # buttons
        self.btnCancel = Button(parent=self.panel, id=wx.ID_CANCEL)
        self.btnCancel.SetToolTip(_("Close dialog"))
        self.btnOk = Button(parent=self.panel, id=wx.ID_OK)
        self.btnOk.SetToolTip(_("Set external format and close dialog"))
        self.btnOk.SetDefault()

        self.dsnInput = GdalSelect(parent=self, panel=self.panel,
                                   ogr=ogr,
                                   exclude=['file', 'protocol'], dest=True)
        self.dsnInput.AttachSettings()

        self.Bind(wx.EVT_BUTTON, self.OnCancel, self.btnCancel)
        self.Bind(wx.EVT_BUTTON, self.OnOK, self.btnOk)

        self._layout()

    def _layout(self):
        dialogSizer = wx.BoxSizer(wx.VERTICAL)

        dialogSizer.Add(self.dsnInput, proportion=1,
                        flag=wx.EXPAND)

        btnSizer = wx.BoxSizer(orient=wx.HORIZONTAL)
        btnSizer.Add(self.btnCancel, proportion=0,
                     flag=wx.LEFT | wx.RIGHT | wx.ALIGN_CENTER,
                     border=10)
        btnSizer.Add(self.btnOk, proportion=0,
                     flag=wx.RIGHT | wx.ALIGN_CENTER,
                     border=10)

        dialogSizer.Add(
            btnSizer,
            proportion=0,
            flag=wx.BOTTOM | wx.TOP | wx.ALIGN_RIGHT,
            border=10)

        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)

        size = wx.Size(
            globalvar.DIALOG_GSELECT_SIZE[0] + 320,
            self.GetBestSize()[1] + 35)
        self.SetMinSize(size)
        self.SetSize((size.width, size.height))
        self.Layout()

    def OnCancel(self, event):
        self.Destroy()

    def OnOK(self, event):
        if self.dsnInput.GetType() == 'native':
            RunCommand('v.external.out',
                       parent=self,
                       flags='r')
        else:
            dsn = self.dsnInput.GetDsn()
            frmt = self.dsnInput.GetFormat()
            options = self.dsnInput.GetOptions()
            if not dsn:
                GMessage(_("No data source selected."), parent=self)
                return

            RunCommand('v.external.out',
                       parent=self,
                       output=dsn, format=frmt,
                       options=options)
        self.Close()


class DxfImportDialog(ImportDialog):
    """Dialog for bulk import of DXF layers"""

    def __init__(self, parent, giface):
        ImportDialog.__init__(self, parent, giface=giface, itype='dxf',
                              title=_("Import DXF layers"))

        self.list.SetValidator(
            LayersListValidator(
                condition='vector',
                callback=self._nameValidationFailed))

        self._giface = giface
        self.dsnInput = filebrowse.FileBrowseButton(
            parent=self.panel,
            id=wx.ID_ANY,
            size=globalvar.DIALOG_GSELECT_SIZE,
            labelText='',
            dialogTitle=_('Choose DXF file to import'),
            buttonText=_('Browse'),
            startDirectory=os.getcwd(),
            fileMode=0,
            changeCallback=self.OnSetDsn,
            fileMask="DXF File (*.dxf)|*.dxf")

        self.add.SetLabel(_("Add imported layers into layer tree"))

        self.add.SetValue(
            UserSettings.Get(
                group='cmd',
                key='addNewLayer',
                subkey='enabled'))

        self.doLayout()

    def OnRun(self, event):
        """Import/Link data (each layes as separate vector map)"""
        data = self.list.GetLayers()
        if not data:
            GMessage(_("No layers selected."), parent=self)
            return

        if not self._validateOutputMapName():
            return

        # hide dialog
        self.Hide()

        inputDxf = self.dsnInput.GetValue()

        for layer, output, itemId in data:

            cmd = self.getSettingsPageCmd()
            cmd.append('input=%s' % inputDxf)
            cmd.append('layer=%s' % layer)
            cmd.append('output=%s' % output)

            for key in self.options.keys():
                if self.options[key].IsChecked():
                    cmd.append('-%s' % key)

            if self.overwrite.IsChecked() or UserSettings.Get(
                    group='cmd', key='overwrite', subkey='enabled'):
                cmd.append('--overwrite')

            # run in Layer Manager
            self._giface.RunCmd(cmd, onDone=self.OnCmdDone, addLayer=False)

    def OnCmdDone(self, event):
        """Load layers and close if required"""
        if not hasattr(self, 'AddLayers'):
            return

        self.AddLayers(event.returncode, event.cmd)

        if self.closeOnFinish.IsChecked():
            self.Close()

    def OnSetDsn(self, event):
        """Input DXF file defined, update list of layer widget"""
        path = event.GetString()
        if not path:
            return

        data = list()
        ret = RunCommand('v.in.dxf',
                         quiet=True,
                         parent=self,
                         read=True,
                         flags='l',
                         input=path)
        if not ret:
            self.list.LoadData()
            return

        for line in ret.splitlines():
            layerId = line.split(':')[0].split(' ')[1]
            layerName = line.split(':')[1].strip()
            grassName = GetValidLayerName(layerName)
            data.append((layerId, layerName.strip(), grassName.strip()))

        self.list.LoadData(data)

    def _getCommand(self):
        """Get command"""
        return 'v.in.dxf'

    def _getBlackListedParameters(self):
        """Get parametrs which will not be showed in Settings page"""
        return ['input', 'output', 'layers']

    def _getBlackListedFlags(self):
        """Get flags which will not be showed in Settings page"""
        return ['overwrite']


class ReprojectionDialog(wx.Dialog):
    """ """

    def __init__(self, parent, giface, data,
                 id=wx.ID_ANY, title=_("Reprojection"),
                 style=wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        self.parent = parent    # GMFrame
        self._giface = giface  # used to add layers

        wx.Dialog.__init__(self, parent, id, title, style=style,
                           name="MultiImportDialog")

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        # list of layers
        columns = [_('Layer id'),
                   _('Name for output GRASS map')]

        self.layerBox = StaticBox(parent=self.panel, id=wx.ID_ANY)
        self.layerSizer = wx.StaticBoxSizer(self.layerBox, wx.HORIZONTAL)

        self.list = GListCtrl(parent=self.panel)

        for i in range(len(columns)):
            self.list.InsertColumn(i, columns[i])

        width = (65, 180)

        for i in range(len(width)):
            self.list.SetColumnWidth(col=i, width=width[i])

        self.list.LoadData(data)
        self.list.SelectAll(True)

        self.labelText = StaticText(parent=self.panel, id=wx.ID_ANY, label=_(
            "Projection of following layers do not match with projection of current location. "))

        label = _("Layers to be reprojected")
        self.layerBox.SetLabel(" %s - %s " %
                               (label, _("right click to (un)select all")))

        #
        # buttons
        #
        # cancel
        self.btn_close = Button(parent=self.panel, id=wx.ID_CANCEL)

        # run
        self.btn_run = Button(
            parent=self.panel,
            id=wx.ID_OK,
            label=_("&Import && reproject"))
        self.btn_run.SetToolTip(_("Reproject selected layers"))
        self.btn_run.SetDefault()

        self.doLayout()

    def doLayout(self):
        """Do layout"""
        dialogSizer = wx.BoxSizer(wx.VERTICAL)

        dialogSizer.Add(self.labelText,
                        flag=wx.ALL | wx.EXPAND, border=5)

        self.layerSizer.Add(self.list, proportion=1,
                            flag=wx.ALL | wx.EXPAND, border=5)

        dialogSizer.Add(
            self.layerSizer,
            proportion=1,
            flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
            border=5)

        #
        # buttons
        #
        btnsizer = wx.BoxSizer(orient=wx.HORIZONTAL)

        btnsizer.Add(self.btn_close, proportion=0,
                     flag=wx.LEFT | wx.RIGHT | wx.ALIGN_CENTER,
                     border=10)

        btnsizer.Add(self.btn_run, proportion=0,
                     flag=wx.RIGHT | wx.ALIGN_CENTER,
                     border=10)

        dialogSizer.Add(
            btnsizer,
            proportion=0,
            flag=wx.BOTTOM | wx.ALIGN_RIGHT,
            border=10)

        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)

        self.Layout()

    def GetData(self, checked):

        return self.list.GetData(checked)
