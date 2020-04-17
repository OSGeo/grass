"""
@package gui_core.forms

@brief Construct simple wxPython GUI from a GRASS command interface
description.

Classes:
 - forms::UpdateThread
 - forms::UpdateQThread
 - forms::TaskFrame
 - forms::CmdPanel
 - forms::GUI
 - forms::GrassGUIApp

This program is just a coarse approach to automatically build a GUI
from a xml-based GRASS user interface description.

You need to have Python 2.4, wxPython 2.8 and python-xml.

The XML stream is read from executing the command given in the
command line, thus you may call it for instance this way:

python <this file.py> r.basins.fill

Or you set an alias or wrap the call up in a nice shell script, GUI
environment ... please contribute your idea.

Updated to wxPython 2.8 syntax and contrib widgets.  Methods added to
make it callable by gui.  Method added to automatically re-run with
pythonw on a Mac.

.. todo::
    verify option value types

Copyright(C) 2000-2015 by the GRASS Development Team

This program is free software under the GPL(>=v2) Read the file
COPYING coming with GRASS for details.

@author Jan-Oliver Wagner <jan@intevation.de>
@author Bernhard Reiter <bernhard@intevation.de>
@author Michael Barton, Arizona State University
@author Daniel Calvelo <dca.gis@gmail.com>
@author Martin Landa <landa.martin@gmail.com>
@author Luca Delucchi <lucadeluge@gmail.com>
@author Stepan Turek <stepan.turek seznam.cz> (CoordinatesSelect)
"""

from __future__ import print_function

import sys
import textwrap
import os
import copy
import locale
import six

if sys.version_info.major == 2:
    import Queue
else:
    import queue as Queue
    unicode = str

import re
import codecs

from threading import Thread

import wx
try:
    import wx.lib.agw.flatnotebook as FN
except ImportError:
    import wx.lib.flatnotebook as FN
import wx.lib.colourselect as csel
import wx.lib.filebrowsebutton as filebrowse
from wx.lib.newevent import NewEvent

try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree  # Python <= 2.4

# needed when started from command line and for testing
if __name__ == '__main__':
    if os.getenv("GISBASE") is None:
        # intentionally not translatable
        sys.exit("Failed to start. GRASS GIS is not running"
                 " or the installation is broken.")
    from grass.script.setup import set_gui_path
    set_gui_path()

from grass.pydispatch.signal import Signal

from grass.script import core as grass
from grass.script import task as gtask

from core import globalvar
from gui_core.widgets import StaticWrapText, ScrolledPanel, ColorTablesComboBox, \
    BarscalesComboBox, NArrowsComboBox
from gui_core.ghelp import HelpPanel
from gui_core import gselect
from core import gcmd
from core import utils
from core.settings import UserSettings
from gui_core.widgets import FloatValidator, GNotebook, FormNotebook, FormListbook
from core.giface import Notification
from gui_core.widgets import LayersList
from gui_core.wrap import BitmapFromImage, Button, StaticText, StaticBox, SpinCtrl, \
    CheckBox, BitmapButton, TextCtrl, NewId
from core.debug import Debug

wxUpdateDialog, EVT_DIALOG_UPDATE = NewEvent()


"""Hide some options in the GUI"""
#_blackList = { 'enabled' : False,
#               'items'   : { 'r.buffer' : {'params' : ['input', 'output'],
#                                           'flags' : ['z', 'overwrite']}}}
_blackList = {'enabled': False,
              'items': {}}


def text_beautify(someString, width=70):
    """Make really long texts shorter, clean up whitespace and remove
    trailing punctuation.
    """
    if width > 0:
        return escape_ampersand(
            os.linesep.join(
                textwrap.wrap(
                    utils.normalize_whitespace(someString),
                    width)).strip(".,;:"))
    else:
        return escape_ampersand(
            utils.normalize_whitespace(someString).strip(".,;:"))


def escape_ampersand(text):
    """Escapes ampersands with additional ampersand for GUI"""
    return text.replace("&", "&&")


class UpdateThread(Thread):
    """Update dialog widgets in the thread"""

    def __init__(self, parent, event, eventId, task):
        Thread.__init__(self)

        self.parent = parent
        self.event = event
        self.eventId = eventId
        self.task = task
        self.setDaemon(True)

        # list of functions which updates the dialog
        self.data = {}

    def run(self):
        # get widget id
        if not self.eventId:
            for p in self.task.params:
                if p.get('gisprompt', False) == False:
                    continue
                prompt = p.get('element', '')
                if prompt == 'vector':
                    name = p.get('name', '')
                    if name in ('map', 'input'):
                        self.eventId = p['wxId'][0]
            if self.eventId is None:
                return

        p = self.task.get_param(self.eventId, element='wxId', raiseError=False)
        if not p or 'wxId-bind' not in p:
            return

        # is this check necessary?
        # get widget prompt
        # pType = p.get('prompt', '')
        # if not pType:
        #     return

        # check for map/input parameter
        pMap = self.task.get_param('map', raiseError=False)

        if not pMap:
            pMap = self.task.get_param('input', raiseError=False)

        if pMap:
            map = pMap.get('value', '')
        else:
            map = None

        # avoid running db.describe several times
        cparams = dict()
        cparams[map] = {'dbInfo': None,
                        'layers': None, }

        # update reference widgets
        for uid in p['wxId-bind']:
            win = self.parent.FindWindowById(uid)
            if not win:
                continue

            name = win.GetName()

            # @todo: replace name by isinstance() and signals

            pBind = self.task.get_param(uid, element='wxId', raiseError=False)
            if pBind:
                pBind['value'] = ''

            # set appropriate types in t.* modules and g.list/remove element
            # selections
            if name == 'Select':
                type_param = self.task.get_param(
                    'type', element='name', raiseError=False)

                if 'all' in type_param.get('value'):
                    etype = type_param.get('values')[:]
                    if 'all' in etype:
                        etype.remove('all')
                    etype = ','.join(etype)
                else:
                    etype = type_param.get('value')

                if globalvar.CheckWxVersion([3]):
                    self.data[win.SetElementList] = {'type': etype}
                else:
                    self.data[win.GetParent().SetElementList] = {'type': etype}

                # t.(un)register has one type for 'input', 'maps'
                maps_param = self.task.get_param(
                    'maps', element='name', raiseError=False)
                if self.task.get_name().startswith('t') and maps_param is not None:
                    if maps_param['wxId'][0] != uid:
                        element_dict = {
                            'raster': 'strds',
                            'vector': 'stvds',
                            'raster_3d': 'str3ds'}
                        self.data[
                            win.GetParent().SetType] = {
                            'etype': element_dict[
                                type_param.get('value')]}

            map = layer = None
            driver = db = None
            if name in ('LayerSelect', 'ColumnSelect', 'SqlWhereSelect'):
                if p.get('element', '') == 'vector':  # -> vector
                    # get map name
                    map = p.get('value', '')

                    # get layer
                    for bid in p['wxId-bind']:
                        p = self.task.get_param(
                            bid, element='wxId', raiseError=False)
                        if not p:
                            continue

                        if p.get('element', '') in ['layer', 'layer_all']:
                            layer = p.get('value', '')
                            if layer != '':
                                layer = p.get('value', '')
                            else:
                                layer = p.get('default', '')
                            break

                elif p.get('element', '') in ['layer', 'layer_all']:  # -> layer
                    # get layer
                    layer = p.get('value', '')
                    if layer != '':
                        layer = p.get('value', '')
                    else:
                        layer = p.get('default', '')

                    # get map name
                    pMapL = self.task.get_param(
                        p['wxId'][0], element='wxId-bind', raiseError=False)
                    if pMapL:
                        gui_deps = pMapL.get('guidependency', None)
                        if gui_deps:
                            gui_deps = gui_deps.split(',')
                        if not gui_deps or (gui_deps and p.get('name', '') in gui_deps):
                            map = pMapL.get('value', '')

            if name == 'TableSelect' or \
                    (name == 'ColumnSelect' and not map):
                pDriver = self.task.get_param(
                    'dbdriver', element='prompt', raiseError=False)
                if pDriver:
                    driver = pDriver.get('value', '')
                pDb = self.task.get_param(
                    'dbname', element='prompt', raiseError=False)
                if pDb:
                    db = pDb.get('value', '')
                if name == 'ColumnSelect':
                    pTable = self.task.get_param(
                        'dbtable', element='element', raiseError=False)
                    if pTable:
                        table = pTable.get('value', '')

            if name == 'LayerSelect':
                # determine format
                native = True

                if pMap:
                    for id in pMap['wxId']:
                        winVec = self.parent.FindWindowById(id)
                        if winVec.GetName() == 'VectorFormat' and \
                                winVec.GetSelection() != 0:
                            native = False
                            break
                # TODO: update only if needed
                if native:
                    if map:
                        self.data[win.InsertLayers] = {'vector': map}
                    else:
                        self.data[win.InsertLayers] = {}
                else:
                    if map:
                        self.data[win.InsertLayers] = {
                            'dsn': map.rstrip('@OGR')}
                    else:
                        self.data[win.InsertLayers] = {}

            elif name == 'TableSelect':
                self.data[win.InsertTables] = {'driver': driver,
                                               'database': db}

            elif name == 'ColumnSelect':
                if map:
                    if map not in cparams:
                        cparams[map] = {'dbInfo': None,
                                        'layers': None, }

                    if not cparams[map]['dbInfo']:
                        cparams[map]['dbInfo'] = gselect.VectorDBInfo(map)
                    self.data[win.GetParent().InsertColumns] = {
                        'vector': map, 'layer': layer,
                        'dbInfo': cparams[map]['dbInfo']}
                else:  # table
                    if driver and db:
                        self.data[win.GetParent().InsertTableColumns] = {
                            'table': pTable.get('value'),
                            'driver': driver, 'database': db}
                    elif pTable:
                        self.data[win.GetParent().InsertTableColumns] = {
                            'table': pTable.get('value')}

            elif name == 'SubGroupSelect':
                self.data[win.Insert] = {'group': p.get('value', '')}

            elif name == 'SignatureSelect':
                if p.get('prompt', 'group') == 'group':
                    group = p.get('value', '')
                    pSubGroup = self.task.get_param(
                        'subgroup', element='prompt', raiseError=False)
                    if pSubGroup:
                        subgroup = pSubGroup.get('value', '')
                    else:
                        subgroup = None
                else:
                    subgroup = p.get('value', '')
                    pGroup = self.task.get_param(
                        'group', element='prompt', raiseError=False)
                    if pGroup:
                        group = pGroup.get('value', '')
                    else:
                        group = None

                self.data[win.Insert] = {'group': group,
                                         'subgroup': subgroup}

            elif name == 'LocationSelect':
                pDbase = self.task.get_param(
                    'dbase', element='element', raiseError=False)
                if pDbase:
                    self.data[
                        win.UpdateItems] = {
                        'dbase': pDbase.get(
                            'value', '')}

            elif name == 'MapsetSelect':
                pDbase = self.task.get_param(
                    'dbase', element='element', raiseError=False)
                pLocation = self.task.get_param(
                    'location', element='element', raiseError=False)
                if pDbase and pLocation:
                    self.data[
                        win.UpdateItems] = {
                        'dbase': pDbase.get(
                            'value', ''), 'location': pLocation.get(
                            'value', '')}

            elif name == 'ProjSelect':
                pDbase = self.task.get_param(
                    'dbase', element='element', raiseError=False)
                pLocation = self.task.get_param(
                    'location', element='element', raiseError=False)
                pMapset = self.task.get_param(
                    'mapset', element='element', raiseError=False)
                if pDbase and pLocation and pMapset:
                    self.data[
                        win.UpdateItems] = {
                        'dbase': pDbase.get(
                            'value', ''), 'location': pLocation.get(
                            'value', ''), 'mapset': pMapset.get(
                            'value', '')}

            elif name == 'SqlWhereSelect':
                if map:
                    self.data[win.GetParent().SetData] = {
                        'vector': map, 'layer': layer }
                # TODO: table?

def UpdateDialog(parent, event, eventId, task):
    return UpdateThread(parent, event, eventId, task)


class UpdateQThread(Thread):
    """Update dialog widgets in the thread"""
    requestId = 0

    def __init__(self, parent, requestQ, resultQ, **kwds):
        Thread.__init__(self, **kwds)

        self.parent = parent  # cmdPanel
        self.setDaemon(True)

        self.requestQ = requestQ
        self.resultQ = resultQ

        self.start()

    def Update(self, callable, *args, **kwds):
        UpdateQThread.requestId += 1

        self.request = None
        self.requestQ.put((UpdateQThread.requestId, callable, args, kwds))

        return UpdateQThread.requestId

    def run(self):
        while True:
            requestId, callable, args, kwds = self.requestQ.get()

            self.request = callable(*args, **kwds)

            self.resultQ.put((requestId, self.request.run()))

            if self.request:
                event = wxUpdateDialog(data=self.request.data)
                wx.PostEvent(self.parent, event)


class TaskFrame(wx.Frame):
    """This is the Frame containing the dialog for options input.

    The dialog is organized in a notebook according to the guisections
    defined by each GRASS command.

    If run with a parent, it may Apply, Ok or Cancel; the latter two
    close the dialog.  The former two trigger a callback.

    If run standalone, it will allow execution of the command.

    The command is checked and sent to the clipboard when clicking
    'Copy'.
    """

    def __init__(self, parent, giface, task_description, id=wx.ID_ANY,
                 get_dcmd=None, layer=None,
                 style=wx.DEFAULT_FRAME_STYLE | wx.TAB_TRAVERSAL, **kwargs):
        self.get_dcmd = get_dcmd
        self.layer = layer
        self.task = task_description
        self.parent = parent             # LayerTree | Modeler | None | ...
        self._giface = giface

        self.dialogClosing = Signal('TaskFrame.dialogClosing')

        # module name + keywords
        title = self.task.get_name()
        try:
            if self.task.keywords != ['']:
                title += " [" + ', '.join(self.task.keywords) + "]"
        except ValueError:
            pass

        wx.Frame.__init__(self, parent=parent, id=id, title=title,
                          name="MainFrame", style=style, **kwargs)

        self.locale = wx.Locale(language=wx.LANGUAGE_DEFAULT)

        self.panel = wx.Panel(parent=self, id=wx.ID_ANY)

        # statusbar
        self.CreateStatusBar()

        # icon
        self.SetIcon(
            wx.Icon(
                os.path.join(
                    globalvar.ICONDIR,
                    'grass_dialog.ico'),
                wx.BITMAP_TYPE_ICO))

        guisizer = wx.BoxSizer(wx.VERTICAL)

        # set apropriate output window
        if self.parent:
            self.standalone = False
        else:
            self.standalone = True

        # logo + description
        topsizer = wx.BoxSizer(wx.HORIZONTAL)

        # GRASS logo
        self.logo = wx.StaticBitmap(
            self.panel, -1,
            wx.Bitmap(
                name=os.path.join(
                    globalvar.IMGDIR,
                    'grass_form.png'),
                type=wx.BITMAP_TYPE_PNG))
        topsizer.Add(self.logo, proportion=0, border=3,
                     flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL)

        # add module description
        if self.task.label:
            module_desc = self.task.label + ' ' + self.task.description
        else:
            module_desc = self.task.description

        self.description = StaticWrapText(parent=self.panel,
                                          label=module_desc)
        topsizer.Add(self.description, proportion=1, border=5,
                     flag=wx.ALL | wx.ALIGN_CENTER_VERTICAL | wx.EXPAND)

        guisizer.Add(topsizer, proportion=0, flag=wx.EXPAND)

        self.panel.SetSizerAndFit(guisizer)
        self.Layout()

        # notebooks
        self.notebookpanel = CmdPanel(
            parent=self.panel,
            giface=self._giface,
            task=self.task,
            frame=self)
        self._gconsole = self.notebookpanel._gconsole
        if self._gconsole:
            self._gconsole.mapCreated.connect(self.OnMapCreated)
            self._gconsole.updateMap.connect(
                lambda: self._giface.updateMap.emit())
        self.goutput = self.notebookpanel.goutput
        if self.goutput:
            self.goutput.showNotification.connect(
                lambda message: self.SetStatusText(message))

        self.notebookpanel.OnUpdateValues = self.updateValuesHook
        guisizer.Add(self.notebookpanel, proportion=1, flag=wx.EXPAND)

        # status bar
        status_text = _("Enter parameters for '") + self.task.name + "'"
        try:
            self.task.get_cmd()
            self.updateValuesHook()
        except ValueError:
            self.SetStatusText(status_text)

        # buttons
        btnsizer = wx.BoxSizer(orient=wx.HORIZONTAL)
        # cancel
        if sys.platform == 'darwin':
            # stock id automatically adds ctrl-c shortcut to close dialog
            self.btn_cancel = Button(parent=self.panel, label=_("Close"))
        else:
            self.btn_cancel = Button(parent=self.panel, id=wx.ID_CLOSE)
        self.btn_cancel.SetToolTip(
            _("Close this window without executing the command (Ctrl+Q)"))
        btnsizer.Add(
            self.btn_cancel,
            proportion=0,
            flag=wx.ALL | wx.ALIGN_CENTER,
            border=10)
        self.btn_cancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        # bind closing to ESC and CTRL+Q
        self.Bind(wx.EVT_MENU, self.OnCancel, id=wx.ID_CLOSE)
        accelTableList = [(wx.ACCEL_NORMAL, wx.WXK_ESCAPE, wx.ID_CLOSE)]
        accelTableList.append((wx.ACCEL_CTRL, ord('Q'), wx.ID_CLOSE))
        # TODO: bind Ctrl-t for tile windows here (trac #2004)

        if self.get_dcmd is not None:  # A callback has been set up
            btn_apply = Button(parent=self.panel, id=wx.ID_APPLY)
            btn_ok = Button(parent=self.panel, id=wx.ID_OK)
            btn_ok.SetDefault()

            btnsizer.Add(btn_apply, proportion=0,
                         flag=wx.ALL | wx.ALIGN_CENTER,
                         border=10)
            btnsizer.Add(btn_ok, proportion=0,
                         flag=wx.ALL | wx.ALIGN_CENTER,
                         border=10)

            btn_apply.Bind(wx.EVT_BUTTON, self.OnApply)
            btn_ok.Bind(wx.EVT_BUTTON, self.OnOK)
        else:  # We're standalone
            # run
            self.btn_run = Button(
                parent=self.panel, id=wx.ID_OK, label=_("&Run"))
            self.btn_run.SetToolTip(_("Run the command (Ctrl+R)"))
            self.btn_run.SetDefault()
            self.btn_run.SetForegroundColour(wx.Colour(35, 142, 35))

            btnsizer.Add(self.btn_run, proportion=0,
                         flag=wx.ALL | wx.ALIGN_CENTER,
                         border=10)

            self.btn_run.Bind(wx.EVT_BUTTON, self.OnRun)
            self.Bind(wx.EVT_MENU, self.OnRun, id=wx.ID_OK)
            accelTableList.append((wx.ACCEL_CTRL, ord('R'), wx.ID_OK))

        # copy
        if sys.platform == 'darwin':
            # stock id automatically adds ctrl-c shortcut to copy command
            self.btn_clipboard = Button(parent=self.panel, label=_("Copy"))
        else:
            self.btn_clipboard = Button(parent=self.panel, id=wx.ID_COPY)
        self.btn_clipboard.SetToolTip(
            _("Copy the current command string to the clipboard"))
        btnsizer.Add(self.btn_clipboard, proportion=0,
                     flag=wx.ALL | wx.ALIGN_CENTER,
                     border=10)
        self.btn_clipboard.Bind(wx.EVT_BUTTON, self.OnCopy)

        # help
        self.btn_help = Button(parent=self.panel, id=wx.ID_HELP)
        self.btn_help.SetToolTip(
            _("Show manual page of the command (Ctrl+H)"))
        self.btn_help.Bind(wx.EVT_BUTTON, self.OnHelp)
        self.Bind(wx.EVT_MENU, self.OnHelp, id=wx.ID_HELP)
        accelTableList.append((wx.ACCEL_CTRL, ord('H'), wx.ID_HELP))

        if self.notebookpanel.notebook.GetPageIndexByName('manual') < 0:
            self.btn_help.Hide()

        # add help button
        btnsizer.Add(
            self.btn_help,
            proportion=0,
            flag=wx.ALL | wx.ALIGN_CENTER,
            border=10)

        guisizer.Add(
            btnsizer,
            proportion=0,
            flag=wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT,
            border=30)
        # abort key bindings
        abortId = NewId()
        self.Bind(wx.EVT_MENU, self.OnAbort, id=abortId)
        accelTableList.append((wx.ACCEL_CTRL, ord('S'), abortId))
        # set accelerator table
        accelTable = wx.AcceleratorTable(accelTableList)
        self.SetAcceleratorTable(accelTable)

        if self._giface and self._giface.GetLayerTree():
            addLayer = False
            for p in self.task.params:
                if p.get('age', 'old') == 'new' and \
                   p.get('prompt', '') in ('raster', 'vector', 'raster_3d'):
                    addLayer = True

            if addLayer:
                # add newly created map into layer tree
                self.addbox = wx.CheckBox(
                    parent=self.panel,
                    label=_('Add created map(s) into layer tree'),
                    style=wx.NO_BORDER)
                self.addbox.SetValue(
                    UserSettings.Get(
                        group='cmd',
                        key='addNewLayer',
                        subkey='enabled'))
                guisizer.Add(self.addbox, proportion=0,
                             flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                             border=5)

        hasNew = False
        for p in self.task.params:
            if p.get('age', 'old') == 'new':
                hasNew = True
                break

        if self.get_dcmd is None and hasNew:
            # close dialog when command is terminated
            self.closebox = CheckBox(
                parent=self.panel,
                label=_('Close dialog on finish'),
                style=wx.NO_BORDER)
            self.closebox.SetValue(
                UserSettings.Get(
                    group='cmd',
                    key='closeDlg',
                    subkey='enabled'))
            self.closebox.SetToolTip(
                _(
                    "Close dialog when command is successfully finished. "
                    "Change this settings in Preferences dialog ('Command' tab)."))
            guisizer.Add(self.closebox, proportion=0,
                         flag=wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                         border=5)
        # bindings
        self.Bind(wx.EVT_CLOSE, self.OnCancel)

        # do layout
        # called automatically by SetSizer()
        self.panel.SetAutoLayout(True)
        self.panel.SetSizerAndFit(guisizer)

        sizeFrame = self.GetBestSize()
        self.SetMinSize(sizeFrame)

        if hasattr(self, "closebox"):
            scale = 0.33
        else:
            scale = 0.50
        self.SetSize(
            wx.Size(
                sizeFrame[0],
                sizeFrame[1] + scale * max(
                    self.notebookpanel.panelMinHeight,
                    self.notebookpanel.constrained_size[1])))

        # thread to update dialog
        # create queues
        self.requestQ = Queue.Queue()
        self.resultQ = Queue.Queue()
        self.updateThread = UpdateQThread(
            self.notebookpanel, self.requestQ, self.resultQ)

        self.Layout()

        # keep initial window size limited for small screens
        width, height = self.GetSize()
        self.SetSize(wx.Size(min(width, 650),
                             min(height, 500)))

        # fix goutput's pane size (required for Mac OSX)
        if self.goutput:
            self.goutput.SetSashPosition(int(self.GetSize()[1] * .75))

    def MakeModal(self, modal=True):
        if globalvar.wxPythonPhoenix:
            if modal and not hasattr(self, '_disabler'):
                self._disabler = wx.WindowDisabler(self)
            if not modal and hasattr(self, '_disabler'):
                del self._disabler
        else:
            super(TaskFrame, self).MakeModal(modal)

    def updateValuesHook(self, event=None):
        """Update status bar data"""
        self.SetStatusText(
            ' '.join(
                [gcmd.DecodeString(each)
                 if isinstance(each, str) else each
                 for each in self.notebookpanel.createCmd(
                     ignoreErrors=True)]))
        if event:
            event.Skip()

    def OnDone(self, event):
        """This function is launched from OnRun() when command is
        finished
        """
        if hasattr(self, "btn_cancel"):
            self.btn_cancel.Enable(True)

        if hasattr(self, "btn_clipboard"):
            self.btn_clipboard.Enable(True)

        if hasattr(self, "btn_help"):
            self.btn_help.Enable(True)

        if hasattr(self, "btn_run"):
            self.btn_run.Enable(True)

        if hasattr(self, "get_dcmd") and \
                self.get_dcmd is None and \
                hasattr(self, "closebox") and \
                self.closebox.IsChecked() and \
                (event.returncode == 0):
            # was closed also when aborted but better is leave it open
            wx.CallLater(2000, self.Close)

    def OnMapCreated(self, name, ltype):
        """Map created or changed

        :param name: map name
        :param ltype: layer type (prompt value)
        """
        if hasattr(self, "addbox") and self.addbox.IsChecked():
            add = True
        else:
            add = False

        if self._giface:
            self._giface.mapCreated.emit(name=name, ltype=ltype, add=add)

    def OnOK(self, event):
        """OK button pressed"""
        cmd = self.OnApply(event)
        if cmd is not None and self.get_dcmd is not None:
            self.OnCancel(event)

    def OnApply(self, event):
        """Apply the command"""
        if self._giface and hasattr(self._giface, "_model"):
            cmd = self.createCmd(ignoreErrors=True, ignoreRequired=True)
        else:
            cmd = self.createCmd()

        if cmd is not None and self.get_dcmd is not None:
            # return d.* command to layer tree for rendering
            self.get_dcmd(cmd, self.layer, {"params": self.task.params,
                                            "flags": self.task.flags},
                          self)
            # echo d.* command to output console
            # self.parent.writeDCommand(cmd)

        return cmd

    def OnRun(self, event):
        """Run the command"""
        cmd = self.createCmd()

        if not cmd or len(cmd) < 1:
            return

        ret = 0
        if self.standalone or cmd[0][0:2] != "d.":
            # Send any non-display command to parent window (probably wxgui.py)
            # put to parents switch to 'Command output'
            self.notebookpanel.notebook.SetSelectionByName('output')

            try:
                if self.task.path:
                    cmd[0] = self.task.path  # full path

                ret = self._gconsole.RunCmd(cmd, onDone=self.OnDone)
            except AttributeError as e:
                print("%s: Probably not running in wxgui.py session?" % (
                      e), file=sys.stderr)
                print("parent window is: %s" % (
                      str(self.parent)), file=sys.stderr)
        else:
            gcmd.Command(cmd)

        if ret != 0:
            self.notebookpanel.notebook.SetSelection(0)
            return

        # update buttons status
        for btn in (self.btn_run,
                    self.btn_cancel,
                    self.btn_clipboard,
                    self.btn_help):
            btn.Enable(False)

    def OnAbort(self, event):
        """Abort running command"""
        from core.gconsole import wxCmdAbort
        event = wxCmdAbort(aborted=True)
        wx.PostEvent(self._gconsole, event)

    def OnCopy(self, event):
        """Copy the command"""
        cmddata = wx.TextDataObject()
        # list -> string
        cmdlist = self.createCmd(ignoreErrors=True)
        # TODO: better protect whitespace with quotes
        for i in range(1, len(cmdlist)):
            if ' ' in cmdlist[i]:
                optname, val = cmdlist[i].split("=", 1)
                cmdlist[i] = '%s="%s"' % (optname, val)
        cmdstring = ' '.join(cmdlist)
        cmddata.SetText(cmdstring)
        if wx.TheClipboard.Open():
            #            wx.TheClipboard.UsePrimarySelection(True)
            wx.TheClipboard.SetData(cmddata)
            wx.TheClipboard.Close()
            self.SetStatusText(_("'%s' copied to clipboard") %
                               (cmdstring))

    def OnCancel(self, event):
        """Cancel button pressed"""
        self.MakeModal(False)
        self.dialogClosing.emit()
        if self.get_dcmd and \
                self.parent and \
                self.parent.GetName() in ('LayerTree',
                                          'MapWindow'):
            Debug.msg(1, "TaskFrame.OnCancel(): known parent")
            # display decorations and
            # pressing OK or cancel after setting layer properties
            if self.task.name in ['d.barscale', 'd.legend', 'd.northarrow', 'd.histogram', 'd.text', 'd.legend.vect'] \
                    or len(self.parent.GetLayerInfo(self.layer, key='cmd')) >= 1:
                # TODO: do this through policy
                self.Hide()
            # canceled layer with nothing set
            elif len(self.parent.GetLayerInfo(self.layer, key='cmd')) < 1:
                # TODO: do this through callback or signal
                try:
                    self.parent.Delete(self.layer)
                except ValueError:
                    # happens when closing dialog of a new layer which was
                    # removed from tree
                    pass
                self._Destroy()
        else:
            Debug.msg(1, "TaskFrame.OnCancel(): no parent")
            # cancel for non-display commands
            self._Destroy()

    def OnHelp(self, event):
        """Show manual page (switch to the 'Manual' notebook page)"""
        if self.notebookpanel.notebook.GetPageIndexByName('manual') > -1:
            self.notebookpanel.notebook.SetSelectionByName('manual')
            self.notebookpanel.OnPageChange(None)

        if event:
            event.Skip()

    def createCmd(self, ignoreErrors=False, ignoreRequired=False):
        """Create command string (python list)"""
        return self.notebookpanel.createCmd(ignoreErrors=ignoreErrors,
                                            ignoreRequired=ignoreRequired)

    def _Destroy(self):
        """Destroy Frame"""
        self.notebookpanel.notebook.Unbind(wx.EVT_NOTEBOOK_PAGE_CHANGED)
        self.notebookpanel.notebook.widget.Unbind(wx.EVT_NOTEBOOK_PAGE_CHANGED)
        self.Destroy()


class CmdPanel(wx.Panel):
    """A panel containing a notebook dividing in tabs the different
    guisections of the GRASS cmd.
    """

    def __init__(self, parent, giface, task, id=wx.ID_ANY,
                 frame=None, *args, **kwargs):
        if frame:
            self.parent = frame
        else:
            self.parent = parent
        self.task = task
        self._giface = giface

        wx.Panel.__init__(self, parent, id=id, *args, **kwargs)

        self.mapCreated = Signal
        self.updateMap = Signal

        # Determine tab layout
        sections = []
        is_section = {}
        not_hidden = [
            p for p in self.task.params +
            self.task.flags if not p.get(
                'hidden',
                False) == True]

        self.label_id = []  # wrap titles on resize

        self.Bind(wx.EVT_SIZE, self.OnSize)

        for task in not_hidden:
            if task.get('required', False) and not task.get('guisection', ''):
                # All required go into Main, even if they had defined another
                # guisection
                task['guisection'] = _('Required')
            if task.get('guisection', '') == '':
                # Undefined guisections end up into Options
                task['guisection'] = _('Optional')
            if task['guisection'] not in is_section:
                # We do it like this to keep the original order, except for
                # Main which goes first
                is_section[task['guisection']] = 1
                sections.append(task['guisection'])
            else:
                is_section[task['guisection']] += 1
        del is_section

        # 'Required' tab goes first, 'Optional' as the last one
        for (newidx, content) in [(0, _('Required')),
                                  (len(sections) - 1, _('Optional'))]:
            if content in sections:
                idx = sections.index(content)
                sections[idx:idx + 1] = []
                sections[newidx:newidx] = [content]

        panelsizer = wx.BoxSizer(orient=wx.VERTICAL)

        # build notebook
        style = UserSettings.Get(
            group='appearance',
            key='commandNotebook',
            subkey='selection')
        if style == 0:  # basic top
            self.notebook = FormNotebook(self, style=wx.BK_TOP)
            self.notebook.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.OnPageChange)
        elif style == 1:  # basic left
            self.notebook = FormNotebook(self, style=wx.BK_LEFT)
            self.notebook.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.OnPageChange)
        elif style == 2:  # fancy green
            self.notebook = GNotebook(
                self, style=globalvar.FNPageStyle | FN.FNB_NO_X_BUTTON)
            self.notebook.SetTabAreaColour(globalvar.FNPageColor)
            self.notebook.Bind(
                FN.EVT_FLATNOTEBOOK_PAGE_CHANGED,
                self.OnPageChange)
        elif style == 3:
            self.notebook = FormListbook(self, style=wx.BK_LEFT)
            self.notebook.Bind(wx.EVT_LISTBOOK_PAGE_CHANGED, self.OnPageChange)
        self.notebook.Refresh()

        tab = {}
        tabsizer = {}
        for section in sections:
            tab[section] = ScrolledPanel(parent=self.notebook)
            tab[section].SetScrollRate(10, 10)
            tabsizer[section] = wx.BoxSizer(orient=wx.VERTICAL)

        #
        # flags
        #
        visible_flags = [
            f for f in self.task.flags if not f.get(
                'hidden', False) == True]
        for f in visible_flags:
            # we don't want another help (checkbox appeared in r58783)
            if f['name'] == 'help':
                continue
            which_sizer = tabsizer[f['guisection']]
            which_panel = tab[f['guisection']]
            # if label is given: description -> tooltip
            if f.get('label', '') != '':
                title = text_beautify(f['label'])
                tooltip = text_beautify(f['description'], width=-1)
            else:
                title = text_beautify(f['description'])
                tooltip = None
            title_sizer = wx.BoxSizer(wx.HORIZONTAL)
            rtitle_txt = StaticText(parent=which_panel,
                                    label='(' + f['name'] + ')')
            chk = CheckBox(
                parent=which_panel,
                label=title,
                style=wx.NO_BORDER)
            self.label_id.append(chk.GetId())
            if tooltip:
                chk.SetToolTip(tooltip)
            chk.SetValue(f.get('value', False))
            title_sizer.Add(chk, proportion=1,
                            flag=wx.EXPAND)
            title_sizer.Add(rtitle_txt, proportion=0,
                            flag=wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
            which_sizer.Add(
                title_sizer,
                proportion=0,
                flag=wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT,
                border=5)
            f['wxId'] = [chk.GetId(), ]
            chk.Bind(wx.EVT_CHECKBOX, self.OnSetValue)

            if self.parent.GetName() == 'MainFrame' and (
                    self._giface and hasattr(self._giface, "_model")):
                parChk = wx.CheckBox(parent=which_panel, id=wx.ID_ANY,
                                     label=_("Parameterized in model"))
                parChk.SetName('ModelParam')
                parChk.SetValue(f.get('parameterized', False))
                if 'wxId' in f:
                    f['wxId'].append(parChk.GetId())
                else:
                    f['wxId'] = [parChk.GetId()]
                parChk.Bind(wx.EVT_CHECKBOX, self.OnSetValue)
                which_sizer.Add(parChk, proportion=0,
                                flag=wx.LEFT, border=20)

            if f['name'] in ('verbose', 'quiet'):
                chk.Bind(wx.EVT_CHECKBOX, self.OnVerbosity)
                vq = UserSettings.Get(
                    group='cmd', key='verbosity', subkey='selection')
                if f['name'] == vq:
                    chk.SetValue(True)
                    f['value'] = True

            if f['name'] == 'overwrite':
                value = UserSettings.Get(
                    group='cmd', key='overwrite', subkey='enabled')
                if value:  # override only when enabled
                    f['value'] = value
                    chk.SetValue(f['value'])

        #
        # parameters
        #
        visible_params = [
            p for p in self.task.params if not p.get(
                'hidden', False) == True]

        try:
            first_param = visible_params[0]
        except IndexError:
            first_param = None

        for p in visible_params:
            which_sizer = tabsizer[p['guisection']]
            which_panel = tab[p['guisection']]
            # if label is given -> label and description -> tooltip
            # otherwise description -> lavel
            if p.get('label', '') != '':
                title = text_beautify(p['label'])
                tooltip = text_beautify(p['description'], width=-1)
            else:
                title = text_beautify(p['description'])
                tooltip = None

            prompt = p.get('prompt', '')

            # title sizer (description, name, type)
            if (len(p.get('values', [])) > 0) and \
                    p.get('multiple', False) and \
                    p.get('gisprompt', False) == False and \
                    p.get('type', '') == 'string':
                title_txt = StaticBox(parent=which_panel, id=wx.ID_ANY)
            else:
                title_sizer = wx.BoxSizer(wx.HORIZONTAL)
                title_txt = StaticText(parent=which_panel)
                if p['key_desc']:
                    ltype = ','.join(p['key_desc'])
                else:
                    ltype = p['type']
                # red star for required options
                if p.get('required', False):
                    required_txt = StaticText(parent=which_panel, label="*")
                    required_txt.SetForegroundColour(wx.RED)
                    required_txt.SetToolTip(_("This option is required"))
                else:
                    required_txt = StaticText(parent=which_panel, label="")
                rtitle_txt = StaticText(
                    parent=which_panel,
                    label='(' + p['name'] + '=' + ltype + ')')
                title_sizer.Add(title_txt, proportion=0,
                                flag=wx.LEFT | wx.TOP | wx.EXPAND, border=5)
                title_sizer.Add(required_txt, proportion=1,
                                flag=wx.EXPAND, border=0)
                title_sizer.Add(
                    rtitle_txt,
                    proportion=0,
                    flag=wx.ALIGN_RIGHT | wx.RIGHT | wx.TOP,
                    border=5)
                which_sizer.Add(title_sizer, proportion=0,
                                flag=wx.EXPAND)
            self.label_id.append(title_txt.GetId())

            # title expansion
            if p.get('multiple', False) and len(p.get('values', '')) == 0:
                title = _("[multiple]") + " " + title
                if p.get('value', '') == '':
                    p['value'] = p.get('default', '')

            if (len(p.get('values', [])) > 0):
                valuelist = list(map(str, p.get('values', [])))
                valuelist_desc = list(map(unicode, p.get('values_desc', [])))
                required_text = "*" if p.get('required', False) else ""
                if p.get('multiple', False) and \
                        p.get('gisprompt', False) == False and \
                        p.get('type', '') == 'string':
                    title_txt.SetLabel(
                        " %s:%s  (%s=%s) " %
                        (title, required_text, p['name'], p['type']))
                    stSizer = wx.StaticBoxSizer(
                        box=title_txt, orient=wx.VERTICAL)
                    if valuelist_desc:
                        hSizer = wx.FlexGridSizer(cols=1, vgap=1, hgap=1)
                    else:
                        hSizer = wx.FlexGridSizer(cols=6, vgap=1, hgap=1)
                    isEnabled = {}
                    # copy default values
                    if p['value'] == '':
                        p['value'] = p.get('default', '')

                    for defval in p.get('value', '').split(','):
                        isEnabled[defval] = 'yes'
                        # for multi checkboxes, this is an array of all wx IDs
                        # for each individual checkbox
                        p['wxId'] = list()
                    idx = 0
                    for val in valuelist:
                        try:
                            label = valuelist_desc[idx]
                        except IndexError:
                            label = val

                        chkbox = wx.CheckBox(parent=which_panel,
                                             label=text_beautify(label))
                        p['wxId'].append(chkbox.GetId())
                        if val in isEnabled:
                            chkbox.SetValue(True)
                        hSizer.Add(chkbox, proportion=0)
                        chkbox.Bind(wx.EVT_CHECKBOX, self.OnUpdateSelection)
                        chkbox.Bind(wx.EVT_CHECKBOX, self.OnCheckBoxMulti)
                        idx += 1

                    stSizer.Add(hSizer, proportion=0,
                                flag=wx.ADJUST_MINSIZE | wx.ALL, border=1)
                    which_sizer.Add(
                        stSizer,
                        proportion=0,
                        flag=wx.EXPAND | wx.TOP | wx.RIGHT | wx.LEFT,
                        border=5)
                elif p.get('gisprompt', False) is False:
                    if len(valuelist) == 1:  # -> textctrl
                        title_txt.SetLabel(
                            "%s (%s %s):" %
                            (title, _('valid range'),
                             str(valuelist[0])))
                        if p.get('type', '') == 'integer' and \
                                not p.get('multiple', False):

                            # for multiple integers use textctrl instead of
                            # spinsctrl
                            try:
                                minValue, maxValue = list(map(
                                    int, valuelist[0].rsplit('-', 1)))
                            except ValueError:
                                minValue = -1e6
                                maxValue = 1e6
                            txt2 = SpinCtrl(
                                parent=which_panel,
                                id=wx.ID_ANY,
                                size=globalvar.DIALOG_SPIN_SIZE,
                                min=minValue,
                                max=maxValue)
                            style = wx.BOTTOM | wx.LEFT
                        else:
                            txt2 = TextCtrl(
                                parent=which_panel, value=p.get(
                                    'default', ''))
                            style = wx.EXPAND | wx.BOTTOM | wx.LEFT

                        value = self._getValue(p)
                        # parameter previously set
                        if value:
                            if isinstance(txt2, SpinCtrl):
                                txt2.SetValue(int(value))
                            else:
                                txt2.SetValue(value)

                        which_sizer.Add(txt2, proportion=0,
                                        flag=style, border=5)

                        p['wxId'] = [txt2.GetId(), ]
                        txt2.Bind(wx.EVT_TEXT, self.OnSetValue)
                    else:

                        title_txt.SetLabel(title + ':')
                        value = self._getValue(p)

                        if p['name'] in ('icon', 'icon_area', 'icon_line'):  # symbols
                            bitmap = wx.Bitmap(
                                os.path.join(
                                    globalvar.SYMBDIR,
                                    value) + '.png')
                            bb = BitmapButton(
                                parent=which_panel, id=wx.ID_ANY, bitmap=bitmap)
                            iconLabel = StaticText(
                                parent=which_panel, id=wx.ID_ANY)
                            iconLabel.SetLabel(value)
                            p['value'] = value
                            p['wxId'] = [bb.GetId(), iconLabel.GetId()]
                            bb.Bind(wx.EVT_BUTTON, self.OnSetSymbol)
                            this_sizer = wx.BoxSizer(wx.HORIZONTAL)
                            this_sizer.Add(
                                bb, proportion=0, flag=wx.ADJUST_MINSIZE |
                                wx.BOTTOM | wx.LEFT, border=5)
                            this_sizer.Add(
                                iconLabel,
                                proportion=0,
                                flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.ALIGN_CENTER_VERTICAL,
                                border=5)
                            which_sizer.Add(this_sizer, proportion=0,
                                            flag=wx.ADJUST_MINSIZE, border=0)
                        else:
                            # list of values (combo)
                            cb = wx.ComboBox(
                                parent=which_panel, id=wx.ID_ANY, value=p.get(
                                    'default', ''),
                                size=globalvar.DIALOG_COMBOBOX_SIZE,
                                choices=valuelist, style=wx.CB_DROPDOWN)
                            if value:
                                cb.SetValue(value)  # parameter previously set
                            which_sizer.Add(
                                cb, proportion=0, flag=wx.ADJUST_MINSIZE |
                                wx.BOTTOM | wx.LEFT, border=5)
                            p['wxId'] = [cb.GetId(), ]
                            cb.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                            cb.Bind(wx.EVT_TEXT, self.OnSetValue)
                            if p.get('guidependency', ''):
                                cb.Bind(
                                    wx.EVT_COMBOBOX, self.OnUpdateSelection)

            # text entry
            if (p.get('type', 'string') in ('string', 'integer', 'float')
                    and len(p.get('values', [])) == 0
                    and p.get('gisprompt', False) == False
                    and p.get('prompt', '') != 'color'):

                title_txt.SetLabel(title + ':')
                p['wxId'] = []
                if p.get('multiple', False) or \
                        p.get('type', 'string') == 'string' or \
                        len(p.get('key_desc', [])) > 1:
                    win = TextCtrl(
                        parent=which_panel, value=p.get(
                            'default', ''))

                    value = self._getValue(p)
                    if value:
                        # parameter previously set
                        win.SetValue(value if p.get('type', 'string') == 'string' else str(value))

                    win.Bind(wx.EVT_TEXT, self.OnSetValue)
                    style = wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT
                    if p.get('name', '') == 'font':
                        font_btn = Button(parent=which_panel, label=_("Select font"))
                        font_btn.Bind(wx.EVT_BUTTON, self.OnSelectFont)
                        font_sizer = wx.BoxSizer(wx.HORIZONTAL)
                        font_sizer.Add(win, proportion=1,
                                       flag=style, border=5)
                        font_sizer.Add(font_btn, proportion=0,
                                       flag=style, border=5)

                        which_sizer.Add(font_sizer, proportion=0,
                                        flag=style, border=5)
                        p['wxId'].append(font_btn.GetId())
                    else:
                        which_sizer.Add(win, proportion=0,
                                        flag=style, border=5)

                elif p.get('type', '') == 'integer':
                    minValue = -1e9
                    maxValue = 1e9
                    value = self._getValue(p)

                    win = SpinCtrl(
                        parent=which_panel,
                        value=p.get(
                            'default',
                            ''),
                        size=globalvar.DIALOG_SPIN_SIZE,
                        min=minValue,
                        max=maxValue)
                    if value:
                        win.SetValue(int(value))  # parameter previously set
                        win.Bind(wx.EVT_SPINCTRL, self.OnSetValue)

                    style = wx.BOTTOM | wx.LEFT | wx.RIGHT
                    which_sizer.Add(win, proportion=0,
                                    flag=style, border=5)
                else:  # float
                    win = TextCtrl(
                        parent=which_panel, value=p.get(
                            'default', ''), validator=FloatValidator())
                    style = wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT
                    which_sizer.Add(win, proportion=0,
                                    flag=style, border=5)

                    value = self._getValue(p)
                    if value:
                        win.SetValue(str(value))  # parameter previously set

                win.Bind(wx.EVT_TEXT, self.OnSetValue)
                p['wxId'].append(win.GetId())

            #
            # element selection tree combobox (maps, icons, regions, etc.)
            #
            if p.get('gisprompt', False):
                title_txt.SetLabel(title + ':')
                # GIS element entry
                if p.get('prompt', '') not in ('color',
                                               'cat',
                                               'cats',
                                               'subgroup',
                                               'sigfile',
                                               'separator',
                                               'dbdriver',
                                               'dbname',
                                               'dbtable',
                                               'dbcolumn',
                                               'layer',
                                               'location',
                                               'mapset',
                                               'dbase',
                                               'coords',
                                               'file',
                                               'dir',
                                               'colortable',
                                               'barscale',
                                               'northarrow',
                                               'datasource',
                                               'datasource_layer',
                                               'sql_query'):
                    multiple = p.get('multiple', False)
                    if p.get('age', '') == 'new':
                        mapsets = [grass.gisenv()['MAPSET'], ]
                    else:
                        mapsets = None
                    if self.task.name in ('r.proj', 'v.proj') \
                            and p.get('name', '') == 'input':
                        selection = gselect.ProjSelect(
                            parent=which_panel, isRaster=self.task.name == 'r.proj')
                        p['wxId'] = [selection.GetId(), ]
                        selection.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                        selection.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                    else:
                        elem = p.get('element', None)
                        # hack for t.* modules
                        if elem in ('stds', 'map'):
                            orig_elem = elem
                            type_param = self.task.get_param(
                                'type', element='name', raiseError=False)
                            if type_param:
                                elem = type_param.get('default', None)
                                # for t.(un)register:
                                maps_param = self.task.get_param(
                                    'maps', element='name', raiseError=False)
                                if maps_param and orig_elem == 'stds':
                                    element_dict = {
                                        'raster': 'strds', 'vector': 'stvds', 'raster_3d': 'str3ds'}
                                    elem = element_dict[
                                        type_param.get('default')]

                        extraItems = None
                        if self._giface:
                            if hasattr(self._giface, "_model"):
                                extraItems = {
                                    _('Graphical Modeler'): self._giface.GetLayerList(
                                        p.get('prompt'))}
                            else:
                                layers = self._giface.GetLayerList()
                                if len(layers) > 0:
                                    mapList = []
                                    extraItems = {_('Map Display'): mapList}
                                    for layer in layers:
                                        if layer.type != p.get('prompt'):
                                            continue
                                        mapList.append(str(layer))
                        selection = gselect.Select(
                            parent=which_panel, id=wx.ID_ANY,
                            size=globalvar.DIALOG_GSELECT_SIZE, type=elem,
                            multiple=multiple, nmaps=len(
                                p.get('key_desc', [])),
                            mapsets=mapsets, fullyQualified=p.get(
                                'age', 'old') == 'old', extraItems=extraItems)

                        value = self._getValue(p)
                        if value:
                            selection.SetValue(value)

                        formatSelector = True
                        # A gselect.Select is a combobox with two children: a textctl and a popupwindow;
                        # we target the textctl here
                        textWin = selection.GetTextCtrl()
                        if globalvar.CheckWxVersion([3]):
                            p['wxId'] = [selection.GetId(), ]
                        else:
                            p['wxId'] = [textWin.GetId(), ]
                        if prompt != 'vector':
                            self.FindWindowById(
                                p['wxId'][0]).Bind(
                                wx.EVT_TEXT, self.OnSetValue)

                    if prompt == 'vector':
                        win = self.FindWindowById(p['wxId'][0])
                        # handlers should be bound in this order
                        # OnUpdateSelection depends on calling OnSetValue first
                        # which is bad
                        win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                        win.Bind(wx.EVT_TEXT, self.OnSetValue)

                        # if formatSelector and p.get('age', 'old') == 'old':
                        #     # OGR supported (read-only)
                        #     self.hsizer = wx.BoxSizer(wx.HORIZONTAL)

                        #     self.hsizer.Add(item = selection,
                        #                     flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_TOP,
                        #                     border = 5)

                        #     # format (native / ogr)
                        #     rbox = wx.RadioBox(parent = which_panel, id = wx.ID_ANY,
                        #                        label = " %s " % _("Format"),
                        #                        style = wx.RA_SPECIFY_ROWS,
                        #                        choices = [_("Native / Linked OGR"), _("Direct OGR")])
                        #     if p.get('value', '').lower().rfind('@ogr') > -1:
                        #         rbox.SetSelection(1)
                        #     rbox.SetName('VectorFormat')
                        #     rbox.Bind(wx.EVT_RADIOBOX, self.OnVectorFormat)

                        #     self.hsizer.Add(item = rbox,
                        #                     flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT |
                        #                     wx.RIGHT | wx.ALIGN_TOP,
                        #                     border = 5)

                        #     ogrSelection = gselect.GdalSelect(parent = self, panel = which_panel, ogr = True,
                        #                                       default = 'dir',
                        #                                       exclude = ['file'])
                        #     self.Bind(gselect.EVT_GDALSELECT, self.OnUpdateSelection)
                        #     self.Bind(gselect.EVT_GDALSELECT, self.OnSetValue)

                        #     ogrSelection.SetName('OgrSelect')
                        #     ogrSelection.Hide()

                        #     which_sizer.Add(item = self.hsizer, proportion = 0)

                        #     p['wxId'].append(rbox.GetId())
                        #     p['wxId'].append(ogrSelection.GetId())
                        #     for win in ogrSelection.GetDsnWin():
                        #         p['wxId'].append(win.GetId())
                        # else:
                        which_sizer.Add(
                            selection,
                            proportion=0,
                            flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                            border=5)
                    elif prompt == 'group':
                        win = self.FindWindowById(p['wxId'][0])
                        win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                        win.Bind(wx.EVT_TEXT, self.OnSetValue)
                        which_sizer.Add(
                            selection,
                            proportion=0,
                            flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                            border=5)
                    else:
                        if prompt in ('stds', 'strds', 'stvds', 'str3ds'):
                            showButton = True
                            try:
                                # if matplotlib is there
                                from timeline import frame
                                showButton = True
                            except ImportError:
                                showButton = False
                        else:
                            showButton = False
                        if showButton:
                            iconTheme = UserSettings.Get(
                                group='appearance', key='iconTheme', subkey='type')
                            bitmap = wx.Bitmap(
                                os.path.join(
                                    globalvar.ICONDIR, iconTheme,
                                    'map-info.png'))
                            bb = BitmapButton(
                                parent=which_panel, bitmap=bitmap)
                            bb.Bind(wx.EVT_BUTTON, self.OnTimelineTool)
                            bb.SetToolTip(
                                _("Show graphical representation of temporal extent of dataset(s) ."))
                            p['wxId'].append(bb.GetId())

                            hSizer = wx.BoxSizer(wx.HORIZONTAL)
                            hSizer.Add(
                                selection,
                                proportion=0,
                                flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                border=5)
                            hSizer.Add(
                                bb,
                                proportion=0,
                                flag=wx.EXPAND | wx.BOTTOM | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                border=5)
                            which_sizer.Add(hSizer)
                        else:
                            which_sizer.Add(
                                selection,
                                proportion=0,
                                flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                border=5)

                # subgroup
                elif prompt == 'subgroup':
                    selection = gselect.SubGroupSelect(parent=which_panel)
                    p['wxId'] = [selection.GetId()]
                    selection.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                    selection.Bind(wx.EVT_TEXT, self.OnSetValue)
                    which_sizer.Add(
                        selection,
                        proportion=0,
                        flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                        border=5)

                # sigrature file
                elif prompt == 'sigfile':
                    selection = gselect.SignatureSelect(
                        parent=which_panel, element=p.get('element', 'sig'))
                    p['wxId'] = [selection.GetId()]
                    selection.Bind(wx.EVT_TEXT, self.OnSetValue)
                    selection.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                    which_sizer.Add(
                        selection,
                        proportion=0,
                        flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                        border=5)

                # separator
                elif prompt == 'separator':
                    win = gselect.SeparatorSelect(parent=which_panel)
                    value = self._getValue(p)
                    win.SetValue(value)
                    p['wxId'] = [win.GetId()]
                    win.Bind(wx.EVT_TEXT, self.OnSetValue)
                    win.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                    which_sizer.Add(
                        win,
                        proportion=0,
                        flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                        border=5)

                # layer, dbdriver, dbname, dbcolumn, dbtable entry
                elif prompt in ('dbdriver',
                                'dbname',
                                'dbtable',
                                'dbcolumn',
                                'layer',
                                'location',
                                'mapset',
                                'dbase'):
                    if p.get('multiple', 'no') == 'yes':
                        win = TextCtrl(
                            parent=which_panel, value=p.get(
                                'default', ''), size=globalvar.DIALOG_TEXTCTRL_SIZE)
                        win.Bind(wx.EVT_TEXT, self.OnSetValue)
                    else:
                        value = self._getValue(p)

                        if prompt == 'layer':
                            if p.get('element', 'layer') == 'layer_all':
                                all = True
                            else:
                                all = False
                            if p.get('age', 'old') == 'old':
                                win = gselect.LayerSelect(parent=which_panel,
                                                          all=all,
                                                          default=p['default'])
                                win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                                win.Bind(wx.EVT_TEXT, self.OnSetValue)
                                win.SetValue(
                                    str(value))    # default or previously set value
                            else:
                                win = SpinCtrl(
                                    parent=which_panel, id=wx.ID_ANY, min=1,
                                    max=100, initial=int(p['default']))
                                win.Bind(wx.EVT_SPINCTRL, self.OnSetValue)
                                win.SetValue(
                                    int(value))    # default or previously set value

                            p['wxId'] = [win.GetId()]

                        elif prompt == 'dbdriver':
                            win = gselect.DriverSelect(
                                parent=which_panel, choices=p.get(
                                    'values', []),
                                value=value)
                            win.Bind(wx.EVT_COMBOBOX, self.OnUpdateSelection)
                            win.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                        elif prompt == 'dbname':
                            win = gselect.DatabaseSelect(parent=which_panel,
                                                         value=value)
                            win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                            win.Bind(wx.EVT_TEXT, self.OnSetValue)
                        elif prompt == 'dbtable':
                            if p.get('age', 'old') == 'old':
                                win = gselect.TableSelect(parent=which_panel)
                                win.Bind(
                                    wx.EVT_COMBOBOX, self.OnUpdateSelection)
                                win.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                            else:
                                win = TextCtrl(
                                    parent=which_panel, value=p.get(
                                        'default', ''),
                                    size=globalvar.DIALOG_TEXTCTRL_SIZE)
                                win.Bind(wx.EVT_TEXT, self.OnSetValue)
                        elif prompt == 'dbcolumn':
                            win = gselect.ColumnSelect(
                                parent=which_panel, value=value, param=p,
                                multiple=p.get('multiple', False))

                            # A gselect.ColumnSelect is a combobox
                            # with two children: a textctl and a
                            # popupwindow; we target the textctl here
                            textWin = win.GetTextCtrl()
                            p['wxId'] = [textWin.GetId(), ]

                            textWin.Bind(wx.EVT_TEXT, self.OnSetValue)
                            win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)

                        elif prompt == 'location':
                            win = gselect.LocationSelect(parent=which_panel,
                                                         value=value)
                            win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                            win.Bind(wx.EVT_COMBOBOX, self.OnUpdateSelection)
                            win.Bind(wx.EVT_TEXT, self.OnSetValue)
                            win.Bind(wx.EVT_COMBOBOX, self.OnSetValue)

                        elif prompt == 'mapset':
                            if p.get('age', 'old') == 'old':
                                new = False
                            else:
                                new = True

                            win = gselect.MapsetSelect(
                                parent=which_panel, value=value, new=new,
                                multiple=p.get('multiple', False))
                            win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                            win.Bind(wx.EVT_COMBOBOX, self.OnUpdateSelection)
                            win.Bind(wx.EVT_TEXT, self.OnSetValue)
                            win.Bind(wx.EVT_COMBOBOX, self.OnSetValue)

                        elif prompt == 'dbase':
                            win = gselect.DbaseSelect(
                                parent=which_panel, changeCallback=self.OnSetValue)
                            win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                            p['wxId'] = [win.GetChildren()[1].GetId()]

                    if 'wxId' not in p:
                        try:
                            p['wxId'] = [win.GetId(), ]
                        except AttributeError:
                            pass

                    flags = wx.BOTTOM | wx.LEFT | wx.RIGHT
                    if prompt == 'dbname':
                        flags |= wx.EXPAND
                    which_sizer.Add(win, proportion=0,
                                    flag=flags, border=5)
                # color entry
                elif prompt == 'color':
                    default_color = (200, 200, 200)
                    label_color = _("Select Color")
                    if p.get('default', '') != '':
                        default_color, label_color = utils.color_resolve(p[
                                                                         'default'])
                    if p.get('value', '') != '' and p.get(
                            'value', '') != 'none':  # parameter previously set
                        if not p.get('multiple', False):
                            default_color, label_color = utils.color_resolve(p[
                                                                             'value'])
                    if p.get(
                            'element', '') == 'color_none' or p.get(
                            'multiple', False):
                        this_sizer = wx.BoxSizer(orient=wx.HORIZONTAL)
                    else:
                        this_sizer = which_sizer
                    colorSize = 150
                    # For color selectors, this is a three-member array, holding the IDs of
                    # the color picker,  the text control for multiple colors (or None),
                    # and either a "transparent" checkbox or None
                    p['wxId'] = [None] * 3
                    if p.get('multiple', False):
                        txt = TextCtrl(parent=which_panel, id=wx.ID_ANY)
                        this_sizer.Add(
                            txt,
                            proportion=1,
                            flag=wx.ADJUST_MINSIZE | wx.LEFT | wx.TOP,
                            border=5)
                        txt.Bind(wx.EVT_TEXT, self.OnSetValue)
                        if p.get('value', ''):
                            txt.SetValue(p['value'])
                        colorSize = 40
                        label_color = ''
                        p['wxId'][1] = txt.GetId()
                        which_sizer.Add(
                            this_sizer, flag=wx.EXPAND | wx.RIGHT, border=5)

                    btn_colour = csel.ColourSelect(
                        parent=which_panel, id=wx.ID_ANY, label=label_color,
                        colour=default_color, pos=wx.DefaultPosition,
                        size=(colorSize, 32))
                    this_sizer.Add(
                        btn_colour,
                        proportion=0,
                        flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT,
                        border=5)
                    btn_colour.Bind(csel.EVT_COLOURSELECT, self.OnColorChange)
                    p['wxId'][0] = btn_colour.GetId()

                    if p.get('element', '') == 'color_none':
                        none_check = wx.CheckBox(
                            which_panel, wx.ID_ANY, _("Transparent"))
                        if p.get('value', '') == "none":
                            none_check.SetValue(True)
                        else:
                            none_check.SetValue(False)
                        this_sizer.Add(
                            none_check, proportion=0,
                            flag=wx.ADJUST_MINSIZE | wx.LEFT | wx.RIGHT | wx.TOP,
                            border=5)
                        which_sizer.Add(this_sizer)
                        none_check.Bind(wx.EVT_CHECKBOX, self.OnColorChange)
                        p['wxId'][2] = none_check.GetId()

                # file selector
                elif p.get('prompt', '') != 'color' and p.get('prompt', '') == 'file':
                    if p.get('age', 'new') == 'new':
                        fmode = wx.FD_SAVE
                    else:
                        fmode = wx.FD_OPEN
                    # check wildcard
                    try:
                        fExt = os.path.splitext(
                            p.get('key_desc', ['*.*'])[0])[1]
                    except:
                        fExt = None
                    if not fExt:
                        fMask = '*'
                    else:
                        fMask = '%s files (*%s)|*%s|Files (*)|*' % (
                            fExt[1:].upper(), fExt, fExt)
                    fbb = filebrowse.FileBrowseButton(
                        parent=which_panel,
                        id=wx.ID_ANY,
                        fileMask=fMask,
                        size=globalvar.DIALOG_GSELECT_SIZE,
                        labelText='',
                        dialogTitle=_('Choose %s') %
                        p.get(
                            'description',
                            _('file')).lower(),
                        buttonText=_('Browse'),
                        startDirectory=os.getcwd(),
                        fileMode=fmode,
                        changeCallback=self.OnSetValue)
                    value = self._getValue(p)
                    if value:
                        fbb.SetValue(value)  # parameter previously set
                    which_sizer.Add(fbb, proportion=0,
                                    flag=wx.EXPAND | wx.RIGHT, border=5)

                    # A file browse button is a combobox with two children:
                    # a textctl and a button;
                    # we have to target the button here
                    p['wxId'] = [fbb.GetChildren()[1].GetId()]
                    if p.get(
                            'age', 'new') == 'old' and p.get(
                            'prompt', '') == 'file' and p.get(
                            'element', '') == 'file' and UserSettings.Get(
                            group='cmd', key='interactiveInput', subkey='enabled'):
                        # widget for interactive input
                        ifbb = TextCtrl(parent=which_panel, id=wx.ID_ANY,
                                        style=wx.TE_MULTILINE,
                                        size=(-1, 75))
                        if p.get('value', '') and os.path.isfile(p['value']):
                            ifbb.Clear()
                            enc = locale.getdefaultlocale()[1]
                            with codecs.open(p['value'], encoding=enc, errors='ignore') as f:
                                nonascii = bytearray(range(0x80, 0x100))
                                for line in f.readlines():
                                    try:
                                        ifbb.AppendText(line)
                                    except UnicodeDecodeError:
                                        # remove non-ascii characters on encoding mismatch (file vs OS)
                                        ifbb.AppendText(line.translate(None, nonascii))
                                ifbb.SetInsertionPoint(0)

                        ifbb.Bind(wx.EVT_TEXT, self.OnFileText)

                        btnLoad = Button(
                            parent=which_panel, id=wx.ID_ANY, label=_("&Load"))
                        btnLoad.SetToolTip(
                            _("Load and edit content of a file"))
                        btnLoad.Bind(wx.EVT_BUTTON, self.OnFileLoad)
                        btnSave = Button(
                            parent=which_panel, id=wx.ID_ANY, label=_("&Save as"))
                        btnSave.SetToolTip(
                            _("Save content to a file for further use"))
                        btnSave.Bind(wx.EVT_BUTTON, self.OnFileSave)

                        fileContentLabel = StaticText(
                            parent=which_panel, id=wx.ID_ANY,
                            label=_('or enter values directly:'))
                        fileContentLabel.SetToolTip(
                            _("Enter file content directly instead of specifying"
                              " a file."
                              " Temporary file will be automatically created."))
                        which_sizer.Add(
                            fileContentLabel,
                            proportion=0,
                            flag=wx.EXPAND | wx.RIGHT | wx.LEFT | wx.BOTTOM,
                            border=5)
                        which_sizer.Add(
                            ifbb,
                            proportion=1,
                            flag=wx.EXPAND | wx.RIGHT | wx.LEFT,
                            border=5)
                        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
                        btnSizer.Add(btnLoad, proportion=0,
                                     flag=wx.ALIGN_RIGHT | wx.RIGHT, border=10)
                        btnSizer.Add(btnSave, proportion=0,
                                     flag=wx.ALIGN_RIGHT)
                        which_sizer.Add(
                            btnSizer,
                            proportion=0,
                            flag=wx.ALIGN_RIGHT | wx.RIGHT | wx.TOP,
                            border=5)

                        p['wxId'].append(ifbb.GetId())
                        p['wxId'].append(btnLoad.GetId())
                        p['wxId'].append(btnSave.GetId())

                # directory selector
                elif p.get('prompt', '') != 'color' and p.get('prompt', '') == 'dir':
                    fbb = filebrowse.DirBrowseButton(
                        parent=which_panel,
                        id=wx.ID_ANY,
                        size=globalvar.DIALOG_GSELECT_SIZE,
                        labelText='',
                        dialogTitle=_('Choose %s') %
                        p.get(
                            'description',
                            _('Directory')),
                        buttonText=_('Browse'),
                        startDirectory=os.getcwd(),
                        newDirectory=True,
                        changeCallback=self.OnSetValue)
                    value = self._getValue(p)
                    if value:
                        fbb.SetValue(value)  # parameter previously set
                    which_sizer.Add(fbb, proportion=0,
                                    flag=wx.EXPAND | wx.RIGHT, border=5)

                    # A file browse button is a combobox with two children:
                    # a textctl and a button;
                    # we have to target the button here
                    p['wxId'] = [fbb.GetChildren()[1].GetId()]

                # interactive inserting of coordinates from map window
                elif prompt == 'coords':
                    # interactive inserting if layer manager is accessible
                    if self._giface:
                        win = gselect.CoordinatesSelect(
                            parent=which_panel, giface=self._giface, multiple=p.get(
                                'multiple', False), param=p)
                        p['wxId'] = [win.GetTextWin().GetId()]
                        win.GetTextWin().Bind(wx.EVT_TEXT, self.OnSetValue)
                        # bind closing event because destructor is not working
                        # properly
                        if hasattr(self.parent, 'dialogClosing'):
                            self.parent.dialogClosing.connect(win.OnClose)

                    # normal text field
                    else:
                        win = TextCtrl(parent=which_panel)
                        p['wxId'] = [win.GetId()]
                        win.Bind(wx.EVT_TEXT, self.OnSetValue)

                    which_sizer.Add(
                        win,
                        proportion=0,
                        flag=wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT,
                        border=5)

                elif prompt in ('cat', 'cats'):
                    # interactive selection of vector categories if layer
                    # manager is accessible
                    if self._giface:
                        win = gselect.VectorCategorySelect(
                            parent=which_panel, giface=self._giface, task=self.task)

                        p['wxId'] = [win.GetTextWin().GetId()]
                        win.GetTextWin().Bind(wx.EVT_TEXT, self.OnSetValue)
                        # bind closing event because destructor is not working
                        # properly
                        if hasattr(self.parent, 'dialogClosing'):
                            self.parent.dialogClosing.connect(win.OnClose)
                    # normal text field
                    else:
                        win = TextCtrl(parent=which_panel)
                        value = self._getValue(p)
                        win.SetValue(value)
                        p['wxId'] = [win.GetId()]
                        win.Bind(wx.EVT_TEXT, self.OnSetValue)

                    which_sizer.Add(
                        win,
                        proportion=0,
                        flag=wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT,
                        border=5)

                elif prompt in ('colortable', 'barscale', 'northarrow'):
                    if prompt == 'colortable':
                        cb = ColorTablesComboBox(
                            parent=which_panel, value=p.get('default', ''),
                            size=globalvar.DIALOG_COMBOBOX_SIZE,
                            choices=valuelist)
                    elif prompt == 'barscale':
                        cb = BarscalesComboBox(
                            parent=which_panel, value=p.get('default', ''),
                            size=globalvar.DIALOG_COMBOBOX_SIZE,
                            choices=valuelist)
                    elif prompt == 'northarrow':
                        cb = NArrowsComboBox(
                            parent=which_panel, value=p.get('default', ''),
                            size=globalvar.DIALOG_COMBOBOX_SIZE,
                            choices=valuelist)

                    value = self._getValue(p)
                    if value:
                        cb.SetValue(value)  # parameter previously set
                    which_sizer.Add(
                        cb,
                        proportion=0,
                        flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT,
                        border=5)
                    p['wxId'] = [cb.GetId(), cb.GetTextCtrl().GetId()]
                    cb.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                    cb.GetTextCtrl().Bind(wx.EVT_TEXT, self.OnSetValue)
                    if p.get('guidependency', ''):
                        cb.Bind(wx.EVT_COMBOBOX, self.OnUpdateSelection)

                elif prompt == 'datasource':
                    win = gselect.GdalSelect(parent=parent, panel=which_panel,
                                             ogr=True)
                    win.Bind(wx.EVT_TEXT, self.OnSetValue)
                    win.Bind(wx.EVT_CHOICE, self.OnSetValue)
                    p['wxId'] = [
                        win.GetId(),
                        win.fileWidgets['browse'].GetChildren()[1].GetId(),
                        win.dirWidgets['browse'].GetChildren()[1].GetId(),
                        win.dbWidgets['choice'].GetId()]
                    value = self._getValue(p)
                    if value:
                        win.fileWidgets['browse'].GetChildren()[1].SetValue(
                            value)  # parameter previously set
                    which_sizer.Add(win, proportion=0,
                                    flag=wx.EXPAND)

                elif prompt == 'datasource_layer':
                    self.win1 = LayersList(
                        parent=which_panel,
                        columns=[
                            _('Layer id'),
                            _('Layer name'),
                            _('Feature type'),
                            _('Projection match')])
                    which_sizer.Add(self.win1, proportion=0,
                                    flag=wx.EXPAND | wx.ALL, border=3)
                    porf = self.task.get_param(
                        'input', element='name', raiseError=False)
                    if porf and 'wxId' in porf:
                        winDataSource = self.FindWindowById(porf['wxId'][0])
                        winDataSource.reloadDataRequired.connect(
                            lambda listData: self.win1.LoadData(
                                listData, False))
                        p['wxId'] = [self.win1.GetId()]

                        def OnCheckItem(index, flag):
                            layers = list()
                            geometry = None
                            for layer, match, listId in self.win1.GetLayers():
                                if '|' in layer:
                                    layer, geometry = layer.split('|', 1)
                                layers.append(layer)
                            porf = self.task.get_param(
                                'layer', element='name', raiseError=False)
                            porf['value'] = ','.join(layers)
                            # geometry is currently discarded
                            # TODO: v.import has no geometry option
                            self.OnUpdateValues()  # TODO: replace by signal

                        self.win1.OnCheckItem = OnCheckItem

                elif prompt == 'sql_query':
                    win = gselect.SqlWhereSelect(
                        parent=which_panel, param=p)
                    p['wxId'] = [win.GetTextWin().GetId()]
                    win.GetTextWin().Bind(wx.EVT_TEXT, self.OnSetValue)
                    which_sizer.Add(
                        win,
                        proportion=0,
                        flag=wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT,
                        border=5)

            if self.parent.GetName() == 'MainFrame' and (
                    self._giface and hasattr(self._giface, "_model")):
                parChk = wx.CheckBox(parent=which_panel, id=wx.ID_ANY,
                                     label=_("Parameterized in model"))
                parChk.SetName('ModelParam')
                parChk.SetValue(p.get('parameterized', False))
                if 'wxId' in p:
                    p['wxId'].append(parChk.GetId())
                else:
                    p['wxId'] = [parChk.GetId()]
                parChk.Bind(wx.EVT_CHECKBOX, self.OnSetValue)
                which_sizer.Add(parChk, proportion=0,
                                flag=wx.LEFT, border=20)

            if title_txt is not None:
                # create tooltip if given
                if len(p['values_desc']) > 0:
                    if tooltip:
                        tooltip += 2 * os.linesep
                    else:
                        tooltip = ''
                    if len(p['values']) == len(p['values_desc']):
                        for i in range(len(p['values'])):
                            tooltip += p['values'][i] + ': ' + \
                                p['values_desc'][i] + os.linesep
                    tooltip.strip(os.linesep)
                if tooltip:
                    title_txt.SetToolTip(tooltip)

            if p == first_param:
                if 'wxId' in p and len(p['wxId']) > 0:
                    win = self.FindWindowById(p['wxId'][0])
                    win.SetFocus()

        #
        # set widget relations for OnUpdateSelection
        #
        pMap = None
        pLayer = []
        pDriver = None
        pDatabase = None
        pTable = None
        pColumn = []
        pGroup = None
        pSubGroup = None
        pSigFile = []
        pDbase = None
        pLocation = None
        pMapset = None
        pSqlWhere = []
        for p in self.task.params:
            if self.task.blackList['enabled'] and self.task.get_name() in self.task.blackList['items'] and \
               p.get('name', '') in self.task.blackList['items'][self.task.get_name()]['params']:
                continue
            guidep = p.get('guidependency', '')

            if guidep:
                # fixed options dependency defined
                options = guidep.split(',')
                for opt in options:
                    pOpt = self.task.get_param(
                        opt, element='name', raiseError=False)
                    if pOpt and id:
                        if 'wxId-bind' not in p:
                            p['wxId-bind'] = list()
                        p['wxId-bind'] += pOpt['wxId']
                continue
            if p.get('gisprompt', False) == False:
                continue

            prompt = p.get('prompt', '')
            if prompt in ('raster', 'vector'):
                name = p.get('name', '')
                if name in ('map', 'input'):
                    pMap = p
            elif prompt == 'layer':
                pLayer.append(p)
            elif prompt == 'dbcolumn':
                pColumn.append(p)
            elif prompt == 'dbdriver':
                pDriver = p
            elif prompt == 'dbname':
                pDatabase = p
            elif prompt == 'dbtable':
                pTable = p
            elif prompt == 'group':
                pGroup = p
            elif prompt == 'subgroup':
                pSubGroup = p
            elif prompt == 'sigfile':
                pSigFile.append(p)
            elif prompt == 'dbase':
                pDbase = p
            elif prompt == 'location':
                pLocation = p
            elif prompt == 'mapset':
                pMapset = p
            elif prompt == 'sql_query':
                pSqlWhere.append(p)

        # collect ids
        pColumnIds = []
        for p in pColumn:
            pColumnIds += p['wxId']
        pLayerIds = []
        for p in pLayer:
            pLayerIds += p['wxId']
        pSigFileIds = []
        for p in pSigFile:
            pSigFileIds += p['wxId']
        pSqlWhereIds = []
        for p in pSqlWhere:
            pSqlWhereIds += p['wxId']

        # set wxId-bindings
        if pMap:
            pMap['wxId-bind'] = []
            if pLayer:
                pMap['wxId-bind'] += pLayerIds
            pMap['wxId-bind'] += copy.copy(pColumnIds)
            pMap['wxId-bind'] += copy.copy(pSqlWhereIds)
        if pLayer:
            for p in pLayer:
                p['wxId-bind'] = copy.copy(pColumnIds)
                p['wxId-bind'] += copy.copy(pSqlWhereIds)


        if pDriver and pTable:
            pDriver['wxId-bind'] = pTable['wxId']

        if pDatabase and pTable:
            pDatabase['wxId-bind'] = pTable['wxId']

        if pTable and pColumnIds:
            pTable['wxId-bind'] = pColumnIds

        if pGroup and pSubGroup:
            if pSigFile:
                pGroup['wxId-bind'] = pSigFileIds + pSubGroup['wxId']
                pSubGroup['wxId-bind'] = pSigFileIds
            else:
                pGroup['wxId-bind'] = pSubGroup['wxId']

        if pDbase and pLocation:
            pDbase['wxId-bind'] = pLocation['wxId']

        if pLocation and pMapset:
            pLocation['wxId-bind'] = pMapset['wxId']

        if pLocation and pMapset and pMap:
            # pLocation['wxId-bind'] +=  pMap['wxId']
            pMapset['wxId-bind'] = pMap['wxId']

        #
        # determine panel size
        #
        maxsizes = (0, 0)
        for section in sections:
            tab[section].SetSizer(tabsizer[section])
            tab[section].SetupScrolling(True, True, 10, 10)
            tab[section].Layout()
            minsecsizes = tabsizer[section].GetSize()
            maxsizes = list(map(lambda x: max(maxsizes[x], minsecsizes[x]), (0, 1)))

        # TODO: be less arbitrary with these 600
        self.panelMinHeight = 100
        self.constrained_size = (
            min(600, maxsizes[0]) + 25, min(400, maxsizes[1]) + 25)
        for section in sections:
            tab[section].SetMinSize(
                (self.constrained_size[0], self.panelMinHeight))

        # add pages to notebook
        imageList = wx.ImageList(16, 16)
        self.notebook.AssignImageList(imageList)

        for section in sections:
            self.notebook.AddPage(
                page=tab[section],
                text=section, name=section)
            index = self.AddBitmapToImageList(section, imageList)
            if index >= 0:
                self.notebook.SetPageImage(section, index)

        # are we running from command line?
        # add 'command output' tab regardless standalone dialog
        if self.parent.GetName() == "MainFrame" and self.parent.get_dcmd is None:
            from core.gconsole import GConsole, EVT_CMD_RUN, EVT_CMD_DONE
            from gui_core.goutput import GConsoleWindow
            self._gconsole = GConsole(
                guiparent=self.notebook, giface=self._giface)
            self.goutput = GConsoleWindow(
                parent=self.notebook,
                gconsole=self._gconsole,
                margin=False)
            self._gconsole.Bind(
                EVT_CMD_RUN, lambda event: self._switchPageHandler(
                    event=event, notification=Notification.MAKE_VISIBLE))
            self._gconsole.Bind(
                EVT_CMD_DONE, lambda event: self._switchPageHandler(
                    event=event, notification=Notification.RAISE_WINDOW))
            self.outpage = self.notebook.AddPage(
                page=self.goutput, text=_("Command output"), name='output')
        else:
            self.goutput = None
            self._gconsole = None

        self.manualTab = HelpPanel(
            parent=self.notebook,
            command=self.task.get_name())
        if not self.manualTab.GetFile():
            self.manualTab.Hide()
        else:
            self.notebook.AddPage(
                page=self.manualTab,
                text=_("Manual"),
                name='manual')
            index = self.AddBitmapToImageList(
                section='manual', imageList=imageList)
            if index >= 0:
                self.notebook.SetPageImage('manual', index)

        if self.manualTab.IsLoaded():
            self.manualTab.SetMinSize(
                (self.constrained_size[0], self.panelMinHeight))

        self.notebook.SetSelection(0)

        panelsizer.Add(self.notebook, proportion=1, flag=wx.EXPAND)
        self.SetSizer(panelsizer)
        panelsizer.Fit(self.notebook)

        self.Bind(EVT_DIALOG_UPDATE, self.OnUpdateDialog)

    def _getValue(self, p):
        """Get value or default value of given parameter

        :param p: parameter directory
        """
        if p.get('value', '') != '':
            return p['value']
        return p.get('default', '')

    def OnFileLoad(self, event):
        """Load file to interactive input"""
        me = event.GetId()
        win = dict()
        for p in self.task.params:
            if 'wxId' in p and me in p['wxId']:
                win['file'] = self.FindWindowById(p['wxId'][0])
                win['text'] = self.FindWindowById(p['wxId'][1])
                break

        if not win:
            return

        path = win['file'].GetValue()
        if not path:
            gcmd.GMessage(parent=self,
                          message=_("Nothing to load."))
            return

        data = ''
        try:
            f = open(path, "r")
        except IOError as e:
            gcmd.GError(parent=self, showTraceback=False,
                        message=_("Unable to load file.\n\nReason: %s") % e)
            return

        try:
            data = f.read()
        finally:
            f.close()

        win['text'].SetValue(data)

    def OnFileSave(self, event):
        """Save interactive input to the file"""
        wId = event.GetId()
        win = {}
        for p in self.task.params:
            if wId in p.get('wxId', []):
                win['file'] = self.FindWindowById(p['wxId'][0])
                win['text'] = self.FindWindowById(p['wxId'][1])
                break

        if not win:
            return

        text = win['text'].GetValue()
        if not text:
            gcmd.GMessage(parent=self,
                          message=_("Nothing to save."))
            return

        dlg = wx.FileDialog(parent=self,
                            message=_("Save input as..."),
                            defaultDir=os.getcwd(),
                            style=wx.FD_SAVE | wx.FD_OVERWRITE_PROMPT)

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            enc = locale.getdefaultlocale()[1]
            f = codecs.open(path, encoding=enc, mode='w', errors='replace')
            try:
                f.write(text + os.linesep)
            finally:
                f.close()

            win['file'].SetValue(path)

        dlg.Destroy()

    def OnFileText(self, event):
        """File input interactively entered"""
        text = event.GetString()
        p = self.task.get_param(
            value=event.GetId(),
            element='wxId',
            raiseError=False)
        if not p:
            return  # should not happen
        win = self.FindWindowById(p['wxId'][0])
        if text:
            filename = win.GetValue()
            if not filename or filename == p[
                    'default']:  # m.proj has - as default
                filename = grass.tempfile()
                win.SetValue(filename)

            enc = locale.getdefaultlocale()[1]
            f = codecs.open(filename, encoding=enc, mode='w', errors='replace')
            try:
                f.write(text)
                if text[-1] != os.linesep:
                    f.write(os.linesep)
            finally:
                f.close()
        else:
            win.SetValue('')

    def OnVectorFormat(self, event):
        """Change vector format (native / ogr).

        Currently unused.
        """
        sel = event.GetSelection()
        idEvent = event.GetId()
        p = self.task.get_param(
            value=idEvent,
            element='wxId',
            raiseError=False)
        if not p:
            return  # should not happen

        # detect windows
        winNative = None
        winOgr = None
        for id in p['wxId']:
            if id == idEvent:
                continue
            name = self.FindWindowById(id).GetName()
            if name == 'Select':
                # fix the mystery (also in nviz_tools.py)
                winNative = self.FindWindowById(id + 1)
            elif name == 'OgrSelect':
                winOgr = self.FindWindowById(id)

        # enable / disable widgets & update values
        rbox = self.FindWindowByName('VectorFormat')
        self.hsizer.Remove(rbox)
        if sel == 0:   # -> native
            winOgr.Hide()
            self.hsizer.Remove(winOgr)

            self.hsizer.Add(
                winNative,
                flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_TOP,
                border=5)
            winNative.Show()
            p['value'] = winNative.GetValue()

        elif sel == 1:  # -> OGR
            sizer = wx.BoxSizer(wx.VERTICAL)

            winNative.Hide()
            self.hsizer.Remove(winNative)

            sizer.Add(winOgr)
            winOgr.Show()
            p['value'] = winOgr.GetDsn()

            self.hsizer.Add(
                sizer,
                flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_TOP,
                border=5)

        self.hsizer.Add(rbox,
                        flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT |
                        wx.RIGHT | wx.ALIGN_TOP,
                        border=5)

        self.hsizer.Layout()
        self.Layout()
        self.OnUpdateValues()
        self.OnUpdateSelection(event)

    def OnUpdateDialog(self, event):
        for fn, kwargs in six.iteritems(event.data):
            fn(**kwargs)

        self.parent.updateValuesHook()

    def OnVerbosity(self, event):
        """Verbosity level changed"""
        verbose = self.FindWindowById(self.task.get_flag('verbose')['wxId'][0])
        quiet = self.FindWindowById(self.task.get_flag('quiet')['wxId'][0])
        if event.IsChecked():
            if event.GetId() == verbose.GetId():
                if quiet.IsChecked():
                    quiet.SetValue(False)
                    self.task.get_flag('quiet')['value'] = False
            else:
                if verbose.IsChecked():
                    verbose.SetValue(False)
                    self.task.get_flag('verbose')['value'] = False

        event.Skip()

    def OnPageChange(self, event):
        if not event:
            sel = self.notebook.GetSelection()
        else:
            sel = event.GetSelection()

        idx = self.notebook.GetPageIndexByName('manual')
        if idx > -1 and sel == idx:
            # calling LoadPage() is strangely time-consuming (only first call)
            # FIXME: move to helpPage.__init__()
            if not self.manualTab.IsLoaded():
                wx.GetApp().Yield()
                self.manualTab.LoadPage()

        self.Layout()

        if event:
            # skip is needed for wx.Notebook on Windows
            event.Skip()
            # this is needed for dialogs launched from layer manager
            # event is somehow propagated?
            event.StopPropagation()

    def _switchPageHandler(self, event, notification):
        self._switchPage(notification=notification)
        event.Skip()

    def _switchPage(self, notification):
        """Manages @c 'output' notebook page according to event notification."""
        if notification == Notification.HIGHLIGHT:
            self.notebook.HighlightPageByName('output')
        if notification == Notification.MAKE_VISIBLE:
            self.notebook.SetSelectionByName('output')
        if notification == Notification.RAISE_WINDOW:
            self.notebook.SetSelectionByName('output')
            self.SetFocus()
            self.Raise()

    def OnColorChange(self, event):
        myId = event.GetId()
        for p in self.task.params:
            if 'wxId' in p and myId in p['wxId']:
                multiple = p['wxId'][1] is not None  # multiple colors
                hasTansp = p['wxId'][2] is not None
                if multiple:
                    # selected color is added at the end of textCtrl
                    colorchooser = wx.FindWindowById(p['wxId'][0])
                    new_color = colorchooser.GetValue()[:]
                    new_label = utils.rgb2str.get(
                        new_color, ':'.join(list(map(str, new_color))))
                    textCtrl = wx.FindWindowById(p['wxId'][1])
                    val = textCtrl.GetValue()
                    sep = ','
                    if val and val[-1] != sep:
                        val += sep
                    val += new_label
                    textCtrl.SetValue(val)
                    p['value'] = val
                elif hasTansp and wx.FindWindowById(p['wxId'][2]).GetValue():
                    p['value'] = 'none'
                else:
                    colorchooser = wx.FindWindowById(p['wxId'][0])
                    new_color = colorchooser.GetValue()[:]
                    # This is weird: new_color is a 4-tuple and new_color[:] is a 3-tuple
                    # under wx2.8.1
                    new_label = utils.rgb2str.get(
                        new_color, ':'.join(list(map(str, new_color))))
                    colorchooser.SetLabel(new_label)
                    colorchooser.SetColour(new_color)
                    colorchooser.Refresh()
                    p['value'] = colorchooser.GetLabel()
        self.OnUpdateValues()

    def OnUpdateValues(self, event=None):
        """If we were part of a richer interface, report back the
        current command being built.

        This method should be set by the parent of this panel if
        needed. It's a hook, actually.  Beware of what is 'self' in
        the method def, though. It will be called with no arguments.
        """
        pass

    def OnCheckBoxMulti(self, event):
        """Fill the values as a ','-separated string according to
        current status of the checkboxes.
        """
        me = event.GetId()
        theParam = None
        for p in self.task.params:
            if 'wxId' in p and me in p['wxId']:
                theParam = p
                myIndex = p['wxId'].index(me)

        # Unpack current value list
        currentValues = {}
        for isThere in theParam.get('value', '').split(','):
            currentValues[isThere] = 1
        theValue = theParam['values'][myIndex]

        if event.IsChecked():
            currentValues[theValue] = 1
        else:
            del currentValues[theValue]

        # Keep the original order, so that some defaults may be recovered
        currentValueList = []
        for v in theParam['values']:
            if v in currentValues:
                currentValueList.append(v)

        # Pack it back
        theParam['value'] = ','.join(currentValueList)

        self.OnUpdateValues()
        event.Skip()

    def OnSetValue(self, event):
        """Retrieve the widget value and set the task value field
        accordingly.

        Use for widgets that have a proper GetValue() method, i.e. not
        for selectors.
        """
        myId = event.GetId()
        me = wx.FindWindowById(myId)
        name = me.GetName()

        found = False
        for porf in self.task.params + self.task.flags:
            if 'wxId' not in porf:
                continue

            if myId in porf['wxId']:
                found = True
                break

        if not found:
            return

        if name == 'GdalSelect':
            porf['value'] = event.dsn
        elif name == 'ModelParam':
            porf['parameterized'] = me.IsChecked()
        elif name == 'GdalSelectDataSource':
            win = self.FindWindowById(porf['wxId'][0])
            porf['value'] = win.GetDsn()
            pLayer = self.task.get_param(
                'layer', element='name', raiseError=False)
            if pLayer:
                pLayer['value'] = ''
        else:
            if isinstance(me, SpinCtrl):
                porf['value'] = str(me.GetValue())
            elif isinstance(me, wx.ComboBox):
                porf['value'] = me.GetValue()
            elif isinstance(me, wx.Choice):
                porf['value'] = me.GetStringSelection()
            else:
                porf['value'] = me.GetValue()

        self.OnUpdateValues(event)

        event.Skip()

    def OnSetSymbol(self, event):
        """Shows dialog for symbol selection"""
        myId = event.GetId()

        for p in self.task.params:
            if 'wxId' in p and myId in p['wxId']:
                from gui_core.dialogs import SymbolDialog
                dlg = SymbolDialog(self, symbolPath=globalvar.SYMBDIR,
                                   currentSymbol=p['value'])
                if dlg.ShowModal() == wx.ID_OK:
                    img = dlg.GetSelectedSymbolPath()
                    p['value'] = dlg.GetSelectedSymbolName()

                    bitmapButton = wx.FindWindowById(p['wxId'][0])
                    label = wx.FindWindowById(p['wxId'][1])

                    bitmapButton.SetBitmapLabel(wx.Bitmap(img + '.png'))
                    label.SetLabel(p['value'])

                    self.OnUpdateValues(event)

                dlg.Destroy()

    def OnTimelineTool(self, event):
        """Show Timeline Tool with dataset(s) from gselect.

        .. todo::
            update from gselect automatically
        """
        myId = event.GetId()

        for p in self.task.params:
            if 'wxId' in p and myId in p['wxId']:
                select = self.FindWindowById(p['wxId'][0])
                if not select.GetValue():
                    gcmd.GMessage(parent=self, message=_("No dataset given."))
                    return
                datasets = select.GetValue().split(',')
                from timeline import frame
                frame.run(parent=self, datasets=datasets)

    def OnSelectFont(self, event):
        """Select font using font dialog"""
        myId = event.GetId()
        for p in self.task.params:
            if 'wxId' in p and myId in p['wxId']:
                from gui_core.dialogs import DefaultFontDialog
                dlg = DefaultFontDialog(parent=self,
                                        title=_('Select font'),
                                        style=wx.DEFAULT_DIALOG_STYLE,
                                        type='font')
                if dlg.ShowModal() == wx.ID_OK:
                    if dlg.font:
                        p['value'] = dlg.font
                        self.FindWindowById(p['wxId'][1]).SetValue(dlg.font)
                        self.OnUpdateValues(event)
                dlg.Destroy()


    def OnUpdateSelection(self, event):
        """Update dialog (layers, tables, columns, etc.)
        """
        if not hasattr(self.parent, "updateThread"):
            if event:
                event.Skip()
            return
        if event:
            self.parent.updateThread.Update(UpdateDialog,
                                            self,
                                            event,
                                            event.GetId(),
                                            self.task)
        else:
            self.parent.updateThread.Update(UpdateDialog,
                                            self,
                                            None,
                                            None,
                                            self.task)

    def createCmd(self, ignoreErrors=False, ignoreRequired=False):
        """Produce a command line string (list) or feeding into GRASS.

        :param ignoreErrors: True then it will return whatever has been
                             built so far, even though it would not be
                             a correct command for GRASS
        """
        try:
            cmd = self.task.get_cmd(ignoreErrors=ignoreErrors,
                                    ignoreRequired=ignoreRequired)
        except ValueError as err:
            dlg = wx.MessageDialog(parent=self,
                                   message=gcmd.DecodeString(str(err)),
                                   caption=_("Error in %s") % self.task.name,
                                   style=wx.OK | wx.ICON_ERROR | wx.CENTRE)
            dlg.ShowModal()
            dlg.Destroy()
            cmd = None

        return cmd

    def OnSize(self, event):
        width = event.GetSize()[0]
        fontsize = self.GetFont().GetPointSize()
        text_width = max(width / (fontsize - 3), 70)

        for id in self.label_id:
            win = self.FindWindowById(id)
            label = win.GetLabel()
            label_new = '\n'.join(textwrap.wrap(label, text_width))
            win.SetLabel(label_new)

        event.Skip()

    def AddBitmapToImageList(self, section, imageList):
        iconTheme = UserSettings.Get(
            group='appearance',
            key='iconTheme',
            subkey='type')
        iconSectionDict = {
            'manual': os.path.join(
                globalvar.ICONDIR,
                iconTheme,
                'help.png')}
        if section in iconSectionDict.keys():
            image = wx.Image(
                iconSectionDict[section]).Scale(
                16, 16, wx.IMAGE_QUALITY_HIGH)
            idx = imageList.Add(BitmapFromImage(image))
            return idx

        return -1


class GUI:

    def __init__(self, parent=None, giface=None, show=True, modal=False,
                 centreOnParent=False, checkError=False):
        """Parses GRASS commands when module is imported and used from
        Layer Manager.
        """
        self.parent = parent
        self.show = show
        self.modal = modal
        self._giface = giface
        self.centreOnParent = centreOnParent
        self.checkError = checkError

        self.grass_task = None
        self.cmd = list()

        global _blackList
        if self.parent:
            _blackList['enabled'] = True
        else:
            _blackList['enabled'] = False

    def GetCmd(self):
        """Get validated command"""
        return self.cmd

    def ParseCommand(self, cmd, completed=None):
        """Parse command

        Note: cmd is given as list

        If command is given with options, return validated cmd list:
         - add key name for first parameter if not given
         - change mapname to mapname@mapset
        """
        dcmd_params = {}
        if completed is None:
            get_dcmd = None
            layer = None
            dcmd_params = None
        else:
            get_dcmd = completed[0]
            layer = completed[1]
            if completed[2]:
                dcmd_params.update(completed[2])

        # parse the interface decription
        try:
            global _blackList
            self.grass_task = gtask.parse_interface(cmd[0],
                                                    blackList=_blackList)
        except (grass.ScriptError, ValueError) as e:
            raise gcmd.GException(e.value)

        # if layer parameters previously set, re-insert them into dialog
        if completed is not None:
            if 'params' in dcmd_params:
                self.grass_task.params = dcmd_params['params']
            if 'flags' in dcmd_params:
                self.grass_task.flags = dcmd_params['flags']

        err = list()
        # update parameters if needed && validate command
        if len(cmd) > 1:
            i = 0
            cmd_validated = [cmd[0]]
            for option in cmd[1:]:
                if option[0] == '-':  # flag
                    if len(option) == 1:  # catch typo like 'g.proj - w'
                        raise gcmd.GException(
                            _("Unable to parse command '%s'") %
                            ' '.join(cmd))
                    if option[1] == '-':
                        self.grass_task.set_flag(option[2:], True)
                    else:
                        self.grass_task.set_flag(option[1], True)
                    cmd_validated.append(option)
                else:  # parameter
                    try:
                        key, value = option.split('=', 1)
                    except ValueError:
                        if self.grass_task.firstParam:
                            if i == 0:  # add key name of first parameter if not given
                                key = self.grass_task.firstParam
                                value = option
                            else:
                                raise gcmd.GException(
                                    _("Unable to parse command '%s'") % ' '.join(cmd))
                        else:
                            continue

                    task = self.grass_task.get_param(key, raiseError=False)
                    if not task:
                        err.append(
                            _("%(cmd)s: parameter '%(key)s' not available") % {
                                'cmd': cmd[0],
                                'key': key})
                        continue

                    self.grass_task.set_param(key, value)
                    cmd_validated.append(key + '=' + value)
                    i += 1

            # update original command list
            cmd = cmd_validated

        if self.show is not None:
            self.mf = TaskFrame(parent=self.parent, giface=self._giface,
                                task_description=self.grass_task,
                                get_dcmd=get_dcmd, layer=layer)
        else:
            self.mf = None

        if get_dcmd is not None:
            # update only propwin reference
            get_dcmd(dcmd=None, layer=layer, params=None,
                     propwin=self.mf)

        if self.show is not None:
            self.mf.notebookpanel.OnUpdateSelection(None)
            if self.show is True:
                if self.parent and self.centreOnParent:
                    self.mf.CentreOnParent()
                else:
                    self.mf.CenterOnScreen()
                self.mf.Show(self.show)
                self.mf.MakeModal(self.modal)
            else:
                self.mf.OnApply(None)

        self.cmd = cmd

        if self.checkError:
            return self.grass_task, err
        else:
            return self.grass_task

    def GetCommandInputMapParamKey(self, cmd):
        """Get parameter key for input raster/vector map

        :param cmd: module name

        :return: parameter key
        :return: None on failure
        """
        # parse the interface decription
        if not self.grass_task:
            tree = etree.fromstring(gtask.get_interface_description(cmd))
            self.grass_task = gtask.processTask(tree).get_task()

            for p in self.grass_task.params:
                if p.get('name', '') in ('input', 'map'):
                    age = p.get('age', '')
                    prompt = p.get('prompt', '')
                    element = p.get('element', '')
                    if age == 'old' and \
                            element in ('cell', 'grid3', 'vector') and \
                            prompt in ('raster', 'raster_3d', 'vector'):
                        return p.get('name', None)
        return None


class GrassGUIApp(wx.App):
    """Stand-alone GRASS command GUI
    """

    def __init__(self, grass_task):
        self.grass_task = grass_task
        wx.App.__init__(self, False)

    def OnInit(self):
        msg = self.grass_task.get_error_msg()
        if msg:
            gcmd.GError(
                msg +
                '\n\n' +
                _('Try to set up GRASS_ADDON_PATH or GRASS_ADDON_BASE variable.'))
            return True

        self.mf = TaskFrame(
            parent=None,
            giface=None,
            task_description=self.grass_task)
        self.mf.CentreOnScreen()
        self.mf.Show(True)
        self.SetTopWindow(self.mf)

        return True


USAGE_MESSAGE = """Usage:
    {name} <grass module>
    {name} <full path to file>
    python {name} <grass module>
Test:
    python {name} test
    python {name} g.region
    python {name} "g.region -p"
    python {name} temporal/t.list/t.list.py"""


if __name__ == "__main__":

    if len(sys.argv) == 1:
        sys.exit(_(USAGE_MESSAGE).format(name=sys.argv[0]))

    if sys.argv[1] != 'test':
        q = wx.LogNull()
        Debug.msg(1, "forms.py called using command: %s" % sys.argv[1])
        cmd = utils.split(sys.argv[1])
        task = gtask.grassTask(cmd[0])
        task.set_options(cmd[1:])
        Debug.msg(1, "forms.py opening form for: %s" %
                  task.get_cmd(ignoreErrors=True, ignoreRequired=True))
        app = GrassGUIApp(task)
        app.MainLoop()
    else:  # Test
        # Test grassTask from within a GRASS session
        if os.getenv("GISBASE") is not None:
            task = gtask.grassTask("d.vect")
            task.get_param('map')['value'] = "map_name"
            task.get_flag('i')['value'] = True
            task.get_param('layer')['value'] = 1
            task.get_param('label_bcolor')['value'] = "red"
            # the default parameter display is added automatically
            assert ' '.join(
                task.get_cmd()) == "d.vect -i map=map_name layer=1 display=shape label_bcolor=red"
            print("Creation of task successful")
        # Test interface building with handmade grassTask,
        # possibly outside of a GRASS session.
        print("Now creating a module dialog (task frame)")
        task = gtask.grassTask()
        task.name = "TestTask"
        task.description = "This is an artificial grassTask() object intended for testing purposes."
        task.keywords = ["grass", "test", "task"]
        task.params = [
            {
                "name": "text",
                "description": "Descriptions go into tooltips if labels are present, like this one",
                "label": "Enter some text",
                "key_desc": ["value"],
                "values_desc": []
            }, {
                "name": "hidden_text",
                "description": "This text should not appear in the form",
                "hidden": True,
                "key_desc": ["value"],
                "values_desc": []
            }, {
                "name": "text_default",
                "description": "Enter text to override the default",
                "default": "default text",
                "key_desc": ["value"],
                "values_desc": []
            }, {
                "name": "text_prefilled",
                "description": "You should see a friendly welcome message here",
                "value": "hello, world",
                "key_desc": ["value"],
                "values_desc": []
            }, {
                "name": "plain_color",
                "description": "This is a plain color, and it is a compulsory parameter",
                "required": False,
                "gisprompt": True,
                "prompt": "color",
                "key_desc": ["value"],
                "values_desc": []
            }, {
                "name": "transparent_color",
                "description": "This color becomes transparent when set to none",
                "guisection": "tab",
                "gisprompt": True,
                "prompt": "color",
                "key_desc": ["value"],
                "values_desc": []
            }, {
                "name": "multi",
                "description": "A multiple selection",
                'default': u'red,green,blue',
                'gisprompt': False,
                'guisection': 'tab',
                'multiple': u'yes',
                'type': u'string',
                'value': '',
                'values': ['red', 'green', u'yellow', u'blue', u'purple', u'other'],
                "key_desc": ["value"],
                "values_desc": []
            }, {
                "name": "single",
                "description": "A single multiple-choice selection",
                'values': ['red', 'green', u'yellow', u'blue', u'purple', u'other'],
                "guisection": "tab",
                "key_desc": ["value"],
                "values_desc": []
            }, {
                "name": "large_multi",
                "description": "A large multiple selection",
                "gisprompt": False,
                "multiple": "yes",
                # values must be an array of strings
                "values": utils.str2rgb.keys() + list(map(str, utils.str2rgb.values())),
                "key_desc": ["value"],
                "values_desc": []
            }, {
                "name": "a_file",
                "description": "A file selector",
                "gisprompt": True,
                "element": "file",
                "key_desc": ["value"],
                "values_desc": []
            }
        ]
        task.flags = [{"name": "a",
                       "description": "Some flag, will appear in Main since it is required",
                       "required": True,
                       "value": False,
                       "suppress_required": False},
                      {"name": "b",
                       "description": "pre-filled flag, will appear in options since it is not required",
                       "value": True,
                       "suppress_required": False},
                      {"name": "hidden_flag",
                       "description": "hidden flag, should not be changeable",
                       "hidden": "yes",
                       "value": True,
                       "suppress_required": False}]
        q = wx.LogNull()
        GrassGUIApp(task).MainLoop()
