"""!
@package psmap.py

@brief GUI for ps.map

Classes:
 - PsMapFrame
 - PsMapBufferedWindow

(C) 2011 by Anna Kratochvilova, and the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com> (bachelor's project)
@author Martin Landa <landa.martin gmail.com> (mentor)
"""

import os
import sys
import textwrap
import Queue
try:
    import Image
    haveImage = True
except ImportError:
    haveImage = False
from math import sin, cos, pi

import grass.script as grass
if int(grass.version()['version'].split('.')[0]) > 6:
    sys.path.append(os.path.join(os.getenv('GISBASE'), 'etc', 'gui', 'wxpython',
                                 'gui_modules'))
else:
    sys.path.append(os.path.join(os.getenv('GISBASE'), 'etc', 'wxpython',
                                 'gui_modules'))
import globalvar
import menu
from   goutput    import CmdThread, EVT_CMD_DONE
from   menudata   import PsMapData
from   toolbars   import PsMapToolbar
from   icon       import Icons, MetaIcon, iconSet
from   gcmd       import RunCommand, GError, GMessage
from   menuform   import GUI
from psmap_dialogs import *

import wx

try:
    import wx.lib.agw.flatnotebook as fnb
except ImportError:
    import wx.lib.flatnotebook as fnb
    
class PsMapFrame(wx.Frame):
    def __init__(self, parent = None, id = wx.ID_ANY,
                 title = _("GRASS GIS Hardcopy Map Output Utility"), **kwargs):
        """!Main window of ps.map GUI
        
        @param parent parent window
        @param id window id
        @param title window title
        
        @param kwargs wx.Frames' arguments
        """
        self.parent = parent

        wx.Frame.__init__(self, parent = parent, id = id, title = title, name = "PsMap", **kwargs)
        self.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        #menubar
        self.menubar = menu.Menu(parent = self, data = PsMapData())
        self.SetMenuBar(self.menubar)
        #toolbar

        self.toolbar = PsMapToolbar(parent = self)
        self.SetToolBar(self.toolbar)
        
        self.actionOld = self.toolbar.action['id']
        self.iconsize = (16, 16)
        #satusbar
        self.statusbar = self.CreateStatusBar(number = 1)
        
        # mouse attributes -- position on the screen, begin and end of
        # dragging, and type of drawing
        self.mouse = {
            'begin': [0, 0], # screen coordinates
            'end'  : [0, 0],
            'use'  : "pointer",
            }
        # available cursors
        self.cursors = {
            "default" : wx.StockCursor(wx.CURSOR_ARROW),
            "cross"   : wx.StockCursor(wx.CURSOR_CROSS),
            "hand"    : wx.StockCursor(wx.CURSOR_HAND),
            "sizenwse": wx.StockCursor(wx.CURSOR_SIZENWSE)
            }
        # pen and brush
        self.pen = {
            'paper': wx.Pen(colour = "BLACK", width = 1),
            'margins': wx.Pen(colour = "GREY", width = 1),
            'map': wx.Pen(colour = wx.Color(86, 122, 17), width = 2),
            'rasterLegend': wx.Pen(colour = wx.Color(219, 216, 4), width = 2),
            'vectorLegend': wx.Pen(colour = wx.Color(219, 216, 4), width = 2),
            'mapinfo': wx.Pen(colour = wx.Color(5, 184, 249), width = 2),
            'scalebar': wx.Pen(colour = wx.Color(150, 150, 150), width = 2),
            'box': wx.Pen(colour = 'RED', width = 2, style = wx.SHORT_DASH),
            'select': wx.Pen(colour = 'BLACK', width = 1, style = wx.SHORT_DASH),
            'resize': wx.Pen(colour = 'BLACK', width = 1)
            }
        self.brush = {
            'paper': wx.WHITE_BRUSH,
            'margins': wx.TRANSPARENT_BRUSH,            
            'map': wx.Brush(wx.Color(151, 214, 90)),
            'rasterLegend': wx.Brush(wx.Color(250, 247, 112)),
            'vectorLegend': wx.Brush(wx.Color(250, 247, 112)),
            'mapinfo': wx.Brush(wx.Color(127, 222, 252)),
            'scalebar': wx.Brush(wx.Color(200, 200, 200)),
            'box': wx.TRANSPARENT_BRUSH,
            'select':wx.TRANSPARENT_BRUSH,
            'resize': wx.BLACK_BRUSH
            } 
        

        # list of objects to draw
        self.objectId = []
        
        # instructions
        self.instruction = Instruction(parent = self, objectsToDraw = self.objectId)
        # open dialogs
        self.openDialogs = dict()
        
        self.pageId = wx.NewId()
        #current page of flatnotebook
        self.currentPage = 0
        #canvas for draft mode
        self.canvas = PsMapBufferedWindow(parent = self, mouse = self.mouse, pen = self.pen,
                                          brush = self.brush, cursors = self.cursors, 
                                          instruction = self.instruction, openDialogs = self.openDialogs,
                                          pageId = self.pageId, objectId = self.objectId,
                                          preview = False)
        
        self.canvas.SetCursor(self.cursors["default"])
        self.getInitMap()
        
        
        # image path
        env = grass.gisenv()
        self.imgName = os.path.join(env['GISDBASE'], env['LOCATION_NAME'], env['MAPSET'], '.tmp', 'tmpImage.png')
        
        #canvas for preview
        self.previewCanvas = PsMapBufferedWindow(parent = self, mouse = self.mouse, cursors = self.cursors,
                                                 pen = self.pen, brush = self.brush, preview = True)
        
        # set WIND_OVERRIDE
        grass.use_temp_region()
        
        # create queues
        self.requestQ = Queue.Queue()
        self.resultQ = Queue.Queue()
        # thread
        self.cmdThread = CmdThread(self, self.requestQ, self.resultQ)
        
        self._layout()
        self.SetMinSize(wx.Size(750, 600))
        
        self.Bind(fnb.EVT_FLATNOTEBOOK_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(fnb.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnPageChanged)
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        self.Bind(EVT_CMD_DONE, self.OnCmdDone)
        
        if not haveImage:
            wx.CallAfter(self._showErrMsg)
        
    def _showErrMsg(self):
        """!Show error message (missing preview)
        """
        GError(parent = self,
               message = _("Python Imaging Library is not available.\n"
                           "'Preview' functionality won't work."),
               showTraceback = False)
        
    def _layout(self):
        """!Do layout
        """
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        if globalvar.hasAgw:
            self.book = fnb.FlatNotebook(parent = self, id = wx.ID_ANY,
                                         agwStyle = fnb.FNB_FANCY_TABS | fnb.FNB_BOTTOM |
                                         fnb.FNB_NO_NAV_BUTTONS | fnb.FNB_NO_X_BUTTON)
        else:
            self.book = fnb.FlatNotebook(parent = self, id = wx.ID_ANY,
                                         style = fnb.FNB_FANCY_TABS | fnb.FNB_BOTTOM |
                                         fnb.FNB_NO_NAV_BUTTONS | fnb.FNB_NO_X_BUTTON)
        #self.book = fnb.FlatNotebook(self, wx.ID_ANY, style = fnb.FNB_BOTTOM)
        self.book.AddPage(self.canvas, "Draft mode")
        self.book.AddPage(self.previewCanvas, "Preview")
        self.book.SetSelection(0)
        
        mainSizer.Add(self.book,1, wx.EXPAND)
        
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)

        
    def InstructionFile(self):
        """!Creates mapping instructions"""
        
        return str(self.instruction)

    def OnPSFile(self, event):
        """!Generate PostScript"""
        filename = self.getFile(wildcard = "PostScript (*.ps)|*.ps|Encapsulated PostScript (*.eps)|*.eps")
        if filename:
            self.PSFile(filename)
    
    def OnPsMapDialog(self, event):
        """!Launch ps.map dialog
        """
        GUI(parent = self).ParseCommand(cmd = ['ps.map'])

    def OnPDFFile(self, event):
        """!Generate PDF from PS with ps2pdf if available"""
        try:
            p = grass.Popen(["ps2pdf"], stderr = grass.PIPE)
            p.stderr.close()
        
        except OSError:
            GMessage(parent = self,
                     message = _("Program ps2pdf is not available. Please install it first to create PDF."))
            return
        
        filename = self.getFile(wildcard = "PDF (*.pdf)|*.pdf")
        if filename:  
            self.PSFile(filename, pdf = True)   
            
    def OnPreview(self, event):
        """!Run ps.map and show result"""
        self.PSFile()
        
    def PSFile(self, filename = None, pdf = False):
        """!Create temporary instructions file and run ps.map with output = filename"""
        instrFile = grass.tempfile()
        instrFileFd = open(instrFile, mode = 'w')
        instrFileFd.write(self.InstructionFile())
        instrFileFd.flush()
        instrFileFd.close()
        
        temp = False
        regOld = grass.region()
        
        if pdf:
            pdfname = filename
        else:
            pdfname = None
        #preview or pdf
        if not filename or (filename and pdf):
            temp = True
            filename = grass.tempfile()
            if not pdf: # lower resolution for preview
                if self.instruction.FindInstructionByType('map'):
                    mapId = self.instruction.FindInstructionByType('map').id
                    SetResolution(dpi = 100, width = self.instruction[mapId]['rect'][2],
                                  height = self.instruction[mapId]['rect'][3])
        
        cmd = ['ps.map', '--overwrite']
        if os.path.splitext(filename)[1] == '.eps':
            cmd.append('-e')
        if self.instruction[self.pageId]['Orientation'] == 'Landscape':
            cmd.append('-r')
        cmd.append('input=%s' % instrFile)
        cmd.append('output=%s' % filename)
        if pdf:
            self.SetStatusText(_('Generating PDF...'), 0)
        elif not temp:
            self.SetStatusText(_('Generating PostScript...'), 0)
        else:
            self.SetStatusText(_('Generating preview...'), 0)
            
        self.cmdThread.RunCmd(cmd, userData = {'instrFile' : instrFile, 'filename' : filename,
                                               'pdfname' : pdfname, 'temp' : temp, 'regionOld' : regOld})
        
    def OnCmdDone(self, event):
        """!ps.map process finished"""
        
        if event.returncode != 0:
            GMessage(parent = self,
                     message = _("Ps.map exited with return code %s") % event.returncode)
            
            grass.try_remove(event.userData['instrFile'])
            if event.userData['temp']:
                grass.try_remove(event.userData['filename']) 
            return
        
        if event.userData['pdfname']:
            try:
                proc = grass.Popen(['ps2pdf', '-dPDFSETTINGS=/prepress', '-r1200', 
                                    event.userData['filename'], event.userData['pdfname']])
                
                ret = proc.wait()                        
                if ret > 0:
                    GMessage(parent = self,
                             message = _("ps2pdf exited with return code %s") % ret)

            except OSError, e:
                GError(parent = self,
                       message = _("Program ps2pdf is not available. Please install it to create PDF.\n\n %s") % e)
                
        # show preview only when user doesn't want to create ps or pdf 
        if haveImage and event.userData['temp'] and not event.userData['pdfname']:
            RunCommand('g.region', cols = event.userData['regionOld']['cols'], rows = event.userData['regionOld']['rows'])
## wx.BusyInfo does not display the message
##            busy = wx.BusyInfo(message = "Generating preview, wait please", parent = self)

            try:
                im = Image.open(event.userData['filename'])
                if self.instruction[self.pageId]['Orientation'] == 'Landscape':
                    im = im.rotate(270)
                
                im.save(self.imgName, format = 'png')
                
            except IOError, e:
                GError(parent = self,
                       message = _("Unable to generate preview. %s") % e)
            
            
                
            rect = self.previewCanvas.ImageRect()
            self.previewCanvas.image = wx.Image(self.imgName, wx.BITMAP_TYPE_PNG)
            self.previewCanvas.DrawImage(rect = rect)
            
##            busy.Destroy()
            self.SetStatusText(_('Preview generated'), 0)
            self.book.SetSelection(1)
            self.currentPage = 1
        
        grass.try_remove(event.userData['instrFile'])
        if event.userData['temp']:
            grass.try_remove(event.userData['filename'])
        
    def getFile(self, wildcard):
        suffix = []
        for filter in wildcard.split('|')[1::2]:
            s = filter.strip('*').split('.')[1]
            if s:
                s = '.' + s
            suffix.append(s)
        raster = self.instruction.FindInstructionByType('raster')
        if raster:
            rasterId = raster.id 
        else:
            rasterId = None


        if rasterId and self.instruction[rasterId]['raster']:
            mapName = self.instruction[rasterId]['raster'].split('@')[0] + suffix[0]
        else:
            mapName = ''
            
        filename = ''
        dlg = wx.FileDialog(self, message = _("Save file as"), defaultDir = "", 
                            defaultFile = mapName, wildcard = wildcard,
                            style = wx.CHANGE_DIR | wx.SAVE | wx.OVERWRITE_PROMPT)
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()
            suffix = suffix[dlg.GetFilterIndex()]
            if not os.path.splitext(filename)[1]:
                filename = filename + suffix
            elif os.path.splitext(filename)[1] != suffix and suffix != '':
                filename = os.path.splitext(filename)[0] + suffix
            
        dlg.Destroy()
        return filename
    
    def OnInstructionFile(self, event):
        filename = self.getFile(wildcard = "*.psmap|*.psmap|Text file(*.txt)|*.txt|All files(*.*)|*.*")        
        if filename:    
            instrFile = open(filename, "w")
            instrFile.write(self.InstructionFile())
            instrFile.close()   
            
    def OnLoadFile(self, event):
        """!Load file and read instructions"""
        #find file
        filename = ''
        dlg = wx.FileDialog(self, message = "Find instructions file", defaultDir = "", 
                            defaultFile = '', wildcard = "All files (*.*)|*.*",
                            style = wx.CHANGE_DIR|wx.OPEN)
        if dlg.ShowModal() == wx.ID_OK:
            filename = dlg.GetPath()
        dlg.Destroy()
        if not filename:
            return
        # load instructions
        #filename = '/home/anna/Desktop/reading.txt'
        readObjectId = []
        readInstruction = Instruction(parent = self, objectsToDraw = readObjectId)
        ok = readInstruction.Read(filename)
        if not ok:
            GMessage(_("Failed to read file %s.") % filename)
        else:
            self.instruction = self.canvas.instruction = readInstruction
            self.objectId = self.canvas.objectId = readObjectId
            self.pageId = self.canvas.pageId = self.instruction.FindInstructionByType('page').id
            self.canvas.UpdateMapLabel()
            self.canvas.dragId = -1
            self.canvas.Clear()
            #self.canvas.ZoomAll()
            
            self.DialogDataChanged(self.objectId)
            
    def OnPageSetup(self, event = None):
        """!Specify paper size, margins and orientation"""
        id = self.instruction.FindInstructionByType('page').id
        dlg = PageSetupDialog(self, id = id, settings = self.instruction) 
        dlg.CenterOnScreen()
        val = dlg.ShowModal()
        if val == wx.ID_OK:
            self.canvas.SetPage()
            self.canvas.RecalculatePosition(ids = self.objectId)
        dlg.Destroy()
        
    def OnPointer(self, event):
        self.toolbar.OnTool(event)
        self.mouse["use"] = "pointer"
        self.canvas.SetCursor(self.cursors["default"])
        self.previewCanvas.SetCursor(self.cursors["default"])
        
    def OnPan(self, event):
        self.toolbar.OnTool(event)
        self.mouse["use"] = "pan"
        self.canvas.SetCursor(self.cursors["hand"])
        self.previewCanvas.SetCursor(self.cursors["hand"])
        
    def OnZoomIn(self, event):
        self.toolbar.OnTool(event)
        self.mouse["use"] = "zoomin"
        self.canvas.SetCursor(self.cursors["cross"])
        self.previewCanvas.SetCursor(self.cursors["cross"])
        
    def OnZoomOut(self, event):
        self.toolbar.OnTool(event)
        self.mouse["use"] = "zoomout"
        self.canvas.SetCursor(self.cursors["cross"])
        self.previewCanvas.SetCursor(self.cursors["cross"])
        
    def OnZoomAll(self, event):
        self.mouseOld = self.mouse['use']
        if self.currentPage == 0:
            self.cursorOld = self.canvas.GetCursor() 
        else:
            self.previewCanvas.GetCursor()
        self.mouse["use"] = "zoomin"
        if self.currentPage == 0:
            self.canvas.ZoomAll()
        else:
            self.previewCanvas.ZoomAll()
        self.mouse["use"] = self.mouseOld 
        if self.currentPage == 0:
            self.canvas.SetCursor(self.cursorOld)
        else:
            self.previewCanvas.SetCursor(self.cursorOld)
        
        
    def OnAddMap(self, event, notebook = False):
        """!Add or edit map frame"""
        if event is not None:
            if event.GetId() != self.toolbar.action['id']:
                self.actionOld = self.toolbar.action['id']
                self.mouseOld = self.mouse['use']
                self.cursorOld = self.canvas.GetCursor()
            self.toolbar.OnTool(event)
        
        if self.instruction.FindInstructionByType('map'):
            mapId = self.instruction.FindInstructionByType('map').id
        else: mapId = None
        id = [mapId, None, None]
        
        if notebook:
            if self.instruction.FindInstructionByType('vector'):
                vectorId = self.instruction.FindInstructionByType('vector').id
            else: vectorId = None
            if self.instruction.FindInstructionByType('raster'):
                rasterId = self.instruction.FindInstructionByType('raster').id
            else: rasterId = None
            id[1] = rasterId
            id[2] = vectorId
        
        
        if mapId: # map exists
            
            self.toolbar.ToggleTool(self.actionOld, True)
            self.toolbar.ToggleTool(self.toolbar.action['id'], False)
            self.toolbar.action['id'] = self.actionOld
            try:
                self.canvas.SetCursor(self.cursorOld) 
            except AttributeError:
                pass
            
##            dlg = MapDialog(parent = self, id  = id, settings = self.instruction,
##                            notebook = notebook)
##            dlg.ShowModal()  
            if notebook:
                #check map, raster, vector and save, destroy them
                if 'map' in self.openDialogs:
                    self.openDialogs['map'].OnOK(event = None)
                if 'raster' in self.openDialogs:
                    self.openDialogs['raster'].OnOK(event = None)
                if 'vector' in self.openDialogs:
                    self.openDialogs['vector'].OnOK(event = None)
                    
                if 'mapNotebook' not in self.openDialogs:
                    dlg = MapDialog(parent = self, id  = id, settings = self.instruction,
                                    notebook = notebook)
                    self.openDialogs['mapNotebook'] = dlg
                self.openDialogs['mapNotebook'].Show()
            else:
                if 'mapNotebook' in self.openDialogs:
                    self.openDialogs['mapNotebook'].notebook.ChangeSelection(0)
                else:
                    if 'map' not in self.openDialogs:
                        dlg = MapDialog(parent = self, id  = id, settings = self.instruction,
                                        notebook = notebook)
                        self.openDialogs['map'] = dlg
                    self.openDialogs['map'].Show()
                    

        else:    # sofar no map
            self.mouse["use"] = "addMap"
            self.canvas.SetCursor(self.cursors["cross"])
            if self.currentPage == 1:
                self.book.SetSelection(0)
                self.currentPage = 0
                
    def OnAddRaster(self, event):
        """!Add raster map"""
        if self.instruction.FindInstructionByType('raster'):
            id = self.instruction.FindInstructionByType('raster').id
        else: id = None
        if self.instruction.FindInstructionByType('map'):
            mapId = self.instruction.FindInstructionByType('map').id
        else: mapId = None

        if not id:
            if not mapId:
                GMessage(message = _("Please, create map frame first."))
                return
            
##        dlg = RasterDialog(self, id = id, settings = self.instruction)
##        dlg.ShowModal()
        if 'mapNotebook' in self.openDialogs:
            self.openDialogs['mapNotebook'].notebook.ChangeSelection(1)
        else:
            if 'raster' not in self.openDialogs:
                dlg = RasterDialog(self, id = id, settings = self.instruction)
                self.openDialogs['raster'] = dlg
            self.openDialogs['raster'].Show()
            
    def OnAddVect(self, event):
        """!Add vector map"""
        if self.instruction.FindInstructionByType('vector'):
            id = self.instruction.FindInstructionByType('vector').id
        else: id = None
        if self.instruction.FindInstructionByType('map'):
            mapId = self.instruction.FindInstructionByType('map').id
        else: mapId = None
        if not id:
            if not mapId:
                GMessage(message = _("Please, create map frame first."))
                return
            
##        dlg = MainVectorDialog(self, id = id, settings = self.instruction)
##        dlg.ShowModal()
        if 'mapNotebook' in self.openDialogs:
            self.openDialogs['mapNotebook'].notebook.ChangeSelection(2)
        else:
            if 'vector' not in self.openDialogs:
                dlg =  MainVectorDialog(self, id = id, settings = self.instruction)
                self.openDialogs['vector'] = dlg
            self.openDialogs['vector'].Show()
        
    def OnDecoration(self, event):
        """!Decorations overlay menu
        """
        decmenu = wx.Menu()
        # legend
        AddLegend = wx.MenuItem(decmenu, wx.ID_ANY, Icons['psMap']["addLegend"].GetLabel())
        AddLegend.SetBitmap(Icons['psMap']["addLegend"].GetBitmap(self.iconsize))
        decmenu.AppendItem(AddLegend)
        self.Bind(wx.EVT_MENU, self.OnAddLegend, AddLegend)
        # mapinfo
        AddMapinfo = wx.MenuItem(decmenu, wx.ID_ANY, Icons['psMap']["addMapinfo"].GetLabel())
        AddMapinfo.SetBitmap(Icons['psMap']["addMapinfo"].GetBitmap(self.iconsize))
        decmenu.AppendItem(AddMapinfo)
        self.Bind(wx.EVT_MENU, self.OnAddMapinfo, AddMapinfo) 
        # scalebar
        AddScalebar = wx.MenuItem(decmenu, wx.ID_ANY, Icons['psMap']["addScalebar"].GetLabel())
        AddScalebar.SetBitmap(Icons['psMap']["addScalebar"].GetBitmap(self.iconsize))
        decmenu.AppendItem(AddScalebar)
        self.Bind(wx.EVT_MENU, self.OnAddScalebar, AddScalebar) 
        # text
        AddText = wx.MenuItem(decmenu, wx.ID_ANY, Icons['psMap']["addText"].GetLabel())
        AddText.SetBitmap(Icons['psMap']["addText"].GetBitmap(self.iconsize))
        decmenu.AppendItem(AddText)
        self.Bind(wx.EVT_MENU, self.OnAddText, AddText) 
        # Popup the menu.  If an item is selected then its handler
        # will be called before PopupMenu returns.
        self.PopupMenu(decmenu)
        decmenu.Destroy()
        
    def OnAddScalebar(self, event):
        """!Add scalebar"""
        if projInfo()['proj'] == 'll':
            GMessage(message = _("Scalebar is not appropriate for this projection"))
            return
        if self.instruction.FindInstructionByType('scalebar'):
            id = self.instruction.FindInstructionByType('scalebar').id
        else: id = None
        
        if 'scalebar' not in self.openDialogs:
            dlg = ScalebarDialog(self, id = id, settings = self.instruction)
            self.openDialogs['scalebar'] = dlg
        self.openDialogs['scalebar'].Show()
        
    def OnAddLegend(self, event, page = 0):
        """!Add raster or vector legend"""
        if self.instruction.FindInstructionByType('rasterLegend'):
            idR = self.instruction.FindInstructionByType('rasterLegend').id
        else: idR = None
        if self.instruction.FindInstructionByType('vectorLegend'):
            idV = self.instruction.FindInstructionByType('vectorLegend').id
        else: idV = None

        if 'rasterLegend' not in self.openDialogs:    
            dlg = LegendDialog(self, id = [idR, idV], settings = self.instruction, page = page)
            self.openDialogs['rasterLegend'] = dlg
            self.openDialogs['vectorLegend'] = dlg
        self.openDialogs['rasterLegend'].notebook.ChangeSelection(page)
        self.openDialogs['rasterLegend'].Show()

    def OnAddMapinfo(self, event):
        if self.instruction.FindInstructionByType('mapinfo'):
            id = self.instruction.FindInstructionByType('mapinfo').id
        else: id = None
        
        if 'mapinfo' not in self.openDialogs:
            dlg = MapinfoDialog(self, id = id, settings = self.instruction)
            self.openDialogs['mapinfo'] = dlg
        self.openDialogs['mapinfo'].Show()
        
    def OnAddText(self, event, id = None):
        """!Show dialog for text adding and editing"""
        position = None
        if 'text' in self.openDialogs:
            position = self.openDialogs['text'].GetPosition()
            self.openDialogs['text'].OnApply(event = None)
            self.openDialogs['text'].Destroy()
        dlg = TextDialog(self, id = id, settings = self.instruction)
        self.openDialogs['text'] = dlg 
        if position: 
            dlg.SetPosition(position)
        dlg.Show()
        
    def getModifiedTextBounds(self, x, y, textExtent, rotation):
        """!computes bounding box of rotated text, not very precisely"""
        w, h = textExtent
        rotation = float(rotation)/180*pi
        H = float(w) * sin(rotation)
        W = float(w) * cos(rotation)
        X, Y = x, y
        if pi/2 < rotation <= 3*pi/2:
            X = x + W 
        if 0 < rotation < pi:
            Y = y - H
        if rotation == 0:
            return wx.Rect(x,y, *textExtent)
        else:
            return wx.Rect(X, Y, abs(W), abs(H)).Inflate(h,h) 

    def makePSFont(self, textDict):
        fontsize = textDict['fontsize'] * self.canvas.currScale
        fontface = textDict['font'].split('-')[0]
        try:
            fontstyle = textDict['font'].split('-')[1]
        except:
            fontstyle = 'normal'
        
        if fontface == "Times":
            family = wx.FONTFAMILY_ROMAN
            face = "times"
        elif fontface == "Helvetica":
            family = wx.FONTFAMILY_SWISS
            face = 'helvetica'
        elif fontface == "Courier":
            family = wx.FONTFAMILY_TELETYPE
            face = 'courier'
        else:
            family = wx.FONTFAMILY_DEFAULT
            face = ''
            
        if fontstyle == 'normal':
            style = wx.FONTSTYLE_NORMAL
            weight = wx.FONTWEIGHT_NORMAL
            
        if 'oblique' in fontstyle:
            style =  wx.FONTSTYLE_SLANT
            
        if 'italic' in fontstyle:
            style =  wx.FONTSTYLE_ITALIC
            
        if 'bold' in fontstyle:
            weight = wx.FONTWEIGHT_BOLD
        else:
            weight = wx.FONTWEIGHT_NORMAL
        
        try:
            fn = wx.Font(pointSize=fontsize, family=family, style=style, weight=weight, face=face)
            return fn
        except:
            return False
       
    def getTextExtent(self, textDict):
        #fontsize = str(fontsize if fontsize >= 4 else 4)
        dc = wx.PaintDC(self) # dc created because of method GetTextExtent, which pseudoDC lacks
       
        fn = self.makePSFont(textDict)

        if fn:
            dc.SetFont(fn)
            w,h,lh = dc.GetMultiLineTextExtent(textDict['text'])
            return (w,h)
#            return dc.GetTextExtent(textDict['text'])
        else:
            return (0,0)
    
    def getInitMap(self):
        """!Create default map frame when no map is selected, needed for coordinates in map units"""
        instrFile = grass.tempfile()
        instrFileFd = open(instrFile, mode = 'w')
        instrFileFd.write(self.InstructionFile())
        instrFileFd.flush()
        instrFileFd.close()
        
        mapInitRect = GetMapBounds(instrFile)
        grass.try_remove(instrFile)
        
        region = grass.region()
        units = UnitConversion(self)
        realWidth = units.convert(value = abs(region['w'] - region['e']), fromUnit = 'meter', toUnit = 'inch')
        scale = mapInitRect.Get()[2]/realWidth  
        
        initMap = self.instruction.FindInstructionByType('initMap')
        if initMap:
            id = initMap.id 
        else:
            id = None

        
        if not id:
            id = wx.NewId()
            initMap = InitMap(id)
            self.instruction.AddInstruction(initMap)
        self.instruction[id].SetInstruction(dict(rect = mapInitRect, scale = scale))

    def OnDelete(self, event):
        if self.canvas.dragId != -1 and self.currentPage == 0:
            if self.instruction[self.canvas.dragId].type == 'map':
                self.deleteObject(self.canvas.dragId)
                self.getInitMap()
                self.canvas.RecalculateEN()
            else:
                self.deleteObject(self.canvas.dragId)   
    
    def deleteObject(self, id):
        """!Deletes object, his id and redraws"""
        #delete from canvas
        self.canvas.pdcObj.RemoveId(id)
        if id == self.canvas.dragId:
            self.canvas.pdcTmp.RemoveAll()
            self.canvas.dragId = -1
        self.canvas.Refresh()
        
        # delete from instructions
        del self.instruction[id]

    def DialogDataChanged(self, id):
        ids = id
        if type(id) == int:
            ids = [id]
        for id in ids:
            itype = self.instruction[id].type
            
            if itype in ('scalebar', 'mapinfo'):
                drawRectangle = self.canvas.CanvasPaperCoordinates(rect = self.instruction[id]['rect'], canvasToPaper = False)
                self.canvas.Draw(pen = self.pen[itype], brush = self.brush[itype],
                                 pdc = self.canvas.pdcObj, drawid = id, pdctype = 'rectText', bb = drawRectangle)
                self.canvas.RedrawSelectBox(id)
                
            if itype == 'text':
                
                if self.instruction[id]['rotate']:
                    rot = float(self.instruction[id]['rotate']) 
                else:
                    rot = 0

                extent = self.getTextExtent(textDict = self.instruction[id].GetInstruction())
                rect = wx.Rect2D(self.instruction[id]['where'][0], self.instruction[id]['where'][1], 0, 0)
                self.instruction[id]['coords'] = list(self.canvas.CanvasPaperCoordinates(rect = rect, canvasToPaper = False)[:2])
                
                #computes text coordinates according to reference point, not precisely
                if self.instruction[id]['ref'].split()[0] == 'lower':
                    self.instruction[id]['coords'][1] -= extent[1]
                elif self.instruction[id]['ref'].split()[0] == 'center':
                    self.instruction[id]['coords'][1] -= extent[1]/2
                if self.instruction[id]['ref'].split()[1] == 'right':
                    self.instruction[id]['coords'][0] -= extent[0] * cos(rot/180*pi)
                    self.instruction[id]['coords'][1] += extent[0] * sin(rot/180*pi)
                elif self.instruction[id]['ref'].split()[1] == 'center':
                    self.instruction[id]['coords'][0] -= extent[0]/2 * cos(rot/180*pi)
                    self.instruction[id]['coords'][1] += extent[0]/2 * sin(rot/180*pi)
                    
                self.instruction[id]['coords'][0] += self.instruction[id]['xoffset']
                self.instruction[id]['coords'][1] -= self.instruction[id]['yoffset']
                coords = self.instruction[id]['coords']
                self.instruction[id]['rect'] = bounds = self.getModifiedTextBounds(coords[0], coords[1], extent, rot)
                self.canvas.DrawRotText(pdc = self.canvas.pdcObj, drawId = id,
                                        textDict = self.instruction[id].GetInstruction(),
                                        coords = coords, bounds = bounds)
                self.canvas.RedrawSelectBox(id)
                
            if itype in ('map', 'vector', 'raster'):
                
                if itype == 'raster':#set resolution
                    resol = RunCommand('r.info', read = True, flags = 's', map = self.instruction[id]['raster'])
                    resol = grass.parse_key_val(resol, val_type = float)
                    RunCommand('g.region', nsres = resol['nsres'], ewres = resol['ewres'])
                    # change current raster in raster legend
                    
                if 'rasterLegend' in self.openDialogs:
                    self.openDialogs['rasterLegend'].updateDialog()
                id = self.instruction.FindInstructionByType('map').id
                
                #check resolution
                if itype == 'raster':
                    SetResolution(dpi = self.instruction[id]['resolution'], 
                                  width = self.instruction[id]['rect'].width,
                                  height = self.instruction[id]['rect'].height)   
                rectCanvas = self.canvas.CanvasPaperCoordinates(rect = self.instruction[id]['rect'],
                                                                canvasToPaper = False)
                self.canvas.RecalculateEN()
                self.canvas.UpdateMapLabel()
                
                self.canvas.Draw(pen = self.pen['map'], brush = self.brush['map'],
                                 pdc = self.canvas.pdcObj, drawid = id, pdctype = 'rectText', bb = rectCanvas)
                # redraw select box  
                self.canvas.RedrawSelectBox(id)
                self.canvas.pdcTmp.RemoveId(self.canvas.idZoomBoxTmp)
                # redraw to get map to the bottom layer
                #self.canvas.Zoom(zoomFactor = 1, view = (0, 0))
                
            if itype == 'rasterLegend':
                if self.instruction[id]['rLegend']:
                    drawRectangle = self.canvas.CanvasPaperCoordinates(rect = self.instruction[id]['rect'], canvasToPaper = False)
                    self.canvas.Draw(pen = self.pen[itype], brush = self.brush[itype],
                                     pdc = self.canvas.pdcObj, drawid = id, pdctype = 'rectText', bb = drawRectangle)
                    self.canvas.RedrawSelectBox(id)
                else:
                    self.deleteObject(id)
                    
            if itype == 'vectorLegend':
                if not self.instruction.FindInstructionByType('vector'):
                    self.deleteObject(id)
                elif self.instruction[id]['vLegend']:
                    drawRectangle = self.canvas.CanvasPaperCoordinates(rect = self.instruction[id]['rect'], canvasToPaper = False)
                    self.canvas.Draw(pen = self.pen[itype], brush = self.brush[itype],
                                     pdc = self.canvas.pdcObj, drawid = id, pdctype = 'rectText', bb = drawRectangle)
                    self.canvas.RedrawSelectBox(id)

                else:
                    self.deleteObject(id)
                
    def OnPageChanged(self, event):
        """!Flatnotebook page has changed"""
        self.currentPage = self.book.GetPageIndex(self.book.GetCurrentPage())
        
        
    def OnPageChanging(self, event):
        """!Flatnotebook page is changing"""
        if self.currentPage == 0 and self.mouse['use'] == 'addMap':
            event.Veto()

    def OnHelp(self, event):
        """!Show help"""
        if self.parent and self.parent.GetName() == 'LayerManager':
            log = self.parent.GetLogWindow()
            log.RunCmd(['g.manual',
                        'entry=wxGUI.PsMap'])
        else:
            RunCommand('g.manual',
                       quiet = True,
                       entry = 'wxGUI.PsMap')
        
    def OnAbout(self, event):
        """!Display About window"""
        info = wx.AboutDialogInfo()
        
        info.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        info.SetName(_('wxGUI Hardcopy Map Utility'))
        info.SetWebSite('http://grass.osgeo.org')
        info.SetDescription(_('(C) 2011 by the GRASS Development Team\n\n') + 
                            '\n'.join(textwrap.wrap(_('This program is free software under the GNU General Public License'
                                                      '(>=v2). Read the file COPYING that comes with GRASS for details.'), 75)))
        
        wx.AboutBox(info)

    def OnCloseWindow(self, event):
        """!Close window"""
        try:
            os.remove(self.imgName)
        except OSError:
            pass
        grass.set_raise_on_error(False)
        self.Destroy()



class PsMapBufferedWindow(wx.Window):
    """!A buffered window class.
    
    @param parent parent window
    @param kwargs other wx.Window parameters
    """
    def __init__(self, parent, id =  wx.ID_ANY,
                 style = wx.NO_FULL_REPAINT_ON_RESIZE,
                 **kwargs):
        wx.Window.__init__(self, parent, id = id, style = style)
        self.parent = parent
    
        self.FitInside()
        
        # store an off screen empty bitmap for saving to file
        self._buffer = None
        # indicates whether or not a resize event has taken place
        self.resize = False 
        
        self.mouse = kwargs['mouse']
        self.cursors = kwargs['cursors']
        self.preview = kwargs['preview']
        self.pen = kwargs['pen']
        self.brush = kwargs['brush']
        
        if kwargs.has_key('instruction'):
            self.instruction = kwargs['instruction']
        if kwargs.has_key('openDialogs'):
            self.openDialogs = kwargs['openDialogs']
        if kwargs.has_key('pageId'):
            self.pageId = kwargs['pageId']
        if kwargs.has_key('objectId'):
            self.objectId = kwargs['objectId']
        
        
        #labels
        self.itemLabels = { 'map': ['MAP FRAME'],
                            'rasterLegend': ['RASTER LEGEND'],
                            'vectorLegend': ['VECTOR LEGEND'],
                            'mapinfo': ['MAP INFO'],
                            'scalebar': ['SCALE BAR']}
        
        # define PseudoDC
        self.pdc = wx.PseudoDC()
        self.pdcObj = wx.PseudoDC()
        self.pdcPaper = wx.PseudoDC()
        self.pdcTmp = wx.PseudoDC()
        self.pdcImage = wx.PseudoDC()
        dc = wx.PaintDC(self)
        self.font = dc.GetFont()
        
        self.SetClientSize((700,510))#?
        self._buffer = wx.EmptyBitmap(*self.GetClientSize())
        
        self.idBoxTmp = wx.NewId()
        self.idZoomBoxTmp = wx.NewId()
        self.idResizeBoxTmp = wx.NewId()
        
        

        self.dragId = -1
        
        if self.preview:
            self.image = None
            self.imageId = 2000
            self.imgName = self.parent.imgName
            
            
            
        self.currScale = None
        
        self.Clear()
        
        self.Bind(wx.EVT_ERASE_BACKGROUND, lambda x: None)
        
        self.Bind(wx.EVT_PAINT, self.OnPaint)
        self.Bind(wx.EVT_SIZE,  self.OnSize)
        self.Bind(wx.EVT_IDLE,  self.OnIdle)
        self.Bind(wx.EVT_MOUSE_EVENTS, self.OnMouse)


    def Clear(self):
        """!Clear canvas and set paper
        """
        bg = wx.LIGHT_GREY_BRUSH
        self.pdcPaper.BeginDrawing()
        self.pdcPaper.SetBackground(bg)
        self.pdcPaper.Clear()
        self.pdcPaper.EndDrawing()
        
        self.pdcObj.RemoveAll()
        self.pdcTmp.RemoveAll()
        


        if not self.preview:
            self.SetPage()

    
    def CanvasPaperCoordinates(self, rect, canvasToPaper = True):
        """!Converts canvas (pixel) -> paper (inch) coordinates and size and vice versa"""
        
        units = UnitConversion(self)
        
        fromU = 'pixel'
        toU = 'inch'
        pRect = self.pdcPaper.GetIdBounds(self.pageId)
        pRectx, pRecty = pRect.x, pRect.y 
        scale = 1/self.currScale
        if not canvasToPaper: # paper -> canvas
            fromU = 'inch'
            toU = 'pixel'
            scale = self.currScale
            pRectx = units.convert(value =  - pRect.x, fromUnit = 'pixel', toUnit = 'inch' ) /scale #inch, real, negative
            pRecty = units.convert(value =  - pRect.y, fromUnit = 'pixel', toUnit = 'inch' ) /scale 
        Width = units.convert(value = rect.width, fromUnit = fromU, toUnit = toU) * scale
        Height = units.convert(value = rect.height, fromUnit = fromU, toUnit = toU) * scale
        X = units.convert(value = (rect.x - pRectx), fromUnit = fromU, toUnit = toU) * scale
        Y = units.convert(value = (rect.y - pRecty), fromUnit = fromU, toUnit = toU) * scale

        return wx.Rect2D(X, Y, Width, Height)

    
    
    def SetPage(self):
        """!Sets and changes page, redraws paper"""
        
        page = self.instruction[self.pageId]
        if not page:
            page = PageSetup(id = self.pageId)
            self.instruction.AddInstruction(page)
        
        ppi = wx.PaintDC(self).GetPPI()
        cW, cH = self.GetClientSize()
        pW, pH = page['Width']*ppi[0], page['Height']*ppi[1]

        if self.currScale is None:
            self.currScale = min(cW/pW, cH/pH)
        pW = pW * self.currScale
        pH = pH * self.currScale
        
        x = cW/2 - pW/2
        y = cH/2 - pH/2
        self.DrawPaper(wx.Rect(x, y, pW, pH))


    def modifyRectangle(self, r):
        """! Recalculates rectangle not to have negative size"""
        if r.GetWidth() < 0:
            r.SetX(r.GetX() + r.GetWidth())
        if r.GetHeight() < 0:
            r.SetY(r.GetY() + r.GetHeight())
        r.SetWidth(abs(r.GetWidth()))
        r.SetHeight(abs(r.GetHeight()))
        return r 
    
    def RecalculateEN(self):
        """!Recalculate east and north for texts (eps, points) after their or map's movement"""
        try:
            mapId = self.instruction.FindInstructionByType('map').id
        except AttributeError:
            mapId = self.instruction.FindInstructionByType('initMap').id
            
        texts = self.instruction.FindInstructionByType('text', list = True)
        for text in texts:
            e, n = PaperMapCoordinates(map = self.instruction[mapId], x = self.instruction[text.id]['where'][0],
                                       y = self.instruction[text.id]['where'][1], paperToMap = True)
            self.instruction[text.id]['east'], self.instruction[text.id]['north'] = e, n
            
    def OnPaint(self, event):
        """!Draw pseudo DC to buffer
        """
        if not self._buffer:
            return
        dc = wx.BufferedPaintDC(self, self._buffer)
        # use PrepareDC to set position correctly
        self.PrepareDC(dc)
        
        dc.SetBackground(wx.LIGHT_GREY_BRUSH)
        dc.Clear()
        
        # draw paper
        if not self.preview:
            self.pdcPaper.DrawToDC(dc)
        # draw to the DC using the calculated clipping rect

        rgn = self.GetUpdateRegion()
        
        if not self.preview:
            self.pdcObj.DrawToDCClipped(dc, rgn.GetBox())
        else: 
            self.pdcImage.DrawToDCClipped(dc, rgn.GetBox())
        self.pdcTmp.DrawToDCClipped(dc, rgn.GetBox())
        
    def OnMouse(self, event):

        if event.GetWheelRotation():
            zoom = event.GetWheelRotation()
            use = self.mouse['use']
            self.mouse['begin'] = event.GetPosition()
            if zoom > 0:
                self.mouse['use'] = 'zoomin'
            else:
                self.mouse['use'] = 'zoomout'
                
            zoomFactor, view = self.ComputeZoom(wx.Rect(0,0,0,0))
            self.Zoom(zoomFactor, view)
            self.mouse['use'] = use
            
        if event.Moving():
            if self.mouse['use'] in ('pointer', 'resize'):
                pos = event.GetPosition()
                foundResize = self.pdcTmp.FindObjects(pos[0], pos[1])
                if foundResize and foundResize[0] == self.idResizeBoxTmp:
                    self.SetCursor(self.cursors["sizenwse"])
                    self.parent.SetStatusText(_('Click and drag to resize object'), 0)
                else:
                    self.parent.SetStatusText('', 0)
                    self.SetCursor(self.cursors["default"])
                    
        elif event.LeftDown():
            self.mouse['begin'] = event.GetPosition()
            self.begin = self.mouse['begin']
            if self.mouse['use'] in ('pan', 'zoomin', 'zoomout', 'addMap'):
                pass
            
            #select
            if self.mouse['use'] == 'pointer':
                found = self.pdcObj.FindObjects(self.mouse['begin'][0], self.mouse['begin'][1])
                foundResize = self.pdcTmp.FindObjects(self.mouse['begin'][0], self.mouse['begin'][1])

                if foundResize and foundResize[0] == self.idResizeBoxTmp:
                    self.mouse['use'] = 'resize'
                    
                    # when resizing, proportions match region
                    if self.instruction[self.dragId].type == 'map':
                        self.constraint = False
                        self.mapBounds = self.pdcObj.GetIdBounds(self.dragId)
                        if self.instruction[self.dragId]['scaleType'] in (0, 1, 2):
                            self.constraint = True
                            self.mapBounds = self.pdcObj.GetIdBounds(self.dragId)
                    
                elif found:
                    self.dragId = found[0]  
                    self.RedrawSelectBox(self.dragId)
                    if self.instruction[self.dragId].type != 'map':
                        self.pdcTmp.RemoveId(self.idResizeBoxTmp)
                        self.Refresh()
                        
                else:
                    self.dragId = -1
                    self.pdcTmp.RemoveId(self.idBoxTmp)
                    self.pdcTmp.RemoveId(self.idResizeBoxTmp)
                    self.Refresh()           
                    
                    
        elif event.Dragging() and event.LeftIsDown():
            #draw box when zooming, creating map 
            if self.mouse['use'] in ('zoomin', 'zoomout', 'addMap'):
                self.mouse['end'] = event.GetPosition()
                r = wx.Rect(self.mouse['begin'][0], self.mouse['begin'][1],
                            self.mouse['end'][0]-self.mouse['begin'][0], self.mouse['end'][1]-self.mouse['begin'][1])
                r = self.modifyRectangle(r)
                self.Draw(pen = self.pen['box'], brush = self.brush['box'], pdc = self.pdcTmp, drawid = self.idZoomBoxTmp,
                          pdctype = 'rect', bb = r)
                
            # panning                
            if self.mouse["use"] == 'pan':
                self.mouse['end'] = event.GetPosition()
                view = self.mouse['begin'][0] - self.mouse['end'][0], self.mouse['begin'][1] - self.mouse['end'][1]
                zoomFactor = 1
                self.Zoom(zoomFactor, view)
                self.mouse['begin'] = event.GetPosition()
                
            #move object
            if self.mouse['use'] == 'pointer' and self.dragId != -1:
                
                self.mouse['end'] = event.GetPosition()
                dx, dy = self.mouse['end'][0] - self.begin[0], self.mouse['end'][1] - self.begin[1]
                self.pdcObj.TranslateId(self.dragId, dx, dy)
                self.pdcTmp.TranslateId(self.idBoxTmp, dx, dy)
                self.pdcTmp.TranslateId(self.idResizeBoxTmp, dx, dy)
                if self.instruction[self.dragId].type == 'text': 
                    self.instruction[self.dragId]['coords'] = self.instruction[self.dragId]['coords'][0] + dx,\
                        self.instruction[self.dragId]['coords'][1] + dy
                self.begin = event.GetPosition()
                self.Refresh()
                
            # resize object
            if self.mouse['use'] == 'resize':
                type = self.instruction[self.dragId].type
                pos = event.GetPosition()
                x, y = self.mapBounds.GetX(), self.mapBounds.GetY()
                width, height = self.mapBounds.GetWidth(), self.mapBounds.GetHeight()
                diffX = pos[0] - self.mouse['begin'][0]
                diffY = pos[1] - self.mouse['begin'][1]
                # match given region
                if self.constraint:
                    if width > height:
                        newWidth = width + diffX
                        newHeight = height + diffX * (float(height) / width)
                    else:
                        newWidth = width + diffY * (float(width) / height)
                        newHeight = height + diffY
                else:
                    newWidth = width + diffX
                    newHeight = height + diffY
                    
                if newWidth < 10 or newHeight < 10:
                    return
                
                bounds = wx.Rect(x, y, newWidth, newHeight)    
                self.Draw(pen = self.pen[type], brush = self.brush[type], pdc = self.pdcObj, drawid = self.dragId,
                          pdctype = 'rectText', bb = bounds)
                self.RedrawSelectBox(self.dragId)
                
        elif event.LeftUp():
            # zoom in, zoom out
            if self.mouse['use'] in ('zoomin','zoomout'):
                zoomR = self.pdcTmp.GetIdBounds(self.idZoomBoxTmp)
                self.pdcTmp.RemoveId(self.idZoomBoxTmp)
                self.Refresh()
                zoomFactor, view = self.ComputeZoom(zoomR)
                self.Zoom(zoomFactor, view)

                
            # draw map frame    
            if self.mouse['use'] == 'addMap':
                rectTmp = self.pdcTmp.GetIdBounds(self.idZoomBoxTmp)
                # too small rectangle, it's usually some mistake
                if rectTmp.GetWidth() < 20 or rectTmp.GetHeight() < 20:
                    self.pdcTmp.RemoveId(self.idZoomBoxTmp)
                    self.Refresh()
                    return
                rectPaper = self.CanvasPaperCoordinates(rect = rectTmp, canvasToPaper = True)                
                
                dlg = MapDialog(parent = self.parent, id = [None, None, None], settings = self.instruction, 
                                rect = rectPaper)
                self.openDialogs['map'] = dlg
                self.openDialogs['map'].Show()
                
                
                self.mouse['use'] = self.parent.mouseOld

                self.SetCursor(self.parent.cursorOld)
                self.parent.toolbar.ToggleTool(self.parent.actionOld, True)
                self.parent.toolbar.ToggleTool(self.parent.toolbar.action['id'], False)
                self.parent.toolbar.action['id'] = self.parent.actionOld
                


            # resize resizable objects (only map sofar)
            if self.mouse['use'] == 'resize':
                mapId = self.instruction.FindInstructionByType('map').id
                
                if self.dragId == mapId:
                    # necessary to change either map frame (scaleType 0,1,2) or region (scaletype 3)
                    newRectCanvas = self.pdcObj.GetIdBounds(mapId)
                    newRectPaper = self.CanvasPaperCoordinates(rect = newRectCanvas, canvasToPaper = True)
                    self.instruction[mapId]['rect'] = newRectPaper
                    
                    if self.instruction[mapId]['scaleType'] in (0, 1, 2):
                        if self.instruction[mapId]['scaleType'] == 0:
                            
                            scale, foo, rect = AutoAdjust(self, scaleType = 0,
                                                          map = self.instruction[mapId]['map'],
                                                          mapType = self.instruction[mapId]['mapType'], 
                                                          rect = self.instruction[mapId]['rect'])
                            
                        elif self.instruction[mapId]['scaleType'] == 1:
                            scale, foo, rect = AutoAdjust(self, scaleType = 1,
                                                          region = self.instruction[mapId]['region'],
                                                          rect = self.instruction[mapId]['rect'])
                        else:
                            scale, foo, rect = AutoAdjust(self, scaleType = 2,
                                                          rect = self.instruction[mapId]['rect'])
                        self.instruction[mapId]['rect'] = rect
                        self.instruction[mapId]['scale'] = scale
                        
                        rectCanvas = self.CanvasPaperCoordinates(rect = rect, canvasToPaper = False)
                        self.Draw(pen = self.pen['map'], brush = self.brush['map'],
                                  pdc = self.pdcObj, drawid = mapId, pdctype = 'rectText', bb = rectCanvas)
                        
                    elif self.instruction[mapId]['scaleType'] == 3:
                        ComputeSetRegion(self, mapDict = self.instruction[mapId].GetInstruction())
                    #check resolution
                    SetResolution(dpi = self.instruction[mapId]['resolution'],
                                  width = self.instruction[mapId]['rect'].width,
                                  height = self.instruction[mapId]['rect'].height)
                    
                    self.RedrawSelectBox(mapId)
                    self.Zoom(zoomFactor = 1, view = (0, 0))
                self.mouse['use'] = 'pointer'
                
            # recalculate the position of objects after dragging    
            if self.mouse['use'] in ('pointer', 'resize') and self.dragId != -1:
                if self.mouse['begin'] != event.GetPosition(): #for double click
                    
                    self.RecalculatePosition(ids = [self.dragId])
                    if self.instruction[self.dragId].type in self.openDialogs:
                        self.openDialogs[self.instruction[self.dragId].type].updateDialog()

        # double click launches dialogs
        elif event.LeftDClick():
            if self.mouse['use'] == 'pointer' and self.dragId != -1:
                itemCall = {    'text':self.parent.OnAddText, 'mapinfo': self.parent.OnAddMapinfo,
                                'scalebar': self.parent.OnAddScalebar,
                                'rasterLegend': self.parent.OnAddLegend, 'vectorLegend': self.parent.OnAddLegend,  
                                'map': self.parent.OnAddMap}
                itemArg = { 'text': dict(event = None, id = self.dragId), 'mapinfo': dict(event = None),
                            'scalebar': dict(event = None),
                            'rasterLegend': dict(event = None), 'vectorLegend': dict(event = None, page = 1),
                            'map': dict(event = None, notebook = True)}
                type = self.instruction[self.dragId].type
                itemCall[type](**itemArg[type])

                
                
                
    def RecalculatePosition(self, ids):
        for id in ids:
            itype = self.instruction[id].type
            if itype == 'map':
                self.instruction[id]['rect'] = self.CanvasPaperCoordinates(rect = self.pdcObj.GetIdBounds(id),
                                                                           canvasToPaper = True)
                self.RecalculateEN()
                
            elif itype in ('mapinfo' ,'rasterLegend', 'vectorLegend'):
                self.instruction[id]['rect'] = self.CanvasPaperCoordinates(rect = self.pdcObj.GetIdBounds(id),
                                                                           canvasToPaper = True)
                self.instruction[id]['where'] = self.CanvasPaperCoordinates(rect = self.pdcObj.GetIdBounds(id),
                                                                            canvasToPaper = True)[:2]            
            elif  itype == 'scalebar':
                self.instruction[id]['rect'] = self.CanvasPaperCoordinates(rect = self.pdcObj.GetIdBounds(id),
                                                                           canvasToPaper = True)

                
                self.instruction[id]['where'] = self.instruction[id]['rect'].GetCentre()
                
            elif  itype == 'text':
                x, y = self.instruction[id]['coords'][0] - self.instruction[id]['xoffset'],\
                    self.instruction[id]['coords'][1] + self.instruction[id]['yoffset']
                extent = self.parent.getTextExtent(textDict = self.instruction[id])
                if self.instruction[id]['rotate'] is not None:
                    rot = float(self.instruction[id]['rotate'])/180*pi 
                else:
                    rot = 0

                if self.instruction[id]['ref'].split()[0] == 'lower':
                    y += extent[1]
                elif self.instruction[id]['ref'].split()[0] == 'center':
                    y += extent[1]/2
                if self.instruction[id]['ref'].split()[1] == 'right':
                    x += extent[0] * cos(rot)
                    y -= extent[0] * sin(rot)
                elif self.instruction[id]['ref'].split()[1] == 'center':
                    x += extent[0]/2 * cos(rot)
                    y -= extent[0]/2 * sin(rot)
                
                self.instruction[id]['where'] = self.CanvasPaperCoordinates(rect = wx.Rect2D(x, y, 0, 0),
                                                                            canvasToPaper = True)[:2]
                self.RecalculateEN()
        
    def ComputeZoom(self, rect):
        """!Computes zoom factor and scroll view"""
        zoomFactor = 1
        cW, cH = self.GetClientSize()
        cW = float(cW)
        if rect.IsEmpty(): # clicked on canvas
            zoomFactor = 1.5
            if self.mouse['use'] == 'zoomout':
                zoomFactor = 1./zoomFactor
            x,y = self.mouse['begin']
            xView = x - x/zoomFactor#x - cW/(zoomFactor * 2)
            yView = y - y/zoomFactor#y - cH/(zoomFactor * 2)

        else:   #dragging    
            rW, rH = float(rect.GetWidth()), float(rect.GetHeight())
            zoomFactor = 1/max(rW/cW, rH/cH)
            # when zooming to full extent, in some cases, there was zoom 1.01..., which causes problem
            if abs(zoomFactor - 1) > 0.01:
                zoomFactor = zoomFactor 
            else:
                zoomFactor = 1.


            if self.mouse['use'] == 'zoomout':
                zoomFactor = min(rW/cW, rH/cH) 
            if rW/rH > cW/cH:
                yView = rect.GetY() - (rW*(cH/cW) - rH)/2
                xView = rect.GetX()
                
                if self.mouse['use'] == 'zoomout':
                    x,y = rect.GetX() + (rW-(cW/cH)*rH)/2, rect.GetY()
                    xView, yView = -x, -y
            else:
                xView = rect.GetX() - (rH*(cW/cH) - rW)/2
                yView = rect.GetY()
                if self.mouse['use'] == 'zoomout':
                    x,y = rect.GetX(), rect.GetY() + (rH-(cH/cW)*rW)/2
                    xView, yView = -x, -y
        return zoomFactor, (int(xView), int(yView))
    
    
    def Zoom(self, zoomFactor, view):
        """! Zoom to specified region, scroll view, redraw"""
        if not self.currScale:
            return
        self.currScale = self.currScale*zoomFactor
        
        if self.currScale > 10 or self.currScale < 0.1:
            self.currScale = self.currScale/zoomFactor
            return 
        if not self.preview:
            # redraw paper
            pRect = self.pdcPaper.GetIdBounds(self.pageId)
            pRect.OffsetXY(-view[0], -view[1])
            pRect = self.ScaleRect(rect = pRect, scale = zoomFactor)
            self.DrawPaper(pRect)
            
            #redraw objects
            for id in self.objectId:
                oRect = self.CanvasPaperCoordinates(
                    rect = self.instruction[id]['rect'], canvasToPaper = False)
                
                type = self.instruction[id].type
                if type == 'text':
                    coords = self.instruction[id]['coords']# recalculate coordinates, they are not equal to BB
                    self.instruction[id]['coords'] = coords = [(int(coord) - view[i]) * zoomFactor
                                                               for i, coord in enumerate(coords)]
                    self.DrawRotText(pdc = self.pdcObj, drawId = id, textDict = self.instruction[id],
                                     coords = coords, bounds = oRect )
                    extent = self.parent.getTextExtent(textDict = self.instruction[id])
                    if self.instruction[id]['rotate']:
                        rot = float(self.instruction[id]['rotate']) 
                    else:
                        rot = 0

                    self.instruction[id]['rect'] = bounds = self.parent.getModifiedTextBounds(coords[0], coords[1], extent, rot)
                    self.pdcObj.SetIdBounds(id, bounds)
                else:
                    self.Draw(pen = self.pen[type], brush = self.brush[type], pdc = self.pdcObj,
                              drawid = id, pdctype = 'rectText', bb = oRect)
            #redraw tmp objects
            if self.dragId != -1:
                self.RedrawSelectBox(self.dragId)
                
        #redraw preview
        else: # preview mode    
            imageRect = self.pdcImage.GetIdBounds(self.imageId)
            imageRect.OffsetXY(-view[0], -view[1])
            imageRect = self.ScaleRect(rect = imageRect, scale = zoomFactor)
            self.DrawImage(imageRect)
        
    def ZoomAll(self):
        """! Zoom to full extent"""  
        if not self.preview:
            bounds = self.pdcPaper.GetIdBounds(self.pageId)
        else:
            bounds = self.pdcImage.GetIdBounds(self.imageId)
        zoomP = bounds.Inflate(bounds.width/20, bounds.height/20)
        zoomFactor, view = self.ComputeZoom(zoomP)
        self.Zoom(zoomFactor, view)
        
    def Draw(self, pen, brush, pdc, drawid = None, pdctype = 'rect', bb = wx.Rect(0,0,0,0)): 
        """! Draw object"""    
        if drawid is None:
            drawid = wx.NewId()
        bb = bb.Get()
        pdc.BeginDrawing()
        pdc.RemoveId(drawid)
        pdc.SetId(drawid)
        pdc.SetPen(pen)
        pdc.SetBrush(brush)
        if pdctype in ('rect', 'rectText'):
            pdc.DrawRectangle(*bb)
        if pdctype == 'rectText':
            dc = wx.PaintDC(self) # dc created because of method GetTextExtent, which pseudoDC lacks
            font = self.font
            size = 10
            font.SetPointSize(size)
            font.SetStyle(wx.ITALIC)
            dc.SetFont(font)
            pdc.SetFont(font)
            text = '\n'.join(self.itemLabels[self.instruction[drawid].type])
            w,h,lh = dc.GetMultiLineTextExtent(text)
            textExtent = (w,h)
            textRect = wx.Rect(0, 0, *textExtent).CenterIn(bb)
            r = map(int, bb)
            while not wx.Rect(*r).ContainsRect(textRect) and size >= 8:
                size -= 2
                font.SetPointSize(size)
                dc.SetFont(font)
                pdc.SetFont(font)
                w,h,lh = dc.GetMutiLineTextExtent(text)
                textExtent = (w,h)
                textRect = wx.Rect(0, 0, *textExtent).CenterIn(bb)
            pdc.SetTextForeground(wx.Color(100,100,100,200)) 
            pdc.SetBackgroundMode(wx.TRANSPARENT)
            pdc.DrawText(text = text, x = textRect.x, y = textRect.y)
            
        pdc.SetIdBounds(drawid, bb)
        pdc.EndDrawing()
        self.Refresh()

        return drawid
    
    def DrawRotText(self, pdc, drawId, textDict, coords, bounds):
        if textDict['rotate']:
            rot = float(textDict['rotate']) 
        else:
            rot = 0

        fontsize = textDict['fontsize'] * self.currScale
        if textDict['background'] != 'none':
            background = textDict['background'] 
        else:
            background = None

        pdc.RemoveId(drawId)
        pdc.SetId(drawId)
        pdc.BeginDrawing()
        # doesn't work
        if background:
            pdc.SetBackground(wx.Brush(convertRGB(background)))
            pdc.SetBackgroundMode(wx.SOLID)
        else:
            pdc.SetBackground(wx.TRANSPARENT_BRUSH)
            pdc.SetBackgroundMode(wx.TRANSPARENT)
 
        fn = self.parent.makePSFont(textDict)

        pdc.SetFont(fn)
        pdc.SetTextForeground(convertRGB(textDict['color']))        
        pdc.DrawRotatedText(textDict['text'], coords[0], coords[1], rot)
        pdc.SetIdBounds(drawId, wx.Rect(*bounds))
        self.Refresh()
        pdc.EndDrawing()
        
    def DrawImage(self, rect):
        """!Draw preview image to pseudoDC"""
        self.pdcImage.ClearId(self.imageId)
        self.pdcImage.SetId(self.imageId)
        img = self.image
        

        if img.GetWidth() != rect.width or img.GetHeight() != rect.height:
            img = img.Scale(rect.width, rect.height)
        bitmap = img.ConvertToBitmap()
        
        self.pdcImage.BeginDrawing()
        self.pdcImage.DrawBitmap(bitmap, rect.x, rect.y)
        self.pdcImage.SetIdBounds(self.imageId, rect)
        self.pdcImage.EndDrawing()
        self.Refresh()
        
    def DrawPaper(self, rect):
        """!Draw paper and margins"""
        page = self.instruction[self.pageId]
        scale = page['Width'] / rect.GetWidth()
        w = (page['Width'] - page['Right'] - page['Left']) / scale
        h = (page['Height'] - page['Top'] - page['Bottom']) / scale
        x = page['Left'] / scale + rect.GetX()
        y = page['Top'] / scale + rect.GetY()
        
        self.pdcPaper.BeginDrawing()
        self.pdcPaper.RemoveId(self.pageId)
        self.pdcPaper.SetId(self.pageId)
        self.pdcPaper.SetPen(self.pen['paper'])
        self.pdcPaper.SetBrush(self.brush['paper'])
        self.pdcPaper.DrawRectangleRect(rect)
        
        self.pdcPaper.SetPen(self.pen['margins'])
        self.pdcPaper.SetBrush(self.brush['margins'])
        self.pdcPaper.DrawRectangle(x, y, w, h)
        
        self.pdcPaper.SetIdBounds(self.pageId, rect)
        self.pdcPaper.EndDrawing()
        self.Refresh()

        
    def ImageRect(self):
        """!Returns image centered in canvas, computes scale"""
        img = wx.Image(self.imgName, wx.BITMAP_TYPE_PNG)
        cW, cH = self.GetClientSize()
        iW, iH = img.GetWidth(), img.GetHeight()

        self.currScale = min(float(cW)/iW, float(cH)/iH)
        iW = iW * self.currScale
        iH = iH * self.currScale
        x = cW/2 - iW/2
        y = cH/2 - iH/2
        imageRect = wx.Rect(x, y, iW, iH)

        return imageRect 
    
    def RedrawSelectBox(self, id):
        """!Redraws select box when selected object changes its size"""
        if self.dragId == id:
            rect = [self.pdcObj.GetIdBounds(id).Inflate(3,3)]
            type = ['select']
            ids = [self.idBoxTmp]
            if self.instruction[id].type == 'map':
                controlP = self.pdcObj.GetIdBounds(id).GetBottomRight()
                rect.append(wx.Rect(controlP.x, controlP.y, 10,10))
                type.append('resize')
                ids.append(self.idResizeBoxTmp)
            for id, type, rect in zip(ids, type, rect):
                self.Draw(pen = self.pen[type], brush = self.brush[type], pdc = self.pdcTmp,
                          drawid = id, pdctype = 'rect', bb = rect)
        
    def UpdateMapLabel(self):
        """!Updates map frame label"""

        vector = self.instruction.FindInstructionByType('vector')
        if vector:
            vectorId = vector.id 
        else:
            vectorId = None

        raster = self.instruction.FindInstructionByType('raster')
        if raster:
            rasterId = raster.id 
        else:
            rasterId = None

        rasterName = 'None'
        if rasterId:
            rasterName = self.instruction[rasterId]['raster'].split('@')[0]
            
        self.itemLabels['map'] = self.itemLabels['map'][0:1]
        self.itemLabels['map'].append("raster: " + rasterName)
        if vectorId: 
            for map in self.instruction[vectorId]['list']:
                self.itemLabels['map'].append('vector: ' + map[0].split('@')[0])
            
    def OnSize(self, event):
        """!Init image size to match window size
        """
        # not zoom all when notebook page is changed
        if self.preview and self.parent.currentPage == 1 or not self.preview and self.parent.currentPage == 0:
            self.ZoomAll()
        self.OnIdle(None)
        event.Skip()
        
    def OnIdle(self, event):
        """!Only re-render a image during idle time instead of
        multiple times during resizing.
        """ 
        
        width, height = self.GetClientSize()
        # Make new off screen bitmap: this bitmap will always have the
        # current drawing in it, so it can be used to save the image
        # to a file, or whatever.
        self._buffer = wx.EmptyBitmap(width, height)
        # re-render image on idle
        self.resize = True

        
    def ScaleRect(self, rect, scale):
        """! Scale rectangle"""
        return wx.Rect(rect.GetLeft()*scale, rect.GetTop()*scale,
                       rect.GetSize()[0]*scale, rect.GetSize()[1]*scale)   
    
def main():
    import gettext
    gettext.install('grasswxpy', os.path.join(os.getenv("GISBASE"), 'locale'), unicode = True)
    
    app = wx.PySimpleApp()
    wx.InitAllImageHandlers()
    frame = PsMapFrame()
    frame.Show()
    
    app.MainLoop()

if __name__ == "__main__":
    main()
