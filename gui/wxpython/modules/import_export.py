"""
@package modules.import_export

@brief Import/export dialogs used in wxGUI.

List of classes:
 - :class:`ImportDialog`
 - :class:`GdalImportDialog`
 - :class:`GdalOutputDialog`
 - :class:`DxfImportDialog`

(C) 2008-2015 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Anna Kratochvilova <kratochanna gmail.com> (GroupDialog, SymbolDialog)
"""

import os

import wx
import wx.lib.filebrowsebutton as filebrowse

from grass.script import core as grass
from grass.script import task as gtask

from core import globalvar
from core.gcmd import RunCommand, GMessage, GWarning
from gui_core.gselect import OgrTypeSelect, GdalSelect, SubGroupSelect
from gui_core.widgets import LayersList
from core.utils import GetValidLayerName, _
from core.settings import UserSettings, GetDisplayVectSettings

class ImportDialog(wx.Dialog):
    """Dialog for bulk import of various data (base class)"""
    def __init__(self, parent, giface, itype,
                 id = wx.ID_ANY, title = _("Multiple import"),
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER):
        self.parent = parent    # GMFrame 
        self._giface = giface  # used to add layers
        self.importType = itype
        self.options = dict()   # list of options
        self.options_par = dict()
        
        self.commandId = -1  # id of running command
        
        wx.Dialog.__init__(self, parent, id, title, style = style,
                           name = "MultiImportDialog")
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        self.layerBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY)
        if self.importType == 'gdal':
            label = _("List of raster layers")
        elif self.importType == 'ogr':
            label = _("List of vector layers")
        else:
            label = _("List of %s layers") % self.importType.upper()
        self.layerBox.SetLabel(" %s - %s " % (label, _("right click to (un)select all")))
        
        # list of layers
        columns = [_('Layer id'),
                   _('Layer name'),
                   _('Name for output GRASS map (editable)')]
        if itype == 'ogr':
            columns.insert(2, _('Feature type'))
            columns.insert(3, _('Projection match'))

        self.list = LayersList(parent = self.panel, columns = columns)
        self.list.LoadData()

        self.optionBox = wx.StaticBox(parent = self.panel, id = wx.ID_ANY,
                                      label = "%s" % _("Options"))
        
        cmd = self._getCommand()
        task = gtask.parse_interface(cmd)
        for f in task.get_options()['flags']:
            name = f.get('name', '')
            desc = f.get('label', '')
            if not desc:
                desc = f.get('description', '')
            if not name and not desc:
                continue
            if cmd == 'r.in.gdal' and name not in ('o', 'e', 'l', 'k'):
                continue
            elif cmd == 'r.external' and name not in ('o', 'e', 'r', 'h', 'v'):
                continue
            elif cmd == 'v.import':
                continue
            elif cmd == 'v.external' and name not in ('b'):
                continue
            elif cmd == 'v.in.dxf' and name not in ('e', 't', 'b', 'f', 'i'):
                continue
            self.options[name] = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                             label = desc)
        
        for p in task.get_options()['params']:
            name = p.get('name', '')
            desc = p.get('label', '')
            if not desc:
                desc = p.get('description', '')
            if not name and not desc:
                continue
            if cmd == 'v.import' and name == 'encoding':
                self.options_par[name] = (_('Encoding'),
                                          wx.TextCtrl(parent = self.panel, id = wx.ID_ANY))
        
        self.overwrite = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                     label = _("Allow output files to overwrite existing files"))
        self.overwrite.SetValue(UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled'))
        
        self.add = wx.CheckBox(parent = self.panel, id = wx.ID_ANY)
        self.closeOnFinish = wx.CheckBox(parent = self.panel, id = wx.ID_ANY,
                                     label = _("Close dialog on finish"))
        self.closeOnFinish.SetValue(UserSettings.Get(group = 'cmd', key = 'closeDlg', subkey = 'enabled'))
        
        #
        # buttons
        #
        # cancel
        self.btn_close = wx.Button(parent = self.panel, id = wx.ID_CLOSE)
        self.btn_close.SetToolTipString(_("Close dialog"))
        self.btn_close.Bind(wx.EVT_BUTTON, self.OnClose)
        # run
        self.btn_run = wx.Button(parent = self.panel, id = wx.ID_OK, label = _("&Import"))
        self.btn_run.SetToolTipString(_("Import selected layers"))
        self.btn_run.SetDefault()
        self.btn_run.Bind(wx.EVT_BUTTON, self.OnRun)

        self.Bind(wx.EVT_CLOSE, lambda evt: self.Destroy())
        
    def doLayout(self):
        """Do layout"""
        dialogSizer = wx.BoxSizer(wx.VERTICAL)
        
        # dsn input
        dialogSizer.Add(item = self.dsnInput, proportion = 0,
                        flag = wx.EXPAND)
        
        #
        # list of DXF layers
        #
        layerSizer = wx.StaticBoxSizer(self.layerBox, wx.HORIZONTAL)

        layerSizer.Add(item = self.list, proportion = 1,
                      flag = wx.ALL | wx.EXPAND, border = 5)
        
        dialogSizer.Add(item = layerSizer, proportion = 1,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)

        # options
        optionSizer = wx.StaticBoxSizer(self.optionBox, wx.VERTICAL)
        for key in self.options.keys():
            optionSizer.Add(item = self.options[key], proportion = 0)
        if self.options_par:
            gridBox = wx.GridBagSizer(vgap = 5, hgap = 5)
            row = 0
            for label, win in self.options_par.itervalues():
                gridBox.Add(item = wx.StaticText(parent = self.panel, id = wx.ID_ANY,
                                                 label = label + ':'),
                            pos = (row, 0), flag = wx.ALIGN_CENTER_VERTICAL)
                gridBox.Add(item = win, pos = (row, 1), flag = wx.EXPAND)
                row += 1
            
            gridBox.AddGrowableCol(1)
            optionSizer.Add(item = gridBox, proportion = 0,
                            flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        
        dialogSizer.Add(item = optionSizer, proportion = 0,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND, border = 5)
        
        dialogSizer.Add(item = self.overwrite, proportion = 0,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        
        dialogSizer.Add(item = self.add, proportion = 0,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        
        dialogSizer.Add(item = self.closeOnFinish, proportion = 0,
                        flag = wx.LEFT | wx.RIGHT | wx.BOTTOM, border = 5)
        #
        # buttons
        #
        btnsizer = wx.BoxSizer(orient = wx.HORIZONTAL)
        
        btnsizer.Add(item = self.btn_close, proportion = 0,
                     flag = wx.LEFT | wx.RIGHT | wx.ALIGN_CENTER,
                     border = 10)
        
        btnsizer.Add(item = self.btn_run, proportion = 0,
                     flag = wx.RIGHT | wx.ALIGN_CENTER,
                     border = 10)
        
        dialogSizer.Add(item = btnsizer, proportion = 0,
                        flag = wx.ALIGN_CENTER_VERTICAL | wx.BOTTOM | wx.ALIGN_RIGHT,
                        border = 10)
        
        # dialogSizer.SetSizeHints(self.panel)
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)
        
        # auto-layout seems not work here - FIXME
        size = wx.Size(globalvar.DIALOG_GSELECT_SIZE[0] + 225, 550)
        self.SetMinSize(size)
        self.SetSize((size.width, size.height + 100))
        # width = self.GetSize()[0]
        # self.list.SetColumnWidth(col = 1, width = width / 2 - 50)
        self.Layout()

    def _getCommand(self):
        """Get command"""
        return ''
    
    def OnClose(self, event = None):
        """Close dialog"""
        self.Close()

    def OnRun(self, event):
        """Import/Link data (each layes as separate vector map)"""
        pass

    def AddLayers(self, returncode, cmd = None, userData = None):
        """Add imported/linked layers into layer tree"""
        if not self.add.IsChecked() or returncode != 0:
            return

        # TODO: if importing map creates more map the folowing does not work
        # * do nothing if map does not exist or
        # * try to determine names using regexp or
        # * persuade import tools to report map names
        self.commandId += 1
        layer, output = self.list.GetLayers()[self.commandId]
        
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
            
            if UserSettings.Get(group = 'rasterLayer', key = 'opaque', subkey = 'enabled'):
                nFlag = True
            else:
                nFlag = False
            
            for i in range(1, nBands+1):
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


class GdalImportDialog(ImportDialog):
    def __init__(self, parent, giface, ogr = False, link = False):
        """Dialog for bulk import of various raster/vector data

        .. todo::
            Split into GdalImportDialog and OgrImportDialog

        :param parent: parent window
        :param ogr: True for OGR (vector) otherwise GDAL (raster)
        :param link: True for linking data otherwise importing data
        """
        self._giface = giface
        self.link = link
        self.ogr  = ogr
        
        if ogr:
            ImportDialog.__init__(self, parent, giface=giface, itype='ogr')
            if link:
                self.SetTitle(_("Link external vector data"))
            else:
                self.SetTitle(_("Import vector data"))
        else:
            ImportDialog.__init__(self, parent, giface=giface, itype='gdal') 
            if link:
                self.SetTitle(_("Link external raster data"))
            else:
                self.SetTitle(_("Import raster data"))
        
        self.dsnInput = GdalSelect(parent = self, panel = self.panel,
                                   ogr = ogr, link = link)
        self.dsnInput.AttachSettings()
        self.dsnInput.reloadDataRequired.connect(lambda data: self.list.LoadData(data))

        if link:
            self.add.SetLabel(_("Add linked layers into layer tree"))
        else:
            self.add.SetLabel(_("Add imported layers into layer tree"))
        
        self.add.SetValue(UserSettings.Get(group = 'cmd', key = 'addNewLayer', subkey = 'enabled'))

        if link:
            self.btn_run.SetLabel(_("&Link"))
            self.btn_run.SetToolTipString(_("Link selected layers"))
        else:
            self.btn_run.SetLabel(_("&Import"))
            self.btn_run.SetToolTipString(_("Import selected layers"))

        self.doLayout()

    def OnRun(self, event):
        """Import/Link data (each layes as separate vector map)"""
        self.commandId = -1
        data = self.list.GetLayers()
        if not data:
            GMessage(_("No layers selected. Operation canceled."),
                     parent = self)
            return
        
        dsn  = self.dsnInput.GetDsn()
        ext  = self.dsnInput.GetFormatExt()
        
        # determine data driver for PostGIS links
        self.popOGR = False
        if self.importType == 'ogr' and \
                self.dsnInput.GetType() == 'db' and \
                self.dsnInput.GetFormat() == 'PostgreSQL' and \
                'GRASS_VECTOR_OGR' not in os.environ:
            self.popOGR = True
            os.environ['GRASS_VECTOR_OGR'] = '1'
        
        for layer, output in data:
            userData = {}
            if self.importType == 'ogr':
                if ext and layer.rfind(ext) > -1:
                    layer = layer.replace('.' + ext, '')
                if '|' in layer:
                    layer, geometry = layer.split('|', 1)
                else:
                    geometry = None
                if self.link:
                    cmd = ['v.external',
                           'input=%s' % dsn,
                           'output=%s' % output,
                           'layer=%s' % layer]
                else:
                    cmd = ['v.import',
                           'input=%s' % dsn,
                           'layer=%s' % layer,
                           'output=%s' % output]
                    if geometry:
                        cmd.append('geometry=%s' % geometry)
            else: # gdal
                if self.dsnInput.GetType() == 'dir':
                    idsn = os.path.join(dsn, layer)
                else:
                    idsn = dsn

                # check number of bands
                nBandsStr = RunCommand('r.in.gdal',
                                       flags = 'p',
                                       input = idsn, read = True)
                nBands = -1
                if nBandsStr:
                    try:
                        nBands = int(nBandsStr.rstrip('\n'))
                    except:
                        pass
                if nBands < 0:
                    GWarning(_("Unable to determine number of raster bands"),
                             parent = self)
                    nBands = 1

                userData['nbands'] = nBands
                if self.link:
                    cmd = ['r.external',
                           'input=%s' % idsn,
                           'output=%s' % output]
                else:
                    cmd = ['r.in.gdal',
                           'input=%s' % idsn,
                           'output=%s' % output]
                    if nBands > 1:
                        cmd.append('-k')
            
            if self.overwrite.IsChecked():
                cmd.append('--overwrite')
            
            for key in self.options.keys():
                if self.options[key].IsChecked():
                    cmd.append('-%s' % key)
            for key in self.options_par.keys():
                value = self.options_par[key][1].GetValue()
                if value:
                    cmd.append('%s=%s' % (key, value))
            
            if UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled') and \
                    '--overwrite' not in cmd:
                cmd.append('--overwrite')
            
            # run in Layer Manager
            self._giface.RunCmd(cmd, onDone = self.OnCmdDone, userData = userData)

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
            if self.ogr:
                return 'v.external'
            else:
                return 'r.external'
        else:
            if self.ogr:
                return 'v.import'
            else:
                return 'r.in.gdal'
        
        return ''

class GdalOutputDialog(wx.Dialog):
    def __init__(self, parent, id = wx.ID_ANY, ogr = False,
                 style = wx.DEFAULT_DIALOG_STYLE | wx.RESIZE_BORDER, *kwargs):
        """Dialog for setting output format for rasters/vectors

        .. todo::
            Split into GdalOutputDialog and OgrOutputDialog

        :param parent: parent window
        :param id: window id
        :param ogr: True for OGR (vector) otherwise GDAL (raster)
        :param style: window style
        :param *kwargs: other wx.Dialog's arguments
        """
        self.parent = parent # GMFrame 
        self.ogr = ogr
        wx.Dialog.__init__(self, parent, id = id, style = style, *kwargs)
        if self.ogr:
            self.SetTitle(_("Define output format for vector data"))
        else:
            self.SetTitle(_("Define output format for raster data"))
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)

        # buttons
        self.btnCancel = wx.Button(parent = self.panel, id = wx.ID_CANCEL)
        self.btnCancel.SetToolTipString(_("Close dialog"))
        self.btnOk = wx.Button(parent = self.panel, id = wx.ID_OK)
        self.btnOk.SetToolTipString(_("Set external format and close dialog"))
        self.btnOk.SetDefault()
        
        self.dsnInput = GdalSelect(parent = self, panel = self.panel,
                                   ogr = ogr,
                                   exclude = ['file', 'protocol'], dest = True)
        self.dsnInput.AttachSettings()
        
        self.Bind(wx.EVT_BUTTON, self.OnCancel, self.btnCancel)
        self.Bind(wx.EVT_BUTTON, self.OnOK, self.btnOk)
        
        self._layout()

    def _layout(self):
        dialogSizer = wx.BoxSizer(wx.VERTICAL)
        
        dialogSizer.Add(item = self.dsnInput, proportion = 1,
                        flag = wx.EXPAND)

        btnSizer = wx.BoxSizer(orient = wx.HORIZONTAL)
        btnSizer.Add(item = self.btnCancel, proportion = 0,
                     flag = wx.LEFT | wx.RIGHT | wx.ALIGN_CENTER,
                     border = 10)
        btnSizer.Add(item = self.btnOk, proportion = 0,
                     flag = wx.RIGHT | wx.ALIGN_CENTER,
                     border = 10)
        
        dialogSizer.Add(item = btnSizer, proportion = 0,
                        flag = wx.ALIGN_CENTER_VERTICAL | wx.BOTTOM | wx.TOP | wx.ALIGN_RIGHT,
                        border = 10)
        
        self.panel.SetAutoLayout(True)
        self.panel.SetSizer(dialogSizer)
        dialogSizer.Fit(self.panel)

        size = wx.Size(globalvar.DIALOG_GSELECT_SIZE[0] + 225, self.GetBestSize()[1] + 35)
        self.SetMinSize(size)
        self.SetSize((size.width, size.height))
        self.Layout()
        
    def OnCancel(self, event):
        self.Destroy()
        
    def OnOK(self, event):
        if self.dsnInput.GetType() == 'native':
            RunCommand('v.external.out',
                       parent = self,
                       flags = 'r')
        else:
            dsn = self.dsnInput.GetDsn()
            frmt = self.dsnInput.GetFormat()
            options = self.dsnInput.GetOptions()
            if not dsn:
                GMessage(_("No data source selected."), parent=self)
                return
            
            RunCommand('v.external.out',
                       parent = self,
                       output = dsn, format = frmt,
                       options = options)
        self.Close()
        
class DxfImportDialog(ImportDialog):
    """Dialog for bulk import of DXF layers"""
    def __init__(self, parent, giface):
        ImportDialog.__init__(self, parent, giface=giface, itype='dxf',
                              title = _("Import DXF layers"))
        self._giface = giface
        self.dsnInput = filebrowse.FileBrowseButton(parent = self.panel, id = wx.ID_ANY, 
                                                    size = globalvar.DIALOG_GSELECT_SIZE, labelText = '',
                                                    dialogTitle = _('Choose DXF file to import'),
                                                    buttonText = _('Browse'),
                                                    startDirectory = os.getcwd(), fileMode = 0,
                                                    changeCallback = self.OnSetDsn,
                                                    fileMask = "DXF File (*.dxf)|*.dxf")
        
        self.add.SetLabel(_("Add imported layers into layer tree"))
        
        self.add.SetValue(UserSettings.Get(group = 'cmd', key = 'addNewLayer', subkey = 'enabled'))
        
        self.doLayout()

    def _getCommand(self):
        """Get command"""
        return 'v.in.dxf'
    
    def OnRun(self, event):
        """Import/Link data (each layes as separate vector map)"""
        data = self.list.GetLayers()
        if not data:
            GMessage(_("No layers selected."), parent=self)
            return
        
        # hide dialog
        self.Hide()
        
        inputDxf = self.dsnInput.GetValue()
        
        for layer, output in data:
            cmd = ['v.in.dxf',
                   'input=%s' % inputDxf,
                   'layers=%s' % layer,
                   'output=%s' % output]

            for key in self.options.keys():
                if self.options[key].IsChecked():
                    cmd.append('-%s' % key)
            
            if self.overwrite.IsChecked() or \
                    UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled'):
                cmd.append('--overwrite')
            
            # run in Layer Manager
            self._giface.RunCmd(cmd, onDone=self.OnCmdDone)

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
                         quiet = True,
                         parent = self,
                         read = True,
                         flags = 'l',
                         input = path)
        if not ret:
            self.list.LoadData()
            return
            
        for line in ret.splitlines():
            layerId = line.split(':')[0].split(' ')[1]
            layerName = line.split(':')[1].strip()
            grassName = GetValidLayerName(layerName)
            data.append((layerId, layerName.strip(), grassName.strip()))
        
        self.list.LoadData(data)
