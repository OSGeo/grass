"""
@package web_services.widgets

@brief Widgets for web services (WMS, WMTS, NasaOnEarh)

List of classes:
 - widgets::WSPanel
 - widgets::LayersList

(C) 2012-2013 by the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Martin Landa <landa.martin gmail.com>
@author Stepan Turek <stepan.turek seznam.cz>
"""

import re
import os
import sys
import six
import shutil

from copy import deepcopy
from core import globalvar
try:
    from xml.etree.ElementTree import ParseError
except ImportError:  # < Python 2.7
    from xml.parsers.expat import ExpatError as ParseError

import wx
if globalvar.wxPythonPhoenix:
    try:
        import agw.flatnotebook as FN
    except ImportError: # if it's not there locally, try the wxPython lib.
        import wx.lib.agw.flatnotebook as FN
else:
    import wx.lib.flatnotebook as FN
import wx.lib.colourselect as csel

from core.debug import Debug
from core.gcmd import GMessage
from core.gconsole import CmdThread, GStderr, EVT_CMD_DONE, EVT_CMD_OUTPUT

from web_services.cap_interface import WMSCapabilities, WMTSCapabilities, OnEarthCapabilities

from gui_core.widgets import GNotebook
from gui_core.widgets import ManageSettingsWidget
from gui_core.wrap import Button, ScrolledPanel, SpinCtrl, StaticBox, \
    StaticText, TextCtrl, TreeCtrl

import grass.script as grass

rinwms_path = os.path.join(os.getenv("GISBASE"), "etc", "r.in.wms")
if rinwms_path not in sys.path:
    sys.path.append(rinwms_path)

from wms_base import WMSDriversInfo
from srs import Srs

from grass.pydispatch.signal import Signal


class WSPanel(wx.Panel):

    def __init__(self, parent, web_service, **kwargs):
        """Show data from capabilities file.

        Signal: capParsed - this signal is emitted when capabilities file is downloaded
                            (after ConnectToServer method was called)

        :param parent:  parent widget
        :param web_service:  web service to be panel generated for
        """
        wx.Panel.__init__(self, parent=parent, id=wx.ID_ANY)

        self.parent = parent
        self.ws = web_service

        self.capParsed = Signal('WSPanel.capParsed')

        # stores widgets, which represents parameters/flags of d.wms
        self.params = {}
        self.flags = {}

        self.o_layer_name = ''

        # stores err output from r.in.wms during getting capabilities
        self.cmd_err_str = ''

        # stores selected layer from layer list
        self.sel_layers = []

        # downloaded and parsed data from server successfully?
        self.is_connected = False

        # common part of command for r.in.wms -c and d.wms
        self.ws_cmdl = None

        # provides information about driver parameters
        self.drv_info = WMSDriversInfo()
        self.drv_props = self.drv_info.GetDrvProperties(self.ws)

        self.ws_drvs = {
            'WMS_1.1.1': {
                'cmd': [
                    'wms_version=1.1.1',
                    'driver=WMS_GRASS'],
                'cap_parser': lambda temp_file: WMSCapabilities(
                    temp_file,
                    '1.1.1'),
            },
            'WMS_1.3.0': {
                'cmd': [
                    'wms_version=1.3.0',
                    'driver=WMS_GRASS'],
                'cap_parser': lambda temp_file: WMSCapabilities(
                    temp_file,
                    '1.3.0'),
            },
            'WMTS': {
                'cmd': ['driver=WMTS_GRASS'],
                'cap_parser': WMTSCapabilities,
            },
            'OnEarth': {
                'cmd': ['driver=OnEarth_GRASS'],
                'cap_parser': OnEarthCapabilities,
            }}

        self.cmdStdErr = GStderr(self)
        self.cmd_thread = CmdThread(self)
        self.cap_file = grass.tempfile()

        reqDataBox = StaticBox(
            parent=self, label=_(" Requested data settings "))
        self._nb_sizer = wx.StaticBoxSizer(reqDataBox, wx.VERTICAL)
        self.notebook = GNotebook(
            parent=self,
            style=FN.FNB_FANCY_TABS | FN.FNB_NO_X_BUTTON | FN.FNB_NODRAG)

        self._requestPage()
        self._advancedSettsPage()

        self._layout()

        self.layerSelected = self.list.layerSelected

        self.Bind(EVT_CMD_DONE, self.OnCapDownloadDone)
        self.Bind(EVT_CMD_OUTPUT, self.OnCmdOutput)

        self.SetMinSize((-1, 300))

    def __del__(self):
        self.cmd_thread.abort(abortall=True)
        grass.try_remove(self.cap_file)

    def _layout(self):
        self._nb_sizer.Add(self.notebook, proportion=1, flag=wx.EXPAND)
        self.SetSizer(self._nb_sizer)

    def _requestPage(self):
        """Create request page"""
        self.req_page_panel = wx.Panel(parent=self, id=wx.ID_ANY)
        self.notebook.AddPage(page=self.req_page_panel,
                              text=_('Request'),
                              name='request')

        # list of layers
        self.layersBox = StaticBox(parent=self.req_page_panel, id=wx.ID_ANY,
                                   label=_("List of layers "))

        style = wx.TR_DEFAULT_STYLE | wx.TR_HAS_BUTTONS | wx.TR_FULL_ROW_HIGHLIGHT
        if self.drv_props['req_multiple_layers']:
            style = style | wx.TR_MULTIPLE
        if 'WMS' not in self.ws:
            style = style | wx.TR_HIDE_ROOT

        self.list = LayersList(parent=self.req_page_panel,
                               web_service=self.ws,
                               style=style)

        self.params['format'] = None

        self.params['srs'] = None
        if 'srs' not in self.drv_props['ignored_params']:
            projText = StaticText(
                parent=self.req_page_panel, id=wx.ID_ANY,
                label=_("Source projection:"))
            self.params['srs'] = wx.Choice(
                parent=self.req_page_panel, id=wx.ID_ANY)

        self.list.Bind(wx.EVT_TREE_SEL_CHANGED, self.OnListSelChanged)

        # layout
        self.req_page_sizer = wx.BoxSizer(wx.VERTICAL)

        layersSizer = wx.StaticBoxSizer(self.layersBox, wx.HORIZONTAL)

        layersSizer.Add(
            self.list,
            proportion=1,
            flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
            border=5)

        self.req_page_sizer.Add(
            layersSizer,
            proportion=1,
            flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
            border=5)

        self.source_sizer = wx.BoxSizer(wx.HORIZONTAL)

        if self.params['format'] is not None:
            self.source_sizer.Add(
                self.params['format'],
                flag=wx.LEFT | wx.RIGHT | wx.BOTTOM,
                border=5)

        if self.params['srs'] is not None:
            self.source_sizer.Add(
                projText,
                flag=wx.ALIGN_CENTER_VERTICAL | wx.ALL,
                border=5)
            self.source_sizer.Add(
                self.params['srs'],
                flag=wx.ALIGN_CENTER_VERTICAL | wx.RIGHT | wx.TOP | wx.BOTTOM,
                border=5)

        self.req_page_sizer.Add(
            self.source_sizer,
            flag=wx.LEFT | wx.RIGHT | wx.BOTTOM | wx.EXPAND,
            border=5)

        self.req_page_panel.SetSizer(self.req_page_sizer)

    def enableButtons(self, enable=True):
        """Enable/disable up, down, buttons
        """
        self.btnUp.Enable(enable)
        self.btnDown.Enable(enable)

    def _advancedSettsPage(self):
        """Create advanced settings page
        """
        # TODO parse maxcol, maxrow, settings from d.wms module?
        # TODO OnEarth driver - add selection of time
        adv_setts_panel = ScrolledPanel(parent=self, id=wx.ID_ANY,
                                        style=wx.TAB_TRAVERSAL | wx.SUNKEN_BORDER)
        self.notebook.AddPage(page=adv_setts_panel,
                              text=_('Advanced request settings'),
                              name='adv_req_setts')

        labels = {}
        self.l_odrder_list = None
        if 'WMS' in self.ws:
            labels['l_order'] = StaticBox(
                parent=adv_setts_panel, id=wx.ID_ANY,
                label=_("Order of layers in raster"))
            self.l_odrder_list = wx.ListBox(
                adv_setts_panel, id=wx.ID_ANY, choices=[],
                style=wx.LB_SINGLE | wx.LB_NEEDED_SB)
            self.btnUp = Button(
                adv_setts_panel, id=wx.ID_ANY, label=_("Up"))
            self.btnDown = Button(
                adv_setts_panel, id=wx.ID_ANY, label=_("Down"))

            self.btnUp.Bind(wx.EVT_BUTTON, self.OnUp)
            self.btnDown.Bind(wx.EVT_BUTTON, self.OnDown)

        labels['method'] = StaticText(parent=adv_setts_panel, id=wx.ID_ANY,
                                      label=_("Reprojection method:"))

        self.reproj_methods = ['nearest', 'linear', 'cubic', 'cubicspline']
        self.params['method'] = wx.Choice(
            parent=adv_setts_panel,
            id=wx.ID_ANY,
            choices=[
                _('Nearest neighbor'),
                _('Linear interpolation'),
                _('Cubic interpolation'),
                _('Cubic spline interpolation')])

        labels['maxcols'] = StaticText(
            parent=adv_setts_panel, id=wx.ID_ANY,
            label=_("Maximum columns to request from server at time:"))
        self.params['maxcols'] = SpinCtrl(
            parent=adv_setts_panel, id=wx.ID_ANY, size=(100, -1))

        labels['maxrows'] = StaticText(
            parent=adv_setts_panel, id=wx.ID_ANY,
            label=_("Maximum rows to request from server at time:"))
        self.params['maxrows'] = SpinCtrl(
            parent=adv_setts_panel, id=wx.ID_ANY, size=(100, -1))

        min = 100
        max = 10000
        self.params['maxcols'].SetRange(min, max)
        self.params['maxrows'].SetRange(min, max)

        val = 500
        self.params['maxcols'].SetValue(val)
        self.params['maxrows'].SetValue(val)

        self.flags['o'] = self.params['bgcolor'] = None
        if 'o' not in self.drv_props['ignored_flags']:
            self.flags['o'] = wx.CheckBox(
                parent=adv_setts_panel, id=wx.ID_ANY,
                label=_("Do not request transparent data"))

            self.flags['o'].Bind(wx.EVT_CHECKBOX, self.OnTransparent)
            labels['bgcolor'] = StaticText(
                parent=adv_setts_panel, id=wx.ID_ANY,
                label=_("Background color:"))
            self.params['bgcolor'] = csel.ColourSelect(
                parent=adv_setts_panel, id=wx.ID_ANY, colour=(
                    255, 255, 255), size=globalvar.DIALOG_COLOR_SIZE)
            self.params['bgcolor'].Enable(False)

        self.params['urlparams'] = None
        if self.params['urlparams'] not in self.drv_props['ignored_params']:
            labels['urlparams'] = StaticText(
                parent=adv_setts_panel, id=wx.ID_ANY,
                label=_("Additional query parameters for server:"))
            self.params['urlparams'] = TextCtrl(
                parent=adv_setts_panel, id=wx.ID_ANY)

        # layout

        border = wx.BoxSizer(wx.VERTICAL)

        if 'WMS' in self.ws:

            boxSizer = wx.StaticBoxSizer(labels['l_order'], wx.VERTICAL)
            gridSizer = wx.GridBagSizer(hgap=3, vgap=3)

            gridSizer.Add(self.l_odrder_list,
                          pos=(0, 0),
                          span=(4, 1),
                          flag=wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                          border=0)

            gridSizer.Add(self.btnUp,
                          pos=(0, 1),
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          border=0)

            gridSizer.Add(self.btnDown,
                          pos=(1, 1),
                          flag=wx.ALIGN_CENTER_VERTICAL,
                          border=0)

            gridSizer.AddGrowableCol(0)
            boxSizer.Add(gridSizer,
                         flag=wx.EXPAND | wx.ALL,
                         border=5)

            border.Add(boxSizer,
                       flag=wx.LEFT | wx.RIGHT | wx.UP | wx.EXPAND,
                       border=5)

        gridSizer = wx.GridBagSizer(hgap=3, vgap=3)

        row = 0
        for k in ['method', 'maxcols', 'maxrows', 'o', 'bgcolor']:

            if k in self.params:
                param = self.params[k]
            elif k in self.flags:
                param = self.flags[k]

            if param is None:
                continue

            if k in labels or k == 'o':
                if k != 'o':
                    label = labels[k]
                else:
                    label = param

                gridSizer.Add(label,
                              flag=wx.ALIGN_LEFT |
                              wx.ALIGN_CENTER_VERTICAL,
                              pos=(row, 0))

            if k != 'o':
                gridSizer.Add(param,
                              flag=wx.ALIGN_RIGHT |
                              wx.ALIGN_CENTER_VERTICAL,
                              pos=(row, 1))
            row += 1

        gridSizer.AddGrowableCol(0)
        border.Add(gridSizer,
                   flag=wx.LEFT | wx.RIGHT | wx.TOP | wx.EXPAND,
                   border=5)

        if self.params['urlparams']:
            gridSizer = wx.GridBagSizer(hgap=3, vgap=3)

            row = 0
            gridSizer.Add(labels['urlparams'],
                          flag=wx.ALIGN_LEFT |
                          wx.ALIGN_CENTER_VERTICAL,
                          pos=(row, 0))

            gridSizer.Add(self.params['urlparams'],
                          flag=wx.ALIGN_RIGHT |
                          wx.ALIGN_CENTER_VERTICAL | wx.EXPAND,
                          pos=(row, 1))

            gridSizer.AddGrowableCol(1)

            border.Add(gridSizer,
                       flag=wx.LEFT | wx.RIGHT | wx.TOP | wx.EXPAND,
                       border=5)

        adv_setts_panel.SetSizer(border)
        adv_setts_panel.SetAutoLayout(True)
        adv_setts_panel.SetupScrolling()

    def OnUp(self, event):
        """Move selected layer up
        """
        if self.l_odrder_list.GetSelections():
            pos = self.l_odrder_list.GetSelection()
            if pos:
                self.sel_layers.insert(pos - 1, self.sel_layers.pop(pos))
            if pos > 0:
                self._updateLayerOrderList(selected=(pos - 1))
            else:
                self._updateLayerOrderList(selected=0)

    def OnDown(self, event):
        """Move selected to down
        """
        if self.l_odrder_list.GetSelections():
            pos = self.l_odrder_list.GetSelection()
            if pos != len(self.sel_layers) - 1:
                self.sel_layers.insert(pos + 1, self.sel_layers.pop(pos))
            if pos < len(self.sel_layers) - 1:
                self._updateLayerOrderList(selected=(pos + 1))
            else:
                self._updateLayerOrderList(selected=len(self.sel_layers) - 1)

    def _updateLayerOrderList(self, selected=None):
        """Update order in list.
        """
        def getlayercaption(l):
            if l['title']:
                cap = (l['title'])
            else:
                cap = (l['name'])

            if l['style']:
                if l['style']['title']:
                    cap += ' / ' + l['style']['title']
                else:
                    cap += ' / ' + l['style']['name']
            return cap

        layer_capts = [getlayercaption(l) for l in self.sel_layers]
        self.l_odrder_list.Set(layer_capts)
        if self.l_odrder_list.IsEmpty():
            self.enableButtons(False)
        else:
            self.enableButtons(True)
            if selected is not None:
                self.l_odrder_list.SetSelection(selected)
                self.l_odrder_list.EnsureVisible(selected)

    def OnTransparent(self, event):
        checked = event.IsChecked()
        if checked:
            self.params['bgcolor'].Enable(True)
        else:
            self.params['bgcolor'].Enable(False)

    def ConnectToServer(self, url, username, password):
        """Download and parse data from capabilities file.

        :param url: server url
        :type url: str
        :param username: username for connection
        :type username: str
        :param password: password for connection
        :type password: str
        """
        self._prepareForNewConn(url, username, password)
        cap_cmd = [
            'r.in.wms',
            '-c',
            ('capfile_output=%s' % self.cap_file),
            '--overwrite'] + self.ws_cmdl

        self.currentPid = self.cmd_thread.GetId()
        self.cmd_thread.RunCmd(cap_cmd, stderr=self.cmdStdErr)

    def OnCmdOutput(self, event):
        """Manage cmd output.
        """
        if Debug.GetLevel() != 0:
            Debug.msg(1, event.text)
        elif event.type != 'message' and event.type != 'warning':
            self.cmd_err_str += event.text + os.linesep

    def _prepareForNewConn(self, url, username, password):
        """Prepare panel for new connection
        """
        self.is_connected = False

        self.sel_layers = []
        self.formats_list = []
        self.projs_list = []

        self.conn = {
            'url': url,
            'password': password,
            'username': username
        }

        conn_cmd = []
        for k, v in six.iteritems(self.conn):
            if v:
                conn_cmd.append("%s=%s" % (k, v))

        self.ws_cmdl = self.ws_drvs[self.ws]['cmd'] + conn_cmd

    def OnCapDownloadDone(self, event):
        """Process donwloaded capabilities file and emits capParsed
        signal (see class constructor).
        """
        if event.pid != self.currentPid:
            return

        if event.returncode != 0:
            if self.cmd_err_str:
                self.cmd_err_str = _(
                    "Unable to download %s capabilities file\nfrom <%s>:\n" %
                    (self.ws.replace('_', ' '),
                     self.conn['url'])) + self.cmd_err_str
            self._postCapParsedEvt(error_msg=self.cmd_err_str)
            self.cmd_err_str = ''
            return

        self._parseCapFile(self.cap_file)

    def _parseCapFile(self, cap_file):
        """Parse capabilities data and emits capParsed signal
        (see class constructor).
        """
        try:
            self.cap = self.ws_drvs[self.ws]['cap_parser'](cap_file)
        except (IOError, ParseError) as error:
            error_msg = _(
                "%s web service was not found in fetched capabilities file from <%s>:\n%s\n" %
                (self.ws, self.conn['url'], str(error)))
            if Debug.GetLevel() != 0:
                Debug.msg(1, error_msg)
                self._postCapParsedEvt(None)
            else:
                self._postCapParsedEvt(error_msg=error_msg)
            return

        self.is_connected = True

        # WMS standard has formats defined for all layers
        if 'WMS' in self.ws:
            self.formats_list = sorted(self._getFormats())
            self._updateFormatRadioBox(self.formats_list)
            self._setDefaultFormatVal()

        self.list.LoadData(self.cap)
        self.OnListSelChanged(event=None)

        self._postCapParsedEvt(None)

    def ParseCapFile(self, url, username, password, cap_file=None,):
        """Parse capabilities data and emits capParsed signal
        (see class constructor).
        """
        self._prepareForNewConn(url, username, password)

        if cap_file is None or not url:
            self._postCapParsedEvt(None)
            return

        shutil.copyfile(cap_file, self.cap_file)

        self._parseCapFile(self.cap_file)

    def UpdateWidgetsByCmd(self, cmd):
        """Update panel widgets accordnig to passed cmd tuple

        :param cmd: cmd in tuple
        """

        dcmd = cmd[1]

        layers = []

        if 'layers' in dcmd:
            layers = dcmd['layers']

        styles = []
        if 'styles' in dcmd:
            styles = dcmd['styles']

        if 'WMS' in self.ws:
            layers = layers.split(',')
            styles = styles.split(',')
        else:
            layers = [layers]
            styles = [styles]

        if len(layers) != len(styles):
            styles = [''] * len(layers)

        l_st_list = []
        for i in range(len(layers)):
            l_st_list.append({'style': styles[i],
                              'layer': layers[i]})

        # WMS standard - first layer in params is most bottom...
        # therefore layers order need to be reversed
        l_st_list = [l for l in reversed(l_st_list)]
        self.list.SelectLayers(l_st_list)

        params = {}
        if 'format' in dcmd:
            params['format'] = dcmd['format']
        if 'srs' in dcmd:
            params['srs'] = 'EPSG:' + dcmd['srs']
        if 'method' in dcmd:
            params['method'] = dcmd['method']

        for p, v in six.iteritems(params):
            if self.params[p]:
                self.params[p].SetStringSelection(v)

        for p, conv_f in [
                ('urlparams', None),
                ('maxcols', int),
                ('maxrows', int)]:
            if p in dcmd:
                v = dcmd[p]
                if conv_f:
                    v = conv_f(v)
                self.params[p].SetValue(v)

        if 'flags' in dcmd and \
           'o' in dcmd['flags']:
            self.flags['o'].SetValue(1)
            self.params['bgcolor'].Enable(True)

        if 'bgcolor' in dcmd and \
           self.params['bgcolor']:
            bgcolor = dcmd['bgcolor'].strip().lower()
            if len(bgcolor) == 8 and \
               '0x' == bgcolor[:2]:

                colour = '#' + bgcolor[2:]
                self.params['bgcolor'].SetColour(colour)

    def IsConnected(self):
        """Was successful in downloading and parsing capabilities data?
        """
        return self.is_connected

    def _postCapParsedEvt(self, error_msg):
        """Helper function
        """
        self.capParsed.emit(error_msg=error_msg)

    def CreateCmd(self):
        """Create d.wms cmd from values of panels widgets

        :return: cmd list
        :return: None if required widgets do not have selected/filled values.
        """

        # check required widgets
        if not self._checkImportValues():
            return None

        # create d.wms command
        lcmd = self.ws_cmdl
        lcmd = ['d.wms'] + lcmd

        layers = "layers="
        styles = 'styles='
        first = True

        # WMS standard - first layer in params is most bottom...
        # therefore layers order need to be reversed
        for layer in reversed(self.sel_layers):
            if not first:
                layers += ','
                styles += ','
            first = False
            layers += layer['name']
            if layer['style'] is not None:
                styles += layer['style']['name']

        lcmd.append(layers)
        lcmd.append(styles)

        if 'format' not in self.drv_props['ignored_params']:
            i_format = self.params['format'].GetSelection()
            lcmd.append("format=%s" % self.formats_list[i_format])

        if 'srs' not in self.drv_props['ignored_params']:
            i_srs = self.params['srs'].GetSelection()
            srs = self.projs_list[i_srs].split(':')[-1]
            epsg_num = int(''.join(re.findall(r'\d+', srs)))

            lcmd.append("srs=%s" % epsg_num)

        for k in ['maxcols', 'maxrows', 'urlparams']:
            lcmd.append(k + '=' + str(self.params[k].GetValue()))

        i_method = self.params['method'].GetSelection()
        lcmd.append('method=' + self.reproj_methods[i_method])

        if 'o' not in self.drv_props['ignored_flags'] and \
                self.flags['o'].IsChecked():
            lcmd.append('-o')

            c = self.params['bgcolor'].GetColour()
            hex_color = wx.Colour(
                c[0],
                c[1],
                c[2]).GetAsString(
                wx.C2S_HTML_SYNTAX)
            lcmd.append("bgcolor=" + '0x' + hex_color[1:])

        lcmd.append("map=" + self.o_layer_name)

        return lcmd

    def OnListSelChanged(self, event):
        """Update widgets according to selected layer in list.
        """
        curr_sel_ls = self.list.GetSelectedLayers()
        # update self.sel_layers (selected layer list)
        if 'WMS' in self.ws:
            for sel_l in self.sel_layers[:]:
                if sel_l not in curr_sel_ls:
                    self.sel_layers.remove(sel_l)

            for l in curr_sel_ls:
                if l not in self.sel_layers:
                    self.sel_layers.append(l)

            self._updateLayerOrderList()
        else:
            self.sel_layers = curr_sel_ls

        # update projection

        self.projs_list = []
        projs_list = []

        intersect_proj = []
        first = True
        for l in curr_sel_ls:
            layer_projs = l['cap_intf_l'].GetLayerData('srs')
            if first:
                projs_list = layer_projs
                first = False
                continue

            projs_list = set(projs_list).intersection(layer_projs)

        if 'srs' not in self.drv_props['ignored_params']:

            for proj in projs_list:
                proj_code = Srs(proj.strip()).getcode()
                proj_spl = proj_code.split(':')
                if proj_spl[0].strip().lower() in self.drv_info.GetSrs():
                    # accept ogc:crs code
                    self.projs_list.append(proj_code)

            cur_sel = self.params['srs'].GetStringSelection()

            self.projs_list = sorted(self.projs_list)
            self.params['srs'].SetItems(self.projs_list)

            if cur_sel:
                self.params['srs'].SetStringSelection(cur_sel)
            else:
                try:
                    i = self.projs_list.index('EPSG:4326')
                    self.params['srs'].SetSelection(i)
                except ValueError:
                    if len(self.projs_list) > 0:
                        self.params['srs'].SetSelection(0)

        # update format

        if 'WMS' not in self.ws and \
           'format' not in self.drv_props['ignored_params']:
            self.formats_list = []
            cur_sel = None

            if self.params['format']:
                cur_sel = self.params['format'].GetStringSelection()

            if len(curr_sel_ls) > 0:
                self.formats_list = sorted(
                    self._getFormats(
                        curr_sel_ls[0]['cap_intf_l']))
                self._updateFormatRadioBox(self.formats_list)

                if cur_sel:
                    if self.params['format']:
                        self.params['format'].SetStringSelection(cur_sel)
                else:
                    self._setDefaultFormatVal()

        self.Layout()

    def _setDefaultFormatVal(self):
        """Set default format value.
        """
        try:
            i = self.formats_list.index('png')
            self.params['format'].SetSelection(i)
        except ValueError:
            pass

    def _updateFormatRadioBox(self, formats_list):
        """Helper function
        """
        if self.params['format']:
            self.req_page_sizer.Detach(self.params['format'])
            self.params['format'].Destroy()
        if len(self.formats_list) > 0:
            self.params['format'] = wx.RadioBox(
                parent=self.req_page_panel,
                id=wx.ID_ANY,
                label=_("Source image format"),
                pos=wx.DefaultPosition,
                choices=formats_list,
                majorDimension=4,
                style=wx.RA_SPECIFY_COLS)
            self.source_sizer.Insert(2, window=self.params['format'],
                                     flag=wx.LEFT | wx.RIGHT | wx.BOTTOM,
                                     border=5)

    def _getFormats(self, layer=None):
        """Get formats

        WMS has formats defined generally for whole cap.
        In WMTS and NASA OnEarh formats are defined for layer.
        """
        formats_label = []
        if layer is None:
            formats_list = self.cap.GetFormats()
        else:
            formats_list = layer.GetLayerData('format')

        for frmt in formats_list:
            frmt = frmt.strip()
            label = self.drv_info.GetFormatLabel(frmt)

            if label:
                formats_label.append(label)

        return formats_label

    def _checkImportValues(self,):
        """Check if required widgets are selected/filled
        """
        warning_str = ""
        show_war = False
        if not self.list or not self.list.GetSelectedLayers():
            warning_str += _("Select layer in layer list.\n")
            show_war = True

        if self.params['format'] is not None and \
           self.params['format'].GetSelection() == -1:
            warning_str += _("Select source image format.\n")
            show_war = True

        if self.params['srs'] is not None and \
           self.params['srs'].GetSelection() == -1:
            warning_str += _("Select source projection.\n")
            show_war = True

        if not self.o_layer_name:
            warning_str += _("Choose output layer name.\n")
            show_war = True

        if show_war:
            GMessage(parent=self.parent, message=warning_str)
            return False

        return True

    def SetOutputLayerName(self, name):
        """Set name of layer to be added to layer tree
        """
        self.o_layer_name = name

    def GetOutputLayerName(self):
        return self.o_layer_name

    def GetCapFile(self):
        """Get path to file where capabilities are saved
        """
        return self.cap_file

    def GetWebService(self):
        """Get web service
        """
        return self.ws

class LayersList(TreeCtrl):

    def __init__(self, parent, web_service, style, pos=wx.DefaultPosition):
        """List of layers and styles available in capabilities file
        """
        self.parent = parent
        self.ws = web_service

        TreeCtrl.__init__(self, parent=parent, id=wx.ID_ANY, style=style)

        self.root = None
        self.Bind(wx.EVT_TREE_SEL_CHANGING, self.OnListSelChanging)

        self.layerSelected = Signal('LayersList.layerSelected')

    def LoadData(self, cap=None):
        """Load data into list
        """
        # detete first all items
        self.DeleteAllItems()

        if not cap:
            return

        def AddLayerChildrenToTree(parent_layer, parent_item):
            """Recursive function which adds all capabilities
            layers/styles to the LayersList.
            """
            def gettitle(layer):
                """Helper function"""
                if layer.GetLayerData('title') is not None:
                    layer_title = layer.GetLayerData('title')
                elif layer.GetLayerData('name') is not None:
                    layer_title = layer.GetLayerData('name')
                else:
                    layer_title = str(layer.GetId())

                return layer_title

            def addlayer(layer, item):
                styles = layer.GetLayerData('styles')

                def_st = None
                for st in styles:

                    if st['name']:
                        style_name = st['name']
                    else:
                        continue

                    if st['title']:
                        style_name = st['title']

                    if st['isDefault']:
                        def_st = st

                    style_item = self.AppendItem(item, style_name)

                    self.SetItemData(style_item, {'type': 'style',
                                                'layer': layer,  # it is parent layer of style
                                                'style': st})

                self.SetItemData(item, {'type': 'layer',  # is it layer or style?
                                      'layer': layer,  # Layer instance from web_services.cap_interface
                                      'style': def_st})  # layer can have assigned default style

            if parent_layer is None:
                parent_layer = cap.GetRootLayer()
                layer_title = gettitle(parent_layer)
                parent_item = self.AddRoot(layer_title)
                addlayer(parent_layer, parent_item)

            for layer in parent_layer.GetChildren():
                item = self.AppendItem(parent_item, gettitle(layer))
                addlayer(layer, item)
                AddLayerChildrenToTree(layer, item)

        AddLayerChildrenToTree(None, None)
        # self.ExpandAll(self.GetRootItem())

    def GetSelectedLayers(self):
        """Get selected layers/styles in LayersList

        :return: dict with these items:
                 * 'name'  : layer name used for request
                   if it is style, it is name of parent layer
                 * 'title' : layer title
                 * 'style' : {'name' : 'style name', title : 'style title'}
                 * 'cap_intf_l' : \*Layer instance from web_services.cap_interface
        """
        sel_layers = self.GetSelections()
        sel_layers_dict = []
        for s in sel_layers:
            try:
                layer = self.GetItemData(s)['layer']
            except ValueError:
                continue
            sel_layers_dict.append({
                'name': layer.GetLayerData('name'),
                'title': layer.GetLayerData('title'),
                'style': self.GetItemData(s)['style'],
                'cap_intf_l': layer
            })
        return sel_layers_dict

    def OnListSelChanging(self, event):
        """Do not allow selecting items, which cannot be requested from server.
        """
        def _emitSelected(layer):
            title = layer.GetLayerData('title')
            self.layerSelected.emit(title=title)

        def _selectRequestableChildren(item, list_to_check, items_to_sel):

            self.Expand(item)
            child_item, cookie = self.GetFirstChild(item)
            while child_item and child_item.IsOk():
                if  self.GetItemData(child_item)['layer'].IsRequestable() \
                        and not self.IsSelected(child_item):
                    items_to_sel.append(child_item)
                elif not self.GetItemData(child_item)['layer'].IsRequestable():
                    list_to_check.append(child_item)

                child_item, cookie = self.GetNextChild(item, cookie)

        cur_item = event.GetItem()
        if not self.GetItemData(cur_item)['layer'].IsRequestable():
            event.Veto()

            if not self.HasFlag(wx.TR_MULTIPLE):
                return

            _emitSelected(self.GetItemData(cur_item)['layer'])

            items_to_chck = []
            items_to_sel = []
            chck_item = cur_item

            while True:
                _selectRequestableChildren(
                    chck_item, items_to_chck, items_to_sel)
                if items_to_chck:
                    chck_item = items_to_chck.pop()
                else:
                    break

            while items_to_sel:
                self.SelectItem(items_to_sel.pop(), select=True)
        else:
            _emitSelected(self.GetItemData(cur_item)['layer'])

    def GetItemCount(self):
        """Required for listmix.ListCtrlAutoWidthMixin
        """
        return 0

    def GetCountPerPage(self):
        """Required for listmix.ListCtrlAutoWidthMixin
        """
        return 0

    def SelectLayers(self, l_st_list):
        """Select layers/styles in LayersList

        :param l_st_list: [{style : 'style_name', layer : 'layer_name'}, ...]
        :return: items from l_st_list which were not found
        """
        def checknext(root_item, l_st_list, items_to_sel):
            def compare(item, l_name, st_name):
                it_l_name = self.GetItemData(item)['layer'].GetLayerData('name')
                it_st = self.GetItemData(item)['style']
                it_type = self.GetItemData(item)['type']

                if it_l_name == l_name and ((not it_st and not st_name) or (
                        it_st and it_st['name'] == st_name and it_type == 'style')):

                    return True

                return False

            (child, cookie) = self.GetFirstChild(root_item)
            while child.IsOk():
                for i, l_st in enumerate(l_st_list):
                    l_name = l_st['layer']
                    st_name = l_st['style']

                    if compare(child, l_name, st_name):
                        items_to_sel[i] = [child, l_st]
                        break

                if len(items_to_sel) == len(l_st_list):
                    if self.ItemHasChildren(child):
                        checknext(child, l_st_list, items_to_sel)
                    child = self.GetNextSibling(child)

        self.UnselectAll()

        l_st_list = deepcopy(l_st_list)
        root_item = self.GetRootItem()

        items_to_sel = [None] * len(l_st_list)
        checknext(root_item, l_st_list, items_to_sel)
        self.CollapseAll()

        # items are selected according to position in l_st_list
        # to be added to Layers order list in right order
        for i in items_to_sel:
            if not i:
                continue

            item, l_st = i
            keep = False
            if self.HasFlag(wx.TR_MULTIPLE):
                keep = True

            self.SelectItem(item, select=keep)
            self.SetFocusedItem(item)
            self.Expand(item)
            l_st_list.remove(l_st)

        return l_st_list


class WSManageSettingsWidget(ManageSettingsWidget):

    def __init__(self, parent, settingsFile, default_servers):

        ManageSettingsWidget.__init__(self, parent, settingsFile)
        self.default_servers = default_servers

    def _layout(self):

        self.btnAddDefaultServers = Button(parent=self, id=wx.ID_ANY,
                                           label=_("Add default"))
        self.btnAddDefaultServers.Bind(wx.EVT_BUTTON, self.OnAddDefaultServers)

        ManageSettingsWidget._layout(self)
        self.settingsSizer.Add(self.btnAddDefaultServers,
                               flag=wx.RIGHT,
                               border=5)

    def OnAddDefaultServers(self, event):

        setts = self.GetSettings()
        self.servers_to_add = {}
        for k, v in six.iteritems(self.default_servers):
            if k not in six.iterkeys(setts):
                self.servers_to_add[k] = v
            elif v != setts[k]:
                GMessage(parent=self,
                         message=_("User defined server with same name "
                                   "as default server <%s> already exists.\n"
                                   "Keeping user defined server") % (k))

        if self.servers_to_add:
            self.AddSettings(self.servers_to_add)
