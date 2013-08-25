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

@todo
 - verify option value types

Copyright(C) 2000-2013 by the GRASS Development Team

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

import sys
import string
import textwrap
import os
import copy
import locale
import Queue
import re
import codecs

from threading import Thread

gisbase = os.getenv("GISBASE")
if gisbase is None:
    print >>sys.stderr, "We don't seem to be properly installed, or we are being run outside GRASS. Expect glitches."
    gisbase = os.path.join(os.path.dirname(sys.argv[0]), os.path.pardir)
    wxbase = gisbase
else:
    wxbase = os.path.join(gisbase, 'etc', 'gui', 'wxpython')

if wxbase not in sys.path:
    sys.path.append(wxbase)

from core import globalvar
import wx
try:
    import wx.lib.agw.flatnotebook as FN
except ImportError:
    import wx.lib.flatnotebook as FN
import wx.lib.colourselect     as csel
import wx.lib.filebrowsebutton as filebrowse
from wx.lib.newevent import NewEvent

try:
    import xml.etree.ElementTree as etree
except ImportError:
    import elementtree.ElementTree as etree # Python <= 2.4

from grass.pydispatch.signal import Signal

from grass.script import core as grass
from grass.script import task as gtask

from gui_core.widgets import StaticWrapText, ScrolledPanel, ColorTablesComboBox
from gui_core.ghelp   import HelpPanel
from gui_core         import gselect
from core             import gcmd
from core             import utils
from core.utils import _
from core.settings    import UserSettings
from gui_core.widgets import FloatValidator, GNotebook, FormNotebook, FormListbook

wxUpdateDialog, EVT_DIALOG_UPDATE = NewEvent()


"""!Hide some options in the GUI"""
#_blackList = { 'enabled' : False,
#               'items'   : { 'r.buffer' : {'params' : ['input', 'output'],
#                                           'flags' : ['z', 'overwrite']}}}
_blackList = { 'enabled' : False,
               'items'   : {} }


def text_beautify(someString , width = 70):
    """!Make really long texts shorter, clean up whitespace and remove
    trailing punctuation.
    """
    if width > 0:
        return escape_ampersand(string.strip(
                os.linesep.join(textwrap.wrap(utils.normalize_whitespace(someString), width)),
                ".,;:"))
    else:
        return escape_ampersand(string.strip(utils.normalize_whitespace(someString), ".,;:"))
    
def escape_ampersand(text):
    """!Escapes ampersands with additional ampersand for GUI"""
    return string.replace(text, "&", "&&")

class UpdateThread(Thread):
    """!Update dialog widgets in the thread"""
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
        
        p = self.task.get_param(self.eventId, element = 'wxId', raiseError = False)
        if not p or 'wxId-bind' not in p:
            return

        # is this check necessary?
        # get widget prompt
        # pType = p.get('prompt', '')
        # if not pType:
        #     return
        
        # check for map/input parameter
        pMap = self.task.get_param('map', raiseError = False)
        
        if not pMap:
            pMap = self.task.get_param('input', raiseError = False)
        
        if pMap:
            map = pMap.get('value', '')
        else:
            map = None
        
        # avoid running db.describe several times
        cparams = dict()
        cparams[map] = { 'dbInfo' : None,
                         'layers' : None, }
        
        # update reference widgets
        for uid in p['wxId-bind']:
            win = self.parent.FindWindowById(uid)
            if not win:
                continue
            
            name = win.GetName()
            pBind = self.task.get_param(uid, element = 'wxId', raiseError = False)
            if pBind:
                pBind['value'] = ''
            
            # set appropriate types in t.* modules element selections
            if name == 'Select':
                type_param = self.task.get_param('type', element = 'name', raiseError = False)
                maps_param = self.task.get_param('maps', element = 'name', raiseError = False)
                self.data[win.GetParent().SetType] = {'etype': type_param.get('value')}
                # t.(un)register has one type for 'input', 'maps'
                if maps_param is not None:
                    if maps_param['wxId'][0] != uid:
                        element_dict = {'rast': 'strds', 'vect': 'stvds', 'rast3d': 'str3ds'}
                        self.data[win.GetParent().SetType] = {'etype': element_dict[type_param.get('value')]}

            map = layer = None
            driver = db = None
            if name in ('LayerSelect', 'ColumnSelect'):
                if p.get('element', '') == 'vector': # -> vector
                    # get map name
                    map = p.get('value', '')
                    
                    # get layer
                    for bid in p['wxId-bind']:
                        p = self.task.get_param(bid, element = 'wxId', raiseError = False)
                        if not p:
                            continue
                        
                        if p.get('element', '') in ['layer', 'layer_all']:
                            layer = p.get('value', '')
                            if layer != '':
                                layer = p.get('value', '')
                            else:
                                layer = p.get('default', '')
                            break
                        
                elif p.get('element', '') in ['layer', 'layer_all']: # -> layer
                    # get layer
                    layer = p.get('value', '')
                    if layer != '':
                        layer = p.get('value', '')
                    else:
                        layer = p.get('default', '')
                    
                    # get map name
                    pMapL = self.task.get_param(p['wxId'][0], element = 'wxId-bind', raiseError = False)
                    if pMapL:
                        map = pMapL.get('value', '')
            
            if name == 'TableSelect' or \
                    (name == 'ColumnSelect' and not map):
                pDriver = self.task.get_param('dbdriver', element = 'prompt', raiseError = False)
                if pDriver:
                    driver = pDriver.get('value', '')
                pDb = self.task.get_param('dbname', element = 'prompt', raiseError = False)
                if pDb:
                    db = pDb.get('value', '')
                if name == 'ColumnSelect':
                    pTable = self.task.get_param('dbtable', element = 'element', raiseError = False)
                    if pTable:
                        table = pTable.get('value', '')

            if name == 'LayerSelect':
                # determine format
                native = True
                
                for id in pMap['wxId']:
                    winVec  = self.parent.FindWindowById(id)
                    if winVec.GetName() == 'VectorFormat' and \
                            winVec.GetSelection() != 0:
                        native = False
                        break
                # TODO: update only if needed
                if native:
                    if map:
                        self.data[win.InsertLayers] = { 'vector' : map }
                    else:
                        self.data[win.InsertLayers] = { }
                else:
                    if map:
                        self.data[win.InsertLayers] = { 'dsn' : map.rstrip('@OGR') }
                    else:
                        self.data[win.InsertLayers] = { }
            
            elif name == 'TableSelect':
                self.data[win.InsertTables] = { 'driver' : driver,
                                                'database' : db }
                
            elif name == 'ColumnSelect':
                if map:
                    if map in cparams:
                        if not cparams[map]['dbInfo']:
                            cparams[map]['dbInfo'] = gselect.VectorDBInfo(map)
                        self.data[win.GetParent().InsertColumns] = { 'vector' : map, 'layer' : layer,
                                                                     'dbInfo' : cparams[map]['dbInfo'] }
                else: # table
                    if driver and db:
                        self.data[win.GetParent().InsertTableColumns] = { 'table' : pTable.get('value'),
                                                                          'driver' : driver,
                                                                          'database' : db }
                    elif pTable:
                        self.data[win.GetParent().InsertTableColumns] = { 'table'  : pTable.get('value') }
            
            elif name == 'SubGroupSelect':
                self.data[win.Insert] = { 'group' : p.get('value', '')}

            elif name == 'SignatureSelect':
                if p.get('prompt', 'group') == 'group':
                    group = p.get('value', '')
                    pSubGroup = self.task.get_param('subgroup', element = 'prompt', raiseError = False)
                    if pSubGroup:
                        subgroup = pSubGroup.get('value', '')
                    else:
                        subgroup = None
                else:
                    subgroup = p.get('value', '')
                    pGroup = self.task.get_param('group', element = 'prompt', raiseError = False)
                    if pGroup:
                        group = pGroup.get('value', '')
                    else:
                        group = None
                
                self.data[win.Insert] = { 'group' : group,
                                          'subgroup' : subgroup}
            
            elif name == 'LocationSelect':
                pDbase = self.task.get_param('dbase', element = 'element', raiseError = False)
                if pDbase:
                    self.data[win.UpdateItems] = { 'dbase' : pDbase.get('value', '')}

            elif name == 'MapsetSelect':
                pDbase = self.task.get_param('dbase', element = 'element', raiseError = False)
                pLocation = self.task.get_param('location', element = 'element', raiseError = False)
                if pDbase and pLocation:
                    self.data[win.UpdateItems] = { 'dbase' : pDbase.get('value', ''),
                                                   'location' : pLocation.get('value', '')}

            elif name == 'ProjSelect':
                pDbase = self.task.get_param('dbase', element = 'element', raiseError = False)
                pLocation = self.task.get_param('location', element = 'element', raiseError = False)
                pMapset = self.task.get_param('mapset', element = 'element', raiseError = False)
                if pDbase and pLocation and pMapset:
                    self.data[win.UpdateItems] = { 'dbase' : pDbase.get('value', ''),
                                                   'location' : pLocation.get('value', ''),
                                                   'mapset' : pMapset.get('value', '')}

def UpdateDialog(parent, event, eventId, task):
    return UpdateThread(parent, event, eventId, task)

class UpdateQThread(Thread):
    """!Update dialog widgets in the thread"""
    requestId = 0
    def __init__(self, parent, requestQ, resultQ, **kwds):
        Thread.__init__(self, **kwds)
        
        self.parent = parent # cmdPanel
        self.setDaemon(True)
        
        self.requestQ = requestQ
        self.resultQ = resultQ
        
        self.start()
        
    def Update(self, callable, *args, **kwds):
        UpdateQThread.requestId +=  1
        
        self.request = None
        self.requestQ.put((UpdateQThread.requestId, callable, args, kwds))
        
        return UpdateQThread.requestId
    
    def run(self):
        while True:
            requestId, callable, args, kwds = self.requestQ.get()
            
            self.request = callable(*args, **kwds)

            self.resultQ.put((requestId, self.request.run()))
           
            if self.request:
                event = wxUpdateDialog(data = self.request.data)
                wx.PostEvent(self.parent, event)

class TaskFrame(wx.Frame):
    """!This is the Frame containing the dialog for options input.

    The dialog is organized in a notebook according to the guisections
    defined by each GRASS command.

    If run with a parent, it may Apply, Ok or Cancel; the latter two
    close the dialog.  The former two trigger a callback.

    If run standalone, it will allow execution of the command.

    The command is checked and sent to the clipboard when clicking
    'Copy'.
    """
    def __init__(self, parent, giface, task_description, id = wx.ID_ANY,
                 get_dcmd = None, layer = None,
                 style = wx.DEFAULT_FRAME_STYLE | wx.TAB_TRAVERSAL, **kwargs):
        self.get_dcmd = get_dcmd
        self.layer    = layer
        self.task     = task_description
        self.parent   = parent             # LayerTree | Modeler | None | ...
        self._giface  = giface
        if parent and parent.GetName() == 'Modeler':
            self.modeler = self.parent
        else:
            self.modeler = None

        self.dialogClosing = Signal('TaskFrame.dialogClosing')
        
        # module name + keywords
        title = self.task.get_name()
        try:
            if self.task.keywords !=  ['']:
                title +=   " [" + ', '.join(self.task.keywords) + "]"
        except ValueError:
            pass
        
        wx.Frame.__init__(self, parent = parent, id = id, title = title,
                          name = "MainFrame", style = style, **kwargs)
        
        self.locale = wx.Locale(language = wx.LANGUAGE_DEFAULT)
        
        self.panel = wx.Panel(parent = self, id = wx.ID_ANY)
        
        # statusbar
        self.CreateStatusBar()
        
        # icon
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass_dialog.ico'), wx.BITMAP_TYPE_ICO))
        
        guisizer = wx.BoxSizer(wx.VERTICAL)
        
        # set apropriate output window
        if self.parent:
            self.standalone = False
        else:
            self.standalone = True
        
        # logo + description
        topsizer = wx.BoxSizer(wx.HORIZONTAL)
        
        # GRASS logo
        self.logo = wx.StaticBitmap(parent = self.panel,
                                    bitmap = wx.Bitmap(name = os.path.join(globalvar.ETCIMGDIR,
                                                                           'grass_form.png'),
                                                     type = wx.BITMAP_TYPE_PNG))
        topsizer.Add(item = self.logo, proportion = 0, border = 3,
                     flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL)
        
        # add module description
        if self.task.label:
            module_desc = self.task.label + ' ' + self.task.description
        else:
            module_desc = self.task.description
        self.description = StaticWrapText(parent = self.panel,
                                          label = module_desc)
        topsizer.Add(item = self.description, proportion = 1, border = 5,
                     flag = wx.ALL | wx.ALIGN_CENTER_VERTICAL | wx.EXPAND)
        
        guisizer.Add(item = topsizer, proportion = 0, flag = wx.EXPAND)
        
        self.panel.SetSizerAndFit(guisizer)
        self.Layout()
        
        # notebooks
        self.notebookpanel = CmdPanel(parent = self.panel, giface = self._giface, task = self.task,
                                      frame = self)
        self._gconsole = self.notebookpanel._gconsole
        if self._gconsole:
            self._gconsole.mapCreated.connect(self.OnMapCreated)
        self.goutput = self.notebookpanel.goutput
        if self.goutput:
            self.goutput.showNotification.connect(lambda message: self.SetStatusText(message))
        
        self.notebookpanel.OnUpdateValues = self.updateValuesHook
        guisizer.Add(item = self.notebookpanel, proportion = 1, flag = wx.EXPAND)
        
        # status bar
        status_text = _("Enter parameters for '") + self.task.name + "'"
        try:
            self.task.get_cmd()
            self.updateValuesHook()
        except ValueError:
            self.SetStatusText(status_text)
        
        # buttons
        btnsizer = wx.BoxSizer(orient = wx.HORIZONTAL)
        # cancel
        self.btn_cancel = wx.Button(parent = self.panel, id = wx.ID_CLOSE)
        self.btn_cancel.SetToolTipString(_("Close this window without executing the command (Ctrl+Q)"))
        btnsizer.Add(item = self.btn_cancel, proportion = 0, flag = wx.ALL | wx.ALIGN_CENTER, border = 10)
        self.btn_cancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        # bind closing to ESC and CTRL+Q
        self.Bind(wx.EVT_MENU, self.OnCancel, id=wx.ID_CLOSE)
        accelTableList = [(wx.ACCEL_NORMAL, wx.WXK_ESCAPE, wx.ID_CLOSE)]
        accelTableList = [(wx.ACCEL_CTRL, ord('Q'), wx.ID_CLOSE)]
        # TODO: bind Ctrl-t for tile windows here (trac #2004)

        if self.get_dcmd is not None: # A callback has been set up
            btn_apply = wx.Button(parent = self.panel, id = wx.ID_APPLY)
            btn_ok = wx.Button(parent = self.panel, id = wx.ID_OK)
            btn_ok.SetDefault()

            btnsizer.Add(item = btn_apply, proportion = 0,
                         flag = wx.ALL | wx.ALIGN_CENTER,
                         border = 10)
            btnsizer.Add(item = btn_ok, proportion = 0,
                         flag = wx.ALL | wx.ALIGN_CENTER,
                         border = 10)
            
            btn_apply.Bind(wx.EVT_BUTTON, self.OnApply)
            btn_ok.Bind(wx.EVT_BUTTON, self.OnOK)
        else: # We're standalone
            # run
            self.btn_run = wx.Button(parent = self.panel, id = wx.ID_OK, label =  _("&Run"))
            self.btn_run.SetToolTipString(_("Run the command (Ctrl+R)"))
            self.btn_run.SetDefault()
            self.btn_run.SetForegroundColour(wx.Colour(35, 142, 35))
            
            # copy
            self.btn_clipboard = wx.Button(parent = self.panel, id = wx.ID_COPY)
            self.btn_clipboard.SetToolTipString(_("Copy the current command string to the clipboard"))
            
            btnsizer.Add(item = self.btn_run, proportion = 0,
                         flag = wx.ALL | wx.ALIGN_CENTER,
                         border = 10)
            
            btnsizer.Add(item = self.btn_clipboard, proportion = 0,
                         flag = wx.ALL | wx.ALIGN_CENTER,
                         border = 10)
            
            self.btn_run.Bind(wx.EVT_BUTTON, self.OnRun)
            self.Bind(wx.EVT_MENU, self.OnRun, id=wx.ID_OK)
            accelTableList.append((wx.ACCEL_CTRL, ord('R'), wx.ID_OK))
            self.btn_clipboard.Bind(wx.EVT_BUTTON, self.OnCopy)
        # help
        self.btn_help = wx.Button(parent = self.panel, id = wx.ID_HELP)
        self.btn_help.SetToolTipString(_("Show manual page of the command (Ctrl+H)"))
        self.btn_help.Bind(wx.EVT_BUTTON, self.OnHelp)
        self.Bind(wx.EVT_MENU, self.OnHelp, id=wx.ID_HELP)
        accelTableList.append((wx.ACCEL_CTRL, ord('H'), wx.ID_HELP))

        if self.notebookpanel.notebook.GetPageIndexByName('manual') < 0:
            self.btn_help.Hide()
        
        # add help button
        btnsizer.Add(item = self.btn_help, proportion = 0, flag = wx.ALL | wx.ALIGN_CENTER, border = 10)
        
        guisizer.Add(item = btnsizer, proportion = 0, flag = wx.ALIGN_CENTER | wx.LEFT | wx.RIGHT,
                     border = 30)
        # abort key bindings
        abortId = wx.NewId()
        self.Bind(wx.EVT_MENU, self.OnAbort, id=abortId)
        accelTableList.append((wx.ACCEL_CTRL, ord('S'), abortId))
        # set accelerator table
        accelTable = wx.AcceleratorTable(accelTableList)
        self.SetAcceleratorTable(accelTable)
        
        if self.parent and not self.modeler:
            addLayer = False
            for p in self.task.params:
                if p.get('age', 'old') == 'new' and \
                   p.get('prompt', '') in ('raster', 'vector', '3d-raster'):
                    addLayer = True
            
            if addLayer:
                # add newly created map into layer tree
                self.addbox = wx.CheckBox(parent = self.panel,
                                          label = _('Add created map(s) into layer tree'), style = wx.NO_BORDER)
                self.addbox.SetValue(UserSettings.Get(group = 'cmd', key = 'addNewLayer', subkey = 'enabled'))
                guisizer.Add(item = self.addbox, proportion = 0,
                             flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                             border = 5)
        
        hasNew = False
        for p in self.task.params:
            if p.get('age', 'old') == 'new':
                hasNew = True
                break
        
        if self.get_dcmd is None and hasNew:
            # close dialog when command is terminated
            self.closebox = wx.CheckBox(parent = self.panel,
                                        label = _('Close dialog on finish'), style = wx.NO_BORDER)
            self.closebox.SetValue(UserSettings.Get(group = 'cmd', key = 'closeDlg', subkey = 'enabled'))
            self.closebox.SetToolTipString(_("Close dialog when command is successfully finished. "
                                             "Change this settings in Preferences dialog ('Command' tab)."))
            guisizer.Add(item = self.closebox, proportion = 0,
                         flag = wx.EXPAND | wx.LEFT | wx.RIGHT | wx.BOTTOM,
                         border = 5)
        # bindings
        self.Bind(wx.EVT_CLOSE,  self.OnCancel)
        
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
        self.SetSize(wx.Size(sizeFrame[0], sizeFrame[1] + scale * max(self.notebookpanel.panelMinHeight,
                                                                      self.notebookpanel.constrained_size[1])))
        
        # thread to update dialog
        # create queues
        self.requestQ = Queue.Queue()
        self.resultQ = Queue.Queue()
        self.updateThread = UpdateQThread(self.notebookpanel, self.requestQ, self.resultQ)
        
        self.Layout()
        
        # keep initial window size limited for small screens
        width, height = self.GetSizeTuple()
        self.SetSize(wx.Size(min(width, 650),
                             min(height, 500)))
        
        # fix goutput's pane size (required for Mac OSX)
        if self.goutput:
            self.goutput.SetSashPosition(int(self.GetSize()[1] * .75))
        
    def updateValuesHook(self, event = None):
        """!Update status bar data"""
        self.SetStatusText(' '.join(self.notebookpanel.createCmd(ignoreErrors = True)))
        if event:
            event.Skip()

    def OnDone(self, cmd, returncode):
        """!This function is launched from OnRun() when command is
        finished

        @param returncode command's return code (0 for success)
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
                    (returncode == 0):
            # was closed also when aborted but better is leave it open
            wx.FutureCall(2000, self.Close)

    def OnMapCreated(self, name, ltype):
        if hasattr(self, "addbox") and self.addbox.IsChecked():
            add = True
        else:
            add = False
        if self._giface:
            self._giface.mapCreated.emit(name=name, ltype=ltype, add=add)
    
    def OnOK(self, event):
        """!OK button pressed"""
        cmd = self.OnApply(event)
        if cmd is not None and self.get_dcmd is not None:
            self.OnCancel(event)

    def OnApply(self, event):
        """!Apply the command"""
        if self.modeler:
            cmd = self.createCmd(ignoreErrors = True, ignoreRequired = True)
        else:
            cmd = self.createCmd()
        
        if cmd is not None and self.get_dcmd is not None:
            # return d.* command to layer tree for rendering
            self.get_dcmd(cmd, self.layer, {"params": self.task.params, 
                                            "flags" : self.task.flags},
                          self)
            # echo d.* command to output console
            # self.parent.writeDCommand(cmd)

        return cmd

    def OnRun(self, event):
        """!Run the command"""
        cmd = self.createCmd()
        
        if not cmd or len(cmd) < 1:
            return
        
        ret = 0
        if self.standalone or cmd[0][0:2] !=  "d.":
            # Send any non-display command to parent window (probably wxgui.py)
            # put to parents switch to 'Command output'
            self.notebookpanel.notebook.SetSelectionByName('output')
            
            try:
                if self.task.path:
                    cmd[0] = self.task.path # full path
                
                ret = self._gconsole.RunCmd(cmd, onDone = self.OnDone)
            except AttributeError, e:
                print >> sys.stderr, "%s: Probably not running in wxgui.py session?" % (e)
                print >> sys.stderr, "parent window is: %s" % (str(self.parent))
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
        """!Abort running command"""
        from core.gconsole import wxCmdAbort
        event = wxCmdAbort(aborted = True)
        wx.PostEvent(self._gconsole, event)

    def OnCopy(self, event):
        """!Copy the command"""
        cmddata = wx.TextDataObject()
        # list -> string
        cmdlist = self.createCmd(ignoreErrors = True)
        # TODO: better protect whitespace with quotes
        for i in range(1, len(cmdlist)):
            if ' ' in cmdlist[i]:
                optname, val =  cmdlist[i].split("=")
                cmdlist[i] = '%s="%s"' % (optname, val)
        cmdstring = ' '.join(cmdlist)
        cmddata.SetText(cmdstring)
        if wx.TheClipboard.Open():
#            wx.TheClipboard.UsePrimarySelection(True)
            wx.TheClipboard.SetData(cmddata)
            wx.TheClipboard.Close()
            self.SetStatusText(_("'%s' copied to clipboard") % \
                                    (cmdstring))

    def OnCancel(self, event):
        """!Cancel button pressed"""
        self.MakeModal(False)
        self.dialogClosing.emit()
        if self.get_dcmd and \
                self.parent and \
                self.parent.GetName() in ('LayerTree',
                                          'MapWindow'):
            # display decorations and 
            # pressing OK or cancel after setting layer properties
            if self.task.name in ['d.barscale','d.legend','d.histogram'] \
                or len(self.parent.GetLayerInfo(self.layer, key = 'cmd')) >=  1:
                self.Hide()
            # canceled layer with nothing set
            elif len(self.parent.GetLayerInfo(self.layer, key = 'cmd')) < 1:
                self.parent.Delete(self.layer)
                self.Destroy()
        else:
            # cancel for non-display commands
            self.Destroy()

    def OnHelp(self, event):
        """!Show manual page (switch to the 'Manual' notebook page)"""
        if self.notebookpanel.notebook.GetPageIndexByName('manual') > -1:
            self.notebookpanel.notebook.SetSelectionByName('manual')
            self.notebookpanel.OnPageChange(None)
        
        if event:    
            event.Skip()
        
    def createCmd(self, ignoreErrors = False, ignoreRequired = False):
        """!Create command string (python list)"""
        return self.notebookpanel.createCmd(ignoreErrors = ignoreErrors,
                                            ignoreRequired = ignoreRequired)

class CmdPanel(wx.Panel):
    """!A panel containing a notebook dividing in tabs the different
    guisections of the GRASS cmd.
    """
    def __init__(self, parent, giface, task, id = wx.ID_ANY, frame = None, *args, **kwargs):
        if frame:
            self.parent = frame
        else:
            self.parent = parent
        self.task = task
        self._giface = giface
        
        wx.Panel.__init__(self, parent, id = id, *args, **kwargs)

        self.mapCreated = Signal

        # Determine tab layout
        sections = []
        is_section = {}
        not_hidden = [ p for p in self.task.params + self.task.flags if not p.get('hidden', False) == True ]

        self.label_id = [] # wrap titles on resize

        self.Bind(wx.EVT_SIZE, self.OnSize)
        
        for task in not_hidden:
            if task.get('required', False):
                # All required go into Main, even if they had defined another guisection
                task['guisection'] = _('Required')
            if task.get('guisection','') == '':
                # Undefined guisections end up into Options
                task['guisection'] = _('Optional')
            if task['guisection'] not in is_section:
                # We do it like this to keep the original order, except for Main which goes first
                is_section[task['guisection']] = 1
                sections.append(task['guisection'])
            else:
                is_section[ task['guisection'] ] +=  1
        del is_section

        # 'Required' tab goes first, 'Optional' as the last one
        for (newidx,content) in [ (0,_('Required')), (len(sections)-1,_('Optional')) ]:
            if content in sections:
                idx = sections.index(content)
                sections[idx:idx+1] = []
                sections[newidx:newidx] =  [content]

        panelsizer = wx.BoxSizer(orient = wx.VERTICAL)

        # build notebook
        style = UserSettings.Get(group = 'appearance', key = 'commandNotebook', subkey = 'selection')
        if style == 0: # basic top
            self.notebook = FormNotebook(self, style = wx.BK_TOP)
            self.notebook.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.OnPageChange)
        elif style == 1: # basic left
            self.notebook = FormNotebook(self, style = wx.BK_LEFT)
            self.notebook.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGED, self.OnPageChange)
        elif style == 2: # fancy green
            self.notebook = GNotebook(self, style = globalvar.FNPageStyle | FN.FNB_NO_X_BUTTON )
            self.notebook.SetTabAreaColour(globalvar.FNPageColor)
            self.notebook.Bind(FN.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnPageChange)
        elif style == 3:
            self.notebook = FormListbook(self, style = wx.BK_LEFT)
            self.notebook.Bind(wx.EVT_LISTBOOK_PAGE_CHANGED, self.OnPageChange)
        self.notebook.Refresh()

        tab = {}
        tabsizer = {}
        for section in sections:
            tab[section] = ScrolledPanel(parent = self.notebook)
            tab[section].SetScrollRate(10, 10)
            tabsizer[section] = wx.BoxSizer(orient = wx.VERTICAL)
        

        #
        # flags
        #
        visible_flags = [ f for f in self.task.flags if not f.get('hidden', False) == True ]
        for f in visible_flags:
            which_sizer = tabsizer[ f['guisection'] ]
            which_panel = tab[ f['guisection'] ]
            # if label is given: description -> tooltip
            if f.get('label','') !=  '':
                title = text_beautify(f['label'])
                tooltip = text_beautify(f['description'], width = -1)
            else:
                title = text_beautify(f['description'])
                tooltip = None
            title_sizer = wx.BoxSizer(wx.HORIZONTAL)
            rtitle_txt = wx.StaticText(parent = which_panel,
                                       label = '(' + f['name'] + ')')
            chk = wx.CheckBox(parent = which_panel, label = title, style = wx.NO_BORDER)
            self.label_id.append(chk.GetId())
            if tooltip:
                chk.SetToolTipString(tooltip)
            chk.SetValue(f.get('value', False))
            title_sizer.Add(item = chk, proportion = 1,
                            flag = wx.EXPAND)
            title_sizer.Add(item = rtitle_txt, proportion = 0,
                            flag = wx.ALIGN_RIGHT | wx.ALIGN_CENTER_VERTICAL)
            which_sizer.Add(item = title_sizer, proportion = 0,
                            flag = wx.EXPAND | wx.TOP | wx.LEFT | wx.RIGHT, border = 5)
            f['wxId'] = [ chk.GetId(), ]
            chk.Bind(wx.EVT_CHECKBOX, self.OnSetValue)
            
            if self.parent.GetName() == 'MainFrame' and self.parent.modeler:
                parChk = wx.CheckBox(parent = which_panel, id = wx.ID_ANY,
                                     label = _("Parameterized in model"))
                parChk.SetName('ModelParam')
                parChk.SetValue(f.get('parameterized', False))
                if 'wxId' in f:
                    f['wxId'].append(parChk.GetId())
                else:
                    f['wxId'] = [ parChk.GetId() ]
                parChk.Bind(wx.EVT_CHECKBOX, self.OnSetValue)
                which_sizer.Add(item = parChk, proportion = 0,
                                flag = wx.LEFT, border = 20)
            
            if f['name'] in ('verbose', 'quiet'):
                chk.Bind(wx.EVT_CHECKBOX, self.OnVerbosity)
                vq = UserSettings.Get(group = 'cmd', key = 'verbosity', subkey = 'selection')
                if f['name'] == vq:
                    chk.SetValue(True)
                    f['value'] = True
            elif f['name'] == 'overwrite' and 'value' not in f:
                chk.SetValue(UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled'))
                f['value'] = UserSettings.Get(group = 'cmd', key = 'overwrite', subkey = 'enabled')
                
        #
        # parameters
        #
        visible_params = [ p for p in self.task.params if not p.get('hidden', False) == True ]
        
        try:
            first_param = visible_params[0]
        except IndexError:
            first_param = None
        
        for p in visible_params:
            which_sizer = tabsizer[p['guisection']]
            which_panel = tab[p['guisection']]
            # if label is given -> label and description -> tooltip
            # otherwise description -> lavel
            if p.get('label','') !=  '':
                title = text_beautify(p['label'])
                tooltip = text_beautify(p['description'], width = -1)
            else:
                title = text_beautify(p['description'])
                tooltip = None
            
            prompt = p.get('prompt', '')
            
            # text style (required -> bold)
            if not p.get('required', False):
                text_style = wx.FONTWEIGHT_NORMAL
            else:
                text_style = wx.FONTWEIGHT_BOLD

            # title sizer (description, name, type)
            if (len(p.get('values', [])) > 0) and \
                    p.get('multiple', False) and \
                    p.get('gisprompt', False) == False and \
                    p.get('type', '') == 'string':
                title_txt = wx.StaticBox(parent = which_panel, id = wx.ID_ANY)
            else:
                title_sizer = wx.BoxSizer(wx.HORIZONTAL)
                title_txt = wx.StaticText(parent = which_panel)
                if p['key_desc']:
                    ltype = ','.join(p['key_desc'])
                else:
                    ltype = p['type']
                rtitle_txt = wx.StaticText(parent = which_panel,
                                           label = '(' + p['name'] + '=' + ltype + ')')
                title_sizer.Add(item = title_txt, proportion = 1,
                                flag = wx.LEFT | wx.TOP | wx.EXPAND, border = 5)
                title_sizer.Add(item = rtitle_txt, proportion = 0,
                                flag = wx.ALIGN_RIGHT | wx.RIGHT | wx.TOP, border = 5)
                which_sizer.Add(item = title_sizer, proportion = 0,
                                flag = wx.EXPAND)
            self.label_id.append(title_txt.GetId())

            # title expansion
            if p.get('multiple', False) and len(p.get('values','')) == 0:
                title = _("[multiple]") + " " + title
                if p.get('value','') ==  '' :
                    p['value'] = p.get('default','')

            if (len(p.get('values', [])) > 0):
                valuelist      = map(str, p.get('values',[]))
                valuelist_desc = map(unicode, p.get('values_desc',[]))

                if p.get('multiple', False) and \
                        p.get('gisprompt',False) == False and \
                        p.get('type', '') == 'string':
                    title_txt.SetLabel(" %s: (%s, %s) " % (title, p['name'], p['type']))
                    stSizer = wx.StaticBoxSizer(box = title_txt, orient = wx.VERTICAL)
                    if valuelist_desc:
                        hSizer = wx.FlexGridSizer(cols = 1, vgap = 1)
                    else:
                        hSizer = wx.FlexGridSizer(cols = 6, vgap = 1, hgap = 1)
                    isEnabled = {}
                    # copy default values
                    if p['value'] == '':
                        p['value'] = p.get('default', '')
                        
                    for defval in p.get('value', '').split(','):
                        isEnabled[ defval ] = 'yes'
                        # for multi checkboxes, this is an array of all wx IDs
                        # for each individual checkbox
                        p[ 'wxId' ] = list()
                    idx = 0
                    for val in valuelist:
                        try:
                            label = valuelist_desc[idx]
                        except IndexError:
                            label = val
                        
                        chkbox = wx.CheckBox(parent = which_panel,
                                             label = text_beautify(label))
                        p[ 'wxId' ].append(chkbox.GetId())
                        if val in isEnabled:
                            chkbox.SetValue(True)
                        hSizer.Add(item = chkbox, proportion = 0)
                        chkbox.Bind(wx.EVT_CHECKBOX, self.OnCheckBoxMulti)
                        idx +=  1
                        
                    stSizer.Add(item = hSizer, proportion = 0,
                                flag = wx.ADJUST_MINSIZE | wx.ALL, border = 1)
                    which_sizer.Add(item = stSizer, proportion = 0,
                                    flag = wx.EXPAND | wx.TOP | wx.RIGHT | wx.LEFT, border = 5)
                elif p.get('gisprompt', False) is False:
                    if len(valuelist) == 1: # -> textctrl
                        title_txt.SetLabel("%s (%s %s):" % (title, _('valid range'),
                                                            str(valuelist[0])))
                        if p.get('type', '') == 'integer' and \
                                not p.get('multiple', False):

                            # for multiple integers use textctrl instead of spinsctrl
                            try:
                                minValue, maxValue = map(int, valuelist[0].split('-'))
                            except ValueError:
                                minValue = -1e6
                                maxValue = 1e6
                            txt2 = wx.SpinCtrl(parent = which_panel, id = wx.ID_ANY, size = globalvar.DIALOG_SPIN_SIZE,
                                               min = minValue, max = maxValue)
                            style = wx.BOTTOM | wx.LEFT
                        else:
                            txt2 = wx.TextCtrl(parent = which_panel, value = p.get('default',''))
                            style = wx.EXPAND | wx.BOTTOM | wx.LEFT
                        
                        value = self._getValue(p)
                        # parameter previously set
                        if value:
                            if isinstance(txt2, wx.SpinCtrl):
                                txt2.SetValue(int(value)) 
                            else:
                                txt2.SetValue(value)
                        
                        which_sizer.Add(item = txt2, proportion = 0,
                                        flag = style, border = 5)

                        p['wxId'] = [ txt2.GetId(), ]
                        txt2.Bind(wx.EVT_TEXT, self.OnSetValue)
                    else:
                        
                        title_txt.SetLabel(title + ':')
                        value = self._getValue(p)
                        
                        if p['name'] == 'icon': # symbols
                            bitmap = wx.Bitmap(os.path.join(globalvar.ETCSYMBOLDIR, value) + '.png')
                            bb = wx.BitmapButton(parent = which_panel, id = wx.ID_ANY,
                                                 bitmap = bitmap)
                            iconLabel = wx.StaticText(parent = which_panel, id = wx.ID_ANY)
                            iconLabel.SetLabel(value)
                            p['value'] = value
                            p['wxId'] = [bb.GetId(), iconLabel.GetId()]
                            bb.Bind(wx.EVT_BUTTON, self.OnSetSymbol)
                            this_sizer = wx.BoxSizer(wx.HORIZONTAL)
                            this_sizer.Add(item = bb, proportion = 0,
                                            flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT, border = 5)
                            this_sizer.Add(item = iconLabel, proportion = 0,
                                            flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.ALIGN_CENTER_VERTICAL, border = 5)
                            which_sizer.Add(item = this_sizer, proportion = 0,
                                            flag = wx.ADJUST_MINSIZE, border = 0)
                        else:
                            # list of values (combo)
                            if p['name'] == 'color':
                                cb = ColorTablesComboBox(parent=which_panel, value=p.get('default',''),
                                                         size=globalvar.DIALOG_COMBOBOX_SIZE,
                                                         choices=valuelist)
                            else:
                                cb = wx.ComboBox(parent=which_panel, id=wx.ID_ANY, value=p.get('default',''),
                                                 size=globalvar.DIALOG_COMBOBOX_SIZE,
                                                 choices=valuelist, style=wx.CB_DROPDOWN)
                            if value:
                                cb.SetValue(value) # parameter previously set
                            which_sizer.Add(item = cb, proportion = 0,
                                            flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT, border = 5)
                            p['wxId'] = [cb.GetId(),]
                            cb.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                            cb.Bind(wx.EVT_TEXT, self.OnSetValue)
                            if p.get('guidependency', ''):
                                cb.Bind(wx.EVT_COMBOBOX, self.OnUpdateSelection)
            
            # text entry
            if (p.get('type','string') in ('string','integer','float')
                and len(p.get('values',[])) == 0
                and p.get('gisprompt',False) == False
                and p.get('prompt','') !=  'color'):

                title_txt.SetLabel(title + ':')
                if p.get('multiple', False) or \
                        p.get('type', 'string') == 'string' or \
                        len(p.get('key_desc', [])) > 1:
                    txt3 = wx.TextCtrl(parent = which_panel, value = p.get('default',''))
                    
                    value = self._getValue(p)
                    if value:
                        # parameter previously set
                        txt3.SetValue(str(value))
                    
                    txt3.Bind(wx.EVT_TEXT, self.OnSetValue)
                    style = wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT
                elif p.get('type', '') == 'integer':
                    minValue = -1e9
                    maxValue = 1e9
                    value = self._getValue(p)
                    
                    txt3 = wx.SpinCtrl(parent = which_panel, value = p.get('default', ''),
                                       size = globalvar.DIALOG_SPIN_SIZE,
                                       min = minValue, max = maxValue)
                    if value:
                        txt3.SetValue(int(value)) # parameter previously set
                        txt3.Bind(wx.EVT_SPINCTRL, self.OnSetValue)

                    style = wx.BOTTOM | wx.LEFT | wx.RIGHT
                else: # float
                    txt3 = wx.TextCtrl(parent = which_panel, value = p.get('default',''),
                                       validator = FloatValidator())
                    style = wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT
                    
                    value = self._getValue(p)
                    if value:
                        txt3.SetValue(str(value)) # parameter previously set
                
                txt3.Bind(wx.EVT_TEXT, self.OnSetValue)
                
                which_sizer.Add(item = txt3, proportion = 0,
                                flag = style, border = 5)
                p['wxId'] = [ txt3.GetId(), ]

            #
            # element selection tree combobox (maps, icons, regions, etc.)
            #
            if p.get('gisprompt', False):
                title_txt.SetLabel(title + ':')
                # GIS element entry
                if p.get('prompt','') not in ('color',
                                              'subgroup',
                                              'sigfile',
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
                                              'dir'):
                    multiple = p.get('multiple', False)
                    if p.get('age', '') == 'new':
                        mapsets = [grass.gisenv()['MAPSET'],]
                    else:
                        mapsets = None
                    if self.task.name in ('r.proj', 'v.proj') \
                            and p.get('name', '') == 'input':
                        if self.task.name == 'r.proj':
                            isRaster = True
                        else:
                            isRaster = False
                        selection = gselect.ProjSelect(parent = which_panel,
                                                       isRaster = isRaster)
                        p['wxId'] = [ selection.GetId(), ]
                        selection.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                        selection.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                    else:
                        elem = p.get('element', None)
                        # hack for t.* modules
                        if elem in ('stds', 'map'):
                            orig_elem = elem
                            type_param = self.task.get_param('type', element = 'name', raiseError = False)
                            if type_param:
                                elem = type_param.get('default', None)
                                # for t.(un)register:
                                maps_param = self.task.get_param('maps', element = 'name', raiseError = False)
                                if maps_param and orig_elem == 'stds':
                                    element_dict = {'rast': 'strds', 'vect': 'stvds', 'rast3d': 'str3ds'}
                                    elem = element_dict[type_param.get('default')]
                        
                        if self.parent.modeler:
                            extraItems = {_('Graphical Modeler') : self.parent.modeler.GetModel().GetMaps(p.get('prompt'))}
                        else:
                            extraItems = None
                        selection = gselect.Select(parent = which_panel, id = wx.ID_ANY,
                                                   size = globalvar.DIALOG_GSELECT_SIZE,
                                                   type = elem, multiple = multiple, nmaps = len(p.get('key_desc', [])),
                                                   mapsets = mapsets, fullyQualified = p.get('age', 'old') == 'old',
                                                   extraItems = extraItems)
                        
                        value = self._getValue(p)
                        if value:
                            selection.SetValue(value)
                        
                        formatSelector = True
                        # A gselect.Select is a combobox with two children: a textctl and a popupwindow;
                        # we target the textctl here
                        textWin = selection.GetTextCtrl()
                        p['wxId'] = [ textWin.GetId(), ]
                        textWin.Bind(wx.EVT_TEXT, self.OnSetValue)
                    
                    if prompt == 'vector':
                        selection.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                        
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
                        which_sizer.Add(item = selection, proportion = 0,
                                        flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                        border = 5)
                    elif prompt == 'group':
                        selection.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                        which_sizer.Add(item = selection, proportion = 0,
                                        flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                        border = 5)
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
                            iconTheme = UserSettings.Get(group='appearance', key='iconTheme', subkey='type')
                            bitmap = wx.Bitmap(os.path.join(globalvar.ETCICONDIR, iconTheme, 'map-info.png'))
                            bb = wx.BitmapButton(parent=which_panel, bitmap=bitmap)
                            bb.Bind(wx.EVT_BUTTON, self.OnTimelineTool)
                            bb.SetToolTipString(_("Show graphical representation of temporal extent of dataset(s) ."))
                            p['wxId'].append(bb.GetId())

                            hSizer = wx.BoxSizer(wx.HORIZONTAL)
                            hSizer.Add(item=selection, proportion=0,
                                       flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                       border=5)
                            hSizer.Add(item=bb, proportion=0,
                                       flag=wx.EXPAND|wx.BOTTOM | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                       border=5)
                            which_sizer.Add(hSizer)
                        else:
                            which_sizer.Add(item=selection, proportion=0,
                                            flag=wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                            border=5)

                # subgroup
                elif prompt == 'subgroup':
                    selection = gselect.SubGroupSelect(parent = which_panel)
                    p['wxId'] = [ selection.GetId() ]
                    selection.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                    selection.Bind(wx.EVT_TEXT, self.OnSetValue)
                    which_sizer.Add(item = selection, proportion = 0,
                                    flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                    border = 5)

                # sigrature file
                elif prompt == 'sigfile':
                    selection = gselect.SignatureSelect(parent = which_panel, element = p.get('element', 'sig'))
                    p['wxId'] = [ selection.GetId() ]
                    selection.Bind(wx.EVT_TEXT, self.OnSetValue)
                    which_sizer.Add(item = selection, proportion = 0,
                                    flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_CENTER_VERTICAL,
                                    border = 5)
                
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
                        win = wx.TextCtrl(parent = which_panel, value = p.get('default',''),
                                          size = globalvar.DIALOG_TEXTCTRL_SIZE)
                        win.Bind(wx.EVT_TEXT, self.OnSetValue)
                    else:
                        value = self._getValue(p)
                        
                        if prompt == 'layer':
                            if p.get('element', 'layer') == 'layer_all':
                                all = True
                            else:
                                all = False
                            if p.get('age', 'old') == 'old':
                                win = gselect.LayerSelect(parent = which_panel,
                                                          all = all,
                                                          default = p['default'])
                                win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                                win.Bind(wx.EVT_TEXT, self.OnSetValue)
                                win.SetValue(str(value))    # default or previously set value
                            else:
                                win = wx.SpinCtrl(parent = which_panel, id = wx.ID_ANY,
                                                  min = 1, max = 100, initial = int(p['default']))
                                win.Bind(wx.EVT_SPINCTRL, self.OnSetValue)
                                win.SetValue(int(value))    # default or previously set value

                            p['wxId'] = [ win.GetId() ]

                        elif prompt == 'dbdriver':
                            win = gselect.DriverSelect(parent = which_panel,
                                                       choices = p.get('values', []),
                                                       value = value)
                            win.Bind(wx.EVT_COMBOBOX, self.OnUpdateSelection)
                            win.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                        elif prompt == 'dbname':
                            win = gselect.DatabaseSelect(parent = which_panel,
                                                         value = value)
                            win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                            win.Bind(wx.EVT_TEXT, self.OnSetValue)
                        elif prompt == 'dbtable':
                            if p.get('age', 'old') == 'old':
                                win = gselect.TableSelect(parent = which_panel)
                                win.Bind(wx.EVT_COMBOBOX, self.OnUpdateSelection)
                                win.Bind(wx.EVT_COMBOBOX, self.OnSetValue)
                            else:
                                win = wx.TextCtrl(parent = which_panel, value = p.get('default',''),
                                                  size = globalvar.DIALOG_TEXTCTRL_SIZE)
                                win.Bind(wx.EVT_TEXT, self.OnSetValue)
                        elif prompt == 'dbcolumn':
                            win = gselect.ColumnSelect(parent = which_panel,
                                                       value = value,
                                                       param = p,
                                                       multiple =  p.get('multiple', False))
                        
                            # A gselect.ColumnSelect is a combobox with two children: a textctl and a popupwindow;
                            # we target the textctl here
                            textWin = win.GetTextCtrl()
                            p['wxId'] = [ textWin.GetId(), ]
                            
                            textWin.Bind(wx.EVT_TEXT, self.OnSetValue)
                            win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)

                        elif prompt == 'location':
                            win = gselect.LocationSelect(parent = which_panel,
                                                         value = value)
                            win.Bind(wx.EVT_COMBOBOX,     self.OnUpdateSelection)
                            win.Bind(wx.EVT_COMBOBOX,     self.OnSetValue)
                        
                        elif prompt == 'mapset':
                            if p.get('age', 'old') == 'old':
                                new = False
                            else:
                                new = True
                            win = gselect.MapsetSelect(parent = which_panel,
                                                       value = value, new = new)
                            win.Bind(wx.EVT_COMBOBOX,     self.OnUpdateSelection)
                            win.Bind(wx.EVT_COMBOBOX,     self.OnSetValue) 
                            win.Bind(wx.EVT_TEXT,         self.OnSetValue)
                            
                        elif prompt == 'dbase':
                            win = gselect.DbaseSelect(parent = which_panel,
                                                      changeCallback = self.OnSetValue)
                            win.Bind(wx.EVT_TEXT, self.OnUpdateSelection)
                            p['wxId'] = [ win.GetChildren()[1].GetId() ]
                            
                    if 'wxId' not in p:
                        try:
                            p['wxId'] = [ win.GetId(), ]
                        except AttributeError:
                            pass
                    
                    flags = wx.BOTTOM | wx.LEFT | wx.RIGHT
                    if prompt == 'dbname':
                        flags |= wx.EXPAND
                    which_sizer.Add(item = win, proportion = 0,
                                    flag = flags, border = 5)
                # color entry
                elif prompt == 'color':
                    default_color = (200,200,200)
                    label_color = _("Select Color")
                    if p.get('default','') !=  '':
                        default_color, label_color = utils.color_resolve(p['default'])
                    if p.get('value','') !=  '' and p.get('value','') != 'none': # parameter previously set
                        default_color, label_color = utils.color_resolve(p['value'])
                    if p.get('element', '') == 'color_none' or p.get('multiple', False):
                        this_sizer = wx.BoxSizer(orient = wx.HORIZONTAL)
                    else:
                        this_sizer = which_sizer
                    colorSize = 150
                    # For color selectors, this is a three-member array, holding the IDs of
                    # the color picker,  the text control for multiple colors (or None),
                    # and either a "transparent" checkbox or None
                    p['wxId'] = [None] * 3
                    if p.get('multiple', False):
                        txt = wx.TextCtrl(parent = which_panel, id = wx.ID_ANY)
                        this_sizer.Add(item = txt, proportion = 1,
                                       flag = wx.ADJUST_MINSIZE | wx.LEFT | wx.TOP, border = 5)
                        txt.Bind(wx.EVT_TEXT, self.OnSetValue)
                        colorSize = 40
                        label_color = ''
                        p['wxId'][1] = txt.GetId()
                        which_sizer.Add(this_sizer, flag = wx.EXPAND | wx.RIGHT, border = 5)

                    btn_colour = csel.ColourSelect(parent = which_panel, id = wx.ID_ANY,
                                                   label = label_color, colour = default_color,
                                                   pos = wx.DefaultPosition, size = (colorSize,-1))
                    this_sizer.Add(item = btn_colour, proportion = 0,
                                   flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT, border = 5)
                    btn_colour.Bind(csel.EVT_COLOURSELECT,  self.OnColorChange)
                    p['wxId'][0] = btn_colour.GetId()

                    if p.get('element', '') == 'color_none':
                        none_check = wx.CheckBox(which_panel, wx.ID_ANY, _("Transparent"))
                        if p.get('value','')  == "none":
                            none_check.SetValue(True)
                        else:
                            none_check.SetValue(False)
                        this_sizer.Add(item = none_check, proportion = 0,
                                       flag = wx.ADJUST_MINSIZE | wx.LEFT | wx.RIGHT | wx.TOP, border = 5)
                        which_sizer.Add(this_sizer)
                        none_check.Bind(wx.EVT_CHECKBOX, self.OnColorChange)
                        p['wxId'][2] = none_check.GetId()


                # file selector
                elif p.get('prompt','') !=  'color' and p.get('prompt', '') == 'file':
                    if p.get('age', 'new') == 'new':
                        fmode = wx.SAVE
                    else:
                        fmode = wx.OPEN
                    # check wildcard
                    try:
                        fExt = os.path.splitext(p.get('key_desc', ['*.*'])[0])[1]
                    except:
                        fExt = None
                    if not fExt:
                        fMask = '*.*'
                    else:
                        fMask = '%s files (*%s)|*%s|Files (*.*)|*.*' % (fExt[1:].upper(), fExt, fExt)
                    fbb = filebrowse.FileBrowseButton(parent = which_panel, id = wx.ID_ANY, fileMask = fMask,
                                                      size = globalvar.DIALOG_GSELECT_SIZE, labelText = '',
                                                      dialogTitle = _('Choose %s') % \
                                                          p.get('description', _('file')).lower(),
                                                      buttonText = _('Browse'),
                                                      startDirectory = os.getcwd(), fileMode = fmode,
                                                      changeCallback = self.OnSetValue)
                    value = self._getValue(p)
                    if value:
                        fbb.SetValue(value) # parameter previously set
                    which_sizer.Add(item = fbb, proportion = 0,
                                    flag = wx.EXPAND | wx.RIGHT, border = 5)
                    
                    # A file browse button is a combobox with two children:
                    # a textctl and a button;
                    # we have to target the button here
                    p['wxId'] = [ fbb.GetChildren()[1].GetId() ]
                    if p.get('age', 'new') == 'old' and \
                            p.get('prompt', '') == 'file' and p.get('element', '') == 'file' and \
                            UserSettings.Get(group = 'cmd', key = 'interactiveInput', subkey = 'enabled'):
                        # widget for interactive input
                        ifbb = wx.TextCtrl(parent = which_panel, id = wx.ID_ANY,
                                           style = wx.TE_MULTILINE,
                                           size = (-1, 75))
                        if p.get('value', '') and os.path.isfile(p['value']):
                            f = open(p['value'])
                            ifbb.SetValue(''.join(f.readlines()))
                            f.close()
                        
                        ifbb.Bind(wx.EVT_TEXT, self.OnFileText)
                        
                        btnLoad = wx.Button(parent = which_panel, id = wx.ID_ANY, label = _("&Load"))
                        btnLoad.Bind(wx.EVT_BUTTON, self.OnFileLoad)
                        btnSave = wx.Button(parent = which_panel, id = wx.ID_SAVEAS)
                        btnSave.Bind(wx.EVT_BUTTON, self.OnFileSave)
                        
                        which_sizer.Add(item = wx.StaticText(parent = which_panel, id = wx.ID_ANY,
                                                             label = _('or enter values interactively')),
                                        proportion = 0,
                                        flag = wx.EXPAND | wx.RIGHT | wx.LEFT | wx.BOTTOM, border = 5)
                        which_sizer.Add(item = ifbb, proportion = 1,
                                        flag = wx.EXPAND | wx.RIGHT | wx.LEFT, border = 5)
                        btnSizer = wx.BoxSizer(wx.HORIZONTAL)
                        btnSizer.Add(item = btnLoad, proportion = 0,
                                     flag = wx.ALIGN_RIGHT | wx.RIGHT, border = 10)
                        btnSizer.Add(item = btnSave, proportion = 0,
                                     flag = wx.ALIGN_RIGHT)
                        which_sizer.Add(item = btnSizer, proportion = 0,
                                        flag = wx.ALIGN_RIGHT | wx.RIGHT | wx.TOP, border = 5)
                        
                        p['wxId'].append(ifbb.GetId())
                        p['wxId'].append(btnLoad.GetId())
                        p['wxId'].append(btnSave.GetId())
                
                # directory selector
                elif p.get('prompt','') != 'color' and p.get('prompt', '') == 'dir':
                    fbb = filebrowse.DirBrowseButton(parent = which_panel, id = wx.ID_ANY,
                                                     size = globalvar.DIALOG_GSELECT_SIZE, labelText = '',
                                                     dialogTitle = _('Choose %s') % \
                                                         p.get('description', _('Directory')),
                                                     buttonText = _('Browse'),
                                                     startDirectory = os.getcwd(),
                                                     changeCallback = self.OnSetValue)
                    value = self._getValue(p)
                    if value:
                        fbb.SetValue(value) # parameter previously set
                    which_sizer.Add(item = fbb, proportion = 0,
                                    flag = wx.EXPAND | wx.RIGHT, border = 5)
                    
                    # A file browse button is a combobox with two children:
                    # a textctl and a button;
                    # we have to target the button here
                    p['wxId'] = [ fbb.GetChildren()[1].GetId() ]
                    
                # interactive inserting of coordinates from map window
                elif prompt == 'coords':
                    # interactive inserting if layer manager is accessible
                    if self._giface:
                        win = gselect.CoordinatesSelect(parent = which_panel,
                                                        giface = self._giface,
                                                        multiple =  p.get('multiple', False),
                                                        param = p)
                        p['wxId'] = [win.GetTextWin().GetId()]
                        win.GetTextWin().Bind(wx.EVT_TEXT, self.OnSetValue)
                        # bind closing event because destructor is not working properly
                        if hasattr(self.parent, 'dialogClosing'):
                            self.parent.dialogClosing.connect(win.OnClose)
                    
                    # normal text field
                    else:
                        win = wx.TextCtrl(parent = which_panel)
                        p['wxId'] = [win.GetId()]
                        win.Bind(wx.EVT_TEXT, self.OnSetValue)
                    
                    which_sizer.Add(item = win, 
                                    proportion = 0,
                                    flag = wx.EXPAND | wx.BOTTOM | wx.LEFT | wx.RIGHT, 
                                    border = 5)
            
            if self.parent.GetName() == 'MainFrame' and self.parent.modeler:
                parChk = wx.CheckBox(parent = which_panel, id = wx.ID_ANY,
                                     label = _("Parameterized in model"))
                parChk.SetName('ModelParam')
                parChk.SetValue(p.get('parameterized', False))
                if 'wxId' in p:
                    p['wxId'].append(parChk.GetId())
                else:
                    p['wxId'] = [ parChk.GetId() ]
                parChk.Bind(wx.EVT_CHECKBOX, self.OnSetValue)
                which_sizer.Add(item = parChk, proportion = 0,
                                flag = wx.LEFT, border = 20)
                
            if title_txt is not None:
                # create tooltip if given
                if len(p['values_desc']) > 0:
                    if tooltip:
                        tooltip +=  2 * os.linesep
                    else:
                        tooltip = ''
                    if len(p['values']) == len(p['values_desc']):
                        for i in range(len(p['values'])):
                            tooltip +=  p['values'][i] + ': ' + p['values_desc'][i] + os.linesep
                    tooltip.strip(os.linesep)
                if tooltip:
                    title_txt.SetToolTipString(tooltip)

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
        pSigFile = None
        pDbase = None
        pLocation = None
        pMapset = None
        for p in self.task.params:
            guidep = p.get('guidependency', '')
            
            if guidep:
                # fixed options dependency defined
                options = guidep.split(',')
                for opt in options:
                    pOpt = self.task.get_param(opt, element = 'name', raiseError = False)
                    if id:
                        if 'wxId-bind' not in p:
                            p['wxId-bind'] = list()
                        p['wxId-bind'] +=  pOpt['wxId']
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
                pSigFile = p
            elif prompt == 'dbase':
                pDbase = p
            elif prompt == 'location':
                pLocation = p
            elif prompt == 'mapset':
                pMapset = p
        
        # collect ids
        pColumnIds = []
        for p in pColumn:
            pColumnIds +=  p['wxId']
        pLayerIds = []
        for p in pLayer:
            pLayerIds +=  p['wxId']
        
        # set wxId-bindings
        if pMap:
            pMap['wxId-bind'] = []
            if pLayer:
                pMap['wxId-bind'] +=  pLayerIds
            pMap['wxId-bind'] += copy.copy(pColumnIds)
        if pLayer:
            for p in pLayer:
                p['wxId-bind'] = copy.copy(pColumnIds)
        
        if pDriver and pTable:
            pDriver['wxId-bind'] = pTable['wxId']

        if pDatabase and pTable:
            pDatabase['wxId-bind'] = pTable['wxId']

        if pTable and pColumnIds:
            pTable['wxId-bind'] = pColumnIds
        
        if pGroup and pSubGroup:
            if pSigFile:
                pGroup['wxId-bind'] = copy.copy(pSigFile['wxId']) + pSubGroup['wxId']
                pSubGroup['wxId-bind'] = pSigFile['wxId']
            else:
                pGroup['wxId-bind'] = pSubGroup['wxId']
        
        if pDbase and pLocation:
            pDbase['wxId-bind'] = pLocation['wxId']

        if pLocation and pMapset:
            pLocation['wxId-bind'] = pMapset['wxId']
        
        if pLocation and pMapset and pMap:
            pLocation['wxId-bind'] +=  pMap['wxId']
            pMapset['wxId-bind'] = pMap['wxId']
        #
        # determine panel size
        #
        maxsizes = (0, 0)
        for section in sections:
            tab[section].SetSizer(tabsizer[section])
            tabsizer[section].Fit(tab[section])
            tab[section].Layout()
            minsecsizes = tabsizer[section].GetSize()
            maxsizes = map(lambda x: max(maxsizes[x], minsecsizes[x]), (0, 1))

        # TODO: be less arbitrary with these 600
        self.panelMinHeight = 100
        self.constrained_size = (min(600, maxsizes[0]) + 25, min(400, maxsizes[1]) + 25)
        for section in sections:
            tab[section].SetMinSize((self.constrained_size[0], self.panelMinHeight))
        
        
        # add pages to notebook
        imageList = wx.ImageList(16, 16)
        self.notebook.AssignImageList(imageList)

        for section in sections:
            self.notebook.AddPage(page = tab[section], text = section, name = section)
            index = self.AddBitmapToImageList(section, imageList)
            if index >= 0:
                self.notebook.SetPageImage(section, index)

        # are we running from command line?
        ### add 'command output' tab regardless standalone dialog
        if self.parent.GetName() == "MainFrame" and self.parent.get_dcmd is None:
            from core.gconsole import GConsole, EVT_CMD_RUN, EVT_CMD_DONE
            from gui_core.goutput import GConsoleWindow
            self._gconsole = GConsole(guiparent = self.notebook)
            self.goutput = GConsoleWindow(parent = self.notebook, gconsole = self._gconsole, margin = False)
            self._gconsole.Bind(EVT_CMD_RUN,
                                lambda event:
                                    self._switchPageHandler(event = event, priority = 2))
            self._gconsole.Bind(EVT_CMD_DONE,
                                lambda event:
                                    self._switchPageHandler(event = event, priority = 3))
            self.outpage = self.notebook.AddPage(page = self.goutput, text = _("Command output"), name = 'output')
        else:
            self.goutput = None
            self._gconsole = None

        self.manualTab = HelpPanel(parent = self.notebook, command = self.task.name)
        if not self.manualTab.GetFile():
            self.manualTab.Hide()
        else:
            self.notebook.AddPage(page = self.manualTab, text = _("Manual"), name = 'manual')
            index = self.AddBitmapToImageList(section = 'manual', imageList = imageList)
            if index >= 0:
                self.notebook.SetPageImage('manual', index)
        
        if self.manualTab.IsLoaded():
            self.manualTab.SetMinSize((self.constrained_size[0], self.panelMinHeight))

        self.notebook.SetSelection(0)

        panelsizer.Add(item = self.notebook, proportion = 1, flag = wx.EXPAND)
        self.SetSizer(panelsizer)
        panelsizer.Fit(self.notebook)
        
        self.Bind(EVT_DIALOG_UPDATE, self.OnUpdateDialog)

    def _getValue(self, p):
        """!Get value or default value of given parameter

        @param p parameter directory
        """
        if p.get('value', '') !=  '':
            return p['value']
        return p.get('default', '')
        
    def OnFileLoad(self, event):
        """!Load file to interactive input"""
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
            gcmd.GMessage(parent = self,
                          message = _("Nothing to load."))
            return
        
        data = ''
        f = open(path, "r")
        try:
            data = f.read()
        finally:
            f.close()
        
        win['text'].SetValue(data)
        
    def OnFileSave(self, event):
        """!Save interactive input to the file"""
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
            gcmd.GMessage(parent = self,
                          message = _("Nothing to save."))
            return
        
        dlg = wx.FileDialog(parent = self,
                            message = _("Save input as..."),
                            defaultDir = os.getcwd(),
                            style = wx.SAVE | wx.FD_OVERWRITE_PROMPT)

        if dlg.ShowModal() == wx.ID_OK:
            path = dlg.GetPath()
            enc = locale.getdefaultlocale()[1]
            f = codecs.open(path, encoding = enc, mode = 'w', errors='replace')
            try:
                f.write(text + os.linesep)
            finally:
                f.close()
            
            win['file'].SetValue(path)
        
        dlg.Destroy()
        
    def OnFileText(self, event):
        """File input interactively entered"""
        text = event.GetString()
        p = self.task.get_param(value = event.GetId(), element = 'wxId', raiseError = False)
        if not p:
            return # should not happen
        win = self.FindWindowById(p['wxId'][0])
        if text:
            filename = win.GetValue()
            if not filename:
                filename = grass.tempfile()
                win.SetValue(filename)
                
            enc = locale.getdefaultlocale()[1]
            f = codecs.open(filename, encoding = enc, mode = 'w', errors='replace')
            try:
                f.write(text)
                if text[-1] != os.linesep:
                    f.write(os.linesep)
            finally:
                f.close()
        else:
            win.SetValue('')
        
    def OnVectorFormat(self, event):
        """!Change vector format (native / ogr).

        Currently unused.        
        """
        sel = event.GetSelection()
        idEvent = event.GetId()
        p = self.task.get_param(value = idEvent, element = 'wxId', raiseError = False)
        if not p:
            return # should not happen
        
        # detect windows
        winNative = None
        winOgr = None
        for id in p['wxId']:
            if id == idEvent:
                continue
            name = self.FindWindowById(id).GetName()
            if name == 'Select':
                winNative = self.FindWindowById(id + 1)  # fix the mystery (also in nviz_tools.py)
            elif name == 'OgrSelect':
                winOgr = self.FindWindowById(id)
        
        # enable / disable widgets & update values
        rbox = self.FindWindowByName('VectorFormat')
        self.hsizer.Remove(rbox)
        if sel == 0:   # -> native
            winOgr.Hide()
            self.hsizer.Remove(winOgr)
            
            self.hsizer.Add(item = winNative,
                            flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_TOP,
                            border = 5)
            winNative.Show()
            p['value'] = winNative.GetValue()
        
        elif sel == 1: # -> OGR
            sizer = wx.BoxSizer(wx.VERTICAL)
            
            winNative.Hide()
            self.hsizer.Remove(winNative)

            sizer.Add(item = winOgr)
            winOgr.Show()
            p['value'] = winOgr.GetDsn()
            
            self.hsizer.Add(item = sizer,
                            flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT | wx.RIGHT | wx.TOP | wx.ALIGN_TOP,
                            border = 5)
        
        self.hsizer.Add(item = rbox,
                        flag = wx.ADJUST_MINSIZE | wx.BOTTOM | wx.LEFT |
                        wx.RIGHT | wx.ALIGN_TOP,
                        border = 5)
        
        self.hsizer.Layout()
        self.Layout()
        self.OnUpdateValues()
        self.OnUpdateSelection(event)
        
    def OnUpdateDialog(self, event):
        for fn, kwargs in event.data.iteritems():
            fn(**kwargs)
        
        self.parent.updateValuesHook()
        
    def OnVerbosity(self, event):
        """!Verbosity level changed"""
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
                wx.Yield()
                self.manualTab.LoadPage()

        self.Layout()

        if event:
            # skip is needed for wx.Notebook on Windows
            event.Skip()
            # this is needed for dialogs launched from layer manager
            # event is somehow propagated?
            event.StopPropagation()

    def _switchPageHandler(self, event, priority):
        self._switchPage(priority = priority)
        event.Skip()

    def _switchPage(self, priority):
        """!Manages @c 'output' notebook page according to event priority."""
        if priority == 1:
            self.notebook.HighlightPageByName('output')
        if priority >= 2:
            self.notebook.SetSelectionByName('output')
        if priority >= 3:
            self.SetFocus()
            self.Raise()

    def OnColorChange(self, event):
        myId = event.GetId()
        for p in self.task.params:
            if 'wxId' in p and myId in p['wxId']:
                multiple = p['wxId'][1] is not None # multiple colors
                hasTansp = p['wxId'][2] is not None
                if multiple:
                    # selected color is added at the end of textCtrl
                    colorchooser = wx.FindWindowById(p['wxId'][0])
                    new_color = colorchooser.GetValue()[:]
                    new_label = utils.rgb2str.get(new_color, ':'.join(map(str, new_color)))
                    textCtrl = wx.FindWindowById(p['wxId'][1])
                    val = textCtrl.GetValue()
                    sep = ','
                    if val and val[-1] != sep:
                        val += sep
                    val += new_label
                    textCtrl.SetValue(val)
                    p[ 'value' ] = val
                elif hasTansp and wx.FindWindowById(p['wxId'][2]).GetValue():
                    p[ 'value' ] = 'none'
                else:
                    colorchooser = wx.FindWindowById(p['wxId'][0])
                    new_color = colorchooser.GetValue()[:]
                    # This is weird: new_color is a 4-tuple and new_color[:] is a 3-tuple
                    # under wx2.8.1
                    new_label = utils.rgb2str.get(new_color, ':'.join(map(str,new_color)))
                    colorchooser.SetLabel(new_label)
                    colorchooser.SetColour(new_color)
                    colorchooser.Refresh()
                    p[ 'value' ] = colorchooser.GetLabel()
        self.OnUpdateValues()

    def OnUpdateValues(self, event = None):
        """!If we were part of a richer interface, report back the
        current command being built.

        This method should be set by the parent of this panel if
        needed. It's a hook, actually.  Beware of what is 'self' in
        the method def, though. It will be called with no arguments.
        """
        pass

    def OnCheckBoxMulti(self, event):
        """!Fill the values as a ','-separated string according to
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

        if event.Checked():
            currentValues[ theValue ] = 1
        else:
            del currentValues[ theValue ]

        # Keep the original order, so that some defaults may be recovered
        currentValueList = [] 
        for v in theParam['values']:
            if v in currentValues:
                currentValueList.append(v)

        # Pack it back
        theParam['value'] = ','.join(currentValueList)

        self.OnUpdateValues()

    def OnSetValue(self, event):
        """!Retrieve the widget value and set the task value field
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
        else:
            if isinstance(me, wx.SpinCtrl):
                porf['value'] = str(me.GetValue())
            else:
                porf['value'] = me.GetValue()
        
        self.OnUpdateValues(event)
        
        event.Skip()
        
    def OnSetSymbol(self, event):
        """!Shows dialog for symbol selection"""
        myId = event.GetId()
        
        for p in self.task.params:
            if 'wxId' in p and myId in p['wxId']:
                from gui_core.dialogs import SymbolDialog
                dlg = SymbolDialog(self, symbolPath = globalvar.ETCSYMBOLDIR,
                                   currentSymbol = p['value'])
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
        """!Show Timeline Tool with dataset(s) from gselect.

        TODO: update from gselect automatically        
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

    def OnUpdateSelection(self, event):
        """!Update dialog (layers, tables, columns, etc.)
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
            
    def createCmd(self, ignoreErrors = False, ignoreRequired = False):
        """!Produce a command line string (list) or feeding into GRASS.

        @param ignoreErrors True then it will return whatever has been
        built so far, even though it would not be a correct command
        for GRASS
        """
        try:
            cmd = self.task.get_cmd(ignoreErrors = ignoreErrors,
                                   ignoreRequired = ignoreRequired)
        except ValueError, err:
            dlg = wx.MessageDialog(parent = self,
                                   message = unicode(err),
                                   caption = _("Error in %s") % self.task.name,
                                   style = wx.OK | wx.ICON_ERROR | wx.CENTRE)
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
        iconTheme = UserSettings.Get(group = 'appearance', key = 'iconTheme', subkey = 'type')
        iconSectionDict = {'manual': os.path.join(globalvar.ETCICONDIR, iconTheme, 'help.png')}
        if section in iconSectionDict.keys():
            image = wx.Image(iconSectionDict[section]).Scale(16, 16, wx.IMAGE_QUALITY_HIGH)
            idx = imageList.Add(wx.BitmapFromImage(image))
            return idx

        return -1


class GUI:
    def __init__(self, parent = None, giface = None, show = True, modal = False,
                 centreOnParent = False, checkError = False):
        """!Parses GRASS commands when module is imported and used from
        Layer Manager.
        """
        self.parent = parent
        self.show   = show
        self.modal  = modal
        self._giface = giface
        self.centreOnParent = centreOnParent
        self.checkError     = checkError
        
        self.grass_task = None
        self.cmd = list()
        
        global _blackList
        if self.parent:
            _blackList['enabled'] = True
        else:
            _blackList['enabled'] = False
        
    def GetCmd(self):
        """!Get validated command"""
        return self.cmd
    
    def ParseCommand(self, cmd, completed = None):
        """!Parse command
        
        Note: cmd is given as list
        
        If command is given with options, return validated cmd list:
         - add key name for first parameter if not given
         - change mapname to mapname@mapset
        """
        dcmd_params = {}
        if completed == None:
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
            self.grass_task = gtask.parse_interface(gcmd.GetRealCmd(cmd[0]),
                                                    blackList = _blackList)
        except (grass.ScriptError, ValueError), e:
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
                if option[0] == '-': # flag
                    if option[1] == '-':
                        self.grass_task.set_flag(option[2:], True)
                    else:
                        self.grass_task.set_flag(option[1], True)
                    cmd_validated.append(option)
                else: # parameter
                    try:
                        key, value = option.split('=', 1)
                    except ValueError:
                        if self.grass_task.firstParam:
                            if i == 0: # add key name of first parameter if not given
                                key = self.grass_task.firstParam
                                value = option
                            else:
                                raise gcmd.GException, _("Unable to parse command '%s'") % ' '.join(cmd)
                        else:
                            continue
                    
                    element = self.grass_task.get_param(key, raiseError = False)
                    if not element:
                        err.append(_("%(cmd)s: parameter '%(key)s' not available") % \
                                       { 'cmd' : cmd[0],
                                         'key' : key })
                        continue
                    element = element['element']
                    
                    if element in ['cell', 'vector']:
                        # mapname -> mapname@mapset
                        try:
                            name, mapset = value.split('@')
                        except ValueError:
                            mapset = grass.find_file(value, element)['mapset']
                            curr_mapset = grass.gisenv()['MAPSET']
                            if mapset and mapset !=  curr_mapset:
                                value = value + '@' + mapset
                    
                    self.grass_task.set_param(key, value)
                    cmd_validated.append(key + '=' + value)
                    i += 1
            
            # update original command list
            cmd = cmd_validated
        
        if self.show is not None:
            self.mf = TaskFrame(parent = self.parent, giface = self._giface,
                                task_description = self.grass_task,
                                get_dcmd = get_dcmd, layer = layer)
        else:
            self.mf = None
        
        if get_dcmd is not None:
            # update only propwin reference
            get_dcmd(dcmd = None, layer = layer, params = None,
                     propwin = self.mf)
        
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
        """!Get parameter key for input raster/vector map
        
        @param cmd module name
        
        @return parameter key
        @return None on failure
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
                            prompt in ('raster', '3d-raster', 'vector'):
                        return p.get('name', None)
        return None

class GrassGUIApp(wx.App):
    """!Stand-alone GRASS command GUI
    """
    def __init__(self, grass_task):
        self.grass_task = grass_task
        wx.App.__init__(self, False)
        
    def OnInit(self):
        msg = self.grass_task.get_error_msg()
        if msg:
            gcmd.GError(msg + '\n\n' +
                        _('Try to set up GRASS_ADDON_PATH or GRASS_ADDON_BASE variable.'))
            return True
        
        self.mf = TaskFrame(parent = None, giface = None, task_description = self.grass_task)
        self.mf.CentreOnScreen()
        self.mf.Show(True)
        self.SetTopWindow(self.mf)
        
        return True

if __name__ == "__main__":
    
    if len(sys.argv) == 1:
        sys.exit(_("usage: %s <grass command>") % sys.argv[0])
    
    if sys.argv[1] !=  'test':
        q = wx.LogNull()
        cmd = utils.split(sys.argv[1])
        task = gtask.grassTask(gcmd.GetRealCmd(cmd[0]))
        task.set_options(cmd[1:])
        app = GrassGUIApp(task)
        app.MainLoop()
    else: #Test
        # Test grassTask from within a GRASS session
        if os.getenv("GISBASE") is not None:
            task = gtask.grassTask("d.vect")
            task.get_param('map')['value'] = "map_name"
            task.get_flag('v')['value'] = True
            task.get_param('layer')['value'] = 1
            task.get_param('bcolor')['value'] = "red"
            assert ' '.join(task.get_cmd()) == "d.vect -v map = map_name layer = 1 bcolor = red"
        # Test interface building with handmade grassTask,
        # possibly outside of a GRASS session.
        task = gtask.grassTask()
        task.name = "TestTask"
        task.description = "This is an artificial grassTask() object intended for testing purposes."
        task.keywords = ["grass","test","task"]
        task.params = [
            {
            "name" : "text",
            "description" : "Descriptions go into tooltips if labels are present, like this one",
            "label" : "Enter some text",
            },{
            "name" : "hidden_text",
            "description" : "This text should not appear in the form",
            "hidden" : True
            },{
            "name" : "text_default",
            "description" : "Enter text to override the default",
            "default" : "default text"
            },{
            "name" : "text_prefilled",
            "description" : "You should see a friendly welcome message here",
            "value" : "hello, world"
            },{
            "name" : "plain_color",
            "description" : "This is a plain color, and it is a compulsory parameter",
            "required" : False,
            "gisprompt" : True,
            "prompt" : "color"
            },{
            "name" : "transparent_color",
            "description" : "This color becomes transparent when set to none",
            "guisection" : "tab",
            "gisprompt" : True,
            "prompt" : "color"
            },{
            "name" : "multi",
            "description" : "A multiple selection",
            'default': u'red,green,blue',
            'gisprompt': False,
            'guisection': 'tab',
            'multiple': u'yes',
            'type': u'string',
            'value': '',
            'values': ['red', 'green', u'yellow', u'blue', u'purple', u'other']
            },{
            "name" : "single",
            "description" : "A single multiple-choice selection",
            'values': ['red', 'green', u'yellow', u'blue', u'purple', u'other'],
            "guisection" : "tab"
            },{
            "name" : "large_multi",
            "description" : "A large multiple selection",
            "gisprompt" : False,
            "multiple" : "yes",
            # values must be an array of strings
            "values" : utils.str2rgb.keys() + map(str, utils.str2rgb.values())
            },{
            "name" : "a_file",
            "description" : "A file selector",
            "gisprompt" : True,
            "element" : "file"
            }
            ]
        task.flags = [
            {
            "name" : "a",
            "description" : "Some flag, will appear in Main since it is required",
            "required" : True
            },{
            "name" : "b",
            "description" : "pre-filled flag, will appear in options since it is not required",
            "value" : True
            },{
            "name" : "hidden_flag",
            "description" : "hidden flag, should not be changeable",
            "hidden" : "yes",
            "value" : True
            }
            ]
        q = wx.LogNull()
        GrassGUIApp(task).MainLoop()

