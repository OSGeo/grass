"""!
@package psmap.frame

@brief GUI for ps.map

Classes:
 - frame::PsMapFrame
 - frame::PsMapBufferedWindow

(C) 2011-2012 by Anna Kratochvilova, and the GRASS Development Team
This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com> (bachelor's project)
@author Martin Landa <landa.martin gmail.com> (mentor)
"""

import os
import sys
import textwrap
import Queue
from math import sin, cos, pi, sqrt

import wx

try:
    import wx.lib.agw.flatnotebook as fnb
except ImportError:
    import wx.lib.flatnotebook as fnb

import grass.script as grass

from core               import globalvar
from gui_core.menu      import Menu
from core.gconsole      import CmdThread, EVT_CMD_DONE
from psmap.toolbars     import PsMapToolbar
from core.gcmd          import RunCommand, GError, GMessage
from core.settings      import UserSettings
from gui_core.forms     import GUI
from gui_core.dialogs   import HyperlinkDialog
from psmap.menudata     import PsMapMenuData

from psmap.dialogs      import *
from psmap.instructions import *
from psmap.utils        import *

class PsMapFrame(wx.Frame):
    def __init__(self, parent = None, id = wx.ID_ANY,
                 title = _("GRASS GIS Cartographic Composer"), **kwargs):
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
        self.menubar = Menu(parent = self, model = PsMapMenuData().GetModel())
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
            'map': wx.Pen(colour = wx.Colour(86, 122, 17), width = 2),
            'rasterLegend': wx.Pen(colour = wx.Colour(219, 216, 4), width = 2),
            'vectorLegend': wx.Pen(colour = wx.Colour(219, 216, 4), width = 2),
            'mapinfo': wx.Pen(colour = wx.Colour(5, 184, 249), width = 2),
            'scalebar': wx.Pen(colour = wx.Colour(150, 150, 150), width = 2),
            'image': wx.Pen(colour = wx.Colour(255, 150, 50), width = 2),
            'northArrow': wx.Pen(colour = wx.Colour(200, 200, 200), width = 2),
            'point': wx.Pen(colour = wx.Colour(100, 100, 100), width = 2),
            'line': wx.Pen(colour = wx.Colour(0, 0, 0), width = 2),
            'box': wx.Pen(colour = 'RED', width = 2, style = wx.SHORT_DASH),
            'select': wx.Pen(colour = 'BLACK', width = 1, style = wx.SHORT_DASH),
            'resize': wx.Pen(colour = 'BLACK', width = 1)
            }
        self.brush = {
            'paper': wx.WHITE_BRUSH,
            'margins': wx.TRANSPARENT_BRUSH,
            'map': wx.Brush(wx.Colour(151, 214, 90)),
            'rasterLegend': wx.Brush(wx.Colour(250, 247, 112)),
            'vectorLegend': wx.Brush(wx.Colour(250, 247, 112)),
            'mapinfo': wx.Brush(wx.Colour(127, 222, 252)),
            'scalebar': wx.Brush(wx.Colour(200, 200, 200)),
            'image': wx.Brush(wx.Colour(255, 200, 50)),
            'northArrow': wx.Brush(wx.Colour(255, 255, 255)),
            'point': wx.Brush(wx.Colour(200, 200, 200)),
            'line': wx.TRANSPARENT_BRUSH,
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
        self.imgName = grass.tempfile()
        
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
        self.SetMinSize(wx.Size(775, 600))
        
        self.Bind(fnb.EVT_FLATNOTEBOOK_PAGE_CHANGING, self.OnPageChanging)
        self.Bind(fnb.EVT_FLATNOTEBOOK_PAGE_CHANGED, self.OnPageChanged)
        self.Bind(wx.EVT_CLOSE, self.OnCloseWindow)
        self.Bind(EVT_CMD_DONE, self.OnCmdDone)
        
        if not havePILImage:
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
        if havePILImage and event.userData['temp'] and not event.userData['pdfname']:
            RunCommand('g.region', cols = event.userData['regionOld']['cols'], rows = event.userData['regionOld']['rows'])
            # wx.BusyInfo does not display the message
            busy = wx.BusyInfo(message = _("Generating preview, wait please"), parent = self)
            wx.Yield()
            try:
                im = PILImage.open(event.userData['filename'])
                if self.instruction[self.pageId]['Orientation'] == 'Landscape':
                    im = im.rotate(270)

                # hack for Windows, change method for loading EPS
                if sys.platform == 'win32':
                    import types
                    im.load = types.MethodType(loadPSForWindows, im)
                im.save(self.imgName, format = 'PNG')
            except IOError, e:
                del busy
                dlg = HyperlinkDialog(self, title=_("Preview not available"),
                                      message=_("Preview is not available probably due to missing Ghostscript."),
                                      hyperlink='http://trac.osgeo.org/grass/wiki/CompileOnWindows#Ghostscript',
                                      hyperlinkLabel=_("Please follow instructions on GRASS Trac Wiki."))
                dlg.ShowModal()
                dlg.Destroy()
                return
            
                
            rect = self.previewCanvas.ImageRect()
            self.previewCanvas.image = wx.Image(self.imgName, wx.BITMAP_TYPE_PNG)
            self.previewCanvas.DrawImage(rect = rect)
            
            del busy
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
        """!Launch file dialog and load selected file"""
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
        self.LoadFile(filename)

    def LoadFile(self, filename):
        """!Load file and read instructions"""
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
            self.canvas.SetPage()
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
            self.getInitMap()
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
            self.cursorOld = self.previewCanvas.GetCursor()
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
        
    def OnAddImage(self, event, id = None):
        """!Show dialog for image adding and editing"""
        position = None
        if 'image' in self.openDialogs:
            position = self.openDialogs['image'].GetPosition()
            self.openDialogs['image'].OnApply(event = None)
            self.openDialogs['image'].Destroy()
        dlg = ImageDialog(self, id = id, settings = self.instruction)
        self.openDialogs['image'] = dlg 
        if position: 
            dlg.SetPosition(position)
        dlg.Show()
        
    def OnAddNorthArrow(self, event, id = None):
        """!Show dialog for north arrow adding and editing"""
        if self.instruction.FindInstructionByType('northArrow'):
            id = self.instruction.FindInstructionByType('northArrow').id
        else: id = None
        
        if 'northArrow' not in self.openDialogs:
            dlg = NorthArrowDialog(self, id = id, settings = self.instruction)
            self.openDialogs['northArrow'] = dlg
        self.openDialogs['northArrow'].Show()
        
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
        
    def OnAddPoint(self, event):
        """!Add point action selected"""
        self.mouse["use"] = "addPoint"
        self.canvas.SetCursor(self.cursors["cross"])
        
    def AddPoint(self, id = None, coordinates = None):
        """!Add point and open property dialog.

        @param id id point id (None if creating new point)
        @param coordinates coordinates of new point
        """
        position = None
        if 'point' in self.openDialogs:
            position = self.openDialogs['point'].GetPosition()
            self.openDialogs['point'].OnApply(event = None)
            self.openDialogs['point'].Destroy()
        dlg = PointDialog(self, id = id, settings = self.instruction,
                          coordinates = coordinates)
        self.openDialogs['point'] = dlg
        if position: 
            dlg.SetPosition(position)
        if coordinates:
            dlg.OnApply(event = None)
        dlg.Show()
        
    def OnAddLine(self, event):
        """!Add line action selected"""
        self.mouse["use"] = "addLine"
        self.canvas.SetCursor(self.cursors["cross"])

    def AddLine(self, id = None, coordinates = None):
        """!Add line and open property dialog.
        
        @param id id line id (None if creating new line)
        @param coordinates coordinates of new line
        """
        position = None
        if 'line' in self.openDialogs:
            position = self.openDialogs['line'].GetPosition()
            self.openDialogs['line'].OnApply(event = None)
            self.openDialogs['line'].Destroy()
        dlg = RectangleDialog(self, id = id, settings = self.instruction,
                              type = 'line', coordinates = coordinates)
        self.openDialogs['line'] = dlg
        if position: 
            dlg.SetPosition(position)
        if coordinates:
            dlg.OnApply(event = None)
        dlg.Show()

    def OnAddRectangle(self, event):
        """!Add rectangle action selected"""
        self.mouse["use"] = "addRectangle"
        self.canvas.SetCursor(self.cursors["cross"])

    def AddRectangle(self, id = None, coordinates = None):
        """!Add rectangle and open property dialog.
        
        @param id id rectangle id (None if creating new rectangle)
        @param coordinates coordinates of new rectangle
        """
        position = None
        if 'rectangle' in self.openDialogs:
            position = self.openDialogs['rectangle'].GetPosition()
            self.openDialogs['rectangle'].OnApply(event = None)
            self.openDialogs['rectangle'].Destroy()
        dlg = RectangleDialog(self, id = id, settings = self.instruction,
                              type = 'rectangle', coordinates = coordinates)
        self.openDialogs['rectangle'] = dlg
        if position: 
            dlg.SetPosition(position)
        if coordinates:
            dlg.OnApply(event = None)
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
        """!creates a wx.Font object from selected postscript font. To be
        used for estimating bounding rectangle of text"""
        
        fontsize = textDict['fontsize'] * self.canvas.currScale
        fontface = textDict['font'].split('-')[0]
        try:
            fontstyle = textDict['font'].split('-')[1]
        except IndexError:
            fontstyle = ''
        
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
            
        style = wx.FONTSTYLE_NORMAL
        weight = wx.FONTWEIGHT_NORMAL
            
        if 'Oblique' in fontstyle:
            style =  wx.FONTSTYLE_SLANT
            
        if 'Italic' in fontstyle:
            style =  wx.FONTSTYLE_ITALIC
            
        if 'Bold' in fontstyle:
            weight = wx.FONTWEIGHT_BOLD
        
        try:
            fn = wx.Font(pointSize = fontsize, family = family, style = style,
                         weight = weight, face = face)
        except:
            fn = wx.Font(pointSize = fontsize, family = wx.FONTFAMILY_DEFAULT, 
                         style = wx.FONTSTYLE_NORMAL, weight = wx.FONTWEIGHT_NORMAL)

        return fn
       
       
    def getTextExtent(self, textDict):
        """!Estimates bounding rectangle of text"""
        #fontsize = str(fontsize if fontsize >= 4 else 4)
        dc = wx.ClientDC(self) # dc created because of method GetTextExtent, which pseudoDC lacks
       
        fn = self.makePSFont(textDict)

        try:
            dc.SetFont(fn)
            w,h,lh = dc.GetMultiLineTextExtent(textDict['text'])
            return (w,h)
        except:
            return (0,0)
    
    def getInitMap(self):
        """!Create default map frame when no map is selected, needed for coordinates in map units"""
        instrFile = grass.tempfile()
        instrFileFd = open(instrFile, mode = 'w')
        instrFileFd.write(self.InstructionFile())
        instrFileFd.flush()
        instrFileFd.close()
        
        page = self.instruction.FindInstructionByType('page')
        mapInitRect = GetMapBounds(instrFile, portrait = (page['Orientation'] == 'Portrait'))
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
            
            if itype in ('scalebar', 'mapinfo', 'image'):
                drawRectangle = self.canvas.CanvasPaperCoordinates(rect = self.instruction[id]['rect'], canvasToPaper = False)
                self.canvas.UpdateLabel(itype = itype, id = id)
                self.canvas.Draw(pen = self.pen[itype], brush = self.brush[itype],
                                 pdc = self.canvas.pdcObj, drawid = id, pdctype = 'rectText', bb = drawRectangle)
                self.canvas.RedrawSelectBox(id)
            if itype == 'northArrow':
                self.canvas.UpdateLabel(itype = itype, id = id)
                drawRectangle = self.canvas.CanvasPaperCoordinates(rect = self.instruction[id]['rect'], canvasToPaper = False)
                self.canvas.Draw(pen = self.pen[itype], brush = self.brush[itype],
                                 pdc = self.canvas.pdcObj, drawid = id, pdctype = 'bitmap', bb = drawRectangle)
                self.canvas.RedrawSelectBox(id)

            if itype in ('point', 'line', 'rectangle'):
                drawRectangle = self.canvas.CanvasPaperCoordinates(rect = self.instruction[id]['rect'], canvasToPaper = False)
                # coords only for line
                coords = None
                if itype == 'line':
                    point1 = self.instruction[id]['where'][0]
                    point2 = self.instruction[id]['where'][1]
                    point1Coords = self.canvas.CanvasPaperCoordinates(rect = Rect2DPS(point1, (0, 0)), canvasToPaper = False)[:2]
                    point2Coords = self.canvas.CanvasPaperCoordinates(rect = Rect2DPS(point2, (0, 0)), canvasToPaper = False)[:2]
                    coords = (point1Coords, point2Coords)

                # fill color is not in line
                fcolor = None
                if 'fcolor' in self.instruction[id].GetInstruction():
                    fcolor = self.instruction[id]['fcolor']
                # width is not in point
                width = None
                if 'width' in self.instruction[id].GetInstruction():
                    width = self.instruction[id]['width']

                self.canvas.DrawGraphics(drawid = id, color = self.instruction[id]['color'], shape = itype,
                                       fcolor = fcolor, width = width, bb = drawRectangle, lineCoords = coords)

                self.canvas.RedrawSelectBox(id)

            if itype == 'text':
                
                if self.instruction[id]['rotate']:
                    rot = float(self.instruction[id]['rotate']) 
                else:
                    rot = 0

                extent = self.getTextExtent(textDict = self.instruction[id].GetInstruction())
                rect = Rect2DPS(self.instruction[id]['where'], (0, 0))
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
                    info = grass.raster_info(self.instruction[id]['raster'])
                    RunCommand('g.region', nsres = info['nsres'], ewres = info['ewres'])
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
                    self.canvas.UpdateLabel(itype = itype, id = id)
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
                    self.canvas.UpdateLabel(itype = itype, id = id)
                    drawRectangle = self.canvas.CanvasPaperCoordinates(rect = self.instruction[id]['rect'], canvasToPaper = False)
                    self.canvas.Draw(pen = self.pen[itype], brush = self.brush[itype],
                                     pdc = self.canvas.pdcObj, drawid = id, pdctype = 'rectText', bb = drawRectangle)
                    self.canvas.RedrawSelectBox(id)

                else:
                    self.deleteObject(id)
                
    def OnPageChanged(self, event):
        """!Flatnotebook page has changed"""
        self.currentPage = self.book.GetPageIndex(self.book.GetCurrentPage())
        if self.currentPage == 1:
            self.SetStatusText(_("Press button with green triangle icon to generate preview."))
        else:
            self.SetStatusText('')

        
        
    def OnPageChanging(self, event):
        """!Flatnotebook page is changing"""
        if self.currentPage == 0 and self.mouse['use'] == 'addMap':
            event.Veto()

    def OnHelp(self, event):
        """!Show help"""
        if self.parent and self.parent.GetName() == 'LayerManager':
            log = self.parent.GetLogWindow()
            log.RunCmd(['g.manual',
                        'entry=wxGUI.psmap'])
        else:
            RunCommand('g.manual',
                       quiet = True,
                       entry = 'wxGUI.psmap')
        
    def OnAbout(self, event):
        """!Display About window"""
        info = wx.AboutDialogInfo()
        
        info.SetIcon(wx.Icon(os.path.join(globalvar.ETCICONDIR, 'grass.ico'), wx.BITMAP_TYPE_ICO))
        info.SetName(_('wxGUI Cartographic Composer'))
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
        self.itemLabelsDict = { 'map': 'MAP FRAME',
                                'rasterLegend': 'RASTER LEGEND',
                                'vectorLegend': 'VECTOR LEGEND',
                                'mapinfo': 'MAP INFO',
                                'scalebar': 'SCALE BAR',
                                'image': 'IMAGE',
                                'northArrow': 'NORTH ARROW'}
        self.itemLabels = {}
        
        # define PseudoDC
        self.pdc = wx.PseudoDC()
        self.pdcObj = wx.PseudoDC()
        self.pdcPaper = wx.PseudoDC()
        self.pdcTmp = wx.PseudoDC()
        self.pdcImage = wx.PseudoDC()
        
        self.SetClientSize((700,510))#?
        self._buffer = wx.EmptyBitmap(*self.GetClientSize())
        
        self.idBoxTmp = wx.NewId()
        self.idZoomBoxTmp = wx.NewId()
        self.idResizeBoxTmp = wx.NewId()
        self.idLinePointsTmp = (wx.NewId(), wx.NewId()) # ids of marks for moving line vertices

        self.resizeBoxSize = wx.Size(8, 8)
        
        

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
        # self.Bind(wx.EVT_MOUSE_EVENTS, self.OnMouse)
        self.Bind(wx.EVT_MOUSE_EVENTS, self.MouseActions)


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
        Width = units.convert(value = rect.GetWidth(), fromUnit = fromU, toUnit = toU) * scale
        Height = units.convert(value = rect.GetHeight(), fromUnit = fromU, toUnit = toU) * scale
        X = units.convert(value = (rect.GetX() - pRectx), fromUnit = fromU, toUnit = toU) * scale
        Y = units.convert(value = (rect.GetY() - pRecty), fromUnit = fromU, toUnit = toU) * scale

        return Rect2D(X, Y, Width, Height)

    
    
    def SetPage(self):
        """!Sets and changes page, redraws paper"""
        
        page = self.instruction[self.pageId]
        if not page:
            page = PageSetup(id = self.pageId)
            self.instruction.AddInstruction(page)
        
        ppi = wx.ClientDC(self).GetPPI()
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
        
        for itemType in ('text', 'image', 'northArrow', 'point', 'line', 'rectangle'):
            items = self.instruction.FindInstructionByType(itemType, list = True)
            for item in items:
                instr = self.instruction[item.id]
                if itemType in ('line', 'rectangle'):
                    if itemType == 'line':
                        e1, n1 = PaperMapCoordinates(mapInstr = self.instruction[mapId], x = instr['where'][0][0],
                                                     y = instr['where'][0][1], paperToMap = True)
                        e2, n2 = PaperMapCoordinates(mapInstr = self.instruction[mapId], x = instr['where'][1][0],
                                                     y = instr['where'][1][1], paperToMap = True)
                    else: 
                        e1, n1 = PaperMapCoordinates(mapInstr = self.instruction[mapId], x = instr['rect'].GetLeft(),
                                                     y = instr['rect'].GetTop(), paperToMap = True)
                        e2, n2 = PaperMapCoordinates(mapInstr = self.instruction[mapId], x = instr['rect'].GetRight(),
                                                     y = instr['rect'].GetBottom(), paperToMap = True)
                    instr['east1'] = e1
                    instr['north1'] = n1
                    instr['east2'] = e2
                    instr['north2'] = n2
                else:
                    e, n = PaperMapCoordinates(mapInstr = self.instruction[mapId], x = instr['where'][0],
                                               y = instr['where'][1], paperToMap = True)
                    instr['east'], instr['north'] = e, n
                
    def OnPaint(self, event):
        """!Draw pseudo DC to buffer
        """
        if not self._buffer:
            return
        dc = wx.BufferedPaintDC(self, self._buffer)
        # use PrepareDC to set position correctly
        # probably does nothing, removed from wxPython 2.9
        # self.PrepareDC(dc)
        
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
        
    def MouseActions(self, event):
        """!Mouse motion and button click notifier
        """
        # zoom with mouse wheel
        if event.GetWheelRotation() != 0:
            self.OnMouseWheel(event)
            
        # left mouse button pressed
        elif event.LeftDown():
            self.OnLeftDown(event)
        
        # left mouse button released
        elif event.LeftUp():
            self.OnLeftUp(event)
        
        # dragging
        elif event.Dragging():
            self.OnDragging(event)
        
        # double click
        elif event.ButtonDClick():
            self.OnButtonDClick(event)
        
        # middle mouse button pressed
        elif event.MiddleDown():
            self.OnMiddleDown(event)
        
        elif event.Moving():
            self.OnMouseMoving(event)
                
    def OnMouseWheel(self, event):
        """!Mouse wheel scrolled.

        Changes zoom."""
        if UserSettings.Get(group = 'display',
                            key = 'mouseWheelZoom',
                            subkey = 'selection') == 2:
            event.Skip()
            return

        zoom = event.GetWheelRotation()
        oldUse = self.mouse['use']
        self.mouse['begin'] = event.GetPosition()
        
        if UserSettings.Get(group = 'display',
                            key = 'scrollDirection',
                            subkey = 'selection'):
            zoom *= -1
            
        if zoom > 0:
            self.mouse['use'] = 'zoomin'
        else:
            self.mouse['use'] = 'zoomout'
            
        zoomFactor, view = self.ComputeZoom(wx.Rect(0, 0, 0, 0))
        self.Zoom(zoomFactor, view)
        self.mouse['use'] = oldUse

    def OnMouseMoving(self, event):
        """!Mouse cursor moving.

        Change cursor when moving over resize marker.
        """
        if self.preview:
            return

        if self.mouse['use'] in ('pointer', 'resize'):
            pos = event.GetPosition()
            foundResize = self.pdcTmp.FindObjects(pos[0], pos[1])
            if foundResize and foundResize[0] in (self.idResizeBoxTmp,) + self.idLinePointsTmp:
                self.SetCursor(self.cursors["sizenwse"])
                self.parent.SetStatusText(_('Click and drag to resize object'), 0)
            else:
                self.parent.SetStatusText('', 0)
                self.SetCursor(self.cursors["default"])
                
    def OnLeftDown(self, event):
        """!Left mouse button pressed.

        Select objects, redraw, prepare for moving/resizing.
        """
        self.mouse['begin'] = event.GetPosition()
        self.begin = self.mouse['begin']
        
        # select
        if self.mouse['use'] == 'pointer':
            found = self.pdcObj.FindObjects(self.mouse['begin'][0], self.mouse['begin'][1])
            foundResize = self.pdcTmp.FindObjects(self.mouse['begin'][0], self.mouse['begin'][1])

            if foundResize and foundResize[0] in (self.idResizeBoxTmp,) + self.idLinePointsTmp:
                self.mouse['use'] = 'resize'
                
                # when resizing, proportions match region
                if self.instruction[self.dragId].type == 'map':
                    self.constraint = False
                    self.mapBounds = self.pdcObj.GetIdBounds(self.dragId)
                    if self.instruction[self.dragId]['scaleType'] in (0, 1, 2):
                        self.constraint = True
                        self.mapBounds = self.pdcObj.GetIdBounds(self.dragId)

                if self.instruction[self.dragId].type == 'line':
                    self.currentLinePoint = self.idLinePointsTmp.index(foundResize[0])
                
            elif found:
                self.dragId = found[0]
                self.RedrawSelectBox(self.dragId)
                if self.instruction[self.dragId].type not in ('map', 'rectangle'):
                    self.pdcTmp.RemoveId(self.idResizeBoxTmp)
                    self.Refresh()
                if self.instruction[self.dragId].type != 'line':
                    for id in self.idLinePointsTmp:
                        self.pdcTmp.RemoveId(id)
                    self.Refresh()
                    
            else:
                self.dragId = -1
                self.pdcTmp.RemoveId(self.idBoxTmp)
                self.pdcTmp.RemoveId(self.idResizeBoxTmp)
                for id in self.idLinePointsTmp:
                    self.pdcTmp.RemoveId(id)
                self.Refresh()

    def OnLeftUp(self, event):
        """!Left mouse button released.

        Recalculate zooming/resizing/moving and redraw.
        """
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
            return

        # resize resizable objects (map, line, rectangle)
        if self.mouse['use'] == 'resize':
            mapObj = self.instruction.FindInstructionByType('map')
            if not mapObj:
                mapObj = self.instruction.FindInstructionByType('initMap')
            mapId = mapObj.id
            
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

            elif self.instruction[self.dragId].type == 'line':
                points = self.instruction[self.dragId]['where']
                self.instruction[self.dragId]['rect'] = Rect2DPP(points[0], points[1])
                self.RecalculatePosition(ids = [self.dragId])

            elif self.instruction[self.dragId].type == 'rectangle':
                self.RecalculatePosition(ids = [self.dragId])

            self.mouse['use'] = 'pointer'
            
        # recalculate the position of objects after dragging
        if self.mouse['use'] in ('pointer', 'resize') and self.dragId != -1:
            if self.mouse['begin'] != event.GetPosition(): #for double click
                
                self.RecalculatePosition(ids = [self.dragId])
                if self.instruction[self.dragId].type in self.openDialogs:
                    self.openDialogs[self.instruction[self.dragId].type].updateDialog()
        
        elif self.mouse['use'] in ('addPoint', 'addLine', 'addRectangle'):
            endCoordinates = self.CanvasPaperCoordinates(rect = wx.Rect(event.GetX(), event.GetY(), 0, 0),
                                                         canvasToPaper = True)[:2]

            diffX = event.GetX() - self.mouse['begin'][0]
            diffY = event.GetY() - self.mouse['begin'][1]

            if self.mouse['use'] == 'addPoint':
                self.parent.AddPoint(coordinates = endCoordinates)
            elif self.mouse['use'] in ('addLine', 'addRectangle'):
                # not too small lines/rectangles
                if sqrt(diffX * diffX + diffY * diffY) < 5:
                    self.pdcTmp.RemoveId(self.idZoomBoxTmp)
                    self.Refresh()
                    return

                beginCoordinates = self.CanvasPaperCoordinates(rect = wx.Rect(self.mouse['begin'][0],
                                                                              self.mouse['begin'][1], 0, 0),
                                                               canvasToPaper = True)[:2]
                if self.mouse['use'] == 'addLine':
                    self.parent.AddLine(coordinates = [beginCoordinates, endCoordinates])
                else:
                    self.parent.AddRectangle(coordinates = [beginCoordinates, endCoordinates])
                self.pdcTmp.RemoveId(self.idZoomBoxTmp)
                self.Refresh()

    def OnButtonDClick(self, event):
        """!Open object dialog for editing."""
        if self.mouse['use'] == 'pointer' and self.dragId != -1:
            itemCall = {'text':self.parent.OnAddText,
                        'mapinfo': self.parent.OnAddMapinfo,
                        'scalebar': self.parent.OnAddScalebar,
                        'image': self.parent.OnAddImage,
                        'northArrow' : self.parent.OnAddNorthArrow,
                        'point': self.parent.AddPoint,
                        'line': self.parent.AddLine,
                        'rectangle': self.parent.AddRectangle,
                        'rasterLegend': self.parent.OnAddLegend,
                        'vectorLegend': self.parent.OnAddLegend,
                        'map': self.parent.OnAddMap}

            itemArg = { 'text': dict(event = None, id = self.dragId),
                        'mapinfo': dict(event = None),
                        'scalebar': dict(event = None),
                        'image': dict(event = None, id = self.dragId),
                        'northArrow': dict(event = None, id = self.dragId),
                        'point': dict(id = self.dragId),
                        'line': dict(id = self.dragId),
                        'rectangle': dict(id = self.dragId),
                        'rasterLegend': dict(event = None),
                        'vectorLegend': dict(event = None, page = 1),
                        'map': dict(event = None, notebook = True)}

            type = self.instruction[self.dragId].type
            itemCall[type](**itemArg[type])

    def OnDragging(self, event):
        """!Process panning/resizing/drawing/moving."""
        if event.MiddleIsDown():
            # panning
            self.mouse['end'] = event.GetPosition()
            self.Pan(begin = self.mouse['begin'], end = self.mouse['end'])
            self.mouse['begin'] = event.GetPosition()

        elif event.LeftIsDown():
            # draw box when zooming, creating map 
            if self.mouse['use'] in ('zoomin', 'zoomout', 'addMap', 'addLine', 'addRectangle'):
                self.mouse['end'] = event.GetPosition()
                r = wx.Rect(self.mouse['begin'][0], self.mouse['begin'][1],
                            self.mouse['end'][0]-self.mouse['begin'][0], self.mouse['end'][1]-self.mouse['begin'][1])
                r = self.modifyRectangle(r)

                if self.mouse['use'] in ('addLine', 'addRectangle'):
                    if self.mouse['use'] == 'addLine':
                        pdcType = 'line'
                        lineCoords = (self.mouse['begin'], self.mouse['end'])
                    else:
                        pdcType = 'rect'
                        lineCoords = None
                        if r[2] < 2 or r[3] < 2:
                            # to avoid strange behavoiur
                            return

                    self.Draw(pen = self.pen['line'], brush = self.brush['line'],
                              pdc = self.pdcTmp, drawid = self.idZoomBoxTmp,
                              pdctype = pdcType, bb = r, lineCoords = lineCoords)

                else:
                    self.Draw(pen = self.pen['box'], brush = self.brush['box'],
                              pdc = self.pdcTmp, drawid = self.idZoomBoxTmp,
                              pdctype = 'rect', bb = r)

            # panning
            if self.mouse["use"] == 'pan':
                self.mouse['end'] = event.GetPosition()
                self.Pan(begin = self.mouse['begin'], end = self.mouse['end'])
                self.mouse['begin'] = event.GetPosition()

            # move object
            if self.mouse['use'] == 'pointer' and self.dragId != -1:
                self.mouse['end'] = event.GetPosition()
                dx, dy = self.mouse['end'][0] - self.begin[0], self.mouse['end'][1] - self.begin[1]
                self.pdcObj.TranslateId(self.dragId, dx, dy)
                self.pdcTmp.TranslateId(self.idBoxTmp, dx, dy)
                self.pdcTmp.TranslateId(self.idResizeBoxTmp, dx, dy)
                for id in self.idLinePointsTmp:
                    self.pdcTmp.TranslateId(id, dx, dy)
                if self.instruction[self.dragId].type == 'text': 
                    self.instruction[self.dragId]['coords'] = self.instruction[self.dragId]['coords'][0] + dx,\
                        self.instruction[self.dragId]['coords'][1] + dy
                self.begin = event.GetPosition()
                self.Refresh()

            # resize object
            if self.mouse['use'] == 'resize':
                pos = event.GetPosition()
                diffX = pos[0] - self.mouse['begin'][0]
                diffY = pos[1] - self.mouse['begin'][1]
                if self.instruction[self.dragId].type == 'map':
                    x, y = self.mapBounds.GetX(), self.mapBounds.GetY()
                    width, height = self.mapBounds.GetWidth(), self.mapBounds.GetHeight()
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
                    self.Draw(pen = self.pen['map'], brush = self.brush['map'], pdc = self.pdcObj, drawid = self.dragId,
                              pdctype = 'rectText', bb = bounds)

                elif self.instruction[self.dragId].type == 'rectangle':
                    instr = self.instruction[self.dragId]
                    rect = self.CanvasPaperCoordinates(rect = instr['rect'], canvasToPaper = False)
                    rect.SetWidth(rect.GetWidth() + diffX)
                    rect.SetHeight(rect.GetHeight() + diffY)

                    if rect.GetWidth() < 5 or rect.GetHeight() < 5:
                        return

                    self.DrawGraphics(drawid = self.dragId, shape = 'rectangle', color = instr['color'],
                                      fcolor = instr['fcolor'], width = instr['width'], bb = rect)

                elif self.instruction[self.dragId].type == 'line':
                    instr = self.instruction[self.dragId]
                    points = instr['where']
                    # moving point
                    if self.currentLinePoint == 0:
                        pPaper = points[1]
                    else:
                        pPaper = points[0]
                    pCanvas = self.CanvasPaperCoordinates(rect = Rect2DPS(pPaper, (0, 0)),
                                                          canvasToPaper = False)[:2]
                    bounds = wx.RectPP(pCanvas, pos)
                    self.DrawGraphics(drawid = self.dragId, shape = 'line', color = instr['color'],
                                      width = instr['width'], bb = bounds, lineCoords = (pos, pCanvas))

                    # update paper coordinates
                    points[self.currentLinePoint] = self.CanvasPaperCoordinates(rect = wx.RectPS(pos, (0, 0)),
                                                                                canvasToPaper = True)[:2]

                self.RedrawSelectBox(self.dragId)

    def OnMiddleDown(self, event):
        """!Middle mouse button pressed."""
        self.mouse['begin'] = event.GetPosition()

    def Pan(self, begin, end):
        """!Move canvas while dragging.
        
        @param begin x,y coordinates of first point
        @param end x,y coordinates of second point
        """
        view = begin[0] - end[0], begin[1] - end[1]
        zoomFactor = 1
        self.Zoom(zoomFactor, view)
                
    def RecalculatePosition(self, ids):
        for id in ids:
            itype = self.instruction[id].type
            if itype in ('map', 'rectangle'):
                self.instruction[id]['rect'] = self.CanvasPaperCoordinates(rect = self.pdcObj.GetIdBounds(id),
                                                                           canvasToPaper = True)
                self.RecalculateEN()
                
            elif itype in ('mapinfo' ,'rasterLegend', 'vectorLegend', 'image', 'northArrow'):
                self.instruction[id]['rect'] = self.CanvasPaperCoordinates(rect = self.pdcObj.GetIdBounds(id),
                                                                           canvasToPaper = True)
                self.instruction[id]['where'] = self.CanvasPaperCoordinates(rect = self.pdcObj.GetIdBounds(id),
                                                                            canvasToPaper = True)[:2] 
                if itype in ('image', 'northArrow'):
                    self.RecalculateEN()

            elif itype == 'point':
                rect = self.pdcObj.GetIdBounds(id)
                self.instruction[id]['rect'] = self.CanvasPaperCoordinates(rect = rect,
                                                                           canvasToPaper = True)
                rect.OffsetXY(rect.GetWidth()/2, rect.GetHeight()/2)
                self.instruction[id]['where'] = self.CanvasPaperCoordinates(rect = rect,
                                                                            canvasToPaper = True)[:2]
                self.RecalculateEN()

            elif itype == 'line':
                rect = self.pdcObj.GetIdBounds(id)
                oldRect = self.instruction[id]['rect']
                newRect = self.CanvasPaperCoordinates(rect = rect, canvasToPaper = True)
                xDiff = newRect[0] - oldRect[0]
                yDiff = newRect[1] - oldRect[1]
                self.instruction[id]['rect'] = newRect

                point1 = wx.Point2D(xDiff, yDiff) + self.instruction[id]['where'][0]
                point2 = wx.Point2D(xDiff, yDiff) + self.instruction[id]['where'][1]
                self.instruction[id]['where'] = [point1, point2]
                
                self.RecalculateEN()

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
                
                self.instruction[id]['where'] = self.CanvasPaperCoordinates(rect = Rect2D(x, y, 0, 0),
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
            try:
                zoomFactor = 1/max(rW/cW, rH/cH)
            except ZeroDivisionError:
                zoomFactor = 1
            # when zooming to full extent, in some cases, there was zoom 1.01..., which causes problem
            if abs(zoomFactor - 1) > 0.01:
                zoomFactor = zoomFactor 
            else:
                zoomFactor = 1.


            if self.mouse['use'] == 'zoomout':
                zoomFactor = min(rW/cW, rH/cH)
            try:
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
            except ZeroDivisionError:
                xView, yView = rect.GetX(), rect.GetY()
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
                elif type == 'northArrow':
                    self.Draw(pen = self.pen[type], brush = self.brush[type], pdc = self.pdcObj,
                              drawid = id, pdctype = 'bitmap', bb = oRect)

                elif type in ('point', 'line', 'rectangle'):
                    instr = self.instruction[id]
                    color = self.instruction[id]['color']
                    width = fcolor = coords = None

                    if type in ('point', 'rectangle'):
                        fcolor = self.instruction[id]['fcolor']
                    if type in ('line', 'rectangle'):
                        width = self.instruction[id]['width']
                    if type in ('line'):
                        point1, point2 = instr['where'][0], instr['where'][1]
                        point1 = self.CanvasPaperCoordinates(rect = Rect2DPS(point1, (0, 0)),
                                                             canvasToPaper = False)[:2]
                        point2 = self.CanvasPaperCoordinates(rect = Rect2DPS(point2, (0, 0)),
                                                             canvasToPaper = False)[:2]
                        coords = (point1, point2)

                    self.DrawGraphics(drawid = id, shape = type, bb = oRect, lineCoords = coords,
                                    color = color, fcolor = fcolor, width = width)

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
        
    def Draw(self, pen, brush, pdc, drawid = None, pdctype = 'rect', bb = wx.Rect(0,0,0,0), lineCoords = None): 
        """! Draw object with given pen and brush.

        @param pdc PseudoDC
        @param pdctype 'bitmap'/'rectText'/'rect'/'point'/'line'
        @param bb bounding box
        @param lineCoords coordinates of line start, end points (wx.Point, wx.Point)
        """    
        if drawid is None:
            drawid = wx.NewId()
        bb = bb.Get()
        pdc.BeginDrawing()
        pdc.RemoveId(drawid)
        pdc.SetId(drawid)
        pdc.SetPen(pen)
        pdc.SetBrush(brush)
        
        if pdctype == 'bitmap':
            if havePILImage:
                file = self.instruction[drawid]['epsfile']
                rotation = self.instruction[drawid]['rotate']
                self.DrawBitmap(pdc = pdc, filePath = file, rotation = rotation, bbox = bb)
            else: # draw only rectangle with label
                pdctype = 'rectText'
                
        if pdctype in ('rect', 'rectText'):
            pdc.DrawRectangle(*bb)
            
        if pdctype == 'rectText':
            dc = wx.ClientDC(self) # dc created because of method GetTextExtent, which pseudoDC lacks
            font = dc.GetFont()
            size = 10
            font.SetPointSize(size)
            font.SetStyle(wx.ITALIC)
            dc.SetFont(font)
            pdc.SetFont(font)
            text = '\n'.join(self.itemLabels[drawid])
            w,h,lh = dc.GetMultiLineTextExtent(text)
            textExtent = (w,h)
            textRect = wx.Rect(0, 0, *textExtent).CenterIn(bb)
            r = map(int, bb)
            while not wx.Rect(*r).ContainsRect(textRect) and size >= 8:
                size -= 2
                font.SetPointSize(size)
                dc.SetFont(font)
                pdc.SetFont(font)
                textExtent = dc.GetTextExtent(text)
                textRect = wx.Rect(0, 0, *textExtent).CenterIn(bb)
            pdc.SetTextForeground(wx.Colour(100,100,100,200)) 
            pdc.SetBackgroundMode(wx.TRANSPARENT)
            pdc.DrawLabel(text = text, rect = textRect)

        elif pdctype == 'point':
            pdc.DrawCircle(x = bb[0] + bb[2] / 2,
                           y = bb[1] + bb[3] / 2,
                           radius = bb[2] / 2)
                           
        elif pdctype == 'line':
            pdc.DrawLinePoint(lineCoords[0], lineCoords[1])

        pdc.SetIdBounds(drawid, bb)
        pdc.EndDrawing()
        self.Refresh()

        return drawid
    
    def DrawGraphics(self, drawid, shape, color, bb, width = None, fcolor = None, lineCoords = None):
        """!Draw point/line/rectangle with given color and width

        @param drawid id of drawn object
        @param shape drawn shape: 'point'/'line'/'rectangle'
        @param color pen outline color ('RRR:GGG:BBB')
        @param fcolor brush fill color, if meaningful ('RRR:GGG:BBB')
        @param width pen width
        @param bb bounding box
        @param lineCoords line coordinates (for line only)
        """
        pdctype = {'point'     : 'point',
                   'line'      : 'line',
                   'rectangle' : 'rect'}

        if color == 'none':
            pen = wx.TRANSPARENT_PEN
        else:
            if width is not None:
                units = UnitConversion(self)
                width = int(units.convert(value = width, fromUnit = 'point', toUnit = 'pixel') * self.currScale)
            else:
                width = 2
            pen = wx.Pen(colour = convertRGB(color), width = width)
            pen.SetCap(wx.CAP_BUTT) # this is how ps.map draws

        brush = wx.TRANSPARENT_BRUSH
        if fcolor and fcolor != 'none':
            brush = wx.Brush(colour = convertRGB(fcolor))
        
        self.Draw(pen = pen, brush = brush, pdc = self.pdcObj, pdctype = pdctype[shape],
                  drawid = drawid, bb = bb, lineCoords = lineCoords)

    def DrawBitmap(self, pdc, filePath, rotation, bbox):
        """!Draw bitmap using PIL"""
        pImg = PILImage.open(filePath)
        if rotation:
            # get rid of black background
            pImg = pImg.convert("RGBA")
            rot = pImg.rotate(rotation, expand = 1)
            new = PILImage.new('RGBA', rot.size, (255,) * 4)
            pImg = PILImage.composite(rot, new, rot)
        pImg = pImg.resize((int(bbox[2]), int(bbox[3])), resample = PILImage.BICUBIC)
        img = PilImageToWxImage(pImg)
        bitmap = img.ConvertToBitmap()
        mask = wx.Mask(bitmap, wx.WHITE)
        bitmap.SetMask(mask)
        pdc.DrawBitmap(bitmap, bbox[0], bbox[1], useMask = True)
        
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
        
        # border is not redrawn when zoom changes, why?
##        if textDict['border'] != 'none' and not rot:
##            units = UnitConversion(self)
##            borderWidth = units.convert(value = textDict['width'],
##                                        fromUnit = 'point', toUnit = 'pixel' ) * self.currScale
##            pdc.SetPen(wx.Pen(colour = convertRGB(textDict['border']), width = borderWidth))
##            pdc.DrawRectangle(*bounds)
            
        if background:
            pdc.SetTextBackground(convertRGB(background))
            pdc.SetBackgroundMode(wx.SOLID)
        else:
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
            rect = self.pdcObj.GetIdBounds(id)
            if self.instruction[id].type != 'line':
                rect = rect.Inflate(3,3)
            # draw select box around object
            self.Draw(pen = self.pen['select'], brush = self.brush['select'], pdc = self.pdcTmp,
                      drawid = self.idBoxTmp, pdctype = 'rect', bb = rect)
            
            # draw small marks signalizing resizing
            if self.instruction[id].type in ('map', 'rectangle'):
                controlP = self.pdcObj.GetIdBounds(id).GetBottomRight()
                rect  = wx.RectPS(controlP, self.resizeBoxSize)
                self.Draw(pen = self.pen['resize'], brush = self.brush['resize'], pdc = self.pdcTmp,
                          drawid = self.idResizeBoxTmp, pdctype = 'rect', bb = rect)

            elif self.instruction[id].type == 'line':
                p1Paper = self.instruction[id]['where'][0]
                p2Paper = self.instruction[id]['where'][1]
                p1Canvas = self.CanvasPaperCoordinates(rect = Rect2DPS(p1Paper, (0, 0)), canvasToPaper = False)[:2]
                p2Canvas = self.CanvasPaperCoordinates(rect = Rect2DPS(p2Paper, (0, 0)), canvasToPaper = False)[:2]
                rect = []
                box = wx.RectS(self.resizeBoxSize)
                rect.append(box.CenterIn(wx.RectPS(p1Canvas, wx.Size())))
                rect.append(box.CenterIn(wx.RectPS(p2Canvas, wx.Size())))
                for i, point in enumerate((p1Canvas, p2Canvas)):
                    self.Draw(pen = self.pen['resize'], brush = self.brush['resize'], pdc = self.pdcTmp,
                              drawid = self.idLinePointsTmp[i], pdctype = 'rect', bb = rect[i])
        
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
            
        mapId = self.instruction.FindInstructionByType('map').id
        self.itemLabels[mapId] = []
        self.itemLabels[mapId].append(self.itemLabelsDict['map'])
        self.itemLabels[mapId].append("raster: " + rasterName)
        if vectorId: 
            for map in self.instruction[vectorId]['list']:
                self.itemLabels[mapId].append('vector: ' + map[0].split('@')[0])
            
    def UpdateLabel(self, itype, id):
        self.itemLabels[id] = []
        self.itemLabels[id].append(self.itemLabelsDict[itype])
        if itype == 'image':
            file = os.path.basename(self.instruction[id]['epsfile'])
            self.itemLabels[id].append(file)
        
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
    

# hack for Windows, loading EPS works only on Unix
# these functions are taken from EpsImagePlugin.py
def loadPSForWindows(self):
    # Load EPS via Ghostscript
    if not self.tile:
        return
    self.im = GhostscriptForWindows(self.tile, self.size, self.fp)
    self.mode = self.im.mode
    self.size = self.im.size
    self.tile = []

def GhostscriptForWindows(tile, size, fp):
    """Render an image using Ghostscript (Windows only)"""
    # Unpack decoder tile
    decoder, tile, offset, data = tile[0]
    length, bbox = data

    import tempfile, os

    file = tempfile.mkstemp()[1]

    # Build ghostscript command - for Windows
    command = ["gswin32c",
               "-q",                    # quite mode
               "-g%dx%d" % size,        # set output geometry (pixels)
               "-dNOPAUSE -dSAFER",     # don't pause between pages, safe mode
               "-sDEVICE=ppmraw",       # ppm driver
               "-sOutputFile=%s" % file # output file
              ]

    command = string.join(command)

    # push data through ghostscript
    try:
        gs = os.popen(command, "w")
        # adjust for image origin
        if bbox[0] != 0 or bbox[1] != 0:
            gs.write("%d %d translate\n" % (-bbox[0], -bbox[1]))
        fp.seek(offset)
        while length > 0:
            s = fp.read(8192)
            if not s:
                break
            length = length - len(s)
            gs.write(s)
        status = gs.close()
        if status:
            raise IOError("gs failed (status %d)" % status)
        im = PILImage.core.open_ppm(file)

    finally:
        try: os.unlink(file)
        except: pass

    return im
