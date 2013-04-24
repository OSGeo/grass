"""!
@package psmap.dialogs

@brief dialogs for wxPsMap

Classes:
 - dialogs::TCValidator
 - dialogs::PenStyleComboBox
 - dialogs::CheckListCtrl
 - dialogs::PsmapDialog
 - dialogs::PageSetupDialog
 - dialogs::MapDialog
 - dialogs::MapFramePanel
 - dialogs::RasterPanel
 - dialogs::VectorPanel
 - dialogs::RasterDialog
 - dialogs::MainVectorDialog
 - dialogs::VPropertiesDialog
 - dialogs::LegendDialog
 - dialogs::MapinfoDialog
 - dialogs::ScalebarDialog
 - dialogs::TextDialog
 - dialogs::ImageDialog
 - dialogs::NorthArrowDialog
 - dialogs::PointDialog
 - dialogs::RectangleDialog

(C) 2011-2012 by Anna Kratochvilova, and the GRASS Development Team

This program is free software under the GNU General Public License
(>=v2). Read the file COPYING that comes with GRASS for details.

@author Anna Kratochvilova <kratochanna gmail.com> (bachelor's project)
@author Martin Landa <landa.martin gmail.com> (mentor)
"""

import os
import sys
import string
from copy import deepcopy

import wx
import wx.lib.scrolledpanel    as scrolled
import wx.lib.filebrowsebutton as filebrowse
from wx.lib.mixins.listctrl import CheckListCtrlMixin, ListCtrlAutoWidthMixin
from wx.lib.expando         import ExpandoTextCtrl, EVT_ETC_LAYOUT_NEEDED
try:
    import wx.lib.agw.floatspin as fs
except ImportError:
    fs = None

import grass.script as grass

from core               import globalvar
from dbmgr.vinfo        import VectorDBInfo
from gui_core.gselect   import Select
from core.gcmd          import RunCommand, GError, GMessage
from gui_core.dialogs   import SymbolDialog
from psmap.utils        import *
from psmap.instructions import *

# grass.set_raise_on_error(True)

PSMAP_COLORS = ['aqua', 'black', 'blue', 'brown', 'cyan', 'gray', 'grey', 'green', 'indigo',
                'magenta','orange', 'purple', 'red', 'violet', 'white', 'yellow']

    
class TCValidator(wx.PyValidator):
    """!validates input in textctrls, combobox, taken from wxpython demo"""
    def __init__(self, flag = None):
        wx.PyValidator.__init__(self)
        self.flag = flag
        self.Bind(wx.EVT_CHAR, self.OnChar)

    def Clone(self):
        return TCValidator(self.flag)

    def Validate(self, win):
        
        tc = self.GetWindow()
        val = tc.GetValue()

        if self.flag == 'DIGIT_ONLY':
            for x in val:
                if x not in string.digits:
                    return False
        return True

    def OnChar(self, event):
        key = event.GetKeyCode()
        if key < wx.WXK_SPACE or key == wx.WXK_DELETE or key > 255:
            event.Skip()
            return
        if self.flag == 'DIGIT_ONLY' and chr(key) in string.digits + '.-':
            event.Skip()
            return
##        if self.flag == 'SCALE' and chr(key) in string.digits + ':':
##            event.Skip()
##            return
        if self.flag == 'ZERO_AND_ONE_ONLY' and chr(key) in '01':
            event.Skip()
            return
        if not wx.Validator_IsSilent():
            wx.Bell()
        # Returning without calling even.Skip eats the event before it
        # gets to the text control
        return  


class PenStyleComboBox(wx.combo.OwnerDrawnComboBox):
    """!Combo for selecting line style, taken from wxpython demo"""

    # Overridden from OwnerDrawnComboBox, called to draw each
    # item in the list
    def OnDrawItem(self, dc, rect, item, flags):
        if item == wx.NOT_FOUND:
            # painting the control, but there is no valid item selected yet
            return

        r = wx.Rect(*rect)  # make a copy
        r.Deflate(3, 5)

        penStyle = wx.SOLID
        if item == 1:
            penStyle = wx.LONG_DASH
        elif item == 2:
            penStyle = wx.DOT
        elif item == 3:
            penStyle = wx.DOT_DASH

        pen = wx.Pen(dc.GetTextForeground(), 3, penStyle)
        dc.SetPen(pen)

        # for painting the items in the popup
        dc.DrawText(self.GetString(item ),
                    r.x + 3,
                    (r.y + 0) + ((r.height/2) - dc.GetCharHeight() )/2
                    )
        dc.DrawLine(r.x+5, r.y+((r.height/4)*3)+1, r.x+r.width - 5, r.y+((r.height/4)*3)+1 )

        
    def OnDrawBackground(self, dc, rect, item, flags):
        """!Overridden from OwnerDrawnComboBox, called for drawing the
        background area of each item."""
        # If the item is selected, or its item # iseven, or we are painting the
        # combo control itself, then use the default rendering.
        if (item & 1 == 0 or flags & (wx.combo.ODCB_PAINTING_CONTROL |
                                      wx.combo.ODCB_PAINTING_SELECTED)):
            wx.combo.OwnerDrawnComboBox.OnDrawBackground(self, dc, rect, item, flags)
            return

        # Otherwise, draw every other background with different colour.
        bgCol = wx.Colour(240,240,250)
        dc.SetBrush(wx.Brush(bgCol))
        dc.SetPen(wx.Pen(bgCol))
        dc.DrawRectangleRect(rect);

    def OnMeasureItem(self, item):
        """!Overridden from OwnerDrawnComboBox, should return the height
        needed to display an item in the popup, or -1 for default"""
        return 30

    def OnMeasureItemWidth(self, item):
        """!Overridden from OwnerDrawnComboBox.  Callback for item width, or
        -1 for default/undetermined"""
        return -1; # default - will be measured from text width  
    
    
class CheckListCtrl(wx.ListCtrl, CheckListCtrlMixin, ListCtrlAutoWidthMixin):
    """!List control for managing order and labels of vector maps in legend"""
    def __init__(self, parent):
        wx.ListCtrl.__init__(self, parent, id = wx.ID_ANY, 
                             style = wx.LC_REPORT|wx.LC_SINGLE_SEL|wx.BORDER_SUNKEN|wx.LC_VRULES|wx.LC_HRULES)
        CheckListCtrlMixin.__init__(self) 
        ListCtrlAutoWidthMixin.__init__(self)
        

class PsmapDialog(wx.Dialog):
    def __init__(self, parent, id,  title, settings, apply = True):
        wx.Dialog.__init__(self, parent = parent, id = wx.ID_ANY, 
                            title = title, size = wx.DefaultSize,
                            style = wx.CAPTION|wx.MINIMIZE_BOX|wx.CLOSE_BOX)
        self.apply = apply
        self.id = id
        self.parent = parent
        self.instruction = settings
        self.objectType = None
        self.unitConv = UnitConversion(self)
        self.spinCtrlSize = (65, -1)
        
        self.Bind(wx.EVT_CLOSE, self.OnClose)
        
    
        
    def AddUnits(self, parent, dialogDict):
        parent.units = dict()
        parent.units['unitsLabel'] = wx.StaticText(parent, id = wx.ID_ANY, label = _("Units:"))
        choices = self.unitConv.getPageUnitsNames()
        parent.units['unitsCtrl'] = wx.Choice(parent, id = wx.ID_ANY, choices = choices)  
        parent.units['unitsCtrl'].SetStringSelection(self.unitConv.findName(dialogDict['unit']))
          
    def AddPosition(self, parent, dialogDict):
        if not hasattr(parent, "position"):
            parent.position = dict()
        parent.position['comment'] = wx.StaticText(parent, id = wx.ID_ANY,\
                    label = _("Position of the top left corner\nfrom the top left edge of the paper"))
        parent.position['xLabel'] = wx.StaticText(parent, id = wx.ID_ANY, label = _("X:"))
        parent.position['yLabel'] = wx.StaticText(parent, id = wx.ID_ANY, label = _("Y:"))
        parent.position['xCtrl'] = wx.TextCtrl(parent, id = wx.ID_ANY, value = str(dialogDict['where'][0]), validator = TCValidator(flag = 'DIGIT_ONLY'))
        parent.position['yCtrl'] = wx.TextCtrl(parent, id = wx.ID_ANY, value = str(dialogDict['where'][1]), validator = TCValidator(flag = 'DIGIT_ONLY'))
        if dialogDict.has_key('unit'):
            x = self.unitConv.convert(value = dialogDict['where'][0], fromUnit = 'inch', toUnit = dialogDict['unit'])
            y = self.unitConv.convert(value = dialogDict['where'][1], fromUnit = 'inch', toUnit = dialogDict['unit'])
            parent.position['xCtrl'].SetValue("%5.3f" % x)
            parent.position['yCtrl'].SetValue("%5.3f" % y)
        
    def AddExtendedPosition(self, panel, gridBagSizer, dialogDict):
        """!Add widgets for setting position relative to paper and to map"""
        panel.position = dict()
        positionLabel = wx.StaticText(panel, id = wx.ID_ANY, label = _("Position is given:"))
        panel.position['toPaper'] = wx.RadioButton(panel, id = wx.ID_ANY, label = _("relatively to paper"), style = wx.RB_GROUP)
        panel.position['toMap'] = wx.RadioButton(panel, id = wx.ID_ANY, label = _("by map coordinates"))
        panel.position['toPaper'].SetValue(dialogDict['XY'])
        panel.position['toMap'].SetValue(not dialogDict['XY'])
        
        gridBagSizer.Add(positionLabel, pos = (0,0), span = (1,3), flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT, border = 0)
        gridBagSizer.Add(panel.position['toPaper'], pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT, border = 0)
        gridBagSizer.Add(panel.position['toMap'], pos = (1,1),flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT, border = 0)
        
        # first box - paper coordinates
        box1   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = "")
        sizerP = wx.StaticBoxSizer(box1, wx.VERTICAL)
        self.gridBagSizerP = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        self.AddPosition(parent = panel, dialogDict = dialogDict)
        panel.position['comment'].SetLabel(_("Position from the top left\nedge of the paper"))
        self.AddUnits(parent = panel, dialogDict = dialogDict)
        self.gridBagSizerP.Add(panel.units['unitsLabel'], pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerP.Add(panel.units['unitsCtrl'], pos = (0,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerP.Add(panel.position['xLabel'], pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerP.Add(panel.position['xCtrl'], pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerP.Add(panel.position['yLabel'], pos = (2,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerP.Add(panel.position['yCtrl'], pos = (2,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerP.Add(panel.position['comment'], pos = (3,0), span = (1,2), flag = wx.ALIGN_BOTTOM, border = 0)
        
        self.gridBagSizerP.AddGrowableCol(1)
        self.gridBagSizerP.AddGrowableRow(3)
        sizerP.Add(self.gridBagSizerP, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        gridBagSizer.Add(sizerP, pos = (2,0),span = (1,1), flag = wx.ALIGN_CENTER_HORIZONTAL|wx.EXPAND, border = 0)
        
        # second box - map coordinates
        box2   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = "")
        sizerM = wx.StaticBoxSizer(box2, wx.VERTICAL)
        self.gridBagSizerM = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        eastingLabel  = wx.StaticText(panel, id = wx.ID_ANY, label = "E:")
        northingLabel  = wx.StaticText(panel, id = wx.ID_ANY, label = "N:")
        panel.position['eCtrl'] = wx.TextCtrl(panel, id = wx.ID_ANY, value = "")
        panel.position['nCtrl'] = wx.TextCtrl(panel, id = wx.ID_ANY, value = "")
        east, north = PaperMapCoordinates(mapInstr = self.instruction[self.mapId], x = dialogDict['where'][0], y = dialogDict['where'][1], paperToMap = True)
        panel.position['eCtrl'].SetValue(str(east))
        panel.position['nCtrl'].SetValue(str(north))
        
        self.gridBagSizerM.Add(eastingLabel, pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerM.Add(northingLabel, pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerM.Add(panel.position['eCtrl'], pos = (0,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerM.Add(panel.position['nCtrl'], pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        self.gridBagSizerM.AddGrowableCol(0)
        self.gridBagSizerM.AddGrowableCol(1)
        sizerM.Add(self.gridBagSizerM, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        gridBagSizer.Add(sizerM, pos = (2,1), flag = wx.ALIGN_LEFT|wx.EXPAND, border = 0)
        
    def AddFont(self, parent, dialogDict, color = True):
        parent.font = dict()
##        parent.font['fontLabel'] = wx.StaticText(parent, id = wx.ID_ANY, label = _("Choose font:"))
##        parent.font['fontCtrl'] = wx.FontPickerCtrl(parent, id = wx.ID_ANY)
##        
##        parent.font['fontCtrl'].SetSelectedFont(
##                        wx.FontFromNativeInfoString(dialogDict['font'] + " " + str(dialogDict['fontsize'])))
##        parent.font['fontCtrl'].SetMaxPointSize(50)
##        
##        if color:
##            parent.font['colorLabel'] = wx.StaticText(parent, id = wx.ID_ANY, label = _("Choose color:"))
##            parent.font['colorCtrl'] = wx.ColourPickerCtrl(parent, id = wx.ID_ANY, style=wx.FNTP_FONTDESC_AS_LABEL)
##            parent.font['colorCtrl'].SetColour(dialogDict['color'])
           
##        parent.font['colorCtrl'].SetColour(convertRGB(dialogDict['color'])) 
           
        parent.font['fontLabel'] = wx.StaticText(parent, id = wx.ID_ANY, label = _("Font:"))
        parent.font['fontSizeLabel'] = wx.StaticText(parent, id = wx.ID_ANY, label = _("Font size:"))
        fontChoices = [ 'Times-Roman', 'Times-Italic', 'Times-Bold', 'Times-BoldItalic', 'Helvetica',\
                        'Helvetica-Oblique', 'Helvetica-Bold', 'Helvetica-BoldOblique', 'Courier',\
                        'Courier-Oblique', 'Courier-Bold', 'Courier-BoldOblique'] 
        parent.font['fontCtrl'] = wx.Choice(parent, id = wx.ID_ANY, choices = fontChoices)
        if dialogDict['font'] in fontChoices:
            parent.font['fontCtrl'].SetStringSelection(dialogDict['font'])
        else:
            parent.font['fontCtrl'].SetStringSelection('Helvetica')
        parent.font['fontSizeCtrl'] = wx.SpinCtrl(parent, id = wx.ID_ANY, min = 4, max = 50, initial = 10)
        parent.font['fontSizeCtrl'].SetValue(dialogDict['fontsize'])
         
        if color:
            parent.font['colorLabel'] = wx.StaticText(parent, id = wx.ID_ANY, label = _("Choose color:"))
            parent.font['colorCtrl'] = wx.ColourPickerCtrl(parent, id = wx.ID_ANY)
            parent.font['colorCtrl'].SetColour(convertRGB(dialogDict['color']))
##            parent.font['colorLabel'] = wx.StaticText(parent, id = wx.ID_ANY, label = _("Color:"))
##            colorChoices = [  'aqua', 'black', 'blue', 'brown', 'cyan', 'gray', 'green', 'indigo', 'magenta',\
##                                'orange', 'purple', 'red', 'violet', 'white', 'yellow']
##            parent.colorCtrl = wx.Choice(parent, id = wx.ID_ANY, choices = colorChoices)
##            parent.colorCtrl.SetStringSelection(parent.rLegendDict['color'])
##            parent.font['colorCtrl'] = wx.ColourPickerCtrl(parent, id = wx.ID_ANY)
##            parent.font['colorCtrl'].SetColour(dialogDict['color'])   
    def OnApply(self, event):
        ok = self.update()
        if ok:
            self.parent.DialogDataChanged(id = self.id)
            return True 
        else:
            return False
        
    def OnOK(self, event):
        """!Apply changes, close dialog"""
        ok = self.OnApply(event)
        if ok:
            self.Close()
    
    def OnCancel(self, event):
        """!Close dialog"""
        self.Close()

    def OnClose(self, event):
        """!Destroy dialog and delete it from open dialogs"""
        if self.objectType:
            for each in  self.objectType:
                if each in self.parent.openDialogs:
                    del self.parent.openDialogs[each]
        event.Skip()
        self.Destroy()
        
    def _layout(self, panel):
        #buttons
        btnCancel = wx.Button(self, wx.ID_CANCEL)
        btnOK = wx.Button(self, wx.ID_OK)
        btnOK.SetDefault()
        if self.apply:
            btnApply = wx.Button(self, wx.ID_APPLY)
        

        # bindigs
        btnOK.Bind(wx.EVT_BUTTON, self.OnOK)
        btnOK.SetToolTipString(_("Close dialog and apply changes"))
        #btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        btnCancel.SetToolTipString(_("Close dialog and ignore changes"))
        btnCancel.Bind(wx.EVT_BUTTON, self.OnCancel)
        if self.apply:
            btnApply.Bind(wx.EVT_BUTTON, self.OnApply)
            btnApply.SetToolTipString(_("Apply changes"))
        
        # sizers
        btnSizer = wx.StdDialogButtonSizer()
        btnSizer.AddButton(btnCancel)
        if self.apply:
            btnSizer.AddButton(btnApply)
        btnSizer.AddButton(btnOK)
        btnSizer.Realize()
        
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        mainSizer.Add(item = panel, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        mainSizer.Add(item = btnSizer, proportion = 0,
                      flag = wx.EXPAND | wx.ALL | wx.ALIGN_CENTER, border = 5)
        
        
        self.SetSizer(mainSizer)
        mainSizer.Layout()
        mainSizer.Fit(self) 
            
class PageSetupDialog(PsmapDialog):
    def __init__(self, parent, id, settings):
        PsmapDialog.__init__(self, parent = parent, id = id, title = "Page setup",  settings = settings)
        
        self.cat = ['Units', 'Format', 'Orientation', 'Width', 'Height', 'Left', 'Right', 'Top', 'Bottom']
        labels = [_('Units'), _('Format'), _('Orientation'), _('Width'), _('Height'),
                  _('Left'), _('Right'), _('Top'), _('Bottom')]
        self.catsLabels = dict(zip(self.cat, labels))
        paperString = RunCommand('ps.map', flags = 'p', read = True, quiet = True)
        self.paperTable = self._toList(paperString) 
        self.unitsList = self.unitConv.getPageUnitsNames()
        self.pageSetupDict = settings[id].GetInstruction()

        self._layout()
        
        if self.pageSetupDict:
            self.getCtrl('Units').SetStringSelection(self.unitConv.findName(self.pageSetupDict['Units']))
            if self.pageSetupDict['Format'] == 'custom':
                self.getCtrl('Format').SetSelection(self.getCtrl('Format').GetCount() - 1)
            else:
                self.getCtrl('Format').SetStringSelection(self.pageSetupDict['Format'])
            if self.pageSetupDict['Orientation'] == 'Portrait':
                self.getCtrl('Orientation').SetSelection(0)
            else:
                self.getCtrl('Orientation').SetSelection(1)
                
            for item in self.cat[3:]:
                val = self.unitConv.convert(value = self.pageSetupDict[item],
                                            fromUnit = 'inch', toUnit = self.pageSetupDict['Units'])
                self.getCtrl(item).SetValue("%4.3f" % val)

       
        if self.getCtrl('Format').GetSelection() != self.getCtrl('Format').GetCount() - 1: # custom
            self.getCtrl('Width').Disable()
            self.getCtrl('Height').Disable()
        else:
            self.getCtrl('Orientation').Disable()
        # events
        self.getCtrl('Units').Bind(wx.EVT_CHOICE, self.OnChoice)
        self.getCtrl('Format').Bind(wx.EVT_CHOICE, self.OnChoice)
        self.getCtrl('Orientation').Bind(wx.EVT_CHOICE, self.OnChoice)
        self.btnOk.Bind(wx.EVT_BUTTON, self.OnOK)

    
    def update(self):
        self.pageSetupDict['Units'] = self.unitConv.findUnit(self.getCtrl('Units').GetStringSelection())
        self.pageSetupDict['Format'] = self.paperTable[self.getCtrl('Format').GetSelection()]['Format']
        if self.getCtrl('Orientation').GetSelection() == 0:
            self.pageSetupDict['Orientation'] = 'Portrait'
        else:
            self.pageSetupDict['Orientation'] = 'Landscape'
        for item in self.cat[3:]:
            self.pageSetupDict[item] = self.unitConv.convert(value = float(self.getCtrl(item).GetValue()),
                                        fromUnit = self.pageSetupDict['Units'], toUnit = 'inch')
            

            
    def OnOK(self, event):
        try:
            self.update()
        except ValueError:
                wx.MessageBox(message = _("Literal is not allowed!"), caption = _('Invalid input'),
                                    style = wx.OK|wx.ICON_ERROR)
        else:
            event.Skip()
        
    def _layout(self):
        size = (110,-1)
        #sizers
        mainSizer = wx.BoxSizer(wx.VERTICAL)
        pageBox = wx.StaticBox(self, id = wx.ID_ANY, label = " %s " % _("Page size"))
        pageSizer = wx.StaticBoxSizer(pageBox, wx.VERTICAL)
        marginBox = wx.StaticBox(self, id = wx.ID_ANY, label = " %s " % _("Margins"))
        marginSizer = wx.StaticBoxSizer(marginBox, wx.VERTICAL)
        horSizer = wx.BoxSizer(wx.HORIZONTAL) 
        #staticText + choice
        choices = [self.unitsList, [item['Format'] for item in self.paperTable], [_('Portrait'), _('Landscape')]]
        propor = [0,1,1]
        border = [5,3,3]
        self.hBoxDict={}
        for i, item in enumerate(self.cat[:3]):
            hBox = wx.BoxSizer(wx.HORIZONTAL)
            stText = wx.StaticText(self, id = wx.ID_ANY, label = self.catsLabels[item] + ':')
            choice = wx.Choice(self, id = wx.ID_ANY, choices = choices[i], size = size)
            hBox.Add(stText, proportion = propor[i], flag = wx.ALIGN_CENTER_VERTICAL|wx.ALL, border = border[i])
            hBox.Add(choice, proportion = 0, flag = wx.ALL, border = border[i])
            if item == 'Units':
                hBox.Add(size,1) 
            self.hBoxDict[item] = hBox    

        #staticText + TextCtrl
        for item in self.cat[3:]:
            hBox = wx.BoxSizer(wx.HORIZONTAL)
            label = wx.StaticText(self, id = wx.ID_ANY, label = self.catsLabels[item] + ':')
            textctrl = wx.TextCtrl(self, id = wx.ID_ANY, size = size, value = '')
            hBox.Add(label, proportion = 1, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALL, border = 3)
            hBox.Add(textctrl, proportion = 0, flag = wx.ALIGN_CENTRE|wx.ALL, border = 3)
            self.hBoxDict[item] = hBox
         
        sizer = list([mainSizer] + [pageSizer]*4 + [marginSizer]*4)
        for i, item in enumerate(self.cat):
                sizer[i].Add(self.hBoxDict[item], 0, wx.GROW|wx.RIGHT|wx.LEFT,5)
        # OK button
        btnSizer = wx.StdDialogButtonSizer()
        self.btnOk = wx.Button(self, wx.ID_OK)
        self.btnOk.SetDefault()
        btnSizer.AddButton(self.btnOk)
        btn = wx.Button(self, wx.ID_CANCEL)
        btnSizer.AddButton(btn)
        btnSizer.Realize()
    
    
        horSizer.Add(pageSizer, proportion = 0, flag = wx.LEFT|wx.RIGHT|wx.BOTTOM, border = 10)
        horSizer.Add(marginSizer, proportion = 0, flag = wx.LEFT|wx.RIGHT|wx.BOTTOM|wx.EXPAND, border = 10)
        mainSizer.Add(horSizer, proportion = 0, border = 10)  
        mainSizer.Add(btnSizer, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT|wx.ALL,  border = 10)      
        self.SetSizer(mainSizer)
        mainSizer.Fit(self)
    
    def OnChoice(self, event):
        currPaper = self.paperTable[self.getCtrl('Format').GetSelection()]
        currUnit = self.unitConv.findUnit(self.getCtrl('Units').GetStringSelection())
        currOrientIdx = self.getCtrl('Orientation').GetSelection()
        newSize = dict()
        for item in self.cat[3:]:
            newSize[item] = self.unitConv.convert(float(currPaper[item]), fromUnit = 'inch', toUnit = currUnit)

        enable = True
        if currPaper['Format'] != _('custom'):
            if currOrientIdx == 1: # portrait
                newSize['Width'], newSize['Height'] = newSize['Height'], newSize['Width']
            for item in self.cat[3:]:
                self.getCtrl(item).ChangeValue("%4.3f" % newSize[item])
            enable = False
        self.getCtrl('Width').Enable(enable)
        self.getCtrl('Height').Enable(enable)
        self.getCtrl('Orientation').Enable(not enable)


    def getCtrl(self, item):
         return self.hBoxDict[item].GetItem(1).GetWindow()
        
    def _toList(self, paperStr):
        
        sizeList = list()
        for line in paperStr.strip().split('\n'):
            d = dict(zip([self.cat[1]]+ self.cat[3:],line.split()))
            sizeList.append(d)
        d = {}.fromkeys([self.cat[1]]+ self.cat[3:], 100)
        d.update(Format = _('custom'))
        sizeList.append(d)
        return sizeList
    
class MapDialog(PsmapDialog):
    """!Dialog for map frame settings and optionally  raster and vector map selection"""
    def __init__(self, parent, id, settings,  rect = None, notebook = False):
        PsmapDialog.__init__(self, parent = parent, id = id, title = "", settings = settings)
 
        self.isNotebook = notebook
        if self.isNotebook:
            self.objectType = ('mapNotebook',) 
        else:
            self.objectType = ('map',)

        
        #notebook
        if self.isNotebook:
            self.notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)
            self.mPanel = MapFramePanel(parent = self.notebook, id = self.id[0], settings = self.instruction, 
                                        rect = rect, notebook = True)
            self.id[0] = self.mPanel.getId()
            self.rPanel = RasterPanel(parent = self.notebook, id = self.id[1], settings = self.instruction, 
                                        notebook = True)
            self.id[1] = self.rPanel.getId()
            self.vPanel = VectorPanel(parent = self.notebook, id = self.id[2], settings = self.instruction,
                                        notebook = True)
            self.id[2] = self.vPanel.getId()
            self._layout(self.notebook)
            self.SetTitle(_("Map settings"))
        else:
            self.mPanel = MapFramePanel(parent = self, id = self.id[0], settings = self.instruction, 
                                        rect = rect, notebook = False)
            self.id[0] = self.mPanel.getId()
            self._layout(self.mPanel)
            self.SetTitle(_("Map frame settings"))
        
        
    def OnApply(self, event):
        """!Apply changes"""
        if self.isNotebook:
            okV = self.vPanel.update()
            okR = self.rPanel.update()
            if okV and self.id[2] in self.instruction:
                self.parent.DialogDataChanged(id = self.id[2])
            if okR and self.id[1] in self.instruction:
                self.parent.DialogDataChanged(id = self.id[1])
            if not okR or not okV:
                return False

        ok = self.mPanel.update()
        if ok:
            self.parent.DialogDataChanged(id = self.id[0])
            return True 
        
        return False
    
    def OnCancel(self, event):
        """!Close dialog and remove tmp red box"""
        self.parent.canvas.pdcTmp.RemoveId(self.parent.canvas.idZoomBoxTmp)
        self.parent.canvas.Refresh() 
        self.Close()
        
    def updateDialog(self):
        """!Update raster and vector information"""
        if self.mPanel.scaleChoice.GetSelection() == 0:
            if self.mPanel.rasterTypeRadio.GetValue():
                if 'raster' in self.parent.openDialogs:
                    if self.parent.openDialogs['raster'].rPanel.rasterYesRadio.GetValue() and \
                            self.parent.openDialogs['raster'].rPanel.rasterSelect.GetValue() == self.mPanel.select.GetValue():
                            self.mPanel.drawMap.SetValue(True)
                    else:
                        self.mPanel.drawMap.SetValue(False)
            else:
                if 'vector' in self.parent.openDialogs:
                    found = False
                    for each in self.parent.openDialogs['vector'].vPanel.vectorList:
                        if each[0] == self.mPanel.select.GetValue():
                            found = True
                    self.mPanel.drawMap.SetValue(found)    
                        
class MapFramePanel(wx.Panel):
    """!wx.Panel with map (scale, region, border) settings"""
    def __init__(self, parent, id, settings, rect, notebook = True):
        wx.Panel.__init__(self, parent, id = wx.ID_ANY, style = wx.TAB_TRAVERSAL)

        self.id = id
        self.instruction = settings
        
        if notebook:
            self.book = parent
            self.book.AddPage(page = self, text = _("Map frame"))
            self.mapDialog = self.book.GetParent()
        else:
            self.mapDialog = parent
            
        if self.id is not None:
            self.mapFrameDict = self.instruction[self.id].GetInstruction() 
        else:
            self.id = wx.NewId()
            mapFrame = MapFrame(self.id)
            self.mapFrameDict = mapFrame.GetInstruction()
            self.mapFrameDict['rect'] = rect

            
        self._layout()

        self.scale = [None]*4
        self.center = [None]*4
        
        
        
        self.selectedMap = self.mapFrameDict['map']
        self.selectedRegion = self.mapFrameDict['region']
        self.scaleType = self.mapFrameDict['scaleType']
        self.mapType = self.mapFrameDict['mapType']
        self.scaleChoice.SetSelection(self.mapFrameDict['scaleType'])
        if self.instruction[self.id]:
            self.drawMap.SetValue(self.mapFrameDict['drawMap'])
        else:
            self.drawMap.SetValue(True)
        if self.mapFrameDict['scaleType'] == 0 and self.mapFrameDict['map']:
            self.select.SetValue(self.mapFrameDict['map'])
            if self.mapFrameDict['mapType'] == 'raster':
                self.rasterTypeRadio.SetValue(True)
                self.vectorTypeRadio.SetValue(False)
            else:
                self.rasterTypeRadio.SetValue(False)
                self.vectorTypeRadio.SetValue(True)
        elif self.mapFrameDict['scaleType'] == 1 and self.mapFrameDict['region']:
            self.select.SetValue(self.mapFrameDict['region'])
        
        
        self.OnMap(None)
        self.scale[self.mapFrameDict['scaleType']] = self.mapFrameDict['scale']
        self.center[self.mapFrameDict['scaleType']] = self.mapFrameDict['center']
        self.OnScaleChoice(None)
        self.OnElementType(None)
        self.OnBorder(None)
        
        
        
    def _layout(self):
        """!Do layout"""
        border = wx.BoxSizer(wx.VERTICAL)
        
        box   = wx.StaticBox (parent = self, id = wx.ID_ANY, label = " %s " % _("Map frame"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)


        #scale options
        frameText = wx.StaticText(self, id = wx.ID_ANY, label = _("Map frame options:"))
        scaleChoices = [_("fit frame to match selected map"),
                        _("fit frame to match saved region"),
                        _("fit frame to match current computational region"),
                        _("fixed scale and map center")]
        self.scaleChoice = wx.Choice(self, id = wx.ID_ANY, choices = scaleChoices)
        
        
        gridBagSizer.Add(frameText, pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.scaleChoice, pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        
        #map and region selection
        self.staticBox = wx.StaticBox (parent = self, id = wx.ID_ANY, label = " %s " % _("Map selection"))
        sizerM = wx.StaticBoxSizer(self.staticBox, wx.HORIZONTAL)
        self.mapSizer = wx.GridBagSizer(hgap = 5, vgap = 5)

        self.rasterTypeRadio = wx.RadioButton(self, id = wx.ID_ANY, label = " %s " % _("raster"), style = wx.RB_GROUP)
        self.vectorTypeRadio = wx.RadioButton(self, id = wx.ID_ANY, label = " %s " % _("vector"))
        self.drawMap = wx.CheckBox(self, id = wx.ID_ANY, label = "add selected map")
        
        self.mapOrRegionText = [_("Map:"), _("Region:")] 
        dc = wx.ClientDC(self)# determine size of labels
        width = max(dc.GetTextExtent(self.mapOrRegionText[0])[0], dc.GetTextExtent(self.mapOrRegionText[1])[0])
        self.mapText = wx.StaticText(self, id = wx.ID_ANY, label = self.mapOrRegionText[0], size = (width, -1))
        self.select = Select(self, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE,
                             type = 'raster', multiple = False,
                             updateOnPopup = True, onPopup = None)
                            
        self.mapSizer.Add(self.rasterTypeRadio, pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.mapSizer.Add(self.vectorTypeRadio, pos = (0, 2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.mapSizer.Add(self.drawMap, pos = (0, 3), flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT, border = 0)
        self.mapSizer.Add(self.mapText, pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.mapSizer.Add(self.select, pos = (1, 1), span = (1, 3), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
                 
        sizerM.Add(self.mapSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        gridBagSizer.Add(sizerM, pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        

        #map scale and center
        boxC   = wx.StaticBox (parent = self, id = wx.ID_ANY, label = " %s " % _("Map scale and center"))
        sizerC = wx.StaticBoxSizer(boxC, wx.HORIZONTAL)
        self.centerSizer = wx.FlexGridSizer(rows = 2, cols = 5, hgap = 5, vgap = 5)        
                
                           
        centerText = wx.StaticText(self, id = wx.ID_ANY, label = _("Center:"))
        self.eastingText = wx.StaticText(self, id = wx.ID_ANY, label = _("E:"))
        self.northingText = wx.StaticText(self, id = wx.ID_ANY, label = _("N:"))
        self.eastingTextCtrl = wx.TextCtrl(self, id = wx.ID_ANY, style = wx.TE_RIGHT, validator = TCValidator(flag = 'DIGIT_ONLY'))
        self.northingTextCtrl = wx.TextCtrl(self, id = wx.ID_ANY, style = wx.TE_RIGHT, validator = TCValidator(flag = 'DIGIT_ONLY'))
        scaleText = wx.StaticText(self, id = wx.ID_ANY, label = _("Scale:"))
        scalePrefixText = wx.StaticText(self, id = wx.ID_ANY, label = _("1 :"))
        self.scaleTextCtrl = wx.TextCtrl(self, id = wx.ID_ANY, value = "", style = wx.TE_RIGHT, validator = TCValidator('DIGIT_ONLY'))
        
        self.centerSizer.Add(centerText, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.RIGHT, border = 10)
        self.centerSizer.Add(self.eastingText, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT, border = 0)
        self.centerSizer.Add(self.eastingTextCtrl, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.centerSizer.Add(self.northingText, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT, border = 0)
        self.centerSizer.Add(self.northingTextCtrl, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        self.centerSizer.Add(scaleText, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.RIGHT, border = 10)
        self.centerSizer.Add(scalePrefixText, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT, border = 0)
        self.centerSizer.Add(self.scaleTextCtrl, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        sizerC.Add(self.centerSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        gridBagSizer.Add(sizerC, pos = (3, 0), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        
        
        #resolution
        flexSizer = wx.FlexGridSizer(rows = 1, cols = 2, hgap = 5, vgap = 5)
        
        resolutionText = wx.StaticText(self, id = wx.ID_ANY, label = _("Map max resolution (dpi):"))
        self.resolutionSpin = wx.SpinCtrl(self, id = wx.ID_ANY, min = 1, max = 1000, initial = 300)
        
        flexSizer.Add(resolutionText, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexSizer.Add(self.resolutionSpin, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.resolutionSpin.SetValue(self.mapFrameDict['resolution'])
        
        gridBagSizer.Add(flexSizer, pos = (4, 0), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        # border
        box   = wx.StaticBox (parent = self, id = wx.ID_ANY, label = " %s " % _("Border"))        
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        self.borderCheck = wx.CheckBox(self, id = wx.ID_ANY, label = (_("draw border around map frame")))
        if self.mapFrameDict['border'] == 'y':
            self.borderCheck.SetValue(True)
        else: 
            self.borderCheck.SetValue(False)
        
        self.borderColorText = wx.StaticText(self, id = wx.ID_ANY, label = _("border color:"))
        self.borderWidthText = wx.StaticText(self, id = wx.ID_ANY, label = _("border width (pts):"))
        self.borderColourPicker = wx.ColourPickerCtrl(self, id = wx.ID_ANY)
        self.borderWidthCtrl = wx.SpinCtrl(self, id = wx.ID_ANY, min = 1, max = 100, initial = 1)
        
        if self.mapFrameDict['border'] == 'y':
            self.borderWidthCtrl.SetValue(int(self.mapFrameDict['width']))
            self.borderColourPicker.SetColour(convertRGB(self.mapFrameDict['color']))
        
        
        gridBagSizer.Add(self.borderCheck, pos = (0, 0), span = (1,2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(self.borderColorText, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.borderWidthText, pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.borderColourPicker, pos = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(self.borderWidthCtrl, pos = (2, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        self.SetSizer(border)
        self.Fit()
        
        
        if projInfo()['proj'] == 'll':
            self.scaleChoice.SetItems(self.scaleChoice.GetItems()[0:3])
            boxC.Hide()
            for each in self.centerSizer.GetChildren():
                each.GetWindow().Hide()

            
        # bindings
        self.scaleChoice.Bind(wx.EVT_CHOICE, self.OnScaleChoice)
        self.select.GetTextCtrl().Bind(wx.EVT_TEXT, self.OnMap)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnElementType, self.vectorTypeRadio)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnElementType, self.rasterTypeRadio)
        self.Bind(wx.EVT_CHECKBOX, self.OnBorder, self.borderCheck)
        
        
     
    def OnMap(self, event):
        """!Selected map or region changing"""
        
        if self.select.GetValue():
            self.selected = self.select.GetValue() 
        else:
            self.selected = None

        if self.scaleChoice.GetSelection() == 0:
            self.selectedMap = self.selected
            if self.rasterTypeRadio.GetValue():
                mapType = 'raster' 
            else:
                mapType = 'vector'

            self.scale[0], self.center[0], foo = AutoAdjust(self, scaleType = 0, map = self.selected,
                                                mapType = mapType, rect = self.mapFrameDict['rect'])
            #self.center[0] = self.RegionCenter(self.RegionDict(scaleType = 0))

        elif self.scaleChoice.GetSelection() == 1:
            self.selectedRegion = self.selected
            self.scale[1], self.center[1],  foo = AutoAdjust(self, scaleType = 1, region = self.selected, rect = self.mapFrameDict['rect'])
            #self.center[1] = self.RegionCenter(self.RegionDict(scaleType = 1))
        elif self.scaleChoice.GetSelection() == 2:
            self.scale[2], self.center[2], foo = AutoAdjust(self, scaleType = 2, rect = self.mapFrameDict['rect'])
            #self.center[2] = self.RegionCenter(self.RegionDict(scaleType = 2))
            
        else:
            self.scale[3] = None        
            self.center[3] = None
            
        self.OnScaleChoice(None)
        
            
    def OnScaleChoice(self, event):
        """!Selected scale type changing"""
        
        scaleType = self.scaleChoice.GetSelection()
        if self.scaleType != scaleType:
            self.scaleType = scaleType
            self.select.SetValue("")
        
        if scaleType in (0, 1): # automatic - region from raster map, saved region
            if scaleType == 0:
                # set map selection
                self.rasterTypeRadio.Show()
                self.vectorTypeRadio.Show()
                self.drawMap.Show()
                self.staticBox.SetLabel(" %s " % _("Map selection"))
                if self.rasterTypeRadio.GetValue():
                    stype = 'raster' 
                else:
                    stype = 'vector'

                self.select.SetElementList(type = stype)
                self.mapText.SetLabel(self.mapOrRegionText[0])
                self.select.SetToolTipString(_("Region is set to match this map,\nraster or vector map must be added later"))
                    
            if scaleType == 1:
                # set region selection
                self.rasterTypeRadio.Hide()
                self.vectorTypeRadio.Hide()
                self.drawMap.Hide()
                self.staticBox.SetLabel(" %s " % _("Region selection"))
                stype = 'region'
                self.select.SetElementList(type = stype)
                self.mapText.SetLabel(self.mapOrRegionText[1])
                self.select.SetToolTipString("")

            for each in self.mapSizer.GetChildren():
                each.GetWindow().Enable()
            for each in self.centerSizer.GetChildren():
                each.GetWindow().Disable()
                    
            if self.scale[scaleType]:
                
                self.scaleTextCtrl.SetValue("%.0f" % (1/self.scale[scaleType]))
            if self.center[scaleType]:
                self.eastingTextCtrl.SetValue(str(self.center[scaleType][0]))
                self.northingTextCtrl.SetValue(str(self.center[scaleType][1]))
        elif scaleType == 2:
            for each in self.mapSizer.GetChildren():
                each.GetWindow().Disable()
            for each in self.centerSizer.GetChildren():
                each.GetWindow().Disable()
                
            if self.scale[scaleType]:
                self.scaleTextCtrl.SetValue("%.0f" % (1/self.scale[scaleType]))
            if self.center[scaleType]:
                self.eastingTextCtrl.SetValue(str(self.center[scaleType][0]))
                self.northingTextCtrl.SetValue(str(self.center[scaleType][1]))
        else: # fixed
            for each in self.mapSizer.GetChildren():
                each.GetWindow().Disable()
            for each in self.centerSizer.GetChildren():
                each.GetWindow().Enable()
                    
            if self.scale[scaleType]:
                self.scaleTextCtrl.SetValue("%.0f" % (1/self.scale[scaleType]))
            if self.center[scaleType]:
                self.eastingTextCtrl.SetValue(str(self.center[scaleType][0]))
                self.northingTextCtrl.SetValue(str(self.center[scaleType][1]))
                
    def OnElementType(self, event):
        """!Changes data in map selection tree ctrl popup"""
        if self.rasterTypeRadio.GetValue():
            mapType = 'raster'
        else:
            mapType = 'vector'
        self.select.SetElementList(type  = mapType)
        if self.mapType != mapType and event is not None:
            self.mapType = mapType
            self.select.SetValue('')
        self.mapType = mapType    
        
    def OnBorder(self, event):
        """!Enables/disable the part relating to border of map frame"""
        for each in (self.borderColorText, self.borderWidthText, self.borderColourPicker, self.borderWidthCtrl):
            each.Enable(self.borderCheck.GetValue())
            
    def getId(self):
        """!Returns id of raster map"""
        return self.id
            
    def update(self):
        """!Save changes"""
        mapFrameDict = dict(self.mapFrameDict)
        # resolution
        mapFrameDict['resolution'] = self.resolutionSpin.GetValue()
        #scale
        scaleType = self.scaleType
        mapFrameDict['scaleType'] = scaleType
        
        if mapFrameDict['scaleType'] == 0:
            if self.select.GetValue():
                mapFrameDict['drawMap'] = self.drawMap.GetValue()
                mapFrameDict['map'] = self.select.GetValue()
                mapFrameDict['mapType'] = self.mapType
                mapFrameDict['region'] = None
                
                if mapFrameDict['drawMap']:

                    if mapFrameDict['mapType'] == 'raster':
                        mapFile = grass.find_file(mapFrameDict['map'], element = 'cell')
                        if mapFile['file'] == '':
                            GMessage("Raster %s not found" % mapFrameDict['map'])
                            return False
                        raster = self.instruction.FindInstructionByType('raster')
                        if raster:
                            raster['raster'] = mapFrameDict['map']
                        else:
                            raster = Raster(wx.NewId())
                            raster['raster'] = mapFrameDict['map']
                            raster['isRaster'] = True
                            self.instruction.AddInstruction(raster)

                    elif mapFrameDict['mapType'] == 'vector':
                        
                        mapFile = grass.find_file(mapFrameDict['map'], element = 'vector')
                        if mapFile['file'] == '':
                            GMessage("Vector %s not found" % mapFrameDict['map'])
                            return False
                        
                        vector = self.instruction.FindInstructionByType('vector')
                        isAdded = False
                        if vector:
                            for each in vector['list']:
                                if each[0] == mapFrameDict['map']:
                                    isAdded = True
                        if not isAdded:
                            topoInfo = grass.vector_info_topo(map = mapFrameDict['map'])
                            if topoInfo:
                                if bool(topoInfo['areas']):
                                    topoType = 'areas'
                                elif bool(topoInfo['lines']):
                                    topoType = 'lines'
                                else:
                                    topoType = 'points'
                                label = '('.join(mapFrameDict['map'].split('@')) + ')'
                           
                                if not vector:
                                    vector = Vector(wx.NewId())
                                    vector['list'] = []
                                    self.instruction.AddInstruction(vector)
                                id = wx.NewId()
                                vector['list'].insert(0, [mapFrameDict['map'], topoType, id, 1, label])
                                vProp = VProperties(id, topoType)
                                vProp['name'], vProp['label'], vProp['lpos'] = mapFrameDict['map'], label, 1
                                self.instruction.AddInstruction(vProp)
                            else:
                                return False
                            
                self.scale[0], self.center[0], self.rectAdjusted = AutoAdjust(self, scaleType = 0, map = mapFrameDict['map'],
                                                                   mapType = self.mapType, rect = self.mapFrameDict['rect'])
                                               
                if self.rectAdjusted:
                    mapFrameDict['rect'] = self.rectAdjusted 
                else:
                    mapFrameDict['rect'] = self.mapFrameDict['rect']

                mapFrameDict['scale'] = self.scale[0]
                
                mapFrameDict['center'] = self.center[0]
                # set region
                if self.mapType == 'raster':
                    RunCommand('g.region', rast = mapFrameDict['map'])
                if self.mapType == 'vector':
                    raster = self.instruction.FindInstructionByType('raster')
                    if raster:
                        rasterId = raster.id 
                    else:
                        rasterId = None

                    if rasterId:
                        
                        RunCommand('g.region', vect = mapFrameDict['map'], rast = self.instruction[rasterId]['raster'])
                    else:
                        RunCommand('g.region', vect = mapFrameDict['map'])
                
                    
                
            else:
                wx.MessageBox(message = _("No map selected!"),
                                    caption = _('Invalid input'), style = wx.OK|wx.ICON_ERROR)
                return False    
            
        elif mapFrameDict['scaleType'] == 1:
            if self.select.GetValue():
                mapFrameDict['drawMap'] = False
                mapFrameDict['map'] = None
                mapFrameDict['mapType'] = None
                mapFrameDict['region'] = self.select.GetValue()
                self.scale[1], self.center[1], self.rectAdjusted = AutoAdjust(self, scaleType = 1, region = mapFrameDict['region'],
                                                                                rect = self.mapFrameDict['rect'])
                if self.rectAdjusted:
                    mapFrameDict['rect'] = self.rectAdjusted 
                else:
                    mapFrameDict['rect'] = self.mapFrameDict['rect']

                mapFrameDict['scale'] = self.scale[1]
                mapFrameDict['center'] = self.center[1]
                # set region
                RunCommand('g.region', region = mapFrameDict['region'])
            else:
                wx.MessageBox(message = _("No region selected!"),
                                    caption = _('Invalid input'), style = wx.OK|wx.ICON_ERROR)
                return False 
                               
        elif scaleType == 2:
            mapFrameDict['drawMap'] = False
            mapFrameDict['map'] = None
            mapFrameDict['mapType'] = None
            mapFrameDict['region'] = None
            self.scale[2], self.center[2], self.rectAdjusted = AutoAdjust(self, scaleType = 2, rect = self.mapFrameDict['rect'])
            if self.rectAdjusted:
                mapFrameDict['rect'] = self.rectAdjusted 
            else:
                mapFrameDict['rect'] = self.mapFrameDict['rect']

            mapFrameDict['scale'] = self.scale[2]
            mapFrameDict['center'] = self.center[2]
            
            env = grass.gisenv()
            windFilePath = os.path.join(env['GISDBASE'], env['LOCATION_NAME'], env['MAPSET'], 'WIND')
            try:
                windFile = open(windFilePath, 'r').read()
                region = grass.parse_key_val(windFile, sep = ':', val_type = float)
            except IOError:
                region = grass.region()
            
            raster = self.instruction.FindInstructionByType('raster')
            if raster:
                rasterId = raster.id 
            else:
                rasterId = None

            if rasterId: # because of resolution
                RunCommand('g.region', n = region['north'], s = region['south'],
                            e = region['east'], w = region['west'], rast = self.instruction[rasterId]['raster'])
            else:
                RunCommand('g.region', n = region['north'], s = region['south'],
                           e = region['east'], w = region['west'])
            
        elif scaleType == 3:
            mapFrameDict['drawMap'] = False
            mapFrameDict['map'] = None
            mapFrameDict['mapType'] = None
            mapFrameDict['region'] = None
            mapFrameDict['rect'] = self.mapFrameDict['rect']
            try:
                scaleNumber = float(self.scaleTextCtrl.GetValue())
                centerE = float(self.eastingTextCtrl.GetValue()) 
                centerN = float(self.northingTextCtrl.GetValue())
            except (ValueError, SyntaxError):
                wx.MessageBox(message = _("Invalid scale or map center!"),
                                    caption = _('Invalid input'), style = wx.OK|wx.ICON_ERROR)
                return False  
            mapFrameDict['scale'] = 1/scaleNumber
            mapFrameDict['center'] = centerE, centerN
        
            ComputeSetRegion(self, mapDict = mapFrameDict)
        
        # check resolution
        SetResolution(dpi = mapFrameDict['resolution'], width = mapFrameDict['rect'].width,
                                                        height = mapFrameDict['rect'].height)
        # border
        if self.borderCheck.GetValue():
            mapFrameDict['border'] = 'y' 
        else:
            mapFrameDict['border'] = 'n'

        if mapFrameDict['border'] == 'y':
            mapFrameDict['width'] = self.borderWidthCtrl.GetValue()
            mapFrameDict['color'] = convertRGB(self.borderColourPicker.GetColour())
            
        if self.id not in self.instruction:
            mapFrame = MapFrame(self.id)
            self.instruction.AddInstruction(mapFrame)
        self.instruction[self.id].SetInstruction(mapFrameDict)

        if self.id not in self.mapDialog.parent.objectId:
            self.mapDialog.parent.objectId.insert(0, self.id)# map frame is drawn first
        return True
        
class RasterPanel(wx.Panel):
    """!Panel for raster map settings"""
    def __init__(self, parent, id, settings,  notebook = True):
        wx.Panel.__init__(self, parent, id = wx.ID_ANY, style = wx.TAB_TRAVERSAL)
        self.instruction = settings
        
        if notebook:
            self.book = parent
            self.book.AddPage(page = self, text = _("Raster map"))
            self.mainDialog = self.book.GetParent()
        else:
            self.mainDialog = parent
        if id:
            self.id = id
            self.rasterDict = self.instruction[self.id].GetInstruction()
        else:
            self.id = wx.NewId()
            raster = Raster(self.id)
            self.rasterDict = raster.GetInstruction()
            
            
        self._layout()
        self.OnRaster(None)
            
    def _layout(self):
        """!Do layout"""
        border = wx.BoxSizer(wx.VERTICAL)
        
        # choose raster map
        
        box   = wx.StaticBox (parent = self, id = wx.ID_ANY, label = " %s " % _("Choose raster map"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        self.rasterNoRadio = wx.RadioButton(self, id = wx.ID_ANY, label = _("no raster map"), style = wx.RB_GROUP)
        self.rasterYesRadio = wx.RadioButton(self, id = wx.ID_ANY, label = _("raster:"))
        
        self.rasterSelect = Select(self, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE,
                             type = 'raster', multiple = False,
                             updateOnPopup = True, onPopup = None)
        if self.rasterDict['isRaster']:
            self.rasterYesRadio.SetValue(True)
            self.rasterNoRadio.SetValue(False)
            self.rasterSelect.SetValue(self.rasterDict['raster'])
        else:
            self.rasterYesRadio.SetValue(False)
            self.rasterNoRadio.SetValue(True)
            mapId = self.instruction.FindInstructionByType('map').id

            if self.instruction[mapId]['map'] and self.instruction[mapId]['mapType'] == 'raster':
                self.rasterSelect.SetValue(self.instruction[mapId]['map'])# raster map from map frame dialog if possible
            else:
                self.rasterSelect.SetValue('')                
        gridBagSizer.Add(self.rasterNoRadio, pos = (0, 0), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)            
        gridBagSizer.Add(self.rasterYesRadio, pos = (1, 0),  flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.rasterSelect, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        #self.rasterSelect.GetTextCtrl().Bind(wx.EVT_TEXT, self.OnRaster)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnRaster, self.rasterNoRadio)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnRaster, self.rasterYesRadio)
        
        self.SetSizer(border)
        self.Fit()
        
    def OnRaster(self, event):
        """!Enable/disable raster selection"""
        self.rasterSelect.Enable(self.rasterYesRadio.GetValue())
        
    def update(self):
        #draw raster
        mapInstr = self.instruction.FindInstructionByType('map')
        if not mapInstr: # no map frame
            GMessage(message = _("Please, create map frame first."))
            return
            
        if self.rasterNoRadio.GetValue() or not self.rasterSelect.GetValue():
            self.rasterDict['isRaster'] = False
            self.rasterDict['raster'] = None
            mapInstr['drawMap'] = False
            if self.id in self.instruction:
                del self.instruction[self.id]

        else:
            self.rasterDict['isRaster'] = True
            self.rasterDict['raster'] = self.rasterSelect.GetValue()
            if self.rasterDict['raster'] != mapInstr['drawMap']:
                mapInstr['drawMap'] = False

            raster = self.instruction.FindInstructionByType('raster')
            if not raster:
                raster = Raster(self.id)
                self.instruction.AddInstruction(raster)
                self.instruction[self.id].SetInstruction(self.rasterDict)
            else:
                self.instruction[raster.id].SetInstruction(self.rasterDict)
            
        if 'map' in self.mainDialog.parent.openDialogs:
            self.mainDialog.parent.openDialogs['map'].updateDialog()
        return True
        
    def getId(self):
        return self.id
  
class VectorPanel(wx.Panel):
    """!Panel for vector maps settings"""
    def __init__(self, parent, id, settings, notebook = True):
        wx.Panel.__init__(self, parent, id = wx.ID_ANY, style = wx.TAB_TRAVERSAL)
        
        self.parent = parent
        self.instruction = settings
        self.tmpDialogDict = {}
        vectors = self.instruction.FindInstructionByType('vProperties', list = True)
        for vector in vectors:
            self.tmpDialogDict[vector.id] = dict(self.instruction[vector.id].GetInstruction())
        
        if id:
            self.id = id
            self.vectorList = deepcopy(self.instruction[id]['list'])
        else:
            self.id = wx.NewId()
            self.vectorList = []

        vLegend = self.instruction.FindInstructionByType('vectorLegend')
        if vLegend:
            self.vLegendId = vLegend.id 
        else:
            self.vLegendId = None

         
        self._layout()
        
        if notebook:
            self.parent.AddPage(page = self, text = _("Vector maps"))
            self.parent = self.parent.GetParent()
            
    def _layout(self):
        """!Do layout"""
        border = wx.BoxSizer(wx.VERTICAL)
        
        # choose vector map
        
        box   = wx.StaticBox (parent = self, id = wx.ID_ANY, label = " %s " % _("Add map"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        text = wx.StaticText(self, id = wx.ID_ANY, label = _("Map:"))
        self.select = Select(self, id = wx.ID_ANY,# size = globalvar.DIALOG_GSELECT_SIZE,
                             type = 'vector', multiple = False,
                             updateOnPopup = True, onPopup = None)
        topologyTypeTr = [_("points"), _("lines"), _("areas")]
        self.topologyTypeList = ["points", "lines", "areas"]
        self.vectorType = wx.RadioBox(self, id = wx.ID_ANY, label = " %s " % _("Data Type"), choices = topologyTypeTr,
                                      majorDimension = 3, style = wx.RA_SPECIFY_COLS)
            
        self.AddVector = wx.Button(self, id = wx.ID_ANY, label = _("Add"))
        
        gridBagSizer.Add(text, pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.select, pos = (0,1), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.vectorType, pos = (1,1), flag = wx.ALIGN_CENTER, border = 0)
        gridBagSizer.Add(self.AddVector, pos = (1,2), flag = wx.ALIGN_BOTTOM|wx.ALIGN_RIGHT, border = 0)
        
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        # manage vector layers
        
        box   = wx.StaticBox (parent = self, id = wx.ID_ANY, label = " %s " % _("Manage vector maps"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap = 5, vgap = 5)

        
        
        text = wx.StaticText(self, id = wx.ID_ANY, label = _("The topmost vector map overlaps the others"))
        self.listbox = wx.ListBox(self, id = wx.ID_ANY, choices = [], style = wx.LB_SINGLE|wx.LB_NEEDED_SB)
        self.btnUp = wx.Button(self, id = wx.ID_ANY, label = _("Up"))
        self.btnDown = wx.Button(self, id = wx.ID_ANY, label = _("Down"))
        self.btnDel = wx.Button(self, id = wx.ID_ANY, label = _("Delete"))
        self.btnProp = wx.Button(self, id = wx.ID_ANY, label = _("Properties..."))
        
        self.updateListBox(selected=0)
        
        
        gridBagSizer.Add(text, pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.listbox, pos = (1,0), span = (4, 1), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(self.btnUp, pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(self.btnDown, pos = (2,1), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(self.btnDel, pos = (3,1), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(self.btnProp, pos = (4,1), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        
        gridBagSizer.AddGrowableCol(0,2)
        gridBagSizer.AddGrowableCol(1,1)
        sizer.Add(gridBagSizer, proportion = 0, flag = wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        self.Bind(wx.EVT_BUTTON, self.OnAddVector, self.AddVector)
        self.Bind(wx.EVT_BUTTON, self.OnDelete, self.btnDel)
        self.Bind(wx.EVT_BUTTON, self.OnUp, self.btnUp)
        self.Bind(wx.EVT_BUTTON, self.OnDown, self.btnDown)
        self.Bind(wx.EVT_BUTTON, self.OnProperties, self.btnProp)
        self.select.GetTextCtrl().Bind(wx.EVT_TEXT, self.OnVector)
        
        self.SetSizer(border)
        self.Fit()
        
        self.Bind(wx.EVT_LISTBOX_DCLICK, self.OnProperties, self.listbox)

    def OnVector(self, event):
        """!Gets info about toplogy and enables/disables choices point/line/area"""
        vmap = self.select.GetValue()   
        try:     
            topoInfo = grass.vector_info_topo(map = vmap)
        except grass.ScriptError:
            return
        
        if topoInfo:
            self.vectorType.EnableItem(2, bool(topoInfo['areas']))
            self.vectorType.EnableItem(1, bool(topoInfo['boundaries']) or bool(topoInfo['lines']))
            self.vectorType.EnableItem(0, bool(topoInfo['centroids'] or bool(topoInfo['points']) ))
            for item in range(2,-1,-1):
                if self.vectorType.IsItemEnabled(item):
                    self.vectorType.SetSelection(item)
                    break
            
            self.AddVector.SetFocus()        
            
    def OnAddVector(self, event):
        """!Adds vector map to list"""
        vmap = self.select.GetValue()
        if vmap:
            mapname = vmap.split('@')[0]
            try:
                mapset = '(' + vmap.split('@')[1] + ')'
            except IndexError:
                mapset = ''
            idx = self.vectorType.GetSelection()
            ttype = self.topologyTypeList[idx]
            record = "%s - %s" % (vmap, ttype)
            id = wx.NewId()
            lpos = 1
            label = mapname + mapset 
            self.vectorList.insert(0, [vmap, ttype, id, lpos, label])
            self.reposition()
            self.listbox.InsertItems([record], 0)
            
            vector = VProperties(id, ttype)
            self.tmpDialogDict[id] = vector.GetInstruction()
            self.tmpDialogDict[id]['name'] = vmap

            
            self.listbox.SetSelection(0)  
            self.listbox.EnsureVisible(0)
            self.btnProp.SetFocus()
            self.enableButtons()
            
    def OnDelete(self, event):
        """!Deletes vector map from the list"""
        if self.listbox.GetSelections():
            pos = self.listbox.GetSelection()
            id = self.vectorList[pos][2]
            del self.vectorList[pos]
            del self.tmpDialogDict[id]
            
            for i in range(pos, len(self.vectorList)):
                if self.vectorList[i][3]:# can be 0
                    self.vectorList[i][3] -= 1
            
            if pos < len(self.vectorList) -1:
                selected = pos
            else:
                selected = len(self.vectorList) -1
            self.updateListBox(selected = selected)
            if self.listbox.IsEmpty():
                self.enableButtons(False)
            
            
    def OnUp(self, event):
        """!Moves selected map to top"""
        if self.listbox.GetSelections():
            pos = self.listbox.GetSelection()
            if pos:
                self.vectorList.insert(pos - 1, self.vectorList.pop(pos))
            if not self.vLegendId:
                self.reposition()
                
            if pos > 0:
                self.updateListBox(selected = (pos - 1)) 
            else:
                self.updateListBox(selected = 0)

            
    def OnDown(self, event):
        """!Moves selected map to bottom"""
        if self.listbox.GetSelections():
            pos = self.listbox.GetSelection()
            if pos != len(self.vectorList) - 1:
                self.vectorList.insert(pos + 1, self.vectorList.pop(pos))
                if not self.vLegendId:
                    self.reposition()
            if pos < len(self.vectorList) -1:
                self.updateListBox(selected = (pos + 1)) 
            else:
                self.updateListBox(selected = len(self.vectorList) -1)

    
    def OnProperties(self, event):
        """!Opens vector map properties dialog"""
        if self.listbox.GetSelections():
            pos = self.listbox.GetSelection()
            id = self.vectorList[pos][2]

            dlg = VPropertiesDialog(self, id = id, settings = self.instruction, 
                                    vectors = self.vectorList, tmpSettings = self.tmpDialogDict[id])
            dlg.ShowModal()
            
            self.parent.FindWindowById(wx.ID_OK).SetFocus()
           
    def enableButtons(self, enable = True):
        """!Enable/disable up, down, properties, delete buttons"""
        self.btnUp.Enable(enable)
        self.btnDown.Enable(enable)
        self.btnProp.Enable(enable)
        self.btnDel.Enable(enable)
    
    def updateListBox(self, selected = None):
        mapList = ["%s - %s" % (item[0], item[1]) for item in self.vectorList]
        self.listbox.Set(mapList)
        if self.listbox.IsEmpty():
            self.enableButtons(False)
        else:
            self.enableButtons(True)
            if selected is not None:
                self.listbox.SetSelection(selected)  
                self.listbox.EnsureVisible(selected)  
              
    def reposition(self):
        """!Update position in legend, used only if there is no vlegend yet"""
        for i in range(len(self.vectorList)):
            if self.vectorList[i][3]:
                self.vectorList[i][3] = i + 1
                
    def getId(self):
        return self.id
        
    def update(self):
        vectors = self.instruction.FindInstructionByType('vProperties', list = True)
        
        for vector in vectors:
            del self.instruction[vector.id]
        if self.id in self.instruction:
            del self.instruction[self.id] 

        if len(self.vectorList) > 0:
            vector = Vector(self.id)
            self.instruction.AddInstruction(vector)

            vector.SetInstruction({'list': deepcopy(self.vectorList)})
            
            # save new vectors
            for item in self.vectorList:
                id = item[2]

                vLayer = VProperties(id, item[1])
                self.instruction.AddInstruction(vLayer)
                vLayer.SetInstruction(self.tmpDialogDict[id])
                vLayer['name'] = item[0]
                vLayer['label'] = item[4]
                vLayer['lpos'] = item[3]
            
        else:
            if self.id in self.instruction:
                del self.instruction[self.id]
                
        if 'map' in self.parent.parent.openDialogs:
            self.parent.parent.openDialogs['map'].updateDialog()

        return True
    
class RasterDialog(PsmapDialog):
    def __init__(self, parent, id, settings):
        PsmapDialog.__init__(self, parent = parent, id = id, title = _("Raster map settings"), settings = settings)
        self.objectType = ('raster',)
        
        self.rPanel = RasterPanel(parent = self, id = self.id, settings = self.instruction, notebook = False)

        self.id = self.rPanel.getId()
        self._layout(self.rPanel)
    
    def update(self):
        ok = self.rPanel.update()
        if ok:
            return True
        return False
    
    def OnApply(self, event):
        ok = self.update()
        if not ok:
            return False

        if self.id in self.instruction:
            self.parent.DialogDataChanged(id = self.id)
        else:
            mapId = self.instruction.FindInstructionByType('map').id
            self.parent.DialogDataChanged(id = mapId)
        return True
    
    def updateDialog(self):
        """!Update information (not used)"""
        pass
##        if 'map' in self.parent.openDialogs:
##            if self.parent.openDialogs['map'].mPanel.rasterTypeRadio.GetValue()\
##                    and self.parent.openDialogs['map'].mPanel.select.GetValue():
##                if self.parent.openDialogs['map'].mPanel.drawMap.IsChecked():
##                    self.rPanel.rasterSelect.SetValue(self.parent.openDialogs['map'].mPanel.select.GetValue())   
                
class MainVectorDialog(PsmapDialog):
    def __init__(self, parent, id, settings):
        PsmapDialog.__init__(self, parent = parent, id = id, title = _("Vector maps settings"), settings = settings)
        self.objectType = ('vector',)
        self.vPanel = VectorPanel(parent = self, id = self.id, settings = self.instruction, notebook = False)

        self.id = self.vPanel.getId()
        self._layout(self.vPanel)
    
    def update(self):
        self.vPanel.update()
        
    def OnApply(self, event):
        self.update()
        if self.id in self.instruction:
            self.parent.DialogDataChanged(id = self.id)
        else:
            mapId = self.instruction.FindInstructionByType('map').id
            self.parent.DialogDataChanged(id = mapId)
        return True
        
    def updateDialog(self):
        """!Update information (not used)"""
        pass
        
class VPropertiesDialog(PsmapDialog):
    def __init__(self, parent, id, settings, vectors, tmpSettings):
        PsmapDialog.__init__(self, parent = parent, id = id, title = "", settings = settings, apply = False)
        
        vectorList = vectors
        self.vPropertiesDict = tmpSettings
        
        # determine map and its type
        for item in vectorList:
            if id == item[2]:
                self.vectorName = item[0]
                self.type = item[1]
        self.SetTitle(_("%s properties") % self.vectorName)
        
        #vector map info
        self.connection = True
        try:
            self.mapDBInfo = VectorDBInfo(self.vectorName)
            self.layers = self.mapDBInfo.layers.keys()
        except grass.ScriptError:
            self.connection = False
            self.layers = []
        if not self.layers:
            self.connection = False
            self.layers = []
            
        self.currLayer = self.vPropertiesDict['layer']
        
        #path to symbols, patterns
        gisbase = os.getenv("GISBASE")
        self.symbolPath = os.path.join(gisbase, 'etc', 'symbol')
        self.symbols = []
        for dir in os.listdir(self.symbolPath):
            for symbol in os.listdir(os.path.join(self.symbolPath, dir)):
                self.symbols.append(os.path.join(dir, symbol))
        self.patternPath = os.path.join(gisbase, 'etc', 'paint', 'patterns')

        #notebook
        notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)
        self.DSpanel = self._DataSelectionPanel(notebook)
        self.EnableLayerSelection(enable = self.connection)
        selectPanel = { 'points': [self._ColorsPointAreaPanel, self._StylePointPanel], 
                        'lines': [self._ColorsLinePanel, self._StyleLinePanel], 
                        'areas': [self._ColorsPointAreaPanel, self._StyleAreaPanel]}
        self.ColorsPanel = selectPanel[self.type][0](notebook)
        
        self.OnOutline(None)
        if self.type in ('points', 'areas'):
            self.OnFill(None)
        self.OnColor(None)
        
        self.StylePanel = selectPanel[self.type][1](notebook)
        if self.type == 'points':
            self.OnSize(None)
            self.OnRotation(None)
            self.OnSymbology(None)
        if self.type == 'areas':
            self.OnPattern(None)
        
        self._layout(notebook)
        
    def _DataSelectionPanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, size = (-1, -1), style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = _("Data selection"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        
        # data type
        self.checkType1 = self.checkType2 = None
        if self.type in ('lines', 'points'):
            box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Feature type"))
            sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
            gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
            if self.type == 'points':
                label = (_("points"), _("centroids"))
            else: 
                label = (_("lines"), _("boundaries"))
            if self.type == 'points':
                name = ("point", "centroid")
            else:
                name = ("line", "boundary")
            self.checkType1 = wx.CheckBox(panel, id = wx.ID_ANY, label = label[0], name = name[0])
            self.checkType2 = wx.CheckBox(panel, id = wx.ID_ANY, label = label[1], name = name[1])
            self.checkType1.SetValue(self.vPropertiesDict['type'].find(name[0]) >= 0)
            self.checkType2.SetValue(self.vPropertiesDict['type'].find(name[1]) >= 0)
            
            gridBagSizer.Add(self.checkType1, pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            gridBagSizer.Add(self.checkType2, pos = (0,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
            border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        # layer selection
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Layer selection"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        self.gridBagSizerL = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        self.warning =  wx.StaticText(panel, id = wx.ID_ANY, label = "")
        if not self.connection:
            self.warning = wx.StaticText(panel, id = wx.ID_ANY, label = _("Database connection is not defined in DB file."))
        text = wx.StaticText(panel, id = wx.ID_ANY, label = _("Select layer:"))
        self.layerChoice = wx.Choice(panel, id = wx.ID_ANY, choices = map(str, self.layers), size = self.spinCtrlSize)
        
        self.layerChoice.SetStringSelection(self.currLayer)
                
        if self.connection:
            table = self.mapDBInfo.layers[int(self.currLayer)]['table'] 
        else:
            table = ""

        self.radioWhere = wx.RadioButton(panel, id = wx.ID_ANY, label = "SELECT * FROM %s WHERE" % table, style = wx.RB_GROUP)
        self.textCtrlWhere = wx.TextCtrl(panel, id = wx.ID_ANY, value = "")
        
        
        if self.connection:
            cols = self.mapDBInfo.GetColumns(self.mapDBInfo.layers[int(self.currLayer)]['table']) 
        else:
            cols = []

        self.choiceColumns = wx.Choice(panel, id = wx.ID_ANY, choices = cols)
        
        self.radioCats = wx.RadioButton(panel, id = wx.ID_ANY, label = "Choose categories ")
        self.textCtrlCats = wx.TextCtrl(panel, id = wx.ID_ANY, value = "")
        self.textCtrlCats.SetToolTipString(_("list of categories (e.g. 1,3,5-7)"))
        
        if self.vPropertiesDict.has_key('cats'):
            self.radioCats.SetValue(True)
            self.textCtrlCats.SetValue(self.vPropertiesDict['cats'])
        if self.vPropertiesDict.has_key('where'):
            self.radioWhere.SetValue(True)
            where = self.vPropertiesDict['where'].strip().split(" ",1)
            self.choiceColumns.SetStringSelection(where[0])
            self.textCtrlWhere.SetValue(where[1])
            
        row = 0
        if not self.connection:
            self.gridBagSizerL.Add(self.warning, pos = (0,0), span = (1,3), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            row = 1
        self.gridBagSizerL.Add(text, pos = (0 + row,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerL.Add(self.layerChoice, pos = (0 + row,1), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        self.gridBagSizerL.Add(self.radioWhere, pos = (1 + row,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerL.Add(self.choiceColumns, pos = (1 + row,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        self.gridBagSizerL.Add(self.textCtrlWhere, pos = (1 + row,2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerL.Add(self.radioCats, pos = (2 + row,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerL.Add(self.textCtrlCats, pos = (2 + row,1), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        
        sizer.Add(self.gridBagSizerL, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        #mask
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Mask"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        
        self.mask = wx.CheckBox(panel, id = wx.ID_ANY, label = _("Use current mask"))
        if self.vPropertiesDict['masked'] == 'y':
            self.mask.SetValue(True) 
        else:
            self.mask.SetValue(False)
        
        sizer.Add(self.mask, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)

        self.Bind(wx.EVT_CHOICE, self.OnLayer, self.layerChoice)
        
        panel.SetSizer(border)
        panel.Fit()
        return panel
    
    def _ColorsPointAreaPanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, size = (-1, -1), style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = _("Colors"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        
        #colors - outline
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Outline"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        self.gridBagSizerO = wx.GridBagSizer(hgap = 5, vgap = 2)
        
        self.outlineCheck = wx.CheckBox(panel, id = wx.ID_ANY, label = _("draw outline"))
        self.outlineCheck.SetValue(self.vPropertiesDict['color'] != 'none')
        
        widthText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Width (pts):"))
        if fs:
            self.widthSpin = fs.FloatSpin(panel, id = wx.ID_ANY, min_val = 0, max_val = 30,
                                          increment = 0.5, value = 1, style = fs.FS_RIGHT)
            self.widthSpin.SetFormat("%f")
            self.widthSpin.SetDigits(2)
        else:
            self.widthSpin = wx.SpinCtrl(panel, id = wx.ID_ANY, min = 1, max = 25, initial = 1,
                                         size = self.spinCtrlSize)
                                        
        if self.vPropertiesDict['color'] == None:
            self.vPropertiesDict['color'] = 'none'
 
        if self.vPropertiesDict['color'] != 'none':
            self.widthSpin.SetValue(self.vPropertiesDict['width'] )
        else:
            self.widthSpin.SetValue(1)

        colorText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Color:"))
        self.colorPicker = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        if self.vPropertiesDict['color'] != 'none':
            self.colorPicker.SetColour(convertRGB(self.vPropertiesDict['color'])) 
        else:
            self.colorPicker.SetColour(convertRGB('black'))

        self.gridBagSizerO.Add(self.outlineCheck, pos = (0, 0), span = (1,2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        self.gridBagSizerO.Add(widthText, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        self.gridBagSizerO.Add(self.widthSpin, pos = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)        
        self.gridBagSizerO.Add(colorText, pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)                
        self.gridBagSizerO.Add(self.colorPicker, pos = (2, 2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        
        sizer.Add(self.gridBagSizerO, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        self.Bind(wx.EVT_CHECKBOX, self.OnOutline, self.outlineCheck)
        
        #colors - fill
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Fill")) 
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        self.gridBagSizerF = wx.GridBagSizer(hgap = 5, vgap = 2)
       
        self.fillCheck = wx.CheckBox(panel, id = wx.ID_ANY, label = _("fill color"))
        self.fillCheck.SetValue(self.vPropertiesDict['fcolor'] != 'none' or self.vPropertiesDict['rgbcolumn'] is not None)

        self.colorPickerRadio = wx.RadioButton(panel, id = wx.ID_ANY, label = _("choose color:"), style = wx.RB_GROUP)
        #set choose color option if there is no db connection
        if self.connection:
            self.colorPickerRadio.SetValue(not self.vPropertiesDict['rgbcolumn'])
        else:
            self.colorPickerRadio.SetValue(False)            
        self.fillColorPicker = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        if self.vPropertiesDict['fcolor'] != 'none':
            self.fillColorPicker.SetColour(convertRGB(self.vPropertiesDict['fcolor'])) 
        else:
            self.fillColorPicker.SetColour(convertRGB('red'))        
        
        self.colorColRadio = wx.RadioButton(panel, id = wx.ID_ANY, label = _("color from map table column:"))
        self.colorColChoice = self.getColsChoice(parent = panel)
        if self.connection:
            if self.vPropertiesDict['rgbcolumn']:
                self.colorColRadio.SetValue(True)
                self.colorColChoice.SetStringSelection(self.vPropertiesDict['rgbcolumn'])
            else:
                self.colorColRadio.SetValue(False)
                self.colorColChoice.SetSelection(0)
        self.colorColChoice.Enable(self.connection)
        self.colorColRadio.Enable(self.connection)
        
        self.gridBagSizerF.Add(self.fillCheck, pos = (0, 0), span = (1,2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerF.Add(self.colorPickerRadio, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        self.gridBagSizerF.Add(self.fillColorPicker, pos = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        self.gridBagSizerF.Add(self.colorColRadio, pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        self.gridBagSizerF.Add(self.colorColChoice, pos = (2, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)        
        
        sizer.Add(self.gridBagSizerF, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)

        self.Bind(wx.EVT_CHECKBOX, self.OnFill, self.fillCheck)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnColor, self.colorColRadio)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnColor, self.colorPickerRadio)
        
        panel.SetSizer(border)
        panel.Fit()
        return panel
    
    def _ColorsLinePanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, size = (-1, -1), style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = _("Colors"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        
        #colors - outline
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Outline"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        self.gridBagSizerO = wx.GridBagSizer(hgap = 5, vgap = 2)
        
        if self.vPropertiesDict['hcolor'] == None:
            self.vPropertiesDict['hcolor'] = 'none'
        if self.vPropertiesDict['color'] == None:
            self.vPropertiesDict['color'] = 'none'
        
        self.outlineCheck = wx.CheckBox(panel, id = wx.ID_ANY, label = _("draw outline"))
        self.outlineCheck.SetValue(self.vPropertiesDict['hcolor'] != 'none')
        self.outlineCheck.SetToolTipString(_("No effect for fill color from table column"))
        
        widthText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Width (pts):"))
        
        if fs:
            self.outWidthSpin = fs.FloatSpin(panel, id = wx.ID_ANY, min_val = 0, max_val = 30,
                                             increment = 0.5, value = 1, style = fs.FS_RIGHT)
            self.outWidthSpin.SetFormat("%f")
            self.outWidthSpin.SetDigits(1)
        else:
            self.outWidthSpin = wx.SpinCtrl(panel, id = wx.ID_ANY, min = 1, max = 30, initial = 1,
                                         size = self.spinCtrlSize)
        
        if self.vPropertiesDict['hcolor'] != 'none':
            self.outWidthSpin.SetValue(self.vPropertiesDict['hwidth'] )
        else:
            self.outWidthSpin.SetValue(1)

        colorText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Color:"))
        self.colorPicker = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        if self.vPropertiesDict['hcolor'] != 'none':
            self.colorPicker.SetColour(convertRGB(self.vPropertiesDict['hcolor']) )
        else:
            self.colorPicker.SetColour(convertRGB('black'))

        
        self.gridBagSizerO.Add(self.outlineCheck, pos = (0, 0), span = (1,2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        self.gridBagSizerO.Add(widthText, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        self.gridBagSizerO.Add(self.outWidthSpin, pos = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)        
        self.gridBagSizerO.Add(colorText, pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)                
        self.gridBagSizerO.Add(self.colorPicker, pos = (2, 2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        
        sizer.Add(self.gridBagSizerO, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        self.Bind(wx.EVT_CHECKBOX, self.OnOutline, self.outlineCheck)
        
        #colors - fill
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Fill"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        self.gridBagSizerF = wx.GridBagSizer(hgap = 5, vgap = 2)
       
        fillText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Color of lines:"))

        self.colorPickerRadio = wx.RadioButton(panel, id = wx.ID_ANY, label = _("choose color:"), style = wx.RB_GROUP)

        #set choose color option if there is no db connection
        if self.connection:
            self.colorPickerRadio.SetValue(not self.vPropertiesDict['rgbcolumn'])
        else:
            self.colorPickerRadio.SetValue(False)            
        self.fillColorPicker = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        if self.vPropertiesDict['color'] != 'none':
            self.fillColorPicker.SetColour(convertRGB(self.vPropertiesDict['color']) )
        else:
            self.fillColorPicker.SetColour(convertRGB('black'))
        
        self.colorColRadio = wx.RadioButton(panel, id = wx.ID_ANY, label = _("color from map table column:"))
        self.colorColChoice = self.getColsChoice(parent = panel)
        if self.connection:
            if self.vPropertiesDict['rgbcolumn']:
                self.colorColRadio.SetValue(True)
                self.colorColChoice.SetStringSelection(self.vPropertiesDict['rgbcolumn'])
            else:
                self.colorColRadio.SetValue(False)
                self.colorColChoice.SetSelection(0)
        self.colorColChoice.Enable(self.connection)
        self.colorColRadio.Enable(self.connection)
        
        self.gridBagSizerF.Add(fillText, pos = (0, 0), span = (1,2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.gridBagSizerF.Add(self.colorPickerRadio, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        self.gridBagSizerF.Add(self.fillColorPicker, pos = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        self.gridBagSizerF.Add(self.colorColRadio, pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        self.gridBagSizerF.Add(self.colorColChoice, pos = (2, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)        
        
        sizer.Add(self.gridBagSizerF, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)

        self.Bind(wx.EVT_RADIOBUTTON, self.OnColor, self.colorColRadio)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnColor, self.colorPickerRadio)

        panel.SetSizer(border)
        panel.Fit()
        return panel
    
    def _StylePointPanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, size = (-1, -1), style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = _("Size and style"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        
        #symbology
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Symbology"))        
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
    
        self.symbolRadio = wx.RadioButton(panel, id = wx.ID_ANY, label = _("symbol:"), style = wx.RB_GROUP)
        self.symbolRadio.SetValue(bool(self.vPropertiesDict['symbol']))
            
        self.symbolName = wx.StaticText(panel, id = wx.ID_ANY)
        self.symbolName.SetLabel(self.vPropertiesDict['symbol'])
        bitmap = wx.Bitmap(os.path.join(globalvar.ETCSYMBOLDIR,
                                        self.vPropertiesDict['symbol']) + '.png')
        self.symbolButton = wx.BitmapButton(panel, id = wx.ID_ANY, bitmap = bitmap)
            
        self.epsRadio = wx.RadioButton(panel, id = wx.ID_ANY, label = _("eps file:"))
        self.epsRadio.SetValue(bool(self.vPropertiesDict['eps']))
        
        self.epsFileCtrl = filebrowse.FileBrowseButton(panel, id = wx.ID_ANY, labelText = '',
                                buttonText =  _("Browse"), toolTip = _("Type filename or click browse to choose file"), 
                                dialogTitle = _("Choose a file"), startDirectory = '', initialValue = '',
                                fileMask = "Encapsulated PostScript (*.eps)|*.eps|All files (*.*)|*.*", fileMode = wx.OPEN)
        if not self.vPropertiesDict['eps']:
            self.epsFileCtrl.SetValue('')
        else: #eps chosen
            self.epsFileCtrl.SetValue(self.vPropertiesDict['eps'])
            
        gridBagSizer.Add(self.symbolRadio, pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.symbolName, pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL | wx.LEFT, border = 10)
        gridBagSizer.Add(self.symbolButton, pos = (0, 2), flag = wx.ALIGN_RIGHT , border = 0)
        gridBagSizer.Add(self.epsRadio, pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.epsFileCtrl, pos = (1, 1), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        
        gridBagSizer.AddGrowableCol(1)
        gridBagSizer.AddGrowableCol(2)
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        self.Bind(wx.EVT_BUTTON, self.OnSymbolSelection, self.symbolButton)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnSymbology, self.symbolRadio)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnSymbology, self.epsRadio)
        
        #size
        
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Size"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        self.sizeRadio = wx.RadioButton(panel, id = wx.ID_ANY, label = _("size:"), style = wx.RB_GROUP)
        self.sizeSpin = wx.SpinCtrl(panel, id = wx.ID_ANY, min = 1, max = 50, initial = 1)
        self.sizecolumnRadio = wx.RadioButton(panel, id = wx.ID_ANY, label = _("size from map table column:"))
        self.sizeColChoice = self.getColsChoice(panel)
        self.scaleText = wx.StaticText(panel, id = wx.ID_ANY, label = _("scale:"))
        self.scaleSpin = wx.SpinCtrl(panel, id = wx.ID_ANY, min = 1, max = 25, initial = 1)
        
        self.sizeRadio.SetValue(self.vPropertiesDict['size'] is not None)
        self.sizecolumnRadio.SetValue(bool(self.vPropertiesDict['sizecolumn']))
        if self.vPropertiesDict['size']:
            self.sizeSpin.SetValue(self.vPropertiesDict['size'])
        else: self.sizeSpin.SetValue(5)
        if self.vPropertiesDict['sizecolumn']:
            self.scaleSpin.SetValue(self.vPropertiesDict['scale'])
            self.sizeColChoice.SetStringSelection(self.vPropertiesDict['sizecolumn'])
        else:
            self.scaleSpin.SetValue(1)
            self.sizeColChoice.SetSelection(0)
        if not self.connection:   
            for each in (self.sizecolumnRadio, self.sizeColChoice, self.scaleSpin, self.scaleText):
                each.Disable()
            
        gridBagSizer.Add(self.sizeRadio, pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.sizeSpin, pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.sizecolumnRadio, pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.sizeColChoice, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(self.scaleText, pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT, border = 0)
        gridBagSizer.Add(self.scaleSpin, pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        gridBagSizer.AddGrowableCol(0)
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        self.Bind(wx.EVT_RADIOBUTTON, self.OnSize, self.sizeRadio)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnSize, self.sizecolumnRadio)
        
        #rotation
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Rotation"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)

        
        self.rotateCheck = wx.CheckBox(panel, id = wx.ID_ANY, label = _("rotate symbols:"))
        self.rotateRadio = wx.RadioButton(panel, id = wx.ID_ANY, label = _("counterclockwise in degrees:"), style = wx.RB_GROUP)
        self.rotateSpin = wx.SpinCtrl(panel, id = wx.ID_ANY, min = 0, max = 360, initial = 0)
        self.rotatecolumnRadio = wx.RadioButton(panel, id = wx.ID_ANY, label = _("from map table column:"))
        self.rotateColChoice = self.getColsChoice(panel)
        
        self.rotateCheck.SetValue(self.vPropertiesDict['rotation'])
        self.rotateRadio.SetValue(self.vPropertiesDict['rotate'] is not None)
        self.rotatecolumnRadio.SetValue(bool(self.vPropertiesDict['rotatecolumn']))
        if self.vPropertiesDict['rotate']:
            self.rotateSpin.SetValue(self.vPropertiesDict['rotate'])
        else:
            self.rotateSpin.SetValue(0)
        if self.vPropertiesDict['rotatecolumn']:
            self.rotateColChoice.SetStringSelection(self.vPropertiesDict['rotatecolumn'])
        else:
            self.rotateColChoice.SetSelection(0)
            
        gridBagSizer.Add(self.rotateCheck, pos = (0, 0), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.rotateRadio, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.rotateSpin, pos = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.rotatecolumnRadio, pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.rotateColChoice, pos = (2, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        
        gridBagSizer.AddGrowableCol(1)
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        self.Bind(wx.EVT_CHECKBOX, self.OnRotation, self.rotateCheck)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnRotationType, self.rotateRadio)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnRotationType, self.rotatecolumnRadio)
        
        panel.SetSizer(border)
        panel.Fit()
        return panel
    
    def _StyleLinePanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, size = (-1, -1), style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = _("Size and style"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        
        #width
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Width"))       
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        widthText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Set width (pts):"))
        if fs:
            self.widthSpin = fs.FloatSpin(panel, id = wx.ID_ANY, min_val = 0, max_val = 30,
                                        increment = 0.5, value = 1, style = fs.FS_RIGHT)
            self.widthSpin.SetFormat("%f")
            self.widthSpin.SetDigits(1)
        else:
            self.widthSpin = wx.SpinCtrl(panel, id = wx.ID_ANY, min = 1, max = 30, initial = 1)
            
        self.cwidthCheck = wx.CheckBox(panel, id = wx.ID_ANY, label = _("multiply width by category value"))
        
        if self.vPropertiesDict['width']:
            self.widthSpin.SetValue(self.vPropertiesDict['width'])
            self.cwidthCheck.SetValue(False)
        else:
            self.widthSpin.SetValue(self.vPropertiesDict['cwidth'])
            self.cwidthCheck.SetValue(True)
        
        gridBagSizer.Add(widthText, pos = (0, 0),  flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.widthSpin, pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.cwidthCheck, pos = (1, 0), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        #style
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Line style"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        styleText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Choose line style:"))
        penStyles = ["solid", "dashed", "dotted", "dashdotted"]
        self.styleCombo = PenStyleComboBox(panel, choices = penStyles, validator = TCValidator(flag = 'ZERO_AND_ONE_ONLY'))
##        self.styleCombo = wx.ComboBox(panel, id = wx.ID_ANY,
##                            choices = ["solid", "dashed", "dotted", "dashdotted"],
##                            validator = TCValidator(flag = 'ZERO_AND_ONE_ONLY'))
##        self.styleCombo.SetToolTipString(_("It's possible to enter a series of 0's and 1's too. "\
##                                    "The first block of repeated zeros or ones represents 'draw', "\
##                                    "the second block represents 'blank'. An even number of blocks "\
##                                    "will repeat the pattern, an odd number of blocks will alternate the pattern."))
        linecapText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Choose linecap:"))
        self.linecapChoice = wx.Choice(panel, id = wx.ID_ANY, choices = ["butt", "round", "extended_butt"])
        
        self.styleCombo.SetValue(self.vPropertiesDict['style'])
        self.linecapChoice.SetStringSelection(self.vPropertiesDict['linecap'])
        
        gridBagSizer.Add(styleText, pos = (0, 0),  flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.styleCombo, pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(linecapText, pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.linecapChoice, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        panel.SetSizer(border)
        panel.Fit()
        return panel
        
    def _StyleAreaPanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, size = (-1, -1), style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = _("Size and style"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        
        #pattern
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Pattern"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        self.patternCheck = wx.CheckBox(panel, id = wx.ID_ANY, label = _("use pattern:"))
        self.patFileCtrl = filebrowse.FileBrowseButton(panel, id = wx.ID_ANY, labelText = _("Choose pattern file:"),
                                buttonText =  _("Browse"), toolTip = _("Type filename or click browse to choose file"), 
                                dialogTitle = _("Choose a file"), startDirectory = self.patternPath, initialValue = '',
                                fileMask = "Encapsulated PostScript (*.eps)|*.eps|All files (*.*)|*.*", fileMode = wx.OPEN)
        self.patWidthText = wx.StaticText(panel, id = wx.ID_ANY, label = _("pattern line width (pts):"))
        self.patWidthSpin = wx.SpinCtrl(panel, id = wx.ID_ANY, min = 1, max = 25, initial = 1)
        self.patScaleText = wx.StaticText(panel, id = wx.ID_ANY, label = _("pattern scale factor:"))
        self.patScaleSpin = wx.SpinCtrl(panel, id = wx.ID_ANY, min = 1, max = 25, initial = 1)
        
        self.patternCheck.SetValue(bool(self.vPropertiesDict['pat']))
        if self.patternCheck.GetValue():
            self.patFileCtrl.SetValue(self.vPropertiesDict['pat'])
            self.patWidthSpin.SetValue(self.vPropertiesDict['pwidth'])
            self.patScaleSpin.SetValue(self.vPropertiesDict['scale'])
        
        gridBagSizer.Add(self.patternCheck, pos = (0, 0),  flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.patFileCtrl, pos = (1, 0), span = (1, 2),flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(self.patWidthText, pos = (2, 0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.patWidthSpin, pos = (2, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.patScaleText, pos = (3, 0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.patScaleSpin, pos = (3, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        
        gridBagSizer.AddGrowableCol(1)
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        self.Bind(wx.EVT_CHECKBOX, self.OnPattern, self.patternCheck)
        
        panel.SetSizer(border)
        panel.Fit()
        return panel

    def OnLayer(self, event):
        """!Change columns on layer change """
        if self.layerChoice.GetStringSelection() == self.currLayer:
            return
        self.currLayer = self.layerChoice.GetStringSelection()
        if self.connection:
            cols = self.mapDBInfo.GetColumns(self.mapDBInfo.layers[int(self.currLayer)]['table']) 
        else:
            cols = []

        self.choiceColumns.SetItems(cols)

        self.choiceColumns.SetSelection(0)
        if self.type in ('points', 'lines'):
            self.colorColChoice.SetItems(cols)
            self.colorColChoice.SetSelection(0)
            
    def OnOutline(self, event):
        for widget in self.gridBagSizerO.GetChildren():
            if widget.GetWindow() != self.outlineCheck:
                widget.GetWindow().Enable(self.outlineCheck.GetValue())
                
    def OnFill(self, event):
        enable = self.fillCheck.GetValue()
        
        self.colorColChoice.Enable(enable)
        self.colorColRadio.Enable(enable)
        self.fillColorPicker.Enable(enable)
        self.colorPickerRadio.Enable(enable)
        if enable:
            self.OnColor(None)
        if not self.connection:
            self.colorColChoice.Disable()
            self.colorColRadio.Disable()
            
    def OnColor(self, event):
        self.colorColChoice.Enable(self.colorColRadio.GetValue())
        self.fillColorPicker.Enable(self.colorPickerRadio.GetValue())
            
    def OnSize(self, event):
        self.sizeSpin.Enable(self.sizeRadio.GetValue())
        self.sizeColChoice.Enable(self.sizecolumnRadio.GetValue())
        self.scaleText.Enable(self.sizecolumnRadio.GetValue())
        self.scaleSpin.Enable(self.sizecolumnRadio.GetValue())
        
    def OnRotation(self, event):
        for each in (self.rotateRadio, self.rotatecolumnRadio, self.rotateColChoice, self.rotateSpin):
            if self.rotateCheck.GetValue():
                each.Enable()
                self.OnRotationType(event = None)     
            else:
                each.Disable()
           
    def OnRotationType(self, event):
        self.rotateSpin.Enable(self.rotateRadio.GetValue())
        self.rotateColChoice.Enable(self.rotatecolumnRadio.GetValue())
        
    def OnPattern(self, event):
        for each in (self.patFileCtrl, self.patWidthText, self.patWidthSpin, self.patScaleText, self.patScaleSpin):
            each.Enable(self.patternCheck.GetValue())
            
    def OnSymbology(self, event):
        useSymbol = self.symbolRadio.GetValue()
        
        self.symbolButton.Enable(useSymbol)
        self.symbolName.Enable(useSymbol)
        self.epsFileCtrl.Enable(not useSymbol)
            
    def OnSymbolSelection(self, event):
        dlg = SymbolDialog(self, symbolPath = globalvar.ETCSYMBOLDIR,
                           currentSymbol = self.symbolName.GetLabel())
        if dlg.ShowModal() == wx.ID_OK:
            img = dlg.GetSelectedSymbolPath()
            name = dlg.GetSelectedSymbolName()
            self.symbolButton.SetBitmapLabel(wx.Bitmap(img + '.png'))
            self.symbolName.SetLabel(name)
            
        dlg.Destroy()
                
    def EnableLayerSelection(self, enable = True):
        for widget in self.gridBagSizerL.GetChildren():
            if widget.GetWindow() != self.warning:
                widget.GetWindow().Enable(enable)
                
    def getColsChoice(self, parent):
        """!Returns a wx.Choice with table columns"""
        if self.connection:
            cols = self.mapDBInfo.GetColumns(self.mapDBInfo.layers[int(self.currLayer)]['table']) 
        else:
            cols = []

        choice = wx.Choice(parent = parent, id = wx.ID_ANY, choices = cols)
        return choice
        
    def update(self):
        #feature type
        if self.type in ('lines', 'points'):
            featureType = None
            if self.checkType1.GetValue():
                featureType = self.checkType1.GetName()
                if self.checkType2.GetValue():
                    featureType += " or " + self.checkType2.GetName()
            elif self.checkType2.GetValue():
                featureType = self.checkType2.GetName()
            if featureType:
                self.vPropertiesDict['type'] = featureType
            
        # is connection
        self.vPropertiesDict['connection'] = self.connection
        if self.connection:
            self.vPropertiesDict['layer'] = self.layerChoice.GetStringSelection()
            if self.radioCats.GetValue() and not self.textCtrlCats.IsEmpty():
                self.vPropertiesDict['cats'] = self.textCtrlCats.GetValue()
            elif self.radioWhere.GetValue() and not self.textCtrlWhere.IsEmpty():
                self.vPropertiesDict['where'] = self.choiceColumns.GetStringSelection() + " " \
                                                                + self.textCtrlWhere.GetValue()
        #mask
        if self.mask.GetValue():
            self.vPropertiesDict['masked'] = 'y' 
        else:
            self.vPropertiesDict['masked'] = 'n'

        #colors
        if self.type in ('points', 'areas'):
            if self.outlineCheck.GetValue():
                self.vPropertiesDict['color'] = convertRGB(self.colorPicker.GetColour())
                self.vPropertiesDict['width'] = self.widthSpin.GetValue()
            else:
                self.vPropertiesDict['color'] = 'none'
                
            if self.fillCheck.GetValue():
                if self.colorPickerRadio.GetValue():
                    self.vPropertiesDict['fcolor'] = convertRGB(self.fillColorPicker.GetColour())
                    self.vPropertiesDict['rgbcolumn'] = None
                if self.colorColRadio.GetValue():
                    self.vPropertiesDict['fcolor'] = 'none'# this color is taken in case of no record in rgb column
                    self.vPropertiesDict['rgbcolumn'] = self.colorColChoice.GetStringSelection()
            else:
                self.vPropertiesDict['fcolor'] = 'none'    
                
        if self.type == 'lines':
                #hcolor only when no rgbcolumn
            if self.outlineCheck.GetValue():# and self.fillCheck.GetValue() and self.colorColRadio.GetValue():
                self.vPropertiesDict['hcolor'] = convertRGB(self.colorPicker.GetColour())
                self.vPropertiesDict['hwidth'] = self.outWidthSpin.GetValue()
                
            else:
                self.vPropertiesDict['hcolor'] = 'none'
                
            if self.colorPickerRadio.GetValue():
                self.vPropertiesDict['color'] = convertRGB(self.fillColorPicker.GetColour())
                self.vPropertiesDict['rgbcolumn'] = None
            if self.colorColRadio.GetValue():
                self.vPropertiesDict['color'] = 'none'# this color is taken in case of no record in rgb column
                self.vPropertiesDict['rgbcolumn'] = self.colorColChoice.GetStringSelection()
        #
        #size and style
        #
        
        if self.type == 'points':
            #symbols
            if self.symbolRadio.GetValue():
                self.vPropertiesDict['symbol'] = self.symbolName.GetLabel()
                self.vPropertiesDict['eps'] = None
            else:
                self.vPropertiesDict['eps'] = self.epsFileCtrl.GetValue()
            #size
            if self.sizeRadio.GetValue():
                self.vPropertiesDict['size'] = self.sizeSpin.GetValue()
                self.vPropertiesDict['sizecolumn'] = None
                self.vPropertiesDict['scale'] = None
            else:
                self.vPropertiesDict['sizecolumn'] = self.sizeColChoice.GetStringSelection()
                self.vPropertiesDict['scale'] = self.scaleSpin.GetValue()
                self.vPropertiesDict['size'] = None
            
            #rotation
            self.vPropertiesDict['rotate'] = None
            self.vPropertiesDict['rotatecolumn'] = None
            self.vPropertiesDict['rotation'] = False
            if self.rotateCheck.GetValue():
                self.vPropertiesDict['rotation'] = True
            if self.rotateRadio.GetValue():
                self.vPropertiesDict['rotate'] = self.rotateSpin.GetValue()
            else:
                self.vPropertiesDict['rotatecolumn'] = self.rotateColChoice.GetStringSelection()
                
        if self.type == 'areas':
            #pattern
            self.vPropertiesDict['pat'] = None 
            if self.patternCheck.GetValue() and bool(self.patFileCtrl.GetValue()):
                self.vPropertiesDict['pat'] = self.patFileCtrl.GetValue()
                self.vPropertiesDict['pwidth'] = self.patWidthSpin.GetValue()
                self.vPropertiesDict['scale'] = self.patScaleSpin.GetValue()
                
        if self.type == 'lines':
            #width
            if self.cwidthCheck.GetValue():
                self.vPropertiesDict['cwidth'] = self.widthSpin.GetValue()
                self.vPropertiesDict['width'] = None
            else:
                self.vPropertiesDict['width'] = self.widthSpin.GetValue()
                self.vPropertiesDict['cwidth'] = None
            #line style
            if self.styleCombo.GetValue():
                self.vPropertiesDict['style'] = self.styleCombo.GetValue() 
            else:
                self.vPropertiesDict['style'] = 'solid'

            self.vPropertiesDict['linecap'] = self.linecapChoice.GetStringSelection()
            
    def OnOK(self, event):
        self.update()
        event.Skip()
        
class LegendDialog(PsmapDialog):
    def __init__(self, parent, id, settings, page):
        PsmapDialog.__init__(self, parent = parent, id = id, title = "Legend settings", settings = settings)
        self.objectType = ('rasterLegend', 'vectorLegend')
        self.instruction = settings
        map = self.instruction.FindInstructionByType('map')
        if map:
            self.mapId = map.id 
        else:
            self.mapId = None

        vector = self.instruction.FindInstructionByType('vector')
        if vector:
            self.vectorId = vector.id 
        else:
            self.vectorId = None

        raster = self.instruction.FindInstructionByType('raster')
        if raster:
            self.rasterId = raster.id 
        else:
            self.rasterId = None

        self.pageId = self.instruction.FindInstructionByType('page').id
        currPage = self.instruction[self.pageId].GetInstruction()
        #raster legend
        if self.id[0] is not None:
            self.rasterLegend = self.instruction[self.id[0]]
            self.rLegendDict = self.rasterLegend.GetInstruction()
        else:
            self.id[0] = wx.NewId()
            self.rasterLegend = RasterLegend(self.id[0])
            self.rLegendDict = self.rasterLegend.GetInstruction()
            self.rLegendDict['where'] = currPage['Left'], currPage['Top']
            
            
        #vector legend    
        if self.id[1] is not None:
            self.vLegendDict = self.instruction[self.id[1]].GetInstruction()
        else:
            self.id[1] = wx.NewId()
            vectorLegend = VectorLegend(self.id[1])
            self.vLegendDict = vectorLegend.GetInstruction()
            self.vLegendDict['where'] = currPage['Left'], currPage['Top']
            
        if self.rasterId:
            self.currRaster = self.instruction[self.rasterId]['raster'] 
        else:
            self.currRaster = None

        #notebook
        self.notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)
        self.panelRaster = self._rasterLegend(self.notebook)
        self.panelVector = self._vectorLegend(self.notebook)  
        self.OnRaster(None)
        self.OnRange(None)
        self.OnIsLegend(None)
        self.OnSpan(None)
        self.OnBorder(None)
                
        self._layout(self.notebook)
        self.notebook.ChangeSelection(page)
        self.notebook.Bind(wx.EVT_NOTEBOOK_PAGE_CHANGING, self.OnPageChanging)
        
    def OnPageChanging(self, event):
        """!Workaround to scroll up to see the checkbox"""
        wx.CallAfter(self.FindWindowByName('rasterPanel').ScrollChildIntoView,
                                            self.FindWindowByName('showRLegend'))
        wx.CallAfter(self.FindWindowByName('vectorPanel').ScrollChildIntoView,
                                            self.FindWindowByName('showVLegend'))
                                            
    def _rasterLegend(self, notebook):
        panel = scrolled.ScrolledPanel(parent = notebook, id = wx.ID_ANY, size = (-1, 500), style = wx.TAB_TRAVERSAL)
        panel.SetupScrolling(scroll_x = False, scroll_y = True)
        panel.SetName('rasterPanel')
        notebook.AddPage(page = panel, text = _("Raster legend"))

        border = wx.BoxSizer(wx.VERTICAL)
        # is legend
        self.isRLegend = wx.CheckBox(panel, id = wx.ID_ANY, label = _("Show raster legend"))
        self.isRLegend.SetValue(self.rLegendDict['rLegend'])
        self.isRLegend.SetName("showRLegend")
        border.Add(item = self.isRLegend, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)

        # choose raster
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Source raster"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer (cols = 2, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(1)
        
        self.rasterDefault = wx.RadioButton(panel, id = wx.ID_ANY, label = _("current raster"), style = wx.RB_GROUP)
        self.rasterOther = wx.RadioButton(panel, id = wx.ID_ANY, label = _("select raster"))
        self.rasterDefault.SetValue(self.rLegendDict['rasterDefault'])#
        self.rasterOther.SetValue(not self.rLegendDict['rasterDefault'])#

        rasterType = getRasterType(map = self.currRaster)

        self.rasterCurrent = wx.StaticText(panel, id = wx.ID_ANY,
                                label = _("%(rast)s: type %(type)s") % { 'rast' : self.currRaster,
                                                                         'type' : rasterType })
        self.rasterSelect = Select(panel, id = wx.ID_ANY, size = globalvar.DIALOG_GSELECT_SIZE,
                                    type = 'raster', multiple = False,
                                    updateOnPopup = True, onPopup = None)
        if not self.rLegendDict['rasterDefault']:
            self.rasterSelect.SetValue(self.rLegendDict['raster'])
        else:
            self.rasterSelect.SetValue('')
        flexSizer.Add(self.rasterDefault, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexSizer.Add(self.rasterCurrent, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.LEFT, border = 10)
        flexSizer.Add(self.rasterOther, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexSizer.Add(self.rasterSelect, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT, border = 0)
        
        sizer.Add(item = flexSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        # type of legend
        
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Type of legend"))        
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        vbox = wx.BoxSizer(wx.VERTICAL)
        self.discrete = wx.RadioButton(parent = panel, id = wx.ID_ANY, 
                        label = " %s " % _("discrete legend (categorical maps)"), style = wx.RB_GROUP)
        self.continuous = wx.RadioButton(parent = panel, id = wx.ID_ANY, 
                        label = " %s " % _("continuous color gradient legend (floating point map)"))
        
        vbox.Add(self.discrete, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 0)
        vbox.Add(self.continuous, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 0)
        sizer.Add(item = vbox, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        # size, position and font
        self.sizePositionFont(legendType = 'raster', parent = panel, mainSizer = border)
        
        # advanced settings
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Advanced legend settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
        # no data
        self.nodata = wx.CheckBox(panel, id = wx.ID_ANY, label = _('draw "no data" box'))
        if self.rLegendDict['nodata'] == 'y':
            self.nodata.SetValue(True)
        else: 
            self.nodata.SetValue(False)
        #tickbar
        self.ticks = wx.CheckBox(panel, id = wx.ID_ANY, label = _("draw ticks across color table"))
        if self.rLegendDict['tickbar'] == 'y':
            self.ticks.SetValue(True)
        else:
            self.ticks.SetValue(False)
        # range
        if self.rasterId and self.instruction[self.rasterId]['raster']:
            rinfo = grass.raster_info(self.instruction[self.rasterId]['raster'])
            self.minim, self.maxim = rinfo['min'], rinfo['max']
        else:
            self.minim, self.maxim = 0,0
        self.range = wx.CheckBox(panel, id = wx.ID_ANY, label = _("range"))
        self.range.SetValue(self.rLegendDict['range'])
        self.minText =  wx.StaticText(panel, id = wx.ID_ANY, label = "min (%s)" % self.minim)
        self.maxText =  wx.StaticText(panel, id = wx.ID_ANY, label = "max (%s)" % self.maxim)       
        self.min = wx.TextCtrl(panel, id = wx.ID_ANY, value = str(self.rLegendDict['min']))
        self.max = wx.TextCtrl(panel, id = wx.ID_ANY, value = str(self.rLegendDict['max']))
        
        gridBagSizer.Add(self.nodata, pos = (0,0), span = (1,5), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.ticks, pos = (1,0), span = (1,5), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.range, pos = (2,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.minText, pos = (2,1), flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT, border = 0)
        gridBagSizer.Add(self.min, pos = (2,2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.maxText, pos = (2,3), flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_RIGHT, border = 0)
        gridBagSizer.Add(self.max, pos = (2,4), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        sizer.Add(gridBagSizer, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
   
        panel.SetSizer(border)
        panel.Fit()
        
        # bindings
        self.Bind(wx.EVT_RADIOBUTTON, self.OnRaster, self.rasterDefault)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnRaster, self.rasterOther)
        self.Bind(wx.EVT_CHECKBOX, self.OnIsLegend, self.isRLegend)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnDiscrete, self.discrete)
        self.Bind(wx.EVT_RADIOBUTTON, self.OnDiscrete, self.continuous)
##        self.Bind(wx.EVT_CHECKBOX, self.OnDefaultSize, panel.defaultSize)
        self.Bind(wx.EVT_CHECKBOX, self.OnRange, self.range)
        self.rasterSelect.GetTextCtrl().Bind(wx.EVT_TEXT, self.OnRaster)
        
        return panel
    
    def _vectorLegend(self, notebook):
        panel = scrolled.ScrolledPanel(parent = notebook, id = wx.ID_ANY, size = (-1, 500), style = wx.TAB_TRAVERSAL)
        panel.SetupScrolling(scroll_x = False, scroll_y = True)
        panel.SetName('vectorPanel')
        notebook.AddPage(page = panel, text = _("Vector legend"))

        border = wx.BoxSizer(wx.VERTICAL)
        # is legend
        self.isVLegend = wx.CheckBox(panel, id = wx.ID_ANY, label = _("Show vector legend"))
        self.isVLegend.SetValue(self.vLegendDict['vLegend'])
        self.isVLegend.SetName("showVLegend")
        border.Add(item = self.isVLegend, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        #vector maps, their order, labels
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Source vector maps"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        vectorText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Choose vector maps and their order in legend"))

        self.vectorListCtrl = CheckListCtrl(panel)
        
        self.vectorListCtrl.InsertColumn(0, _("Vector map"))
        self.vectorListCtrl.InsertColumn(1, _("Label"))
        if self.vectorId:
            vectors = sorted(self.instruction[self.vectorId]['list'], key = lambda x: x[3])
            
            for vector in vectors:
                index = self.vectorListCtrl.InsertStringItem(sys.maxint, vector[0].split('@')[0])
                self.vectorListCtrl.SetStringItem(index, 1, vector[4])
                self.vectorListCtrl.SetItemData(index, index)
                self.vectorListCtrl.CheckItem(index, True)
                if vector[3] == 0:
                    self.vectorListCtrl.CheckItem(index, False)
        if not self.vectorId:
            self.vectorListCtrl.SetColumnWidth(0, 100)
        else:
            self.vectorListCtrl.SetColumnWidth(0, wx.LIST_AUTOSIZE)
        self.vectorListCtrl.SetColumnWidth(1, wx.LIST_AUTOSIZE)
        
        self.btnUp = wx.Button(panel, id = wx.ID_ANY, label = _("Up"))
        self.btnDown = wx.Button(panel, id = wx.ID_ANY, label = _("Down"))
        self.btnLabel = wx.Button(panel, id = wx.ID_ANY, label = _("Edit label"))
      
        gridBagSizer.Add(vectorText, pos = (0,0), span = (1,2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.vectorListCtrl, pos = (1,0), span = (3,1), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(self.btnUp, pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.btnDown, pos = (2,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.btnLabel, pos = (3,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        gridBagSizer.AddGrowableCol(0,3)
        gridBagSizer.AddGrowableCol(1,1)
        sizer.Add(gridBagSizer, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        # size, position and font
        self.sizePositionFont(legendType = 'vector', parent = panel, mainSizer = border)
         
        # border
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Border"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexGridSizer = wx.FlexGridSizer(cols = 2, hgap = 5, vgap = 5)
        
        self.borderCheck = wx.CheckBox(panel, id = wx.ID_ANY, label = _("draw border around legend"))
        self.borderColorCtrl = wx.ColourPickerCtrl(panel, id = wx.ID_ANY, style = wx.FNTP_FONTDESC_AS_LABEL)
        if self.vLegendDict['border'] == 'none':
            self.borderColorCtrl.SetColour(wx.BLACK)
            self.borderCheck.SetValue(False)
        else:
            self.borderColorCtrl.SetColour(convertRGB(self.vLegendDict['border']))
            self.borderCheck.SetValue(True)
            
        flexGridSizer.Add(self.borderCheck, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)    
        flexGridSizer.Add(self.borderColorCtrl, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        sizer.Add(item = flexGridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        self.Bind(wx.EVT_BUTTON, self.OnUp, self.btnUp)
        self.Bind(wx.EVT_BUTTON, self.OnDown, self.btnDown)  
        self.Bind(wx.EVT_BUTTON, self.OnEditLabel, self.btnLabel)
        self.Bind(wx.EVT_CHECKBOX, self.OnIsLegend, self.isVLegend)    
        self.Bind(wx.EVT_CHECKBOX, self.OnSpan, panel.spanRadio)  
        self.Bind(wx.EVT_CHECKBOX, self.OnBorder, self.borderCheck)
        self.Bind(wx.EVT_FONTPICKER_CHANGED, self.OnFont, panel.font['fontCtrl']) 
        
        panel.SetSizer(border)
        
        panel.Fit()
        return panel
    
    def sizePositionFont(self, legendType, parent, mainSizer):
        """!Insert widgets for size, position and font control"""
        if legendType == 'raster':
            legendDict = self.rLegendDict  
        else:
            legendDict = self.vLegendDict
        panel = parent
        border = mainSizer
        
        # size and position        
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Size and position"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        #unit
        self.AddUnits(parent = panel, dialogDict = legendDict)
        unitBox = wx.BoxSizer(wx.HORIZONTAL)
        unitBox.Add(panel.units['unitsLabel'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.LEFT, border = 10)
        unitBox.Add(panel.units['unitsCtrl'], proportion = 1, flag = wx.ALL, border = 5)
        sizer.Add(unitBox, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        hBox = wx.BoxSizer(wx.HORIZONTAL)
        posBox = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " %_("Position"))
        posSizer = wx.StaticBoxSizer(posBox, wx.VERTICAL)       
        sizeBox = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Size"))
        sizeSizer = wx.StaticBoxSizer(sizeBox, wx.VERTICAL) 
        posGridBagSizer = wx.GridBagSizer(hgap = 10, vgap = 5)
        
        #position
        self.AddPosition(parent = panel, dialogDict = legendDict)
        
        posGridBagSizer.Add(panel.position['xLabel'], pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        posGridBagSizer.Add(panel.position['xCtrl'], pos = (0,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        posGridBagSizer.Add(panel.position['yLabel'], pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        posGridBagSizer.Add(panel.position['yCtrl'], pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        posGridBagSizer.Add(panel.position['comment'], pos = (2,0), span = (1,2), flag =wx.ALIGN_BOTTOM, border = 0)
        posGridBagSizer.AddGrowableRow(2)
        posSizer.Add(posGridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        
        #size
        width = wx.StaticText(panel, id = wx.ID_ANY, label = _("Width:"))
        if legendDict['width']:
            w = self.unitConv.convert(value = float(legendDict['width']), fromUnit = 'inch', toUnit = legendDict['unit'])
        else: 
            w = ''
        panel.widthCtrl = wx.TextCtrl(panel, id = wx.ID_ANY, value = str(w), validator = TCValidator("DIGIT_ONLY"))
        panel.widthCtrl.SetToolTipString(_("Leave the edit field empty, to use default values."))
        
        if legendType == 'raster':
##            panel.defaultSize = wx.CheckBox(panel, id = wx.ID_ANY, label = _("Use default size"))
##            panel.defaultSize.SetValue(legendDict['defaultSize'])
            
            panel.heightOrColumnsLabel = wx.StaticText(panel, id = wx.ID_ANY, label = _("Height:"))
            if legendDict['height']:
                h = self.unitConv.convert(value = float(legendDict['height']), fromUnit = 'inch', toUnit = legendDict['unit'])
            else:
                h = ''
            panel.heightOrColumnsCtrl = wx.TextCtrl(panel, id = wx.ID_ANY, value = str(h), validator = TCValidator("DIGIT_ONLY"))
            
            self.rSizeGBSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
##            self.rSizeGBSizer.Add(panel.defaultSize, pos = (0,0), span = (1,2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            self.rSizeGBSizer.Add(width, pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            self.rSizeGBSizer.Add(panel.widthCtrl, pos = (0,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            self.rSizeGBSizer.Add(panel.heightOrColumnsLabel, pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            self.rSizeGBSizer.Add(panel.heightOrColumnsCtrl, pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            sizeSizer.Add(self.rSizeGBSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
            
        if legendType == 'vector':
            panel.widthCtrl.SetToolTipString(_("Width of the color symbol (for lines)\nin front of the legend text")) 
            #columns
            minVect, maxVect = 0, 0
            if self.vectorId:
                minVect = 1
                maxVect = min(10, len(self.instruction[self.vectorId]['list']))
            cols = wx.StaticText(panel, id = wx.ID_ANY, label = _("Columns:"))
            panel.colsCtrl = wx.SpinCtrl(panel, id = wx.ID_ANY, value = "",
                                        min = minVect, max = maxVect, initial = legendDict['cols'])
            #span
            panel.spanRadio = wx.CheckBox(panel, id = wx.ID_ANY, label = _("column span:"))
            panel.spanTextCtrl = wx.TextCtrl(panel, id = wx.ID_ANY, value = '')
            panel.spanTextCtrl.SetToolTipString(_("Column separation distance between the left edges\n"\
                                                "of two columns in a multicolumn legend"))
            if legendDict['span']:
                panel.spanRadio.SetValue(True)
                s = self.unitConv.convert(value = float(legendDict['span']), fromUnit = 'inch', toUnit = legendDict['unit'])    
                panel.spanTextCtrl.SetValue(str(s))
            else:
                panel.spanRadio.SetValue(False)
                
            self.vSizeGBSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
            self.vSizeGBSizer.Add(width, pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            self.vSizeGBSizer.Add(panel.widthCtrl, pos = (0,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            self.vSizeGBSizer.Add(cols, pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            self.vSizeGBSizer.Add(panel.colsCtrl, pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            self.vSizeGBSizer.Add(panel.spanRadio, pos = (2,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            self.vSizeGBSizer.Add(panel.spanTextCtrl, pos = (2,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
            self.vSizeGBSizer.AddGrowableCol(1)
            sizeSizer.Add(self.vSizeGBSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)        
        
        hBox.Add(posSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 3)
        hBox.Add(sizeSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 3)
        sizer.Add(hBox, proportion = 0, flag = wx.EXPAND, border = 0)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
              
        # font        
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Font settings"))
        fontSizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer (cols = 2, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(1)
        
        if legendType == 'raster':
            self.AddFont(parent = panel, dialogDict = legendDict, color = True)
        else:
            self.AddFont(parent = panel, dialogDict = legendDict, color = False)            
        flexSizer.Add(panel.font['fontLabel'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexSizer.Add(panel.font['fontCtrl'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexSizer.Add(panel.font['fontSizeLabel'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexSizer.Add(panel.font['fontSizeCtrl'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        if legendType == 'raster':
            flexSizer.Add(panel.font['colorLabel'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
            flexSizer.Add(panel.font['colorCtrl'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        fontSizer.Add(item = flexSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = fontSizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)    

    #   some enable/disable methods  
        
    def OnIsLegend(self, event):
        """!Enables and disables controls, it depends if raster or vector legend is checked"""
        page = self.notebook.GetSelection()
        if page == 0 or event is None:
            children = self.panelRaster.GetChildren()
            if self.isRLegend.GetValue():
                for i,widget in enumerate(children):
                        widget.Enable()
                self.OnRaster(None)
                self.OnRange(None)
                self.OnDiscrete(None)
            else:
                for widget in children:
                    if widget.GetName() != 'showRLegend':
                        widget.Disable()
        if page == 1 or event is None:
            children = self.panelVector.GetChildren()
            if self.isVLegend.GetValue():
                for i, widget in enumerate(children):
                        widget.Enable()
                self.OnSpan(None)
                self.OnBorder(None)
            else:
                for widget in children:
                    if widget.GetName() != 'showVLegend':
                        widget.Disable()
                    
    def OnRaster(self, event):
        if self.rasterDefault.GetValue():#default
            self.rasterSelect.Disable()
            type = getRasterType(self.currRaster)
        else:#select raster
            self.rasterSelect.Enable()
            map = self.rasterSelect.GetValue()
            type = getRasterType(map)
  
        if type == 'CELL':
            self.discrete.SetValue(True)
        elif type in ('FCELL', 'DCELL'):
            self.continuous.SetValue(True)
        if event is None:
            if self.rLegendDict['discrete'] == 'y':
                self.discrete.SetValue(True)
            elif self.rLegendDict['discrete'] == 'n':
                self.continuous.SetValue(True)
        self.OnDiscrete(None)
        
    def OnDiscrete(self, event):
        """! Change control according to the type of legend"""
        enabledSize = self.panelRaster.heightOrColumnsCtrl.IsEnabled()
        self.panelRaster.heightOrColumnsCtrl.Destroy()
        if self.discrete.GetValue():
            self.panelRaster.heightOrColumnsLabel.SetLabel(_("Columns:"))
            self.panelRaster.heightOrColumnsCtrl = wx.SpinCtrl(self.panelRaster, id = wx.ID_ANY, value = "", min = 1, max = 10, initial = self.rLegendDict['cols'])
            self.panelRaster.heightOrColumnsCtrl.Enable(enabledSize)
            self.nodata.Enable()
            self.range.Disable()
            self.min.Disable()
            self.max.Disable()
            self.minText.Disable()
            self.maxText.Disable()
            self.ticks.Disable()
        else:
            self.panelRaster.heightOrColumnsLabel.SetLabel(_("Height:"))
            if self.rLegendDict['height']:
                h = self.unitConv.convert(value = float(self.rLegendDict['height']), fromUnit = 'inch', toUnit = self.rLegendDict['unit'])
            else:
                h = ''
            self.panelRaster.heightOrColumnsCtrl = wx.TextCtrl(self.panelRaster, id = wx.ID_ANY,
                                                    value = str(h), validator = TCValidator("DIGIT_ONLY"))
            self.panelRaster.heightOrColumnsCtrl.Enable(enabledSize)
            self.nodata.Disable()
            self.range.Enable()
            if self.range.GetValue():
                self.minText.Enable()
                self.maxText.Enable()
                self.min.Enable()
                self.max.Enable()
            self.ticks.Enable()
        
        self.rSizeGBSizer.Add(self.panelRaster.heightOrColumnsCtrl, pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        self.panelRaster.Layout()
        self.panelRaster.Fit()   
        
    def OnRange(self, event):
        if not self.range.GetValue():
            self.min.Disable()        
            self.max.Disable()
            self.minText.Disable()
            self.maxText.Disable()
        else:
            self.min.Enable()        
            self.max.Enable() 
            self.minText.Enable()
            self.maxText.Enable()           
     
    def OnUp(self, event):
        """!Moves selected map up, changes order in vector legend"""
        if self.vectorListCtrl.GetFirstSelected() != -1:
            pos = self.vectorListCtrl.GetFirstSelected()
            if pos:
                idx1 = self.vectorListCtrl.GetItemData(pos) - 1
                idx2 = self.vectorListCtrl.GetItemData(pos - 1) + 1
                self.vectorListCtrl.SetItemData(pos, idx1) 
                self.vectorListCtrl.SetItemData(pos - 1, idx2) 
                self.vectorListCtrl.SortItems(cmp)
                if pos > 0:
                    selected = (pos - 1) 
                else:
                    selected = 0

                self.vectorListCtrl.Select(selected)
       
    def OnDown(self, event):
        """!Moves selected map down, changes order in vector legend"""
        if self.vectorListCtrl.GetFirstSelected() != -1:
            pos = self.vectorListCtrl.GetFirstSelected()
            if pos != self.vectorListCtrl.GetItemCount() - 1:
                idx1 = self.vectorListCtrl.GetItemData(pos) + 1
                idx2 = self.vectorListCtrl.GetItemData(pos + 1) - 1
                self.vectorListCtrl.SetItemData(pos, idx1) 
                self.vectorListCtrl.SetItemData(pos + 1, idx2) 
                self.vectorListCtrl.SortItems(cmp)
                if pos < self.vectorListCtrl.GetItemCount() -1:
                    selected = (pos + 1) 
                else:
                    selected = self.vectorListCtrl.GetItemCount() -1

                self.vectorListCtrl.Select(selected)
                
    def OnEditLabel(self, event):
        """!Change legend label of vector map"""
        if self.vectorListCtrl.GetFirstSelected() != -1:
            idx = self.vectorListCtrl.GetFirstSelected()
            default = self.vectorListCtrl.GetItem(idx, 1).GetText()
            dlg = wx.TextEntryDialog(self, message = _("Edit legend label:"), caption = _("Edit label"),
                                    defaultValue = default, style = wx.OK|wx.CANCEL|wx.CENTRE)
            if dlg.ShowModal() == wx.ID_OK:
                new = dlg.GetValue()
                self.vectorListCtrl.SetStringItem(idx, 1, new)
            dlg.Destroy()
        
    def OnSpan(self, event):
        self.panelVector.spanTextCtrl.Enable(self.panelVector.spanRadio.GetValue())
    def OnFont(self, event):
        """!Changes default width according to fontsize, width [inch] = fontsize[pt]/24"""   
##        fontsize = self.panelVector.font['fontCtrl'].GetSelectedFont().GetPointSize() 
        fontsize = self.panelVector.font['fontSizeCtrl'].GetValue()
        unit = self.unitConv.findUnit(self.panelVector.units['unitsCtrl'].GetStringSelection())
        w = fontsize/24.
        width = self.unitConv.convert(value = w, fromUnit = 'inch', toUnit = unit)
        self.panelVector.widthCtrl.SetValue("%3.2f" % width)
        
    def OnBorder(self, event):
        """!Enables/disables colorPickerCtrl for border"""    
        self.borderColorCtrl.Enable(self.borderCheck.GetValue())
    
    def updateRasterLegend(self):
        """!Save information from raster legend dialog to dictionary"""

        #is raster legend
        if not self.isRLegend.GetValue():
            self.rLegendDict['rLegend'] = False
        else:
            self.rLegendDict['rLegend'] = True
        #units
        currUnit = self.unitConv.findUnit(self.panelRaster.units['unitsCtrl'].GetStringSelection())
        self.rLegendDict['unit'] = currUnit
        # raster
        if self.rasterDefault.GetValue():
            self.rLegendDict['rasterDefault'] = True
            self.rLegendDict['raster'] = self.currRaster
        else:
            self.rLegendDict['rasterDefault'] = False
            self.rLegendDict['raster'] = self.rasterSelect.GetValue()
        if self.rLegendDict['rLegend'] and not self.rLegendDict['raster']:
            wx.MessageBox(message = _("No raster map selected!"),
                                    caption = _('No raster'), style = wx.OK|wx.ICON_ERROR)
            return False
            
        if self.rLegendDict['raster']:
            # type and range of map
            rasterType = getRasterType(self.rLegendDict['raster'])
            if rasterType is None:
                return False
            self.rLegendDict['type'] = rasterType
            
            
            #discrete
            if self.discrete.GetValue():
                self.rLegendDict['discrete'] = 'y'
            else:
                self.rLegendDict['discrete'] = 'n'   
                    
            # font 
            self.rLegendDict['font'] = self.panelRaster.font['fontCtrl'].GetStringSelection()
            self.rLegendDict['fontsize'] = self.panelRaster.font['fontSizeCtrl'].GetValue()
            color = self.panelRaster.font['colorCtrl'].GetColour()
            self.rLegendDict['color'] = convertRGB(color)

            # position
            x = self.unitConv.convert(value = float(self.panelRaster.position['xCtrl'].GetValue()), fromUnit = currUnit, toUnit = 'inch')
            y = self.unitConv.convert(value = float(self.panelRaster.position['yCtrl'].GetValue()), fromUnit = currUnit, toUnit = 'inch')
            self.rLegendDict['where'] = (x, y)
            # estimated size
            width = self.panelRaster.widthCtrl.GetValue()
            try:
                width = float(width)
                width = self.unitConv.convert(value = width, fromUnit = currUnit, toUnit = 'inch')
            except ValueError:
                width = None
            self.rLegendDict['width'] = width
            if self.rLegendDict['discrete'] == 'n':
                height = self.panelRaster.heightOrColumnsCtrl.GetValue()    
                try:
                    height = float(height)
                    height = self.unitConv.convert(value = height, fromUnit = currUnit, toUnit = 'inch')
                except ValueError:
                    height = None
                self.rLegendDict['height'] = height
            else:
                cols = self.panelRaster.heightOrColumnsCtrl.GetValue()
                self.rLegendDict['cols'] = cols
            drawHeight = self.rasterLegend.EstimateHeight(raster = self.rLegendDict['raster'], discrete = self.rLegendDict['discrete'],
                                            fontsize = self.rLegendDict['fontsize'], cols = self.rLegendDict['cols'],
                                            height = self.rLegendDict['height'])
            drawWidth = self.rasterLegend.EstimateWidth(raster = self.rLegendDict['raster'], discrete = self.rLegendDict['discrete'],
                                            fontsize = self.rLegendDict['fontsize'], cols = self.rLegendDict['cols'],
                                            width = self.rLegendDict['width'], paperInstr = self.instruction[self.pageId])
            self.rLegendDict['rect'] = Rect2D(x = x, y = y, width = drawWidth, height = drawHeight)
                        
            # no data
            if self.rLegendDict['discrete'] == 'y':
                if self.nodata.GetValue():
                    self.rLegendDict['nodata'] = 'y'
                else:
                    self.rLegendDict['nodata'] = 'n'
            # tickbar
            elif self.rLegendDict['discrete'] == 'n':
                if self.ticks.GetValue():
                    self.rLegendDict['tickbar'] = 'y'
                else:
                    self.rLegendDict['tickbar'] = 'n'
            # range
                if self.range.GetValue():
                    self.rLegendDict['range'] = True
                    self.rLegendDict['min'] = self.min.GetValue()
                    self.rLegendDict['max'] = self.max.GetValue()
                else:
                    self.rLegendDict['range'] = False
         
        if not self.id[0] in self.instruction:
            rasterLegend = RasterLegend(self.id[0])
            self.instruction.AddInstruction(rasterLegend)
        self.instruction[self.id[0]].SetInstruction(self.rLegendDict)

        if self.id[0] not in self.parent.objectId:
            self.parent.objectId.append(self.id[0])
        return True
    
    def updateVectorLegend(self):
        """!Save information from vector legend dialog to dictionary"""

        vector = self.instruction.FindInstructionByType('vector')
        if vector:
            self.vectorId = vector.id 
        else:
            self.vectorId = None

        #is vector legend
        if not self.isVLegend.GetValue():
            self.vLegendDict['vLegend'] = False
        else:
            self.vLegendDict['vLegend'] = True   
        if self.vLegendDict['vLegend'] == True and self.vectorId is not None:
            # labels
            #reindex order
            idx = 1
            for item in range(self.vectorListCtrl.GetItemCount()):
                if self.vectorListCtrl.IsChecked(item):
                    self.vectorListCtrl.SetItemData(item, idx)
                    idx += 1
                else:
                    self.vectorListCtrl.SetItemData(item, 0)
            if idx == 1: 
                self.vLegendDict['vLegend'] = False     
            else:
                vList = self.instruction[self.vectorId]['list']
                for i, vector in enumerate(vList):
                    item = self.vectorListCtrl.FindItem(start = -1, str = vector[0].split('@')[0])
                    vList[i][3] = self.vectorListCtrl.GetItemData(item)
                    vList[i][4] = self.vectorListCtrl.GetItem(item, 1).GetText()
                vmaps = self.instruction.FindInstructionByType('vProperties', list = True)
                for vmap, vector in zip(vmaps, vList):
                    self.instruction[vmap.id]['lpos'] = vector[3]
                    self.instruction[vmap.id]['label'] = vector[4]
                #units
                currUnit = self.unitConv.findUnit(self.panelVector.units['unitsCtrl'].GetStringSelection())
                self.vLegendDict['unit'] = currUnit
                # position
                x = self.unitConv.convert(value = float(self.panelVector.position['xCtrl'].GetValue()),
                                                                fromUnit = currUnit, toUnit = 'inch')
                y = self.unitConv.convert(value = float(self.panelVector.position['yCtrl'].GetValue()),
                                                                fromUnit = currUnit, toUnit = 'inch')
                self.vLegendDict['where'] = (x, y)
                
                # font 
                self.vLegendDict['font'] = self.panelVector.font['fontCtrl'].GetStringSelection()
                self.vLegendDict['fontsize'] = self.panelVector.font['fontSizeCtrl'].GetValue()
                dc = wx.ClientDC(self)
                font = dc.GetFont()
                dc.SetFont(wx.Font(pointSize = self.vLegendDict['fontsize'], family = font.GetFamily(),
                                   style = font.GetStyle(), weight = wx.FONTWEIGHT_NORMAL))
                #size
                width = self.unitConv.convert(value = float(self.panelVector.widthCtrl.GetValue()),
                                              fromUnit = currUnit, toUnit = 'inch')
                self.vLegendDict['width'] = width
                self.vLegendDict['cols'] = self.panelVector.colsCtrl.GetValue()
                if self.panelVector.spanRadio.GetValue() and self.panelVector.spanTextCtrl.GetValue():
                    self.vLegendDict['span'] = self.panelVector.spanTextCtrl.GetValue()
                else:
                    self.vLegendDict['span'] = None
                    
                # size estimation
                vectors = self.instruction[self.vectorId]['list']
                labels = [vector[4] for vector in vectors if vector[3] != 0]
                extent = dc.GetTextExtent(max(labels, key = len))
                wExtent = self.unitConv.convert(value = extent[0], fromUnit = 'pixel', toUnit = 'inch')
                hExtent = self.unitConv.convert(value = extent[1], fromUnit = 'pixel', toUnit = 'inch')
                w = (width + wExtent) * self.vLegendDict['cols']
                h = len(labels) * hExtent / self.vLegendDict['cols']
                h *= 1.1
                self.vLegendDict['rect'] = Rect2D(x, y, w, h)
                
                #border
                if self.borderCheck.GetValue():
                    color = self.borderColorCtrl.GetColour()
                    self.vLegendDict['border'] = convertRGB(color)
                    
                else:
                    self.vLegendDict['border'] = 'none'
                                 
        if not self.id[1] in self.instruction:
            vectorLegend = VectorLegend(self.id[1])
            self.instruction.AddInstruction(vectorLegend)
        self.instruction[self.id[1]].SetInstruction(self.vLegendDict)
        if self.id[1] not in self.parent.objectId:
            self.parent.objectId.append(self.id[1])
        return True
    
    def update(self):
        okR = self.updateRasterLegend()
        okV = self.updateVectorLegend()
        if okR and okV:
            return True
        return False
        
    def updateDialog(self):
        """!Update legend coordinates after moving"""
        
        # raster legend    
        if 'rect' in self.rLegendDict:
            x, y = self.rLegendDict['rect'][:2]
            currUnit = self.unitConv.findUnit(self.panelRaster.units['unitsCtrl'].GetStringSelection())
            x = self.unitConv.convert(value = x, fromUnit = 'inch', toUnit = currUnit)
            y = self.unitConv.convert(value = y, fromUnit = 'inch', toUnit = currUnit)
            self.panelRaster.position['xCtrl'].SetValue("%5.3f" % x)
            self.panelRaster.position['yCtrl'].SetValue("%5.3f" % y)
        #update name and type of raster
        raster = self.instruction.FindInstructionByType('raster')
        if raster:
            self.rasterId = raster.id 
        else:
            self.rasterId = None 

        if raster:
            currRaster = raster['raster'] 
        else:
            currRaster = None
            
        rasterType = getRasterType(map = currRaster)
        self.rasterCurrent.SetLabel(_("%(rast)s: type %(type)s") % \
                                        { 'rast' : currRaster, 'type' : str(rasterType) })
        
        # vector legend       
        if 'rect' in self.vLegendDict:
            x, y = self.vLegendDict['rect'][:2]
            currUnit = self.unitConv.findUnit(self.panelVector.units['unitsCtrl'].GetStringSelection())
            x = self.unitConv.convert(value = x, fromUnit = 'inch', toUnit = currUnit)
            y = self.unitConv.convert(value = y, fromUnit = 'inch', toUnit = currUnit)
            self.panelVector.position['xCtrl'].SetValue("%5.3f" % x)
            self.panelVector.position['yCtrl'].SetValue("%5.3f" % y)
        # update vector maps
        if self.instruction.FindInstructionByType('vector'):
            vectors = sorted(self.instruction.FindInstructionByType('vector')['list'], key = lambda x: x[3])
            self.vectorListCtrl.DeleteAllItems()
            for vector in vectors:
                index = self.vectorListCtrl.InsertStringItem(sys.maxint, vector[0].split('@')[0])
                self.vectorListCtrl.SetStringItem(index, 1, vector[4])
                self.vectorListCtrl.SetItemData(index, index)
                self.vectorListCtrl.CheckItem(index, True)
                if vector[3] == 0:
                    self.vectorListCtrl.CheckItem(index, False)
            self.panelVector.colsCtrl.SetRange(1, min(10, len(self.instruction.FindInstructionByType('vector')['list'])))
            self.panelVector.colsCtrl.SetValue(1)
        else:
            self.vectorListCtrl.DeleteAllItems()
            self.panelVector.colsCtrl.SetRange(0,0)
            self.panelVector.colsCtrl.SetValue(0)
             
class MapinfoDialog(PsmapDialog):
    def __init__(self, parent, id, settings):
        PsmapDialog.__init__(self, parent = parent, id = id, title = _("Mapinfo settings"), settings = settings)
        
        self.objectType = ('mapinfo',)
        if self.id is not None:
            self.mapinfo = self.instruction[self.id]
            self.mapinfoDict = self.mapinfo.GetInstruction()
        else:
            self.id = wx.NewId()
            self.mapinfo = Mapinfo(self.id)
            self.mapinfoDict = self.mapinfo.GetInstruction()
            page = self.instruction.FindInstructionByType('page').GetInstruction()
            self.mapinfoDict['where'] = page['Left'], page['Top']

        self.panel = self._mapinfoPanel()
        
        self._layout(self.panel)
        self.OnIsBackground(None)
        self.OnIsBorder(None)

    def _mapinfoPanel(self):
        panel = wx.Panel(parent = self, id = wx.ID_ANY, size = (-1, -1), style = wx.TAB_TRAVERSAL)
        #panel.SetupScrolling(scroll_x = False, scroll_y = True)
        border = wx.BoxSizer(wx.VERTICAL)
                
        # position     
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Position"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        self.AddPosition(parent = panel, dialogDict = self.mapinfoDict)
        self.AddUnits(parent = panel, dialogDict = self.mapinfoDict)
        gridBagSizer.Add(panel.units['unitsLabel'], pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.units['unitsCtrl'], pos = (0,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.position['xLabel'], pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.position['xCtrl'], pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.position['yLabel'], pos = (2,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.position['yCtrl'], pos = (2,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.position['comment'], pos = (3,0), span = (1,2), flag =wx.ALIGN_BOTTOM, border = 0)
        
        gridBagSizer.AddGrowableCol(1)
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        # font
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Font settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        self.AddFont(parent = panel, dialogDict = self.mapinfoDict)#creates font color too, used below
        
        gridBagSizer.Add(panel.font['fontLabel'], pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.font['fontCtrl'], pos = (0,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.font['fontSizeLabel'], pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.font['fontSizeCtrl'], pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.font['colorLabel'], pos = (2,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        gridBagSizer.Add(panel.font['colorCtrl'], pos = (2,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        gridBagSizer.AddGrowableCol(1)
        sizer.Add(item = gridBagSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        # colors
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " %_("Color settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer (cols = 2, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(1)
        
        self.colors = {}
        self.colors['borderCtrl'] = wx.CheckBox(panel, id = wx.ID_ANY, label = _("use border color:"))
        self.colors['backgroundCtrl'] = wx.CheckBox(panel, id = wx.ID_ANY, label = _("use background color:"))
        self.colors['borderColor'] = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        self.colors['backgroundColor'] = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        
        if self.mapinfoDict['border'] == None:
            self.mapinfoDict['border'] = 'none'
        if self.mapinfoDict['border'] != 'none':
            self.colors['borderCtrl'].SetValue(True) 
            self.colors['borderColor'].SetColour(convertRGB(self.mapinfoDict['border']))
        else:
            self.colors['borderCtrl'].SetValue(False)
            self.colors['borderColor'].SetColour(convertRGB('black'))
            
        if self.mapinfoDict['background'] == None:
            self.mapinfoDict['background'] == 'none'
        if self.mapinfoDict['background'] != 'none':
            self.colors['backgroundCtrl'].SetValue(True) 
            self.colors['backgroundColor'].SetColour(convertRGB(self.mapinfoDict['background']))
        else:
            self.colors['backgroundCtrl'].SetValue(False)
            self.colors['backgroundColor'].SetColour(convertRGB('white'))
                    
        flexSizer.Add(self.colors['borderCtrl'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexSizer.Add(self.colors['borderColor'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexSizer.Add(self.colors['backgroundCtrl'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexSizer.Add(self.colors['backgroundColor'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        sizer.Add(item = flexSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        panel.SetSizer(border)
        
        self.Bind(wx.EVT_CHECKBOX, self.OnIsBorder, self.colors['borderCtrl'])
        self.Bind(wx.EVT_CHECKBOX, self.OnIsBackground, self.colors['backgroundCtrl'])
        
        return panel
    
    def OnIsBackground(self, event):
        if self.colors['backgroundCtrl'].GetValue():
            self.colors['backgroundColor'].Enable()
            self.update()
        else:
            self.colors['backgroundColor'].Disable()
                        
    def OnIsBorder(self, event):
        if self.colors['borderCtrl'].GetValue():
            self.colors['borderColor'].Enable()
            self.update()
        else:
            self.colors['borderColor'].Disable() 
                                           
    def update(self):

        #units
        currUnit = self.unitConv.findUnit(self.panel.units['unitsCtrl'].GetStringSelection())
        self.mapinfoDict['unit'] = currUnit
        
        # position
        if self.panel.position['xCtrl'].GetValue():
            x = self.panel.position['xCtrl'].GetValue() 
        else:
            x = self.mapinfoDict['where'][0]

        if self.panel.position['yCtrl'].GetValue():
            y = self.panel.position['yCtrl'].GetValue() 
        else:
            y = self.mapinfoDict['where'][1]

        x = self.unitConv.convert(value = float(self.panel.position['xCtrl'].GetValue()), fromUnit = currUnit, toUnit = 'inch')
        y = self.unitConv.convert(value = float(self.panel.position['yCtrl'].GetValue()), fromUnit = currUnit, toUnit = 'inch')
        self.mapinfoDict['where'] = (x, y)
        
        # font
        self.mapinfoDict['font'] =  self.panel.font['fontCtrl'].GetStringSelection()
        self.mapinfoDict['fontsize'] = self.panel.font['fontSizeCtrl'].GetValue()

        #colors
        color = self.panel.font['colorCtrl'].GetColour()
        self.mapinfoDict['color'] = convertRGB(color)
        
        if self.colors['backgroundCtrl'].GetValue():    
            background = self.colors['backgroundColor'].GetColour()
            self.mapinfoDict['background'] = convertRGB(background)
        else:
            self.mapinfoDict['background'] = 'none'

        if self.colors['borderCtrl'].GetValue():    
            border = self.colors['borderColor'].GetColour()
            self.mapinfoDict['border'] = convertRGB(border)
        else:
            self.mapinfoDict['border'] = 'none'
        
        # estimation of size
        self.mapinfoDict['rect'] = self.mapinfo.EstimateRect(self.mapinfoDict)

        if self.id not in self.instruction:
            mapinfo = Mapinfo(self.id)
            self.instruction.AddInstruction(mapinfo)
            
        self.instruction[self.id].SetInstruction(self.mapinfoDict)

        if self.id not in self.parent.objectId:
            self.parent.objectId.append(self.id)
            
        self.updateDialog()

        return True
    
    def updateDialog(self):
        """!Update mapinfo coordinates, after moving"""
        x, y = self.mapinfoDict['where']
        currUnit = self.unitConv.findUnit(self.panel.units['unitsCtrl'].GetStringSelection())
        x = self.unitConv.convert(value = x, fromUnit = 'inch', toUnit = currUnit)
        y = self.unitConv.convert(value = y, fromUnit = 'inch', toUnit = currUnit)
        self.panel.position['xCtrl'].SetValue("%5.3f" % x)
        self.panel.position['yCtrl'].SetValue("%5.3f" % y)
             
class ScalebarDialog(PsmapDialog):
    """!Dialog for scale bar"""
    def __init__(self, parent, id, settings):
        PsmapDialog.__init__(self, parent = parent, id = id, title = "Scale bar settings", settings = settings)
        self.objectType = ('scalebar',)
        if self.id is not None:
            self.scalebar = self.instruction[id]
            self.scalebarDict = self.scalebar.GetInstruction()
        else:
            self.id = wx.NewId()
            self.scalebar = Scalebar(self.id)
            self.scalebarDict = self.scalebar.GetInstruction()
            page = self.instruction.FindInstructionByType('page').GetInstruction()
            self.scalebarDict['where'] = page['Left'], page['Top']

        self.panel = self._scalebarPanel()
        
        self._layout(self.panel)
        
        self.mapUnit = projInfo()['units'].lower()
        if projInfo()['proj'] == 'xy':
            self.mapUnit = 'meters'
        if self.mapUnit not in self.unitConv.getAllUnits():
            wx.MessageBox(message = _("Units of current projection are not supported,\n meters will be used!"),
                            caption = _('Unsupported units'),
                                    style = wx.OK|wx.ICON_ERROR)
            self.mapUnit = 'meters'
            
    def _scalebarPanel(self):
        panel = wx.Panel(parent = self, id = wx.ID_ANY, style = wx.TAB_TRAVERSAL)
        border = wx.BoxSizer(wx.VERTICAL)
        #        
        # position
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Position"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        self.AddUnits(parent = panel, dialogDict = self.scalebarDict)
        self.AddPosition(parent = panel, dialogDict = self.scalebarDict)
        
        if self.scalebarDict['rect']: # set position, ref point is center and not left top corner
            
            x = self.unitConv.convert(value = self.scalebarDict['where'][0] - self.scalebarDict['rect'].Get()[2]/2,
                                                    fromUnit = 'inch', toUnit = self.scalebarDict['unit'])
            y = self.unitConv.convert(value = self.scalebarDict['where'][1] - self.scalebarDict['rect'].Get()[3]/2,
                                                    fromUnit = 'inch', toUnit = self.scalebarDict['unit'])
            panel.position['xCtrl'].SetValue("%5.3f" % x)
            panel.position['yCtrl'].SetValue("%5.3f" % y)
        
        gridBagSizer.Add(panel.units['unitsLabel'], pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.units['unitsCtrl'], pos = (0,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.position['xLabel'], pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.position['xCtrl'], pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.position['yLabel'], pos = (2,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.position['yCtrl'], pos = (2,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(panel.position['comment'], pos = (3,0), span = (1,2), flag =wx.ALIGN_BOTTOM, border = 0)
        
        gridBagSizer.AddGrowableCol(1)
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        #
        # size
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Size"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        lengthText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Length:"))
        heightText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Height:"))
        
        self.lengthTextCtrl = wx.TextCtrl(panel, id = wx.ID_ANY, validator = TCValidator('DIGIT_ONLY'))
        self.lengthTextCtrl.SetToolTipString(_("Scalebar length is given in map units"))
        
        self.heightTextCtrl = wx.TextCtrl(panel, id = wx.ID_ANY, validator = TCValidator('DIGIT_ONLY'))
        self.heightTextCtrl.SetToolTipString(_("Scalebar height is real height on paper"))
        
        choices = [_('default')] + self.unitConv.getMapUnitsNames()
        self.unitsLength = wx.Choice(panel, id = wx.ID_ANY, choices = choices)
        choices = self.unitConv.getPageUnitsNames()
        self.unitsHeight = wx.Choice(panel, id = wx.ID_ANY, choices = choices)
        
        # set values
        unitName = self.unitConv.findName(self.scalebarDict['unitsLength'])
        if unitName:
            self.unitsLength.SetStringSelection(unitName)
        else:
            if self.scalebarDict['unitsLength'] == 'auto':
                 self.unitsLength.SetSelection(0)
            elif self.scalebarDict['unitsLength'] == 'nautmiles':
                 self.unitsLength.SetStringSelection(self.unitConv.findName("nautical miles"))
        self.unitsHeight.SetStringSelection(self.unitConv.findName(self.scalebarDict['unitsHeight']))
        if self.scalebarDict['length']:
            self.lengthTextCtrl.SetValue(str(self.scalebarDict['length']))
        else: #estimate default
            reg = grass.region()
            w = int((reg['e'] - reg['w'])/3)
            w = round(w, -len(str(w)) + 2) #12345 -> 12000
            self.lengthTextCtrl.SetValue(str(w))
            
        h = self.unitConv.convert(value = self.scalebarDict['height'], fromUnit = 'inch',
                                                toUnit =  self.scalebarDict['unitsHeight']) 
        self.heightTextCtrl.SetValue(str(h))
        
        gridBagSizer.Add(lengthText, pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.lengthTextCtrl, pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.unitsLength, pos = (0, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(heightText, pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.heightTextCtrl, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.unitsHeight, pos = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
      
        gridBagSizer.AddGrowableCol(1)
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        #
        #style
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Style"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        
        sbTypeText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Type:"))
        self.sbCombo = wx.combo.BitmapComboBox(panel, style = wx.CB_READONLY)
        # only temporary, images must be moved away
        imagePath = os.path.join(globalvar.ETCIMGDIR, "scalebar-fancy.png"), os.path.join(globalvar.ETCIMGDIR, "scalebar-simple.png") 
        for item, path in zip(['fancy', 'simple'], imagePath):
            if not os.path.exists(path):
                bitmap = wx.EmptyBitmap(0,0)
            else:
                bitmap = wx.Bitmap(path)
            self.sbCombo.Append(item = '', bitmap = bitmap, clientData = item[0])
        #self.sbCombo.Append(item = 'simple', bitmap = wx.Bitmap("./images/scalebar-simple.png"), clientData = 's')
        if self.scalebarDict['scalebar'] == 'f':
            self.sbCombo.SetSelection(0)
        elif self.scalebarDict['scalebar'] == 's':
            self.sbCombo.SetSelection(1)
            
        sbSegmentsText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Number of segments:"))
        self.sbSegmentsCtrl = wx.SpinCtrl(panel, id = wx.ID_ANY, min = 1, max = 30, initial = 4)
        self.sbSegmentsCtrl.SetValue(self.scalebarDict['segment'])
        
        sbLabelsText1 = wx.StaticText(panel, id = wx.ID_ANY, label = _("Label every "))
        sbLabelsText2 = wx.StaticText(panel, id = wx.ID_ANY, label = _("segments"))
        self.sbLabelsCtrl = wx.SpinCtrl(panel, id = wx.ID_ANY, min = 1, max = 30, initial = 1)
        self.sbLabelsCtrl.SetValue(self.scalebarDict['numbers'])
        
        #font
        fontsizeText = wx.StaticText(panel, id = wx.ID_ANY, label = _("Font size:"))
        self.fontsizeCtrl = wx.SpinCtrl(panel, id = wx.ID_ANY, min = 4, max = 30, initial = 10)
        self.fontsizeCtrl.SetValue(self.scalebarDict['fontsize'])
        
        self.backgroundCheck = wx.CheckBox(panel, id = wx.ID_ANY, label = _("transparent text background"))
        if self.scalebarDict['background'] == 'y':
            self.backgroundCheck.SetValue(False)
        else:
            self.backgroundCheck.SetValue(True)

        gridBagSizer.Add(sbTypeText, pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.sbCombo, pos = (0,1), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL|wx.EXPAND, border = 0)
        gridBagSizer.Add(sbSegmentsText, pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.sbSegmentsCtrl, pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(sbLabelsText1, pos = (2,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.sbLabelsCtrl, pos = (2,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(sbLabelsText2, pos = (2,2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(fontsizeText, pos = (3,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.fontsizeCtrl, pos = (3,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.backgroundCheck, pos = (4, 0), span = (1,3), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.ALIGN_CENTER_VERTICAL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        panel.SetSizer(border)
        
        return panel
                           
    def update(self):
        """!Save information from dialog"""

        #units
        currUnit = self.unitConv.findUnit(self.panel.units['unitsCtrl'].GetStringSelection())
        self.scalebarDict['unit'] = currUnit
        # position
        if self.panel.position['xCtrl'].GetValue():
            x = self.panel.position['xCtrl'].GetValue() 
        else:
            x = self.scalebarDict['where'][0]

        if self.panel.position['yCtrl'].GetValue():
            y = self.panel.position['yCtrl'].GetValue() 
        else:
            y = self.scalebarDict['where'][1]

        x = self.unitConv.convert(value = float(self.panel.position['xCtrl'].GetValue()), fromUnit = currUnit, toUnit = 'inch')
        y = self.unitConv.convert(value = float(self.panel.position['yCtrl'].GetValue()), fromUnit = currUnit, toUnit = 'inch')
        
        #style
        self.scalebarDict['scalebar'] = self.sbCombo.GetClientData(self.sbCombo.GetSelection())
        self.scalebarDict['segment'] = self.sbSegmentsCtrl.GetValue()
        self.scalebarDict['numbers'] = self.sbLabelsCtrl.GetValue()
        self.scalebarDict['fontsize'] = self.fontsizeCtrl.GetValue()
        if self.backgroundCheck.GetValue():
            self.scalebarDict['background'] = 'n' 
        else:
            self.scalebarDict['background'] = 'y'

        
        # size
        
        # height
        self.scalebarDict['unitsHeight'] = self.unitConv.findUnit(self.unitsHeight.GetStringSelection())
        try:
            height = float(self.heightTextCtrl.GetValue())  
            height = self.unitConv.convert(value = height, fromUnit = self.scalebarDict['unitsHeight'], toUnit = 'inch') 
        except (ValueError, SyntaxError):
            height = 0.1 #default in inch
        self.scalebarDict['height'] = height    
        
        #length
        if self.unitsLength.GetSelection() == 0:
            selected = 'auto'
        else:
            selected = self.unitConv.findUnit(self.unitsLength.GetStringSelection())
            if selected == 'nautical miles':
                selected = 'nautmiles'
        self.scalebarDict['unitsLength'] = selected
        try:
            length = float(self.lengthTextCtrl.GetValue())
        except (ValueError, SyntaxError):
            wx.MessageBox(message = _("Length of scale bar is not defined"),
                                    caption = _('Invalid input'), style = wx.OK|wx.ICON_ERROR)
            return False
        self.scalebarDict['length'] = length
            
        # estimation of size
        map = self.instruction.FindInstructionByType('map')
        if not map:
            map = self.instruction.FindInstructionByType('initMap')
        mapId = map.id
         
        rectSize = self.scalebar.EstimateSize(scalebarDict = self.scalebarDict,
                                                                scale = self.instruction[mapId]['scale'])
        self.scalebarDict['rect'] = Rect2D(x = x, y = y, width = rectSize[0], height = rectSize[1])
        self.scalebarDict['where'] = self.scalebarDict['rect'].GetCentre() 

        if self.id not in self.instruction:
            scalebar = Scalebar(self.id)
            self.instruction.AddInstruction(scalebar)
        self.instruction[self.id].SetInstruction(self.scalebarDict)
        if self.id not in self.parent.objectId:
            self.parent.objectId.append(self.id)
            
        return True
    
    def updateDialog(self):
        """!Update scalebar coordinates, after moving"""
        x, y = self.scalebarDict['rect'][:2]
        currUnit = self.unitConv.findUnit(self.panel.units['unitsCtrl'].GetStringSelection())
        x = self.unitConv.convert(value = x, fromUnit = 'inch', toUnit = currUnit)
        y = self.unitConv.convert(value = y, fromUnit = 'inch', toUnit = currUnit)
        self.panel.position['xCtrl'].SetValue("%5.3f" % x)
        self.panel.position['yCtrl'].SetValue("%5.3f" % y)
        
 
        
class TextDialog(PsmapDialog):
    def __init__(self, parent, id, settings):
        PsmapDialog.__init__(self, parent = parent, id = id, title = "Text settings", settings = settings)
        self.objectType = ('text',)
        if self.id is not None:
            self.textDict = self.instruction[id].GetInstruction()
        else:
            self.id = wx.NewId()  
            text = Text(self.id)
            self.textDict = text.GetInstruction()
            page = self.instruction.FindInstructionByType('page').GetInstruction()
            self.textDict['where'] = page['Left'], page['Top'] 
                
        map = self.instruction.FindInstructionByType('map')
        if not map:
            map = self.instruction.FindInstructionByType('initMap')
        self.mapId = map.id

        self.textDict['east'], self.textDict['north'] = PaperMapCoordinates(mapInstr = map, x = self.textDict['where'][0], y = self.textDict['where'][1], paperToMap = True)
        
        notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)     
        self.textPanel = self._textPanel(notebook)
        self.positionPanel = self._positionPanel(notebook)
        self.OnBackground(None)
        self.OnHighlight(None)
        self.OnBorder(None)
        self.OnPositionType(None)
        self.OnRotation(None)
     
        self._layout(notebook)

    def _textPanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = _("Text"))
        
        border = wx.BoxSizer(wx.VERTICAL)
        
        # text entry    
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Text"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        
        textLabel = wx.StaticText(panel, id = wx.ID_ANY, label = _("Enter text:"))
        self.textCtrl = ExpandoTextCtrl(panel, id = wx.ID_ANY, value = self.textDict['text'])
        
        sizer.Add(textLabel, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALL, border = 5)
        sizer.Add(self.textCtrl, proportion = 1, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)        
        
        #font       
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Font settings"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexGridSizer = wx.FlexGridSizer (rows = 3, cols = 2, hgap = 5, vgap = 5)
        flexGridSizer.AddGrowableCol(1)
        
        self.AddFont(parent = panel, dialogDict = self.textDict)
        
        flexGridSizer.Add(panel.font['fontLabel'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexGridSizer.Add(panel.font['fontCtrl'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexGridSizer.Add(panel.font['fontSizeLabel'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexGridSizer.Add(panel.font['fontSizeCtrl'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        flexGridSizer.Add(panel.font['colorLabel'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)        
        flexGridSizer.Add(panel.font['colorCtrl'], proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        sizer.Add(item = flexGridSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        #text effects        
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Text effects"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridBagSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        self.effect = {}
        self.effect['backgroundCtrl'] = wx.CheckBox(panel, id = wx.ID_ANY, label = _("text background"))
        self.effect['backgroundColor'] = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        
        self.effect['highlightCtrl'] = wx.CheckBox(panel, id = wx.ID_ANY, label = _("highlight"))
        self.effect['highlightColor'] = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        self.effect['highlightWidth'] = wx.SpinCtrl(panel, id = wx.ID_ANY, size = self.spinCtrlSize, min = 0, max = 5, initial = 1)
        self.effect['highlightWidthLabel'] = wx.StaticText(panel, id = wx.ID_ANY, label = _("Width (pts):"))
        
        self.effect['borderCtrl'] = wx.CheckBox(panel, id = wx.ID_ANY, label = _("text border"))
        self.effect['borderColor'] = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        self.effect['borderWidth'] = wx.SpinCtrl(panel, id = wx.ID_ANY, size = self.spinCtrlSize, min = 1, max = 25, initial = 1)
        self.effect['borderWidthLabel'] = wx.StaticText(panel, id = wx.ID_ANY, label = _("Width (pts):"))

        #set values
        if self.textDict['background'] == None:
            self.textDict['background'] = 'none'
        if self.textDict['background'] != 'none':
            self.effect['backgroundCtrl'].SetValue(True) 
            self.effect['backgroundColor'].SetColour(convertRGB(self.textDict['background']))
        else:
            self.effect['backgroundCtrl'].SetValue(False)
            self.effect['backgroundColor'].SetColour(convertRGB('white'))

        if self.textDict['hcolor'] == None:
             self.textDict['hcolor'] = 'none'
        if self.textDict['hcolor'] != 'none':
            self.effect['highlightCtrl'].SetValue(True) 
            self.effect['highlightColor'].SetColour(convertRGB(self.textDict['hcolor']))
        else:
            self.effect['highlightCtrl'].SetValue(False)
            self.effect['highlightColor'].SetColour(convertRGB('grey'))

        self.effect['highlightWidth'].SetValue(float(self.textDict['hwidth']))
        
        if self.textDict['border'] == None:
            self.textDict['border'] = 'none'
        if self.textDict['border'] != 'none':
            self.effect['borderCtrl'].SetValue(True) 
            self.effect['borderColor'].SetColour(convertRGB(self.textDict['border'])) 
        else:
            self.effect['borderCtrl'].SetValue(False)
            self.effect['borderColor'].SetColour(convertRGB('black'))

        self.effect['borderWidth'].SetValue(float(self.textDict['width']))
        
        gridBagSizer.Add(self.effect['backgroundCtrl'], pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.effect['backgroundColor'], pos = (0,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.effect['highlightCtrl'], pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.effect['highlightColor'], pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.effect['highlightWidthLabel'], pos = (1,2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.effect['highlightWidth'], pos = (1,3), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.effect['borderCtrl'], pos = (2,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.effect['borderColor'], pos = (2,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.effect['borderWidthLabel'], pos = (2,2), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizer.Add(self.effect['borderWidth'], pos = (2,3), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        sizer.Add(item = gridBagSizer, proportion = 1, flag = wx.ALL | wx.EXPAND, border = 1)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        self.Bind(EVT_ETC_LAYOUT_NEEDED, self.OnRefit, self.textCtrl)
        self.Bind(wx.EVT_CHECKBOX, self.OnBackground, self.effect['backgroundCtrl'])
        self.Bind(wx.EVT_CHECKBOX, self.OnHighlight, self.effect['highlightCtrl'])
        self.Bind(wx.EVT_CHECKBOX, self.OnBorder, self.effect['borderCtrl'])
        
        panel.SetSizer(border)
        panel.Fit()
        
        return panel 
        
    def _positionPanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = _("Position"))

        border = wx.BoxSizer(wx.VERTICAL) 

        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Position"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        #Position
        self.AddExtendedPosition(panel, gridBagSizer, self.textDict)
        gridBagSizer.AddGrowableCol(0)
        gridBagSizer.AddGrowableCol(1)
        
        #offset
        box3   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " %_("Offset"))
        sizerO = wx.StaticBoxSizer(box3, wx.VERTICAL)
        gridBagSizerO = wx.GridBagSizer (hgap = 5, vgap = 5)
        self.xoffLabel = wx.StaticText(panel, id = wx.ID_ANY, label = _("horizontal (pts):"))
        self.yoffLabel = wx.StaticText(panel, id = wx.ID_ANY, label = _("vertical (pts):"))
        self.xoffCtrl = wx.SpinCtrl(panel, id = wx.ID_ANY, size = (50, -1), min = -50, max = 50, initial = 0)
        self.yoffCtrl = wx.SpinCtrl(panel, id = wx.ID_ANY, size = (50, -1), min = -50, max = 50, initial = 0) 
        self.xoffCtrl.SetValue(self.textDict['xoffset'])       
        self.yoffCtrl.SetValue(self.textDict['yoffset'])
        gridBagSizerO.Add(self.xoffLabel, pos = (0,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizerO.Add(self.yoffLabel, pos = (1,0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizerO.Add(self.xoffCtrl, pos = (0,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridBagSizerO.Add(self.yoffCtrl, pos = (1,1), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        
        sizerO.Add(gridBagSizerO, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        gridBagSizer.Add(sizerO, pos = (3,0), flag = wx.ALIGN_CENTER_HORIZONTAL|wx.EXPAND, border = 0)
        # reference point
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " %_(" Reference point"))
        sizerR = wx.StaticBoxSizer(box, wx.VERTICAL)
        flexSizer = wx.FlexGridSizer(rows = 3, cols = 3, hgap = 5, vgap = 5)
        flexSizer.AddGrowableCol(0)
        flexSizer.AddGrowableCol(1)
        flexSizer.AddGrowableCol(2)
        ref = []
        for row in ["upper", "center", "lower"]:
            for col in ["left", "center", "right"]:
                ref.append(row + " " + col)
        self.radio = [wx.RadioButton(panel, id = wx.ID_ANY, label = '', style = wx.RB_GROUP, name = ref[0])]
        self.radio[0].SetValue(False)
        flexSizer.Add(self.radio[0], proportion = 0, flag = wx.ALIGN_CENTER, border = 0)
        for i in range(1,9):
            self.radio.append(wx.RadioButton(panel, id = wx.ID_ANY, label = '', name = ref[i]))
            self.radio[-1].SetValue(False)
            flexSizer.Add(self.radio[-1], proportion = 0, flag = wx.ALIGN_CENTER, border = 0)
        self.FindWindowByName(self.textDict['ref']).SetValue(True)
                
        sizerR.Add(flexSizer, proportion = 1, flag = wx.EXPAND, border = 0)
        gridBagSizer.Add(sizerR, pos = (3,1), flag = wx.ALIGN_LEFT|wx.EXPAND, border = 0)
        
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
                
        #rotation
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Text rotation"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)

        self.rotCtrl = wx.CheckBox(panel, id = wx.ID_ANY, label = _("rotate text (counterclockwise)"))
        self.rotValue = wx.SpinCtrl(panel, wx.ID_ANY, size = (50, -1), min = 0, max = 360, initial = 0)
        if self.textDict['rotate']:
            self.rotValue.SetValue(int(self.textDict['rotate']))
            self.rotCtrl.SetValue(True)
        else:
            self.rotValue.SetValue(0)
            self.rotCtrl.SetValue(False)
        sizer.Add(self.rotCtrl, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT|wx.ALL, border = 5)
        sizer.Add(self.rotValue, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL|wx.ALIGN_LEFT|wx.ALL, border = 5)
        
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        panel.SetSizer(border)
        panel.Fit()
          
        self.Bind(wx.EVT_RADIOBUTTON, self.OnPositionType, panel.position['toPaper']) 
        self.Bind(wx.EVT_RADIOBUTTON, self.OnPositionType, panel.position['toMap'])
        self.Bind(wx.EVT_CHECKBOX, self.OnRotation, self.rotCtrl)
        
        return panel
     
    def OnRefit(self, event):
        self.Fit()
        
    def OnRotation(self, event):
        if self.rotCtrl.GetValue():
            self.rotValue.Enable()
        else: 
            self.rotValue.Disable()
            
    def OnPositionType(self, event):
        if self.positionPanel.position['toPaper'].GetValue():
            for widget in self.gridBagSizerP.GetChildren():
                widget.GetWindow().Enable()
            for widget in self.gridBagSizerM.GetChildren():
                widget.GetWindow().Disable()
        else:
            for widget in self.gridBagSizerM.GetChildren():
                widget.GetWindow().Enable()
            for widget in self.gridBagSizerP.GetChildren():
                widget.GetWindow().Disable()
                
    def OnBackground(self, event):
        if self.effect['backgroundCtrl'].GetValue():
            self.effect['backgroundColor'].Enable()
            self.update()
        else:
            self.effect['backgroundColor'].Disable()
    
    def OnHighlight(self, event):
        if self.effect['highlightCtrl'].GetValue():
            self.effect['highlightColor'].Enable()
            self.effect['highlightWidth'].Enable()
            self.effect['highlightWidthLabel'].Enable()
            self.update()
        else:
            self.effect['highlightColor'].Disable()
            self.effect['highlightWidth'].Disable()
            self.effect['highlightWidthLabel'].Disable()
            
    def OnBorder(self, event):
        if self.effect['borderCtrl'].GetValue():
            self.effect['borderColor'].Enable()
            self.effect['borderWidth'].Enable()
            self.effect['borderWidthLabel'].Enable()
            self.update()
        else:
            self.effect['borderColor'].Disable()
            self.effect['borderWidth'].Disable()
            self.effect['borderWidthLabel'].Disable()
            
    def update(self): 
        #text
        self.textDict['text'] = self.textCtrl.GetValue()
        if not self.textDict['text']:
            wx.MessageBox(_("No text entered!"), _("Error"))
            return False
            
        #font
        self.textDict['font'] = self.textPanel.font['fontCtrl'].GetStringSelection()
        self.textDict['fontsize'] = self.textPanel.font['fontSizeCtrl'].GetValue()
        color = self.textPanel.font['colorCtrl'].GetColour()
        self.textDict['color'] = convertRGB(color)

        #effects
        if self.effect['backgroundCtrl'].GetValue():
            background = self.effect['backgroundColor'].GetColour()
            self.textDict['background'] = convertRGB(background)
        else:
            self.textDict['background'] = 'none'        
                
        if self.effect['borderCtrl'].GetValue():
            border = self.effect['borderColor'].GetColour()
            self.textDict['border'] = convertRGB(border)
        else:
            self.textDict['border'] = 'none' 
                     
        self.textDict['width'] = self.effect['borderWidth'].GetValue()
        
        if self.effect['highlightCtrl'].GetValue():
            highlight = self.effect['highlightColor'].GetColour()
            self.textDict['hcolor'] = convertRGB(highlight)
        else:
            self.textDict['hcolor'] = 'none'

        self.textDict['hwidth'] = self.effect['highlightWidth'].GetValue()
        
        #offset
        self.textDict['xoffset'] = self.xoffCtrl.GetValue()
        self.textDict['yoffset'] = self.yoffCtrl.GetValue()

        #position
        if self.positionPanel.position['toPaper'].GetValue():
            self.textDict['XY'] = True
            currUnit = self.unitConv.findUnit(self.positionPanel.units['unitsCtrl'].GetStringSelection())
            self.textDict['unit'] = currUnit
            if self.positionPanel.position['xCtrl'].GetValue():
                x = self.positionPanel.position['xCtrl'].GetValue() 
            else:
                x = self.textDict['where'][0]

            if self.positionPanel.position['yCtrl'].GetValue():
                y = self.positionPanel.position['yCtrl'].GetValue() 
            else:
                y = self.textDict['where'][1]

            x = self.unitConv.convert(value = float(x), fromUnit = currUnit, toUnit = 'inch')
            y = self.unitConv.convert(value = float(y), fromUnit = currUnit, toUnit = 'inch')
            self.textDict['where'] = x, y
            self.textDict['east'], self.textDict['north'] = PaperMapCoordinates(self.instruction[self.mapId], x, y, paperToMap = True)
        else:
            self.textDict['XY'] = False
            if self.positionPanel.position['eCtrl'].GetValue():
                self.textDict['east'] = self.positionPanel.position['eCtrl'].GetValue() 
            else:
                self.textDict['east'] = self.textDict['east']

            if self.positionPanel.position['nCtrl'].GetValue():
                self.textDict['north'] = self.positionPanel.position['nCtrl'].GetValue() 
            else:
                self.textDict['north'] = self.textDict['north']

            self.textDict['where'] = PaperMapCoordinates(mapInstr = self.instruction[self.mapId], x = float(self.textDict['east']),
                                                            y = float(self.textDict['north']), paperToMap = False)
        #rotation
        if self.rotCtrl.GetValue():
            self.textDict['rotate'] = self.rotValue.GetValue()
        else:
            self.textDict['rotate'] = None
        #reference point
        for radio in self.radio:
            if radio.GetValue() == True:
                self.textDict['ref'] = radio.GetName()
                
        if self.id not in self.instruction:
            text = Text(self.id)
            self.instruction.AddInstruction(text)
        self.instruction[self.id].SetInstruction(self.textDict)
        
        if self.id not in self.parent.objectId:
            self.parent.objectId.append(self.id)

#        self.updateDialog()

        return True
    
    def updateDialog(self):
        """!Update text coordinates, after moving"""
        # XY coordinates
        x, y = self.textDict['where'][:2]
        currUnit = self.unitConv.findUnit(self.positionPanel.units['unitsCtrl'].GetStringSelection())
        x = self.unitConv.convert(value = x, fromUnit = 'inch', toUnit = currUnit)
        y = self.unitConv.convert(value = y, fromUnit = 'inch', toUnit = currUnit)
        self.positionPanel.position['xCtrl'].SetValue("%5.3f" % x)
        self.positionPanel.position['yCtrl'].SetValue("%5.3f" % y)
        # EN coordinates
        e, n = self.textDict['east'], self.textDict['north']
        self.positionPanel.position['eCtrl'].SetValue(str(self.textDict['east']))
        self.positionPanel.position['nCtrl'].SetValue(str(self.textDict['north']))
        
class ImageDialog(PsmapDialog):
    """!Dialog for setting image properties.
    
    It's base dialog for North Arrow dialog.
    """
    def __init__(self, parent, id, settings, imagePanelName = _("Image")):
        PsmapDialog.__init__(self, parent = parent, id = id, title = "Image settings",
                             settings = settings)
        
        self.objectType = ('image',)
        if self.id is not None:
            self.imageObj = self.instruction[self.id]
            self.imageDict = self.instruction[id].GetInstruction()
        else:
            self.id = wx.NewId()
            self.imageObj = self._newObject()
            self.imageDict = self.imageObj.GetInstruction()
            page = self.instruction.FindInstructionByType('page').GetInstruction()
            self.imageDict['where'] = page['Left'], page['Top'] 
                
        map = self.instruction.FindInstructionByType('map')
        if not map:
            map = self.instruction.FindInstructionByType('initMap')
        self.mapId = map.id

        self.imageDict['east'], self.imageDict['north'] = PaperMapCoordinates(mapInstr = map, x = self.imageDict['where'][0], y = self.imageDict['where'][1], paperToMap = True)
        
        notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)
        self.imagePanelName = imagePanelName
        self.imagePanel = self._imagePanel(notebook)
        self.positionPanel = self._positionPanel(notebook)
        self.OnPositionType(None)
        
        if self.imageDict['epsfile']:
            self.imagePanel.image['dir'].SetValue(os.path.dirname(self.imageDict['epsfile']))
        else:
            self.imagePanel.image['dir'].SetValue(self._getImageDirectory())
        self.OnDirChanged(None)
     
        self._layout(notebook)
        
        
    def _newObject(self):
        """!Create corresponding instruction object"""
        return Image(self.id, self.instruction)
        
    def _imagePanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, size = (-1, -1), style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = self.imagePanelName)
        border = wx.BoxSizer(wx.VERTICAL)
        #
        # choose image
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Image"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        # choose directory
        panel.image = {}
        if self.imageDict['epsfile']:
            startDir = os.path.dirname(self.imageDict['epsfile'])
        else:
            startDir = self._getImageDirectory()
        dir = filebrowse.DirBrowseButton(parent = panel, id = wx.ID_ANY,
                                         labelText = _("Choose a directory:"),
                                         dialogTitle = _("Choose a directory with images"),
                                         buttonText = _('Browse'),
                                         startDirectory = startDir,
                                         changeCallback = self.OnDirChanged)
        panel.image['dir'] = dir
       
        
        sizer.Add(item = dir, proportion = 0, flag = wx.EXPAND, border = 0)
        
        # image list
        hSizer = wx.BoxSizer(wx.HORIZONTAL)
        
        imageList = wx.ListBox(parent = panel, id = wx.ID_ANY)
        panel.image['list'] = imageList
        imageList.Bind(wx.EVT_LISTBOX, self.OnImageSelectionChanged)
        
        hSizer.Add(item = imageList, proportion = 1, flag = wx.EXPAND | wx.RIGHT, border = 10)
        
        # image preview
        vSizer = wx.BoxSizer(wx.VERTICAL)
        self.previewSize = (150, 150)
        img = wx.EmptyImage(*self.previewSize)
        panel.image['preview'] = wx.StaticBitmap(parent = panel, id = wx.ID_ANY,
                                                bitmap = wx.BitmapFromImage(img))
        vSizer.Add(item = panel.image['preview'], proportion = 0, flag = wx.EXPAND | wx.BOTTOM, border = 5)
        panel.image['sizeInfo'] = wx.StaticText(parent = panel, id = wx.ID_ANY)
        vSizer.Add(item = panel.image['sizeInfo'], proportion = 0, flag = wx.ALIGN_CENTER, border = 0)
        
        hSizer.Add(item = vSizer, proportion = 0, flag = wx.EXPAND, border = 0)
        sizer.Add(item = hSizer, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 3)
        
        epsInfo = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                label = _("Note: only EPS format supported"))
        sizer.Add(item = epsInfo, proportion = 0, flag = wx.ALIGN_CENTER_VERTICAL | wx.ALL, border = 3)
        
        
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        #
        # rotation
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Scale And Rotation"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        scaleLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Scale:"))
        if fs:
            panel.image['scale'] = fs.FloatSpin(panel, id = wx.ID_ANY, min_val = 0, max_val = 50,
                                          increment = 0.5, value = 1, style = fs.FS_RIGHT, size = self.spinCtrlSize)
            panel.image['scale'].SetFormat("%f")
            panel.image['scale'].SetDigits(1)
        else:
            panel.image['scale'] = wx.TextCtrl(panel, id = wx.ID_ANY, size = self.spinCtrlSize,
                                                  validator = TCValidator(flag = 'DIGIT_ONLY'))
        
        if self.imageDict['scale']:
            if fs:
                value = float(self.imageDict['scale'])
            else:
                value = str(self.imageDict['scale'])
        else:
            if fs:
                value = 0
            else:
                value = '0'
        panel.image['scale'].SetValue(value)
            
        gridSizer.Add(item = scaleLabel, pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = panel.image['scale'], pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        
        
        rotLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Rotation angle (deg):"))
        if fs:
            panel.image['rotate'] = fs.FloatSpin(panel, id = wx.ID_ANY, min_val = 0, max_val = 360,
                                          increment = 0.5, value = 0, style = fs.FS_RIGHT, size = self.spinCtrlSize)
            panel.image['rotate'].SetFormat("%f")
            panel.image['rotate'].SetDigits(1)
        else:
            panel.image['rotate'] = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = self.spinCtrlSize,
                                                min = 0, max = 359, initial = 0)
        panel.image['rotate'].SetToolTipString(_("Counterclockwise rotation in degrees"))
        if self.imageDict['rotate']:
            panel.image['rotate'].SetValue(int(self.imageDict['rotate']))
        else:
            panel.image['rotate'].SetValue(0)
            
        gridSizer.Add(item = rotLabel, pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridSizer.Add(item = panel.image['rotate'], pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        
        self._addConvergence(panel = panel, gridBagSizer = gridSizer)
        sizer.Add(item = gridSizer, proportion = 0, flag = wx.EXPAND | wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        panel.SetSizer(border)
        panel.Fit()
        
        return panel
        
    def _positionPanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, size = (-1, -1), style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = _("Position"))
        border = wx.BoxSizer(wx.VERTICAL)
        #
        # set position
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Position"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        self.AddExtendedPosition(panel, gridBagSizer, self.imageDict)
        
        self.Bind(wx.EVT_RADIOBUTTON, self.OnPositionType, panel.position['toPaper']) 
        self.Bind(wx.EVT_RADIOBUTTON, self.OnPositionType, panel.position['toMap'])
        
        
        gridBagSizer.AddGrowableCol(0)
        gridBagSizer.AddGrowableCol(1)
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.ALIGN_CENTER_VERTICAL| wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        panel.SetSizer(border)
        panel.Fit()
        
        return panel
        
    def OnDirChanged(self, event):
        """!Image directory changed"""
        path = self.imagePanel.image['dir'].GetValue()
        try:
            files = os.listdir(path)
        except OSError: # no such directory
            files = []
        imageList = []
        
        # no setter for startDirectory?
        try:
            self.imagePanel.image['dir'].startDirectory = path
        except AttributeError: # for sure
            pass
        for file in files:
            if os.path.splitext(file)[1].lower() == '.eps':
                imageList.append(file)
        
        imageList.sort()
        self.imagePanel.image['list'].SetItems(imageList)
        if self.imageDict['epsfile']:
            file = os.path.basename(self.imageDict['epsfile'])
            self.imagePanel.image['list'].SetStringSelection(file)
        elif imageList:
            self.imagePanel.image['list'].SetSelection(0)
        self.OnImageSelectionChanged(None)
        
    def OnPositionType(self, event):
        if self.positionPanel.position['toPaper'].GetValue():
            for widget in self.gridBagSizerP.GetChildren():
                widget.GetWindow().Enable()
            for widget in self.gridBagSizerM.GetChildren():
                widget.GetWindow().Disable()
        else:
            for widget in self.gridBagSizerM.GetChildren():
                widget.GetWindow().Enable()
            for widget in self.gridBagSizerP.GetChildren():
                widget.GetWindow().Disable()
                
    def _getImageDirectory(self):
        """!Default image directory"""
        return os.getcwd()
        
    def _addConvergence(self, panel, gridBagSizer):
        pass
        
    def OnImageSelectionChanged(self, event):
        """!Image selected, show preview and size"""
        if not self.imagePanel.image['dir']: # event is emitted when closing dialog an it causes error
            return
            
        if not havePILImage:
            self.DrawWarningText(_("PIL\nmissing"))
            return
        
        imageName = self.imagePanel.image['list'].GetStringSelection()
        if not imageName:
            self.ClearPreview()
            return
        basePath = self.imagePanel.image['dir'].GetValue()
        file = os.path.join(basePath, imageName)
        if not os.path.exists(file):
            return
            
        if os.path.splitext(file)[1].lower() == '.eps':
            try:
                pImg = PILImage.open(file)
                img = PilImageToWxImage(pImg)
            except IOError, e:
                GError(message = _("Unable to read file %s") % file)
                self.ClearPreview()
                return
            self.SetSizeInfoLabel(img)
            img = self.ScaleToPreview(img)
            bitmap = img.ConvertToBitmap()
            self.DrawBitmap(bitmap)
            
        else:
            # TODO: read other formats and convert by PIL to eps
            pass
    
    def ScaleToPreview(self, img):
        """!Scale image to preview size"""
        w = img.GetWidth()
        h = img.GetHeight()
        if w <= self.previewSize[0] and h <= self.previewSize[1]:
            return img
        if w > h:
            newW = self.previewSize[0]
            newH = self.previewSize[0] * h / w
        else:
            newH = self.previewSize[0]
            newW = self.previewSize[0] * w / h
        return img.Scale(newW, newH, wx.IMAGE_QUALITY_HIGH)
        
    def DrawWarningText(self, warning):
        """!Draw text on preview window"""
        buffer = wx.EmptyBitmap(*self.previewSize)
        dc = wx.MemoryDC()
        dc.SelectObject(buffer)
        dc.SetBrush(wx.Brush(wx.Colour(250, 250, 250)))
        dc.Clear()
        extent = dc.GetTextExtent(warning)
        posX = self.previewSize[0] / 2 - extent[0] / 2
        posY = self.previewSize[1] / 2 - extent[1] / 2
        dc.DrawText(warning, posX, posY)
        self.imagePanel.image['preview'].SetBitmap(buffer)
        dc.SelectObject(wx.NullBitmap)
        
    def DrawBitmap(self, bitmap):
        """!Draw bitmap, center it if smaller than preview size"""
        if bitmap.GetWidth() <= self.previewSize[0] and bitmap.GetHeight() <= self.previewSize[1]:
            buffer = wx.EmptyBitmap(*self.previewSize)
            dc = wx.MemoryDC()
            dc.SelectObject(buffer)
            dc.SetBrush(dc.GetBrush())
            dc.Clear()
            posX = self.previewSize[0] / 2 - bitmap.GetWidth() / 2
            posY = self.previewSize[1] / 2 - bitmap.GetHeight() / 2
            dc.DrawBitmap(bitmap, posX, posY)
            self.imagePanel.image['preview'].SetBitmap(buffer)
            dc.SelectObject(wx.NullBitmap)
        else:
            self.imagePanel.image['preview'].SetBitmap(bitmap)
            
    def SetSizeInfoLabel(self, image):
        """!Update image size label"""
        self.imagePanel.image['sizeInfo'].SetLabel(_("size: %(width)s x %(height)s pts") % \
                                                       { 'width'  : image.GetWidth(),
                                                         'height' : image.GetHeight() })
        self.imagePanel.image['sizeInfo'].GetContainingSizer().Layout()
        
    def ClearPreview(self):
        """!Clear preview window"""
        buffer = wx.EmptyBitmap(*self.previewSize)
        dc = wx.MemoryDC()
        dc.SelectObject(buffer)
        dc.SetBrush(wx.WHITE_BRUSH)
        dc.Clear()
        mask = wx.Mask(buffer, wx.WHITE)
        buffer.SetMask(mask)
        self.imagePanel.image['preview'].SetBitmap(buffer)
        dc.SelectObject(wx.NullBitmap)
        
    def update(self): 
        # epsfile
        selected = self.imagePanel.image['list'].GetStringSelection()
        basePath = self.imagePanel.image['dir'].GetValue()
        if not selected:
            GMessage(parent = self, message = _("No image selected."))
            return False
            
        self.imageDict['epsfile'] = os.path.join(basePath, selected)
        
        #position
        if self.positionPanel.position['toPaper'].GetValue():
            self.imageDict['XY'] = True
            currUnit = self.unitConv.findUnit(self.positionPanel.units['unitsCtrl'].GetStringSelection())
            self.imageDict['unit'] = currUnit
            if self.positionPanel.position['xCtrl'].GetValue():
                x = self.positionPanel.position['xCtrl'].GetValue() 
            else:
                x = self.imageDict['where'][0]

            if self.positionPanel.position['yCtrl'].GetValue():
                y = self.positionPanel.position['yCtrl'].GetValue() 
            else:
                y = self.imageDict['where'][1]

            x = self.unitConv.convert(value = float(x), fromUnit = currUnit, toUnit = 'inch')
            y = self.unitConv.convert(value = float(y), fromUnit = currUnit, toUnit = 'inch')
            self.imageDict['where'] = x, y
            
        else:
            self.imageDict['XY'] = False
            if self.positionPanel.position['eCtrl'].GetValue():
                e = self.positionPanel.position['eCtrl'].GetValue() 
            else:
                self.imageDict['east'] = self.imageDict['east']

            if self.positionPanel.position['nCtrl'].GetValue():
                n = self.positionPanel.position['nCtrl'].GetValue() 
            else:
                self.imageDict['north'] = self.imageDict['north']

            x, y = PaperMapCoordinates(mapInstr = self.instruction[self.mapId], x = float(self.imageDict['east']),
                                       y = float(self.imageDict['north']), paperToMap = False)

        #rotation
        rot = self.imagePanel.image['rotate'].GetValue()
        if rot == 0:
            self.imageDict['rotate'] = None
        else:
            self.imageDict['rotate'] = rot
        
        #scale
        self.imageDict['scale'] = self.imagePanel.image['scale'].GetValue()
                
        # scale
        w, h = self.imageObj.GetImageOrigSize(self.imageDict['epsfile'])
        if self.imageDict['rotate']:
            self.imageDict['size'] = BBoxAfterRotation(w, h, self.imageDict['rotate'])
        else:
            self.imageDict['size'] = w, h
            
        w = self.unitConv.convert(value = self.imageDict['size'][0],
                                  fromUnit = 'point', toUnit = 'inch')
        h = self.unitConv.convert(value = self.imageDict['size'][1],
                                  fromUnit = 'point', toUnit = 'inch')
                                  
    
        self.imageDict['rect'] = Rect2D(x = x, y = y,
                                        width = w * self.imageDict['scale'],
                                        height = h * self.imageDict['scale'])
        
        if self.id not in self.instruction:
            image = self._newObject()
            self.instruction.AddInstruction(image)
        self.instruction[self.id].SetInstruction(self.imageDict)
        
        if self.id not in self.parent.objectId:
            self.parent.objectId.append(self.id)

        return True
        
    def updateDialog(self):
        """!Update text coordinates, after moving"""
        # XY coordinates
        x, y = self.imageDict['where'][:2]
        currUnit = self.unitConv.findUnit(self.positionPanel.units['unitsCtrl'].GetStringSelection())
        x = self.unitConv.convert(value = x, fromUnit = 'inch', toUnit = currUnit)
        y = self.unitConv.convert(value = y, fromUnit = 'inch', toUnit = currUnit)
        self.positionPanel.position['xCtrl'].SetValue("%5.3f" % x)
        self.positionPanel.position['yCtrl'].SetValue("%5.3f" % y)
        # EN coordinates
        e, n = self.imageDict['east'], self.imageDict['north']
        self.positionPanel.position['eCtrl'].SetValue(str(self.imageDict['east']))
        self.positionPanel.position['nCtrl'].SetValue(str(self.imageDict['north']))
        
        
class NorthArrowDialog(ImageDialog):
    def __init__(self, parent, id, settings):
        ImageDialog.__init__(self, parent = parent, id = id, settings = settings,
                             imagePanelName = _("North Arrow"))
        
        self.objectType = ('northArrow',)
        self.SetTitle(_("North Arrow settings"))
    
    def _newObject(self):
        return NorthArrow(self.id, self.instruction)
        
    def _getImageDirectory(self):
        gisbase = os.getenv("GISBASE")
        return os.path.join(gisbase, 'etc', 'paint', 'decorations')
    
    def _addConvergence(self, panel, gridBagSizer):
        convergence = wx.Button(parent = panel, id = wx.ID_ANY,
                                               label = _("Compute convergence"))
        gridBagSizer.Add(item = convergence, pos = (1, 2),
                      flag = wx.ALIGN_CENTER_VERTICAL | wx.EXPAND)
        convergence.Bind(wx.EVT_BUTTON, self.OnConvergence)
        panel.image['convergence'] = convergence
        
    def OnConvergence(self, event):
        ret = RunCommand('g.region', read = True, flags = 'ng')
        if ret:
            convergence = float(ret.strip().split('=')[1])
            if convergence < 0:
                self.imagePanel.image['rotate'].SetValue(abs(convergence))
            else:
                self.imagePanel.image['rotate'].SetValue(360 - convergence)
            
        
class PointDialog(PsmapDialog):
    """!Dialog for setting point properties."""
    def __init__(self, parent, id, settings, coordinates = None, pointPanelName = _("Point")):
        PsmapDialog.__init__(self, parent = parent, id = id, title = "Point settings",
                             settings = settings)
        
        self.objectType = ('point',)
        if self.id is not None:
            self.pointObj = self.instruction[self.id]
            self.pointDict = self.instruction[id].GetInstruction()
        else:
            self.id = wx.NewId()
            self.pointObj = Point(self.id)
            self.pointDict = self.pointObj.GetInstruction()
            self.pointDict['where'] = coordinates 
        self.defaultDict = self.pointObj.defaultInstruction
                
        mapObj = self.instruction.FindInstructionByType('map')
        if not mapObj:
            mapObj = self.instruction.FindInstructionByType('initMap')
        self.mapId = mapObj.id
        
        self.pointDict['east'], self.pointDict['north'] = PaperMapCoordinates(mapInstr = mapObj, x = self.pointDict['where'][0], y = self.pointDict['where'][1], paperToMap = True)
        
        notebook = wx.Notebook(parent = self, id = wx.ID_ANY, style = wx.BK_DEFAULT)
        self.pointPanelName = pointPanelName
        self.pointPanel = self._pointPanel(notebook)
        self.positionPanel = self._positionPanel(notebook)
        self.OnPositionType(None)
        
     
        self._layout(notebook)
        
    def _pointPanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, size = (-1, -1), style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = self.pointPanelName)
        border = wx.BoxSizer(wx.VERTICAL)
        #
        # choose image
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Symbol"))
        sizer = wx.StaticBoxSizer(box, wx.HORIZONTAL)
        
        gridSizer = wx.GridBagSizer(hgap = 5, vgap = 5)

        gridSizer.Add(item = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Select symbol:")),
                      pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        
        self.symbolLabel = wx.StaticText(parent = panel, id = wx.ID_ANY,
                                          label = self.pointDict['symbol'])
        gridSizer.Add(item = self.symbolLabel, pos = (0, 1),
                      flag = wx.ALIGN_CENTER_VERTICAL )
        bitmap = wx.Bitmap(os.path.join(globalvar.ETCSYMBOLDIR,
                                        self.pointDict['symbol']) + '.png')
        self.symbolButton = wx.BitmapButton(panel, id = wx.ID_ANY, bitmap = bitmap)
        self.symbolButton.Bind(wx.EVT_BUTTON, self.OnSymbolSelection)

        gridSizer.Add(self.symbolButton, pos = (0, 2), flag = wx.ALIGN_CENTER_VERTICAL)
        self.noteLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, 
                                       label = _("Note: Selected symbol is not displayed\n"
                                                 "in draft mode (only in preview mode)"))
        gridSizer.Add(self.noteLabel, pos = (1, 0), span = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL)

        gridSizer.AddGrowableCol(1)
        sizer.Add(item = gridSizer, proportion = 1, flag = wx.EXPAND | wx.ALL, border = 5)
        
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        #
        # outline/fill color
        #

        # outline
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Color"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        outlineLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Outline color:"))
        self.outlineColorCtrl = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        self.outlineTranspCtrl = wx.CheckBox(panel, id = wx.ID_ANY, label = _("transparent"))

        if self.pointDict['color'] != 'none':
            self.outlineTranspCtrl.SetValue(False)
            self.outlineColorCtrl.SetColour(convertRGB(self.pointDict['color']))
        else:
            self.outlineTranspCtrl.SetValue(True)
            self.outlineColorCtrl.SetColour(convertRGB(self.defaultDict['color']))

        gridSizer.Add(item = outlineLabel, pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.outlineColorCtrl, pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.outlineTranspCtrl, pos = (0, 2), flag = wx.ALIGN_CENTER_VERTICAL)

        fillLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Fill color:"))
        self.fillColorCtrl = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        self.fillTranspCtrl = wx.CheckBox(panel, id = wx.ID_ANY, label = _("transparent"))

        if self.pointDict['fcolor'] != 'none':
            self.fillTranspCtrl.SetValue(False)
            self.fillColorCtrl.SetColour(convertRGB(self.pointDict['fcolor']))
        else:
            self.fillTranspCtrl.SetValue(True)
            self.fillColorCtrl.SetColour(convertRGB(self.defaultDict['fcolor']))

        gridSizer.Add(item = fillLabel, pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.fillColorCtrl, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.fillTranspCtrl, pos = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL)
        
        sizer.Add(item = gridSizer, proportion = 0, flag = wx.EXPAND | wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)

        #
        # size and rotation
        #

        # size
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Size and Rotation"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        sizeLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Size (pt):"))
        self.sizeCtrl = wx.SpinCtrl(panel, id = wx.ID_ANY, size = self.spinCtrlSize)
        self.sizeCtrl.SetToolTipString(_("Symbol size in points"))
        self.sizeCtrl.SetValue(self.pointDict['size'])
        
        gridSizer.Add(item = sizeLabel, pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.sizeCtrl, pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        
        # rotation
        rotLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Rotation angle (deg):"))
        if fs:
            self.rotCtrl = fs.FloatSpin(panel, id = wx.ID_ANY, min_val = -360, max_val = 360,
                                          increment = 1, value = 0, style = fs.FS_RIGHT, size = self.spinCtrlSize)
            self.rotCtrl.SetFormat("%f")
            self.rotCtrl.SetDigits(1)
        else:
            self.rotCtrl = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = self.spinCtrlSize,
                                                min = -360, max = 360, initial = 0)
        self.rotCtrl.SetToolTipString(_("Counterclockwise rotation in degrees"))
        self.rotCtrl.SetValue(float(self.pointDict['rotate']))
            
        gridSizer.Add(item = rotLabel, pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL, border = 0)
        gridSizer.Add(item = self.rotCtrl, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        
        sizer.Add(item = gridSizer, proportion = 0, flag = wx.EXPAND | wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        panel.SetSizer(border)
        panel.Fit()
        
        return panel
        
    def _positionPanel(self, notebook):
        panel = wx.Panel(parent = notebook, id = wx.ID_ANY, size = (-1, -1), style = wx.TAB_TRAVERSAL)
        notebook.AddPage(page = panel, text = _("Position"))
        border = wx.BoxSizer(wx.VERTICAL)
        #
        # set position
        #
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Position"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        gridBagSizer = wx.GridBagSizer(hgap = 5, vgap = 5)
        
        self.AddExtendedPosition(panel, gridBagSizer, self.pointDict)
        
        self.Bind(wx.EVT_RADIOBUTTON, self.OnPositionType, panel.position['toPaper']) 
        self.Bind(wx.EVT_RADIOBUTTON, self.OnPositionType, panel.position['toMap'])
        
        
        gridBagSizer.AddGrowableCol(0)
        gridBagSizer.AddGrowableCol(1)
        sizer.Add(gridBagSizer, proportion = 1, flag = wx.ALIGN_CENTER_VERTICAL| wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        
        panel.SetSizer(border)
        panel.Fit()
        
        return panel
        
    def OnPositionType(self, event):
        if self.positionPanel.position['toPaper'].GetValue():
            for widget in self.gridBagSizerP.GetChildren():
                widget.GetWindow().Enable()
            for widget in self.gridBagSizerM.GetChildren():
                widget.GetWindow().Disable()
        else:
            for widget in self.gridBagSizerM.GetChildren():
                widget.GetWindow().Enable()
            for widget in self.gridBagSizerP.GetChildren():
                widget.GetWindow().Disable()
                
    def OnSymbolSelection(self, event):
        dlg = SymbolDialog(self, symbolPath = globalvar.ETCSYMBOLDIR,
                           currentSymbol = self.symbolLabel.GetLabel())
        if dlg.ShowModal() == wx.ID_OK:
            img = dlg.GetSelectedSymbolPath()
            name = dlg.GetSelectedSymbolName()
            self.symbolButton.SetBitmapLabel(wx.Bitmap(img + '.png'))
            self.symbolLabel.SetLabel(name)
            
        dlg.Destroy()
        
    def update(self): 
        # symbol
        self.pointDict['symbol'] = self.symbolLabel.GetLabel()

        
        #position
        if self.positionPanel.position['toPaper'].GetValue():
            self.pointDict['XY'] = True
            currUnit = self.unitConv.findUnit(self.positionPanel.units['unitsCtrl'].GetStringSelection())
            self.pointDict['unit'] = currUnit
            if self.positionPanel.position['xCtrl'].GetValue():
                x = self.positionPanel.position['xCtrl'].GetValue() 
            else:
                x = self.pointDict['where'][0]

            if self.positionPanel.position['yCtrl'].GetValue():
                y = self.positionPanel.position['yCtrl'].GetValue() 
            else:
                y = self.pointDict['where'][1]

            x = self.unitConv.convert(value = float(x), fromUnit = currUnit, toUnit = 'inch')
            y = self.unitConv.convert(value = float(y), fromUnit = currUnit, toUnit = 'inch')
            self.pointDict['where'] = x, y
            
        else:
            self.pointDict['XY'] = False
            if self.positionPanel.position['eCtrl'].GetValue():
                e = self.positionPanel.position['eCtrl'].GetValue() 
            else:
                self.pointDict['east'] = self.pointDict['east']

            if self.positionPanel.position['nCtrl'].GetValue():
                n = self.positionPanel.position['nCtrl'].GetValue() 
            else:
                self.pointDict['north'] = self.pointDict['north']

            x, y = PaperMapCoordinates(mapInstr = self.instruction[self.mapId], x = float(self.pointDict['east']),
                                       y = float(self.pointDict['north']), paperToMap = False)

        #rotation
        self.pointDict['rotate'] = self.rotCtrl.GetValue()
        
        # size
        self.pointDict['size'] = self.sizeCtrl.GetValue()
            
        w = h = self.unitConv.convert(value = self.pointDict['size'],
                                  fromUnit = 'point', toUnit = 'inch')
                                  
        # outline color
        if self.outlineTranspCtrl.GetValue():
            self.pointDict['color'] = 'none'
        else:
            self.pointDict['color'] = convertRGB(self.outlineColorCtrl.GetColour())

        # fill color
        if self.fillTranspCtrl.GetValue():
            self.pointDict['fcolor'] = 'none'
        else:
            self.pointDict['fcolor'] = convertRGB(self.fillColorCtrl.GetColour())

        self.pointDict['rect'] = Rect2D(x = x - w / 2, y = y - h / 2, width = w, height = h)
        
        if self.id not in self.instruction:
            point = Point(self.id)
            self.instruction.AddInstruction(point)
        self.instruction[self.id].SetInstruction(self.pointDict)
        
        if self.id not in self.parent.objectId:
            self.parent.objectId.append(self.id)

        return True
        
    def updateDialog(self):
        """!Update text coordinates, after moving"""
        # XY coordinates
        x, y = self.pointDict['where'][:2]
        currUnit = self.unitConv.findUnit(self.positionPanel.units['unitsCtrl'].GetStringSelection())
        x = self.unitConv.convert(value = x, fromUnit = 'inch', toUnit = currUnit)
        y = self.unitConv.convert(value = y, fromUnit = 'inch', toUnit = currUnit)
        self.positionPanel.position['xCtrl'].SetValue("%5.3f" % x)
        self.positionPanel.position['yCtrl'].SetValue("%5.3f" % y)
        # EN coordinates
        e, n = self.pointDict['east'], self.pointDict['north']
        self.positionPanel.position['eCtrl'].SetValue(str(self.pointDict['east']))
        self.positionPanel.position['nCtrl'].SetValue(str(self.pointDict['north']))
        
class RectangleDialog(PsmapDialog):
    def __init__(self, parent, id, settings, type = 'rectangle', coordinates = None):
        """!

        @param coordinates begin and end point coordinate (wx.Point, wx.Point)
        """
        if type == 'rectangle':
            title = _("Rectangle settings")
        else:
            title = _("Line settings")
        PsmapDialog.__init__(self, parent = parent, id = id, title = title, settings = settings)
        
        self.objectType = (type,)

        if self.id is not None:
            self.rectObj = self.instruction[self.id]
            self.rectDict = self.rectObj.GetInstruction()
        else:
            self.id = wx.NewId()
            if type == 'rectangle':
                self.rectObj = Rectangle(self.id)
            else:
                self.rectObj = Line(self.id)
            self.rectDict = self.rectObj.GetInstruction()

            self.rectDict['rect'] = Rect2DPP(coordinates[0], coordinates[1])
            self.rectDict['where'] = coordinates

        self.defaultDict = self.rectObj.defaultInstruction
        self.panel = self._rectPanel()
        
        self._layout(self.panel)

    def _rectPanel(self):
        panel = wx.Panel(parent = self, id = wx.ID_ANY, style = wx.TAB_TRAVERSAL)
        border = wx.BoxSizer(wx.VERTICAL)
                
        # color
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Color"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        gridSizer = wx.GridBagSizer (hgap = 5, vgap = 5)
        
        outlineLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Outline color:"))
        self.outlineColorCtrl = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
        self.outlineTranspCtrl = wx.CheckBox(panel, id = wx.ID_ANY, label = _("transparent"))

        if self.rectDict['color'] != 'none':
            self.outlineTranspCtrl.SetValue(False)
            self.outlineColorCtrl.SetColour(convertRGB(self.rectDict['color']))
        else:
            self.outlineTranspCtrl.SetValue(True)
            self.outlineColorCtrl.SetColour(convertRGB(self.defaultDict['color']))

        # transparent outline makes sense only for rectangle
        if self.objectType == ('line',):
            self.outlineTranspCtrl.Hide()

        gridSizer.Add(item = outlineLabel, pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.outlineColorCtrl, pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.outlineTranspCtrl, pos = (0, 2), flag = wx.ALIGN_CENTER_VERTICAL)

        # fill color only in rectangle
        if self.objectType == ('rectangle',):
            fillLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Fill color:"))
            self.fillColorCtrl = wx.ColourPickerCtrl(panel, id = wx.ID_ANY)
            self.fillTranspCtrl = wx.CheckBox(panel, id = wx.ID_ANY, label = _("transparent"))

            if self.rectDict['fcolor'] != 'none':
                self.fillTranspCtrl.SetValue(False)
                self.fillColorCtrl.SetColour(convertRGB(self.rectDict['fcolor']))
            else:
                self.fillTranspCtrl.SetValue(True)
                self.fillColorCtrl.SetColour(wx.WHITE)

            gridSizer.Add(item = fillLabel, pos = (1, 0), flag = wx.ALIGN_CENTER_VERTICAL)
            gridSizer.Add(item = self.fillColorCtrl, pos = (1, 1), flag = wx.ALIGN_CENTER_VERTICAL)
            gridSizer.Add(item = self.fillTranspCtrl, pos = (1, 2), flag = wx.ALIGN_CENTER_VERTICAL)

        sizer.Add(gridSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)
        gridSizer = wx.GridBagSizer (hgap = 5, vgap = 5)

        # width
        box   = wx.StaticBox (parent = panel, id = wx.ID_ANY, label = " %s " % _("Line style"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)
        
        widthLabel = wx.StaticText(parent = panel, id = wx.ID_ANY, label = _("Line width:"))
        if fs:
            self.widthCtrl = fs.FloatSpin(panel, id = wx.ID_ANY, min_val = 0, max_val = 50,
                                          increment = 1, value = 0, style = fs.FS_RIGHT, size = self.spinCtrlSize)
            self.widthCtrl.SetFormat("%f")
            self.widthCtrl.SetDigits(1)
        else:
            self.widthCtrl = wx.SpinCtrl(parent = panel, id = wx.ID_ANY, size = self.spinCtrlSize,
                                                min = -360, max = 360, initial = 0)
        self.widthCtrl.SetToolTipString(_("Line width in points"))
        self.widthCtrl.SetValue(float(self.rectDict['width']))

        gridSizer.Add(item = widthLabel, pos = (0, 0), flag = wx.ALIGN_CENTER_VERTICAL)
        gridSizer.Add(item = self.widthCtrl, pos = (0, 1), flag = wx.ALIGN_CENTER_VERTICAL)

        sizer.Add(gridSizer, proportion = 1, flag = wx.EXPAND|wx.ALL, border = 5)
        border.Add(item = sizer, proportion = 0, flag = wx.ALL | wx.EXPAND, border = 5)

        panel.SetSizer(border)
        
        return panel
        

    def update(self):
        mapInstr = self.instruction.FindInstructionByType('map')
        if not mapInstr:
            mapInstr = self.instruction.FindInstructionByType('initMap')
        self.mapId = mapInstr.id
        point1 = self.rectDict['where'][0]
        point2 = self.rectDict['where'][1]
        self.rectDict['east1'], self.rectDict['north1'] = PaperMapCoordinates(mapInstr = mapInstr,
                                                                                x = point1[0],
                                                                                y = point1[1],
                                                                                paperToMap = True)
        self.rectDict['east2'], self.rectDict['north2'] = PaperMapCoordinates(mapInstr = mapInstr,
                                                                                x = point2[0],
                                                                                y = point2[1],
                                                                                paperToMap = True)
        # width
        self.rectDict['width'] = self.widthCtrl.GetValue()
        
        # outline color
        if self.outlineTranspCtrl.GetValue():
            self.rectDict['color'] = 'none'
        else:
            self.rectDict['color'] = convertRGB(self.outlineColorCtrl.GetColour())

        # fill color
        if self.objectType == ('rectangle',):
            if self.fillTranspCtrl.GetValue():
                self.rectDict['fcolor'] = 'none'
            else:
                self.rectDict['fcolor'] = convertRGB(self.fillColorCtrl.GetColour())

        if self.id not in self.instruction:
            if self.objectType == ('rectangle',):
                rect = Rectangle(self.id)
            else:
                rect = Line(self.id)
            self.instruction.AddInstruction(rect)
            
        self.instruction[self.id].SetInstruction(self.rectDict)

        if self.id not in self.parent.objectId:
            self.parent.objectId.append(self.id)
            
        self.updateDialog()

        return True

    def updateDialog(self):
        """!Update text coordinates, after moving"""
        pass


class LabelsDialog(PsmapDialog):
    def __init__(self, parent, id, settings):
        PsmapDialog.__init__(self, parent = parent, id = id, title = _("Vector labels"),
                             settings=settings)
        self.objectType = ('labels',)
        if self.id is not None:
            self.labels = self.instruction[self.id]
        else:
            self.id = wx.NewId()
            self.labels = Labels(self.id)
        self.labelsDict = self.labels.GetInstruction()
        self.panel = self._labelPanel()

        self._layout(self.panel)

    def _labelPanel(self):
        panel = wx.Panel(parent=self, id=wx.ID_ANY, style=wx.TAB_TRAVERSAL)

        border = wx.BoxSizer(wx.VERTICAL)

        box   = wx.StaticBox(parent=panel, id=wx.ID_ANY,
                             label=" %s " % _("Vector label files created beforehand by v.label module"))
        sizer = wx.StaticBoxSizer(box, wx.VERTICAL)


        self.select = Select(parent=panel, multiple=True, type='labels', fullyQualified=False)
        self.select.SetValue(','.join(self.labelsDict['labels']))
        self.select.SetFocus()
        sizer.Add(item=self.select, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        helpText = wx.StaticText(panel, id=wx.ID_ANY, label=_("You can select multiple label files."))
        helpText.SetForegroundColour(wx.SystemSettings_GetColour(wx.SYS_COLOUR_GRAYTEXT))
        sizer.Add(item=helpText, proportion=0, flag=wx.EXPAND | wx.LEFT | wx.RIGHT, border=5)

        border.Add(sizer, proportion=1, flag=wx.EXPAND | wx.ALL, border=5)
        panel.SetSizer(border)

        return panel

    def update(self):
        value = self.select.GetValue()
        if not value:
            self.labelsDict['labels'] = []
        else:
            self.labelsDict['labels'] = value.split(',')
        if self.id not in self.instruction:
            labels = Labels(self.id)
            self.instruction.AddInstruction(labels)

        self.instruction[self.id].SetInstruction(self.labelsDict)

        return True
